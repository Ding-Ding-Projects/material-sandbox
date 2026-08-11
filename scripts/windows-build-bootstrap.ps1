[CmdletBinding()]
param(
    [ValidateSet('Build', 'Installer')]
    [string]$Mode = 'Build',

    [ValidateSet('x64', 'ARM64')]
    [string]$Architecture = 'x64',

    [switch]$Silent,
    [switch]$PlanOnly,
    [switch]$SelfTest
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$script:ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$script:RepositoryRoot = Split-Path -Parent $script:ScriptRoot
$script:InstallerRoot = Join-Path $script:RepositoryRoot 'Installer'
$script:QtRoot = Join-Path $script:RepositoryRoot 'Qt'
$script:QtVersion = '6.8.3'
$script:WindowsKitVersion = '10.0.26100.0'
$script:Toolset = 'v143'
$script:MountedEwdkImage = $null

$localAppData = [Environment]::GetFolderPath('LocalApplicationData')
if ([string]::IsNullOrWhiteSpace($localAppData)) {
    throw 'LOCALAPPDATA could not be resolved for the user-scoped build toolchain.'
}
$script:ToolRoot = if ([string]::IsNullOrWhiteSpace($env:SBIE_TOOLCHAIN_ROOT)) {
    Join-Path $localAppData 'SandboxieBuildToolchain'
} else {
    [IO.Path]::GetFullPath($env:SBIE_TOOLCHAIN_ROOT)
}
$script:CacheRoot = Join-Path $script:ToolRoot 'downloads'

$script:Pins = [ordered]@{
    Aqt = [pscustomobject]@{
        Name = 'aqtinstall standalone'; Version = '3.3.0'; FileName = 'aqt_x64-3.3.0.exe'
        Uri = 'https://github.com/miurahr/aqtinstall/releases/download/v3.3.0/aqt_x64.exe'
        Bytes = [long]15185398; Sha256 = '4f74d4c95c464d238d7e17ec2d9b7f22a7c333f0f5270a62584e2b47fc765150'
    }
    Jom = [pscustomobject]@{
        Name = 'Qt jom'; Version = '1.1.4'; FileName = 'jom_1_1_4.zip'
        Uri = 'https://download.qt.io/official_releases/jom/jom_1_1_4.zip'
        Bytes = [long]1696930; Sha256 = 'd533c1ef49214229681e90196ed2094691e8c4a0a0bef0b2c901debcb562682b'
    }
    OpenSsl = [pscustomobject]@{
        Name = 'Sandboxie OpenSSL binaries'; Version = '3.4.0'; FileName = 'openssl-3.4.0.zip'
        Uri = 'https://github.com/xanasoft/openssl-builds/releases/download/openssl-3.4.0/openssl-3.4.0.zip'
        Bytes = [long]27859446; Sha256 = '915048cecda90ecf328a749c3a47732a1eda2f09a5aae18c14d6ec412b22b622'
    }
    SevenZipX64 = [pscustomobject]@{
        Name = '7-Zip x64'; Version = '23.01'; FileName = '7z2301-x64.msi'
        Uri = 'https://7-zip.org/a/7z2301-x64.msi'
        Bytes = [long]1933312; Sha256 = '0ba639b6dacdf573d847c911bd147c6384381a54dac082b1e8c77bc73d58958b'
    }
    SevenZipArm64 = [pscustomobject]@{
        Name = '7-Zip ARM64'; Version = '23.01'; FileName = '7z2301-arm64.exe'
        Uri = 'https://7-zip.org/a/7z2301-arm64.exe'
        Bytes = [long]1527518; Sha256 = '6fa4cb35cbebb0a46b8bbc22d1686a340e183c1f875d8b714efdc39af93debda'
    }
    QtTranslations = [pscustomobject]@{
        Name = 'Qt translations source'; Version = '6.8.3'; FileName = 'qttranslations-everywhere-src-6.8.3.zip'
        Uri = 'https://download.qt.io/archive/qt/6.8/6.8.3/submodules/qttranslations-everywhere-src-6.8.3.zip'
        Bytes = [long]3159682; Sha256 = '2a51d6d2a143b17fb0f9a0ac5fbab67602a950447504ec8cb8e23db12ccd3beb'
    }
    ImDisk = [pscustomobject]@{
        Name = 'ImDisk Toolkit x64'; Version = '20250206'; FileName = 'ImDiskTk-x64-20250206.zip'
        Uri = 'https://master.dl.sourceforge.net/project/imdisk-toolkit/20250206/ImDiskTk-x64.zip?viasf=1'
        Bytes = [long]712902; Sha256 = 'c2105a677ed2269a90565bd4b73b86b9450cfa303673abb978c1eb4ac67e4ba5'
    }
    InnoSetup = [pscustomobject]@{
        Name = 'Inno Setup'; Version = '6.7.3'; FileName = 'innosetup-6.7.3.exe'
        Uri = 'https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe'
        Bytes = [long]10592232; Sha256 = '9c73c3bae7ed48d44112a0f48e66742c00090bdb5bef71d9d3c056c66e97b732'
    }
    Ewdk = [pscustomobject]@{
        Name = 'Enterprise WDK'; Version = '10.0.26100.6584'; FileName = 'EWDK-10.0.26100.6584.iso'
        Uri = 'https://download.microsoft.com/download/be985897-caaa-4497-9ea4-17fa7065cd8a/EWDK_ge_release_svc_prod1_26100_250904-1728.iso'
        Bytes = [long]20002537472; Sha256 = '9f48251dd24ad31aac206d8256e95bda5f90a9783982c45a8aafeb9054562379'
    }
}

function Write-Phase {
    param([string]$Name, [string]$Message)
    Write-Host ('[{0}] {1}' -f $Name, $Message)
}

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-MissingRelativeFiles {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string[]]$RelativePaths
    )
    $missing = @()
    foreach ($relative in $RelativePaths) {
        $candidate = Join-Path $Root $relative
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf) -or (Get-Item -LiteralPath $candidate).Length -eq 0) {
            $missing += $relative
        }
    }
    return $missing
}

function Assert-PinnedFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Pin
    )
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw ('{0} {1} is missing at {2}.' -f $Pin.Name, $Pin.Version, $Path)
    }
    $item = Get-Item -LiteralPath $Path
    if ([long]$item.Length -ne [long]$Pin.Bytes) {
        throw ('{0} {1} has {2} bytes; expected {3}.' -f $Pin.Name, $Pin.Version, $item.Length, $Pin.Bytes)
    }
    $actual = Get-Sha256 -Path $Path
    if ($actual -ne $Pin.Sha256) {
        throw ('{0} {1} SHA-256 mismatch: expected {2}, received {3}.' -f $Pin.Name, $Pin.Version, $Pin.Sha256, $actual)
    }
}

function Get-PinnedDownload {
    param([Parameter(Mandatory = $true)]$Pin)

    New-Item -ItemType Directory -Force -Path $script:CacheRoot | Out-Null
    $destination = Join-Path $script:CacheRoot $Pin.FileName
    if (Test-Path -LiteralPath $destination -PathType Leaf) {
        try {
            Assert-PinnedFile -Path $destination -Pin $Pin
            Write-Phase 'bootstrap' ('Reusing verified {0} {1} at {2}.' -f $Pin.Name, $Pin.Version, $destination)
            return $destination
        } catch {
            Write-Phase 'bootstrap' ('Cached {0} failed verification; a verified replacement will be downloaded.' -f $Pin.Name)
        }
    }

    $partial = $destination + '.partial'
    Write-Phase 'bootstrap' ('Downloading {0} {1} from {2} to {3}.' -f $Pin.Name, $Pin.Version, $Pin.Uri, $partial)
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($null -ne $curl) {
        $curlArgs = @('--fail', '--location', '--silent', '--show-error', '--retry', '5', '--continue-at', '-', '--output', $partial, $Pin.Uri)
        & $curl.Source @curlArgs
        if ($LASTEXITCODE -ne 0) {
            throw ('{0} {1} download from {2} failed with exit code {3}.' -f $Pin.Name, $Pin.Version, $Pin.Uri, $LASTEXITCODE)
        }
    } else {
        Invoke-WebRequest -UseBasicParsing -Uri $Pin.Uri -OutFile $partial
    }

    try {
        Assert-PinnedFile -Path $partial -Pin $Pin
    } catch {
        $rejected = $partial + '.rejected-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
        Move-Item -LiteralPath $partial -Destination $rejected -Force
        throw
    }
    Move-Item -LiteralPath $partial -Destination $destination -Force
    Write-Phase 'bootstrap' ('Verified {0} {1}: {2}.' -f $Pin.Name, $Pin.Version, $Pin.Sha256)
    return $destination
}

function Invoke-External {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [Parameter(Mandatory = $true)][string]$Description,
        [string]$WorkingDirectory = $script:RepositoryRoot
    )
    Write-Phase 'run' $Description
    Push-Location $WorkingDirectory
    try {
        $output = & $FilePath @Arguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    $output | ForEach-Object { Write-Host $_ }
    if ($exitCode -ne 0) {
        throw ('{0} failed with exit code {1}.' -f $Description, $exitCode)
    }
}

function Invoke-CommandFile {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$Description,
        [switch]$CaptureOutput
    )
    $tempRoot = Join-Path $script:ToolRoot 'temp'
    New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
    $commandFile = Join-Path $tempRoot ('bootstrap-{0}.cmd' -f [guid]::NewGuid().ToString('N'))
    [IO.File]::WriteAllLines($commandFile, $Lines, [Text.Encoding]::ASCII)
    try {
        Write-Phase 'run' $Description
        if ($CaptureOutput) {
            $output = & $commandFile
            $exitCode = $LASTEXITCODE
            if ($exitCode -ne 0) {
                throw ('{0} failed with exit code {1}.' -f $Description, $exitCode)
            }
            return $output
        }
        & $commandFile
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) {
            throw ('{0} failed with exit code {1}.' -f $Description, $exitCode)
        }
    } finally {
        if (Test-Path -LiteralPath $commandFile) {
            Remove-Item -LiteralPath $commandFile -Force
        }
    }
}

function Get-VersionFromText {
    param([Parameter(Mandatory = $true)][string]$Text)
    $values = [ordered]@{}
    foreach ($name in @('VERSION_MJR', 'VERSION_MIN', 'VERSION_REV', 'VERSION_UPD')) {
        $pattern = '(?m)^\s*#define\s+' + [regex]::Escape($name) + '\s+(\d+)\s*(?://.*)?$'
        $matches = [regex]::Matches($Text, $pattern)
        if ($matches.Count -ne 1) {
            throw ('Expected exactly one active {0} definition; found {1}.' -f $name, $matches.Count)
        }
        $value = [int]$matches[0].Groups[1].Value
        if ($value -lt 0 -or $value -gt 65535) {
            throw ('{0} must be in the PE version range 0..65535; received {1}.' -f $name, $value)
        }
        $values[$name] = $value
    }
    $parts = @($values.VERSION_MJR, $values.VERSION_MIN, $values.VERSION_REV)
    if ($values.VERSION_UPD -gt 0) { $parts += $values.VERSION_UPD }
    return [pscustomobject]@{
        Display = ($parts -join '.')
        Binary = ('{0}.{1}.{2}.{3}' -f $values.VERSION_MJR, $values.VERSION_MIN, $values.VERSION_REV, $values.VERSION_UPD)
        Major = $values.VERSION_MJR
        Minor = $values.VERSION_MIN
        Revision = $values.VERSION_REV
        Update = $values.VERSION_UPD
    }
}

function Get-SourceVersion {
    $versionHeader = Join-Path $script:RepositoryRoot 'SandboxiePlus\version.h'
    if (-not (Test-Path -LiteralPath $versionHeader -PathType Leaf)) {
        throw ('Version header is missing: {0}' -f $versionHeader)
    }
    return Get-VersionFromText -Text ([IO.File]::ReadAllText($versionHeader))
}

function Assert-PermanentUnsignedIss {
    param([string]$Text = '')
    if ([string]::IsNullOrEmpty($Text)) {
        $issPath = Join-Path $script:InstallerRoot 'Sandboxie-Plus.iss'
        $Text = [IO.File]::ReadAllText($issPath)
    }
    $activeLines = @($Text -split "`r?`n" | Where-Object { $_.Trim() -ne '' -and -not $_.TrimStart().StartsWith(';') })
    if ($activeLines | Where-Object { $_ -match '^\s*Sign[T]ool\s*=' }) {
        throw 'The canonical Inno source contains an active signing-tool directive.'
    }
    if ($activeLines | Where-Object { $_ -match '^\s*SignedUninstaller\s*=\s*yes\s*$' }) {
        throw 'The canonical Inno source enables a signed uninstaller.'
    }
    $offCount = @($activeLines | Where-Object { $_ -match '^\s*SignedUninstaller\s*=\s*no\s*$' }).Count
    if ($offCount -ne 1) {
        throw ('The canonical Inno source must contain exactly one active SignedUninstaller=no directive; found {0}.' -f $offCount)
    }
}

function Select-ExactlyOne {
    param([object[]]$Items, [string]$Description)
    $all = @($Items)
    if ($all.Count -ne 1) {
        throw ('Expected exactly one {0}; found {1}.' -f $Description, $all.Count)
    }
    return $all[0]
}

function Get-BuildPlan {
    param([string]$TargetArchitecture)
    $common = @(
        'msbuild /t:Rebuild Sandboxie\SandboxDll.sln /p:Configuration=SbieRelease /p:Platform=Win32',
        ('msbuild /t:Rebuild Sandboxie\Sandbox.sln /p:Configuration=SbieRelease /p:Platform={0}' -f $TargetArchitecture)
    )
    if ($TargetArchitecture -eq 'ARM64') {
        $common += 'msbuild /t:Rebuild Sandboxie\SandboxDll.sln /p:Configuration=SbieRelease /p:Platform=ARM64EC'
    }
    $common += ('msbuild /t:Rebuild Sandboxie\SandboxDrv.sln /p:Configuration=SbieRelease /p:Platform={0}' -f $TargetArchitecture)
    $common += ('SandboxiePlus\qmake_plus.cmd {0} build_qt6 build_only' -f $TargetArchitecture)
    $common += ('msbuild /restore /t:Rebuild SandboxiePlus\SbieShell\SbieShell.sln /p:Configuration=Release /p:Platform={0} /p:PlatformToolset=v143' -f $TargetArchitecture)
    $common += ('msbuild /t:Rebuild SandboxieTools\SandboxieTools.sln /p:Configuration=Release /p:Platform={0}' -f $TargetArchitecture)
    return $common
}

function Show-Plan {
    param([string]$SelectedMode, [string]$TargetArchitecture, $Version)
    Write-Host ('PLAN mode={0}' -f $SelectedMode)
    Write-Host ('PLAN architecture={0}' -f $TargetArchitecture)
    Write-Host ('PLAN source-version={0}' -f $Version.Display)
    Write-Host ('PLAN qt-root={0}' -f $script:QtRoot)
    Write-Host ('PLAN stage=Installer\Release\SbiePlus_{0}-<run-id>' -f $(if ($TargetArchitecture -eq 'x64') { 'x64' } else { 'a64' }))
    if ($SelectedMode -eq 'Installer') {
        Write-Host 'PLAN output=Installer\Output\<run-id>'
    }
    foreach ($entry in $script:Pins.GetEnumerator()) {
        $pin = $entry.Value
        Write-Host ('PIN {0} version={1} sha256={2} source={3}' -f $entry.Key, $pin.Version, $pin.Sha256, $pin.Uri)
    }
    Write-Host ('PIN Qt version={0} architectures=win64_msvc2022_64,win64_msvc2022_arm64_cross_compiled base-modules=qtdeclarative,qttools source=https://download.qt.io' -f $script:QtVersion)
    foreach ($command in (Get-BuildPlan -TargetArchitecture $TargetArchitecture)) {
        Write-Host ('BUILD {0}' -f $command)
    }
    Write-Host ('STAGE Installer\copy_build.cmd {0} build_qt6' -f $TargetArchitecture)
    Write-Host 'VERIFY full-stage-inventory=required'
    if ($SelectedMode -eq 'Installer') {
        Write-Host 'VERIFY exact-output-count=1'
        Write-Host 'VERIFY authenticode=NotSigned'
        Write-Host 'VERIFY provenance=commit,tree-state,version,architecture,sha256'
    }
}

function Ensure-SevenZip {
    $target = Join-Path $script:ToolRoot '7zip\23.01'
    $sevenZip = Join-Path $target 'Files\7-Zip\7z.exe'
    $valid = (Test-Path -LiteralPath $sevenZip -PathType Leaf)
    if ($valid) {
        $valid = ((Get-Sha256 -Path $sevenZip) -eq '8cebb25e240db3b6986fcaed6bc0b900fa09dad763a56fb71273529266c5c525')
    }
    if (-not $valid) {
        $msi = Get-PinnedDownload -Pin $script:Pins.SevenZipX64
        $stage = Join-Path $script:ToolRoot ('staging\7zip-{0}' -f [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Force -Path $stage | Out-Null
        $lines = @(
            '@echo off',
            ('"{0}" /a "{1}" /qn /norestart TARGETDIR="{2}"' -f (Join-Path $env:SystemRoot 'System32\msiexec.exe'), $msi, $stage),
            'exit /b %errorlevel%'
        )
        Invoke-CommandFile -Lines $lines -Description 'Extracting pinned 7-Zip x64 into the user-scoped toolchain.'
        $candidate = Join-Path $stage 'Files\7-Zip\7z.exe'
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            throw ('7-Zip administrative extraction did not produce {0}.' -f $candidate)
        }
        if ((Get-Sha256 -Path $candidate) -ne '8cebb25e240db3b6986fcaed6bc0b900fa09dad763a56fb71273529266c5c525') {
            throw 'The extracted 7-Zip executable did not match the pinned payload.'
        }
        if (Test-Path -LiteralPath $target) {
            Move-Item -LiteralPath $target -Destination ($target + '.replaced-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ'))
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Move-Item -LiteralPath $stage -Destination $target
        $sevenZip = Join-Path $target 'Files\7-Zip\7z.exe'
    }

    $runtimeRoot = Join-Path $script:InstallerRoot '7-Zip'
    $x64Runtime = Join-Path $runtimeRoot '7-Zip-x64'
    New-Item -ItemType Directory -Force -Path $x64Runtime | Out-Null
    Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $sevenZip) '7z.dll') -Destination (Join-Path $x64Runtime '7z.dll') -Force
    if ((Get-Sha256 -Path (Join-Path $x64Runtime '7z.dll')) -ne '77222e81cb7004e8c3e077aada02b555a3d38fb05b50c64afd36ca230a8fd5b9') {
        throw 'The staged x64 7z.dll did not match the pinned 23.01 payload.'
    }

    $arm64Runtime = Join-Path $runtimeRoot '7-Zip-ARM64'
    $arm64Dll = Join-Path $arm64Runtime '7z.dll'
    if (-not (Test-Path -LiteralPath $arm64Dll -PathType Leaf) -or (Get-Sha256 -Path $arm64Dll) -ne '3ef21c62189641ce995dbbba710d2777b2a8a435a30191a255be036e8652755a') {
        $arm64Package = Get-PinnedDownload -Pin $script:Pins.SevenZipArm64
        $arm64Stage = Join-Path $script:ToolRoot ('staging\7zip-arm64-{0}' -f [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Force -Path $arm64Stage | Out-Null
        Invoke-External -FilePath $sevenZip -Arguments @('x', '-y', ('-o{0}' -f $arm64Stage), $arm64Package) -Description 'Extracting pinned ARM64 7-Zip runtime.'
        $candidateDll = Join-Path $arm64Stage '7z.dll'
        if (-not (Test-Path -LiteralPath $candidateDll -PathType Leaf) -or (Get-Sha256 -Path $candidateDll) -ne '3ef21c62189641ce995dbbba710d2777b2a8a435a30191a255be036e8652755a') {
            throw 'The extracted ARM64 7z.dll did not match the pinned 23.01 payload.'
        }
        New-Item -ItemType Directory -Force -Path $arm64Runtime | Out-Null
        Copy-Item -LiteralPath $candidateDll -Destination $arm64Dll -Force
    }
    return $sevenZip
}

function Ensure-Qt {
    param([string]$TargetArchitecture, [string]$SevenZip)
    $aqt = Get-PinnedDownload -Pin $script:Pins.Aqt
    New-Item -ItemType Directory -Force -Path $script:QtRoot | Out-Null
    $qtVersionRoot = Join-Path $script:QtRoot $script:QtVersion
    $hostRoot = Join-Path $qtVersionRoot 'msvc2022_64'
    $hostRequired = @(
        'bin\qmake.exe', 'bin\lrelease.exe', 'bin\Qt6Core.dll', 'bin\Qt6Gui.dll', 'bin\Qt6Network.dll',
        'bin\Qt6Widgets.dll', 'bin\Qt6Qml.dll', 'bin\Qt6Concurrent.dll',
        'plugins\platforms\qdirect2d.dll', 'plugins\platforms\qminimal.dll', 'plugins\platforms\qoffscreen.dll',
        'plugins\platforms\qwindows.dll', 'plugins\styles\qmodernwindowsstyle.dll',
        'plugins\tls\qcertonlybackend.dll', 'plugins\tls\qopensslbackend.dll', 'plugins\tls\qschannelbackend.dll',
        ('include\QtCore\{0}\QtCore\private\qglobal_p.h' -f $script:QtVersion)
    )
    $hostMissing = @(Get-MissingRelativeFiles -Root $hostRoot -RelativePaths $hostRequired)
    if ($hostMissing.Count -gt 0) {
        # Qt 6.8.3 publishes qtdeclarative and qttools in the base desktop
        # package set. Passing them again through --modules makes aqtinstall
        # reject the repository metadata before downloading anything.
        $args = @('install-qt', '--outputdir', $script:QtRoot, '--base', 'https://download.qt.io', '--timeout', '30', '--external', $SevenZip, 'windows', 'desktop', $script:QtVersion, 'win64_msvc2022_64')
        Invoke-External -FilePath $aqt -Arguments $args -Description ('Installing Qt {0} MSVC 2022 x64 from the official Qt repository.' -f $script:QtVersion)
    }
    $hostMissing = @(Get-MissingRelativeFiles -Root $hostRoot -RelativePaths $hostRequired)
    if ($hostMissing.Count -gt 0) {
        throw ('Qt {0} x64 installation is incomplete under {1}; missing: {2}.' -f $script:QtVersion, $hostRoot, ($hostMissing -join ', '))
    }

    $hostQmake = Join-Path $hostRoot 'bin\qmake.exe'
    $hostLrelease = Join-Path $hostRoot 'bin\lrelease.exe'

    if ($TargetArchitecture -eq 'ARM64') {
        $armRoot = Join-Path $qtVersionRoot 'msvc2022_arm64'
        $armRequired = @(
            'bin\Qt6Core.dll', 'bin\Qt6Gui.dll', 'bin\Qt6Network.dll', 'bin\Qt6Widgets.dll',
            'bin\Qt6Qml.dll', 'bin\Qt6Concurrent.dll', 'plugins\platforms\qdirect2d.dll',
            'plugins\platforms\qminimal.dll', 'plugins\platforms\qoffscreen.dll', 'plugins\platforms\qwindows.dll',
            'plugins\styles\qmodernwindowsstyle.dll', 'plugins\tls\qcertonlybackend.dll',
            'plugins\tls\qopensslbackend.dll', 'plugins\tls\qschannelbackend.dll',
            ('include\QtCore\{0}\QtCore\private\qglobal_p.h' -f $script:QtVersion)
        )
        $armMissing = @(Get-MissingRelativeFiles -Root $armRoot -RelativePaths $armRequired)
        if ($armMissing.Count -gt 0) {
            # Keep the ARM64 install on the same base package contract as the
            # x64 host install; the Qt 6.8.3 metadata rejects duplicate module
            # selectors here as well.
            $args = @('install-qt', '--outputdir', $script:QtRoot, '--base', 'https://download.qt.io', '--timeout', '30', '--external', $SevenZip, 'windows', 'desktop', $script:QtVersion, 'win64_msvc2022_arm64_cross_compiled')
            Invoke-External -FilePath $aqt -Arguments $args -Description ('Installing Qt {0} MSVC 2022 ARM64 from the official Qt repository.' -f $script:QtVersion)
        }
        $armMissing = @(Get-MissingRelativeFiles -Root $armRoot -RelativePaths $armRequired)
        if ($armMissing.Count -gt 0) {
            throw ('Qt {0} ARM64 installation is incomplete under {1}; missing: {2}.' -f $script:QtVersion, $armRoot, ($armMissing -join ', '))
        }
    }

    $jomTarget = Join-Path $script:QtRoot 'Tools\QtCreator\bin\jom.exe'
    if (-not (Test-Path -LiteralPath $jomTarget -PathType Leaf)) {
        $jomArchive = Get-PinnedDownload -Pin $script:Pins.Jom
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $jomTarget) | Out-Null
        Invoke-External -FilePath $SevenZip -Arguments @('x', '-y', ('-o{0}' -f (Split-Path -Parent $jomTarget)), $jomArchive) -Description 'Extracting pinned Qt jom.'
    }
    if (-not (Test-Path -LiteralPath $jomTarget -PathType Leaf)) {
        throw ('Qt jom bootstrap did not produce {0}.' -f $jomTarget)
    }
    return [pscustomobject]@{ HostQmake = $hostQmake; Lrelease = $hostLrelease; Jom = $jomTarget }
}

function Ensure-OpenSsl {
    param([string]$SevenZip)
    $target = Join-Path $script:InstallerRoot 'OpenSSL'
    $marker = Join-Path $target '.bootstrap-sha256'
    $required = [ordered]@{
        'Win_x64\bin\libssl-3-x64.dll' = 'bf862a2ff42a2b10a8b15809a7d6ff01fa6f786ae4e8871d2ae27025f022e80a'
        'Win_x64\bin\libcrypto-3-x64.dll' = 'eea017eaca93659b604b582d6272bac6667f7b5a8abce226325db59d1ac47cff'
        'Win_x86\bin\libssl-3.dll' = '6bedec3aed258dad517ddaae26b22f24fd90a2bf054ee865148ad87668b7a38d'
        'Win_x86\bin\libcrypto-3.dll' = '7f0c5e2fa57b2227b29036b5992b7a75af3caf964f3f7f474b93ac4dd9c92c91'
        'Win_arm64\bin\libssl-1_1-arm64.dll' = '2200234f8529b4272a4022b451269e4fc694b6b0c71b7e0420858c20721fdaa9'
        'Win_arm64\bin\libcrypto-1_1-arm64.dll' = 'b9702116484b90811a0c0e55f403057c43cb5980b7dce33f494ba422db2f31ec'
    }
    $valid = (Test-Path -LiteralPath $marker -PathType Leaf) -and ([IO.File]::ReadAllText($marker).Trim() -eq $script:Pins.OpenSsl.Sha256)
    foreach ($relative in $required.Keys) {
        $path = Join-Path $target $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Sha256 -Path $path) -ne $required[$relative]) { $valid = $false }
    }
    if (-not $valid) {
        $archive = Get-PinnedDownload -Pin $script:Pins.OpenSsl
        $stage = Join-Path $script:ToolRoot ('staging\openssl-{0}' -f [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Force -Path $stage | Out-Null
        Invoke-External -FilePath $SevenZip -Arguments @('x', '-y', ('-o{0}' -f $stage), $archive) -Description 'Extracting pinned Sandboxie OpenSSL binaries.'
        foreach ($relative in $required.Keys) {
            $path = Join-Path $stage $relative
            if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Sha256 -Path $path) -ne $required[$relative]) {
                throw ('The pinned OpenSSL archive is missing or changed {0}.' -f $relative)
            }
        }
        if (Test-Path -LiteralPath $target) {
            Move-Item -LiteralPath $target -Destination ($target + '.replaced-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ'))
        }
        Move-Item -LiteralPath $stage -Destination $target
        [IO.File]::WriteAllText($marker, $script:Pins.OpenSsl.Sha256 + [Environment]::NewLine, [Text.Encoding]::ASCII)
    }
}

function Ensure-QtTranslations {
    param([string]$SevenZip, [string]$Lrelease)
    $target = Join-Path $script:InstallerRoot 'qttranslations\qm'
    $marker = Join-Path $target '.bootstrap-sha256'
    $manifestPath = Join-Path $target '.bootstrap-manifest.json'
    $valid = (Test-Path -LiteralPath $marker -PathType Leaf) -and
        ([IO.File]::ReadAllText($marker).Trim() -eq $script:Pins.QtTranslations.Sha256) -and
        (Test-Path -LiteralPath $manifestPath -PathType Leaf)
    if ($valid) {
        try {
            $manifest = [IO.File]::ReadAllText($manifestPath) | ConvertFrom-Json
            if ($manifest.sourceSha256 -ne $script:Pins.QtTranslations.Sha256 -or @($manifest.files).Count -eq 0) { $valid = $false }
            foreach ($entry in @($manifest.files)) {
                $file = Join-Path $target $entry.name
                if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or (Get-Sha256 -Path $file) -ne $entry.sha256) { $valid = $false }
            }
            $actualQm = @(Get-ChildItem -LiteralPath $target -Filter '*.qm' -File -ErrorAction SilentlyContinue)
            if ($actualQm.Count -ne @($manifest.files).Count) { $valid = $false }
        } catch {
            $valid = $false
        }
    }
    foreach ($pattern in @('qt_*.qm', 'qtbase_*.qm', 'qtmultimedia_*.qm')) {
        if (@(Get-ChildItem -LiteralPath $target -Filter $pattern -File -ErrorAction SilentlyContinue).Count -eq 0) { $valid = $false }
    }
    if ($valid) { return }

    $archive = Get-PinnedDownload -Pin $script:Pins.QtTranslations
    $sourceRoot = Join-Path $script:ToolRoot ('sources\qttranslations-{0}' -f $script:QtVersion)
    $translationSource = Join-Path $sourceRoot ('qttranslations-everywhere-src-{0}\translations' -f $script:QtVersion)
    if (-not (Test-Path -LiteralPath $translationSource -PathType Container)) {
        $stage = Join-Path $script:ToolRoot ('staging\qttranslations-source-{0}' -f [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Force -Path $stage | Out-Null
        Invoke-External -FilePath $SevenZip -Arguments @('x', '-y', ('-o{0}' -f $stage), $archive) -Description 'Extracting pinned Qt translation sources.'
        if (Test-Path -LiteralPath $sourceRoot) {
            Move-Item -LiteralPath $sourceRoot -Destination ($sourceRoot + '.replaced-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ'))
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $sourceRoot) | Out-Null
        Move-Item -LiteralPath $stage -Destination $sourceRoot
    }
    if (-not (Test-Path -LiteralPath $translationSource -PathType Container)) {
        throw ('Qt translation source directory is missing: {0}' -f $translationSource)
    }

    $outputStage = Join-Path $script:ToolRoot ('staging\qttranslations-qm-{0}' -f [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Force -Path $outputStage | Out-Null
    $sources = @(Get-ChildItem -LiteralPath $translationSource -File | Where-Object { $_.Name -match '^(qt_|qtbase_|qtmultimedia_).+\.ts$' })
    if ($sources.Count -eq 0) { throw 'No required Qt translation sources were found.' }
    foreach ($source in $sources) {
        $output = Join-Path $outputStage ([IO.Path]::ChangeExtension($source.Name, '.qm'))
        Invoke-External -FilePath $Lrelease -Arguments @($source.FullName, '-qm', $output) -Description ('Compiling Qt translation {0}.' -f $source.Name)
    }
    foreach ($pattern in @('qt_*.qm', 'qtbase_*.qm', 'qtmultimedia_*.qm')) {
        if (@(Get-ChildItem -LiteralPath $outputStage -Filter $pattern -File).Count -eq 0) {
            throw ('Qt translation compilation produced no {0} files.' -f $pattern)
        }
    }
    $manifest = [ordered]@{
        schema = 1
        sourceSha256 = $script:Pins.QtTranslations.Sha256
        qtVersion = $script:QtVersion
        files = @(Get-ChildItem -LiteralPath $outputStage -Filter '*.qm' -File | Sort-Object Name | ForEach-Object {
            [ordered]@{ name = $_.Name; bytes = [long]$_.Length; sha256 = (Get-Sha256 -Path $_.FullName) }
        })
    }
    [IO.File]::WriteAllText((Join-Path $outputStage '.bootstrap-manifest.json'), (($manifest | ConvertTo-Json -Depth 5) + [Environment]::NewLine), (New-Object Text.UTF8Encoding($false)))
    if (Test-Path -LiteralPath $target) {
        Move-Item -LiteralPath $target -Destination ($target + '.replaced-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ'))
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    Move-Item -LiteralPath $outputStage -Destination $target
    [IO.File]::WriteAllText($marker, $script:Pins.QtTranslations.Sha256 + [Environment]::NewLine, [Text.Encoding]::ASCII)
}

function Ensure-ImDiskAssets {
    param([string]$SevenZip)
    $cabTarget = Join-Path $script:InstallerRoot 'imdisk_files.cab'
    $batchTarget = Join-Path $script:InstallerRoot 'imdisk_install.bat'
    $cabHash = '52e385f2043a362292622f0275a09d71df6eccd82e8161dcfaf276c1036fd003'
    $batchHash = '353bf1b819f57da2e5a3b22a703b625fc6113668dbae5ce8fd50a7e27e4a245b'
    if ((Test-Path -LiteralPath $cabTarget -PathType Leaf) -and (Test-Path -LiteralPath $batchTarget -PathType Leaf)) {
        if ((Get-Sha256 -Path $cabTarget) -eq $cabHash -and (Get-Sha256 -Path $batchTarget) -eq $batchHash) { return }
    }
    $archive = Get-PinnedDownload -Pin $script:Pins.ImDisk
    $stage = Join-Path $script:ToolRoot ('staging\imdisk-{0}' -f [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Force -Path $stage | Out-Null
    Invoke-External -FilePath $SevenZip -Arguments @('e', '-y', ('-o{0}' -f $stage), $archive, 'ImDiskTk20250206\files.cab', 'ImDiskTk20250206\install.bat') -Description 'Extracting pinned ImDisk Toolkit installer payload.'
    $cabSource = Join-Path $stage 'files.cab'
    $batchSource = Join-Path $stage 'install.bat'
    if ((Get-Sha256 -Path $cabSource) -ne $cabHash) { throw 'The extracted ImDisk cabinet did not match the pinned payload.' }
    if ((Get-Sha256 -Path $batchSource) -ne '7a6c2110bbfb92f88b331cd438bb90208cf8e2d8ac19058d0ea744f6016a06a6') { throw 'The extracted ImDisk installer batch did not match the pinned payload.' }
    $cabPartial = Join-Path $script:InstallerRoot ('.imdisk_files.cab.partial-{0}' -f [guid]::NewGuid().ToString('N'))
    $batchPartial = Join-Path $script:InstallerRoot ('.imdisk_install.bat.partial-{0}' -f [guid]::NewGuid().ToString('N'))
    Copy-Item -LiteralPath $cabSource -Destination $cabPartial
    $batchText = [IO.File]::ReadAllText($batchSource, [Text.Encoding]::ASCII).Replace('files.cab', 'imdisk_files.cab')
    [IO.File]::WriteAllText($batchPartial, $batchText, [Text.Encoding]::ASCII)
    if ((Get-Sha256 -Path $cabPartial) -ne $cabHash -or (Get-Sha256 -Path $batchPartial) -ne $batchHash) {
        throw 'The staged ImDisk installer payload did not match the reviewed transformation.'
    }
    Move-Item -LiteralPath $cabPartial -Destination $cabTarget -Force
    Move-Item -LiteralPath $batchPartial -Destination $batchTarget -Force
}

function Get-InstalledToolchain {
    param([string]$TargetArchitecture)
    $programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
    $kitRoot = Join-Path $programFilesX86 'Windows Kits\10'
    $wdkTarget = Join-Path $kitRoot ('build\{0}\WindowsDriver.Common.targets' -f $script:WindowsKitVersion)
    $sdkHeader = Join-Path $kitRoot ('Include\{0}\um\Windows.h' -f $script:WindowsKitVersion)
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf) -or -not (Test-Path -LiteralPath $wdkTarget -PathType Leaf) -or -not (Test-Path -LiteralPath $sdkHeader -PathType Leaf)) {
        return $null
    }
    $installationPath = (& $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
    if ([string]::IsNullOrWhiteSpace($installationPath)) { return $null }
    $devCommand = Join-Path $installationPath 'Common7\Tools\VsDevCmd.bat'
    $msbuild = Join-Path $installationPath 'MSBuild\Current\Bin\MSBuild.exe'
    $toolsets = @(Get-ChildItem -LiteralPath (Join-Path $installationPath 'VC\Tools\MSVC') -Directory -ErrorAction SilentlyContinue)
    if (-not (Test-Path -LiteralPath $devCommand -PathType Leaf) -or -not (Test-Path -LiteralPath $msbuild -PathType Leaf) -or $toolsets.Count -eq 0) {
        return $null
    }
    $latestToolset = $toolsets | Sort-Object Name -Descending | Select-Object -First 1
    $compilerPlatform = if ($TargetArchitecture -eq 'ARM64') { 'arm64' } else { 'x64' }
    $compiler = Join-Path $latestToolset.FullName ('bin\Hostx64\{0}\cl.exe' -f $compilerPlatform)
    if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) { return $null }
    return [pscustomobject]@{
        Source = 'installed Visual Studio 2022 Build Tools plus Windows SDK/WDK'
        EnvironmentScript = $devCommand
        LaunchScript = $null
        InstallationPath = $installationPath
        Version = ([Diagnostics.FileVersionInfo]::GetVersionInfo($msbuild).ProductVersion)
    }
}

function Get-EwdkToolchain {
    $iso = Get-PinnedDownload -Pin $script:Pins.Ewdk
    $diskImage = Get-DiskImage -ImagePath $iso -ErrorAction SilentlyContinue
    $wasAttached = ($null -ne $diskImage -and $diskImage.Attached)
    if (-not $wasAttached) {
        Write-Phase 'bootstrap' ('Mounting verified Enterprise WDK {0} from {1}.' -f $script:Pins.Ewdk.Version, $iso)
        $diskImage = Mount-DiskImage -ImagePath $iso -PassThru
        $script:MountedEwdkImage = $iso
    }
    $volume = $diskImage | Get-Volume | Where-Object { $null -ne $_.DriveLetter } | Select-Object -First 1
    if ($null -eq $volume) { throw ('The Enterprise WDK image mounted without a drive letter: {0}' -f $iso) }
    $root = ('{0}:\' -f $volume.DriveLetter)
    $launch = Join-Path $root 'LaunchBuildEnv.cmd'
    if (-not (Test-Path -LiteralPath $launch -PathType Leaf)) { throw ('Enterprise WDK is missing {0}.' -f $launch) }
    $devCommand = Get-ChildItem -LiteralPath $root -Filter VsDevCmd.bat -File -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match 'Visual Studio\\2022\\.*\\Common7\\Tools' } | Select-Object -First 1
    if ($null -eq $devCommand) { throw 'Enterprise WDK did not contain the pinned Visual Studio 2022 developer command file.' }
    return [pscustomobject]@{
        Source = 'user-cached Enterprise WDK'
        EnvironmentScript = $devCommand.FullName
        LaunchScript = $launch
        InstallationPath = $root
        Version = $script:Pins.Ewdk.Version
    }
}

function Import-ToolchainEnvironment {
    param($Toolchain, [string]$TargetArchitecture)
    $target = if ($TargetArchitecture -eq 'ARM64') { 'arm64' } else { 'x64' }
    $lines = @('@echo off')
    if ($null -ne $Toolchain.LaunchScript) {
        $lines += ('call "{0}"' -f $Toolchain.LaunchScript)
        $lines += 'if errorlevel 1 exit /b %errorlevel%'
    }
    $lines += ('call "{0}" -no_logo -host_arch=x64 -arch={1} -winsdk={2}' -f $Toolchain.EnvironmentScript, $target, $script:WindowsKitVersion)
    $lines += 'if errorlevel 1 exit /b %errorlevel%'
    $lines += 'set'
    $output = @(Invoke-CommandFile -Lines $lines -Description ('Importing the {0} developer environment.' -f $Toolchain.Source) -CaptureOutput)
    foreach ($line in $output) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }
    $env:SBIE_TOOLCHAIN_READY = '1'
    $env:SBIE_QT_ROOT = $script:QtRoot
    $env:WindowsTargetPlatformVersion = $script:WindowsKitVersion
    $msbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($null -eq $msbuild) { throw 'The imported developer environment did not expose msbuild.exe.' }
    if ([string]::IsNullOrWhiteSpace($env:VCToolsRedistDir)) { throw 'The imported developer environment did not expose VCToolsRedistDir.' }
    return $msbuild.Source
}

function Invoke-MsBuild {
    param([string]$MsBuild, [string]$Solution, [string]$Configuration, [string]$Platform, [string[]]$Additional = @())
    $args = @('/nologo', '/verbosity:minimal', '/m:8', '/t:Rebuild', ('/p:Configuration={0}' -f $Configuration), ('/p:Platform={0}' -f $Platform), ('/p:WindowsTargetPlatformVersion={0}' -f $script:WindowsKitVersion))
    $args += $Additional
    $args += (Join-Path $script:RepositoryRoot $Solution)
    Invoke-External -FilePath $MsBuild -Arguments $args -Description ('Building {0} ({1}|{2}).' -f $Solution, $Configuration, $Platform)
}

function Invoke-FullBuild {
    param([string]$TargetArchitecture, [string]$MsBuild)
    $started = [DateTime]::UtcNow
    Invoke-MsBuild -MsBuild $MsBuild -Solution 'Sandboxie\SandboxDll.sln' -Configuration 'SbieRelease' -Platform 'Win32'
    Invoke-MsBuild -MsBuild $MsBuild -Solution 'Sandboxie\Sandbox.sln' -Configuration 'SbieRelease' -Platform $TargetArchitecture
    if ($TargetArchitecture -eq 'ARM64') {
        Invoke-MsBuild -MsBuild $MsBuild -Solution 'Sandboxie\SandboxDll.sln' -Configuration 'SbieRelease' -Platform 'ARM64EC'
    }
    Invoke-MsBuild -MsBuild $MsBuild -Solution 'Sandboxie\SandboxDrv.sln' -Configuration 'SbieRelease' -Platform $TargetArchitecture

    $env:SBIE_CLEAN_BUILD = '1'
    Invoke-External -FilePath (Join-Path $script:RepositoryRoot 'SandboxiePlus\qmake_plus.cmd') -Arguments @($TargetArchitecture, 'build_qt6', 'build_only') -Description ('Building Sandboxie Plus with Qt {0} for {1}.' -f $script:QtVersion, $TargetArchitecture)

    $shellAdditional = @('/restore', '/p:RestorePackagesConfig=true', ('/p:PlatformToolset={0}' -f $script:Toolset))
    Invoke-MsBuild -MsBuild $MsBuild -Solution 'SandboxiePlus\SbieShell\SbieShell.sln' -Configuration 'Release' -Platform $TargetArchitecture -Additional $shellAdditional
    Invoke-MsBuild -MsBuild $MsBuild -Solution 'SandboxieTools\SandboxieTools.sln' -Configuration 'Release' -Platform $TargetArchitecture
    return $started
}

function Get-RequiredStageFiles {
    param([string]$TargetArchitecture)
    $required = @(
        'Qt6Core.dll','Qt6Gui.dll','Qt6Network.dll','Qt6Widgets.dll','Qt6Qml.dll','Qt6Concurrent.dll',
        'platforms\qdirect2d.dll','platforms\qminimal.dll','platforms\qoffscreen.dll','platforms\qwindows.dll',
        'styles\qmodernwindowsstyle.dll','tls\qcertonlybackend.dll','tls\qopensslbackend.dll','tls\qschannelbackend.dll',
        '7z.dll','MiscHelpers.dll','MiscHelpers.pdb','QSbieAPI.dll','QSbieAPI.pdb','QtSingleApp.dll','QtSingleApp.pdb',
        'UGlobalHotkey.dll','UGlobalHotkey.pdb','SandMan.exe','SandMan.pdb','translations.7z','troubleshooting.7z',
        'SbieSvc.exe','SbieSvc.pdb','SbieDll.dll','SbieDll.pdb','SbieDrv.sys','SbieDrv.pdb','SbieCtrl.exe','SbieCtrl.pdb',
        'Start.exe','Start.pdb','kmdutil.exe','kmdutil.pdb','SbieIni.exe','SbieIni.pdb','SbieMsg.dll','SboxHostDll.dll','SboxHostDll.pdb',
        'SandboxieBITS.exe','SandboxieBITS.pdb','SandboxieCrypto.exe','SandboxieCrypto.pdb','SandboxieDcomLaunch.exe','SandboxieDcomLaunch.pdb',
        'SandboxieRpcSs.exe','SandboxieRpcSs.pdb','SandboxieWUAU.exe','SandboxieWUAU.pdb',
        '32\SbieSvc.exe','32\SbieSvc.pdb','32\SbieDll.dll','32\SbieDll.pdb','SbieShellExt.dll','SbieShellPkg.msix',
        'Templates.ini','Manifest0.txt','Manifest1.txt','Manifest2.txt','SbieSettings.ini',
        'ImBox.exe','ImBox.pdb','UpdUtil.exe','UpdUtil.pdb','MiniDump.exe','MiniDump.pdb'
    )
    if ($TargetArchitecture -eq 'x64') {
        $required += @('libssl-3-x64.dll','libcrypto-3-x64.dll')
    } else {
        $required += @('libssl-1_1-ARM64.dll','libcrypto-1_1-ARM64.dll','64\SbieDll.dll','64\SbieDll.pdb')
    }
    return $required
}

function Assert-ArchiveContains {
    param([string]$SevenZip, [string]$Archive, [string[]]$Patterns)
    $listing = (& $SevenZip l -ba $Archive 2>&1) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw ('Could not list archive {0}.' -f $Archive) }
    foreach ($pattern in $Patterns) {
        if ($listing -notmatch $pattern) { throw ('Archive {0} does not contain required member pattern {1}.' -f $Archive, $pattern) }
    }
}

function Get-PeMachineFromBytes {
    param([Parameter(Mandatory = $true)][byte[]]$Bytes)
    if ($Bytes.Length -lt 64 -or $Bytes[0] -ne 0x4d -or $Bytes[1] -ne 0x5a) {
        throw 'Input is not a Windows PE image.'
    }
    $header = [BitConverter]::ToInt32($Bytes, 0x3c)
    if ($header -lt 0 -or ($header + 6) -gt $Bytes.Length -or
        $Bytes[$header] -ne 0x50 -or $Bytes[$header + 1] -ne 0x45 -or
        $Bytes[$header + 2] -ne 0 -or $Bytes[$header + 3] -ne 0) {
        throw 'Input has an invalid Windows PE header.'
    }
    return [BitConverter]::ToUInt16($Bytes, $header + 4)
}

function Assert-PeMachine {
    param([string]$Path, [int]$ExpectedMachine, [string]$ExpectedName)
    $actual = Get-PeMachineFromBytes -Bytes ([IO.File]::ReadAllBytes($Path))
    if ($actual -ne $ExpectedMachine) {
        throw ('PE architecture mismatch for {0}: expected {1} (0x{2:x4}), received 0x{3:x4}.' -f $Path, $ExpectedName, $ExpectedMachine, $actual)
    }
}

function Assert-FreshBuildOutputs {
    param([string]$Stage, [string]$TargetArchitecture, [DateTime]$BuildStarted)
    $required = @(
        'MiscHelpers.dll', 'QSbieAPI.dll', 'QtSingleApp.dll', 'UGlobalHotkey.dll', 'SandMan.exe',
        'SbieSvc.exe', 'SbieDll.dll', 'SbieDrv.sys', 'SbieCtrl.exe', 'Start.exe', 'kmdutil.exe',
        'SbieIni.exe', 'SbieMsg.dll', 'SboxHostDll.dll', 'SandboxieBITS.exe', 'SandboxieCrypto.exe',
        'SandboxieDcomLaunch.exe', 'SandboxieRpcSs.exe', 'SandboxieWUAU.exe',
        'SbieShellExt.dll', 'SbieShellPkg.msix', 'ImBox.exe', 'UpdUtil.exe', 'MiniDump.exe',
        '32\SbieSvc.exe', '32\SbieDll.dll'
    )
    if ($TargetArchitecture -eq 'ARM64') { $required += '64\SbieDll.dll' }
    $old = @()
    foreach ($relative in $required) {
        $item = Get-Item -LiteralPath (Join-Path $Stage $relative)
        if ($item.LastWriteTimeUtc -lt $BuildStarted.AddSeconds(-2)) { $old += $relative }
    }
    if ($old.Count -gt 0) {
        throw ('Staged build outputs predate this build invocation: {0}.' -f ($old -join ', '))
    }
}

function Assert-Stage {
    param([string]$Stage, [string]$TargetArchitecture, [string]$SevenZip)
    $required = @(Get-RequiredStageFiles -TargetArchitecture $TargetArchitecture)
    $missing = @()
    foreach ($relative in $required) {
        $path = Join-Path $Stage $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -eq 0) { $missing += $relative }
    }
    if ($missing.Count -gt 0) { throw ('Stage is missing {0} required files: {1}' -f $missing.Count, ($missing -join ', ')) }

    $crtSource = Join-Path $env:VCToolsRedistDir ('{0}\Microsoft.VC143.CRT' -f $TargetArchitecture)
    $crtFiles = @(Get-ChildItem -LiteralPath $crtSource -File -ErrorAction SilentlyContinue)
    if ($crtFiles.Count -eq 0) { throw ('No VC143 CRT files were found at {0}.' -f $crtSource) }
    foreach ($source in $crtFiles) {
        $destination = Join-Path $Stage $source.Name
        if (-not (Test-Path -LiteralPath $destination -PathType Leaf)) { throw ('Stage is missing VC runtime {0}.' -f $source.Name) }
        if ((Get-Sha256 -Path $source.FullName) -ne (Get-Sha256 -Path $destination)) { throw ('Staged VC runtime {0} does not match its source.' -f $source.Name) }
    }

    Assert-ArchiveContains -SevenZip $SevenZip -Archive (Join-Path $Stage 'translations.7z') -Patterns @(
        '(?im)\bsandman_[^\r\n\\/ ]+\.qm\s*$', '(?im)\bqt_[^\r\n\\/ ]+\.qm\s*$',
        '(?im)\bqtbase_[^\r\n\\/ ]+\.qm\s*$', '(?im)\bqtmultimedia_[^\r\n\\/ ]+\.qm\s*$'
    )
    Assert-ArchiveContains -SevenZip $SevenZip -Archive (Join-Path $Stage 'troubleshooting.7z') -Patterns @(
        '(?im)\blayout\.json\s*$', '(?im)\bAppCompatibility\.js\s*$', '(?im)(?:^|[\\/])UI[\\/]shell\.js\s*$',
        '(?im)(?:^|[\\/])Sandboxing[\\/]SBIEMSG[\\/]SBIEMSG\.js\s*$'
    )

    $targetMachine = if ($TargetArchitecture -eq 'x64') { 0x8664 } else { 0xaa64 }
    $targetName = if ($TargetArchitecture -eq 'x64') { 'x64' } else { 'ARM64' }
    foreach ($relative in @('SandMan.exe', 'SbieSvc.exe', 'SbieDll.dll', 'SbieDrv.sys', 'tls\qopensslbackend.dll')) {
        Assert-PeMachine -Path (Join-Path $Stage $relative) -ExpectedMachine $targetMachine -ExpectedName $targetName
    }
    $sslFiles = if ($TargetArchitecture -eq 'x64') { @('libssl-3-x64.dll', 'libcrypto-3-x64.dll') } else { @('libssl-1_1-ARM64.dll', 'libcrypto-1_1-ARM64.dll') }
    foreach ($relative in $sslFiles) {
        Assert-PeMachine -Path (Join-Path $Stage $relative) -ExpectedMachine $targetMachine -ExpectedName $targetName
    }
    Assert-PeMachine -Path (Join-Path $Stage '32\SbieDll.dll') -ExpectedMachine 0x014c -ExpectedName 'Win32'
    if ($TargetArchitecture -eq 'ARM64') {
        Assert-PeMachine -Path (Join-Path $Stage '64\SbieDll.dll') -ExpectedMachine 0xa641 -ExpectedName 'ARM64EC'
    }
    Write-Phase 'verify' ('Stage contract passed for {0}: explicit-files={1}, crt-files={2}.' -f $TargetArchitecture, $required.Count, $crtFiles.Count)
}

function Get-GitProvenance {
    $commit = (& git -C $script:RepositoryRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $commit -notmatch '^[0-9a-f]{40}$') { throw 'The source commit could not be resolved.' }
    $status = @(& git -C $script:RepositoryRoot status --porcelain=v1 --untracked-files=all)
    if ($LASTEXITCODE -ne 0) { throw 'The source tree state could not be resolved.' }
    return [pscustomobject]@{ Commit = $commit; Clean = ($status.Count -eq 0); Status = $status }
}

function Assert-PeVersion {
    param([string]$Path, $Version)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw ('Expected a Windows PE file at {0}.' -f $Path) }
    $info = [Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    $actual = [version]$info.FileVersion
    $revision = if ($actual.Revision -lt 0) { 0 } else { $actual.Revision }
    if ($actual.Major -ne $Version.Major -or $actual.Minor -ne $Version.Minor -or $actual.Build -ne $Version.Revision -or $revision -ne $Version.Update) {
        throw ('File version mismatch for {0}: expected {1}, received {2}.' -f $Path, $Version.Binary, $actual)
    }
    return $actual.ToString()
}

function Ensure-InnoSetup {
    $target = Join-Path $script:ToolRoot 'inno-setup\6.7.3'
    $iscc = Join-Path $target 'ISCC.exe'
    if (Test-Path -LiteralPath $iscc -PathType Leaf) {
        $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($iscc).FileVersion
        if ($version -like '6.7.3.*') { return $iscc }
    }
    $installer = Get-PinnedDownload -Pin $script:Pins.InnoSetup
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    $lines = @(
        '@echo off',
        ('"{0}" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CURRENTUSER /DIR="{1}"' -f $installer, $target),
        'exit /b %errorlevel%'
    )
    Invoke-CommandFile -Lines $lines -Description ('Installing pinned Inno Setup {0} in the user-scoped toolchain.' -f $script:Pins.InnoSetup.Version)
    if (-not (Test-Path -LiteralPath $iscc -PathType Leaf)) { throw ('Inno Setup did not produce {0}.' -f $iscc) }
    $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($iscc).FileVersion
    if ($version -notlike '6.7.3.*') { throw ('Inno Setup version mismatch at {0}: {1}.' -f $iscc, $version) }
    return $iscc
}

function Invoke-SelfTest {
    $checks = 0
    $version = Get-VersionFromText -Text "#define VERSION_MJR 1`n#define VERSION_MIN 18`n#define VERSION_REV 2`n#define VERSION_UPD 0`n"
    if ($version.Display -ne '1.18.2' -or $version.Binary -ne '1.18.2.0') { throw 'Version parsing self-test failed.' }; $checks++
    foreach ($bad in @(
        "#define VERSION_MJR 1`n#define VERSION_MIN 18`n#define VERSION_REV 2`n",
        "#define VERSION_MJR 1`n#define VERSION_MJR 2`n#define VERSION_MIN 18`n#define VERSION_REV 2`n#define VERSION_UPD 0`n",
        "#define VERSION_MJR 1`n#define VERSION_MIN 18`n#define VERSION_REV two`n#define VERSION_UPD 0`n",
        "#define VERSION_MJR 1`n#define VERSION_MIN 18`n#define VERSION_REV 2`n#define VERSION_UPD 65536`n"
    )) {
        $threw = $false; try { $null = Get-VersionFromText -Text $bad } catch { $threw = $true }
        if (-not $threw) { throw 'Malformed version input was accepted.' }; $checks++
    }
    $fixtureDirective = ('Sign' + 'Tool')
    Assert-PermanentUnsignedIss -Text (('; ' + $fixtureDirective + "=ignored`nSignedUninstaller=no`n")); $checks++
    $threw = $false; try { Assert-PermanentUnsignedIss -Text (($fixtureDirective + "=forbidden`nSignedUninstaller=no`n")) } catch { $threw = $true }
    if (-not $threw) { throw 'Active Inno signing directive was accepted.' }; $checks++
    foreach ($items in @(@(), @('one','two'))) {
        $threw = $false; try { $null = Select-ExactlyOne -Items $items -Description 'fixture output' } catch { $threw = $true }
        if (-not $threw) { throw 'Invalid installer output count was accepted.' }; $checks++
    }
    if ((Select-ExactlyOne -Items @('one') -Description 'fixture output') -ne 'one') { throw 'Single installer output was rejected.' }; $checks++
    if (@(Get-BuildPlan -TargetArchitecture 'x64').Count -ne 6) { throw 'x64 build graph self-test failed.' }; $checks++
    if (@(Get-BuildPlan -TargetArchitecture 'ARM64').Count -ne 7) { throw 'ARM64 build graph self-test failed.' }; $checks++
    if (@(Get-RequiredStageFiles -TargetArchitecture 'x64').Count -ne 73) { throw 'x64 stage inventory self-test failed.' }; $checks++
    if (@(Get-RequiredStageFiles -TargetArchitecture 'ARM64').Count -ne 75) { throw 'ARM64 stage inventory self-test failed.' }; $checks++
    $peFixture = New-Object byte[] 70
    $peFixture[0] = 0x4d; $peFixture[1] = 0x5a
    [BitConverter]::GetBytes([int]64).CopyTo($peFixture, 0x3c)
    $peFixture[64] = 0x50; $peFixture[65] = 0x45
    [BitConverter]::GetBytes([uint16]0xaa64).CopyTo($peFixture, 68)
    if ((Get-PeMachineFromBytes -Bytes $peFixture) -ne 0xaa64) { throw 'PE architecture self-test failed.' }; $checks++
    $archiveListing = "2026-01-01 00:00:00 ....A 1 1 qt_en.qm`n2026-01-01 00:00:00 ....A 1 1 sandman_en.qm"
    if ($archiveListing -notmatch '(?im)\bsandman_[^\r\n\\/ ]+\.qm\s*$' -or $archiveListing -notmatch '(?im)\bqt_[^\r\n\\/ ]+\.qm\s*$') {
        throw 'Archive member matching self-test failed.'
    }; $checks++
    $spacePath = Join-Path 'C:\checkout with spaces' 'Installer\Release\SbiePlus_x64-run'
    if ($spacePath -notmatch 'checkout with spaces') { throw 'Path-with-spaces self-test failed.' }; $checks++
    $externalOutput = @(Invoke-External -FilePath $env:ComSpec -Arguments @('/d', '/c', 'echo bootstrap-stdout-fixture') -Description 'Testing external-output isolation.' -WorkingDirectory $script:RepositoryRoot)
    if ($externalOutput.Count -ne 0) { throw 'Invoke-External leaked child output into its return value.' }; $checks++
    Write-Host ('windows-build-bootstrap-self-test checks={0}' -f $checks)
}

try {
    if ($SelfTest) {
        Invoke-SelfTest
        exit 0
    }
    if ($Mode -eq 'Installer' -and $Architecture -ne 'x64') {
        throw 'The supported local installer is x64. build-installer.bat intentionally ignores ambient SBIE_ARCH.'
    }

    $sourceVersion = Get-SourceVersion
    Assert-PermanentUnsignedIss
    if ($PlanOnly) {
        Show-Plan -SelectedMode $Mode -TargetArchitecture $Architecture -Version $sourceVersion
        exit 0
    }

    $initialProvenance = Get-GitProvenance
    if ($Mode -eq 'Installer' -and -not $initialProvenance.Clean) {
        throw ('Installer provenance requires a clean source tree. Commit or remove these changes first: {0}' -f ($initialProvenance.Status -join '; '))
    }

    $overallStarted = [DateTime]::UtcNow
    Write-Phase 'bootstrap' ('Preparing a fresh-Windows {0} build for {1} from commit {2}.' -f $Mode, $Architecture, $initialProvenance.Commit)
    $sevenZip = Ensure-SevenZip
    $qt = Ensure-Qt -TargetArchitecture $Architecture -SevenZip $sevenZip
    Ensure-OpenSsl -SevenZip $sevenZip
    Ensure-QtTranslations -SevenZip $sevenZip -Lrelease $qt.Lrelease
    if ($Mode -eq 'Installer') { Ensure-ImDiskAssets -SevenZip $sevenZip }

    $toolchain = Get-InstalledToolchain -TargetArchitecture $Architecture
    if ($null -eq $toolchain) {
        Write-Phase 'bootstrap' ('No compatible installed VS 2022 + SDK/WDK {0} toolchain was found; using the pinned user-cached EWDK.' -f $script:WindowsKitVersion)
        $toolchain = Get-EwdkToolchain
    } else {
        Write-Phase 'bootstrap' ('Using {0} version {1} at {2}.' -f $toolchain.Source, $toolchain.Version, $toolchain.InstallationPath)
    }
    $msbuild = Import-ToolchainEnvironment -Toolchain $toolchain -TargetArchitecture $Architecture
    $env:SBIE_7ZIP_EXE = $sevenZip
    $buildStarted = Invoke-FullBuild -TargetArchitecture $Architecture -MsBuild $msbuild

    $runId = '{0}-{1}-{2}' -f [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ'), $PID, [guid]::NewGuid().ToString('N').Substring(0, 8)
    $stageStem = if ($Architecture -eq 'x64') { 'SbiePlus_x64' } else { 'SbiePlus_a64' }
    $stageLeaf = '{0}-{1}' -f $stageStem, $runId
    $stage = Join-Path (Join-Path $script:InstallerRoot 'Release') $stageLeaf
    if (Test-Path -LiteralPath $stage) { throw ('Run-scoped stage already exists: {0}' -f $stage) }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $stage) | Out-Null
    $env:SBIE_INSTALL_STAGE = $stage
    Invoke-External -FilePath (Join-Path $script:InstallerRoot 'copy_build.cmd') -Arguments @($Architecture, 'build_qt6') -Description ('Staging the complete {0} build into {1}.' -f $Architecture, $stage)
    Assert-Stage -Stage $stage -TargetArchitecture $Architecture -SevenZip $sevenZip
    Assert-FreshBuildOutputs -Stage $stage -TargetArchitecture $Architecture -BuildStarted $buildStarted
    $sandMan = Join-Path $stage 'SandMan.exe'
    $sandManVersion = Assert-PeVersion -Path $sandMan -Version $sourceVersion

    $finalProvenance = Get-GitProvenance
    if ($finalProvenance.Commit -ne $initialProvenance.Commit) { throw 'The source commit changed during the build.' }
    if ($Mode -eq 'Installer' -and -not $finalProvenance.Clean) { throw 'The source tree became dirty during the installer build.' }

    if ($Mode -eq 'Build') {
        Write-Phase 'complete' ('Runnable build: {0}' -f $sandMan)
        Write-Phase 'complete' ('Version: {0}; commit: {1}; stage validated.' -f $sandManVersion, $finalProvenance.Commit)
        if (-not $Silent) {
            $answer = Read-Host 'Launch the built SandMan executable now? [y/N]'
            if ($answer -match '^(?i)y(?:es)?$') { Start-Process -FilePath $sandMan }
        }
        exit 0
    }

    $iscc = Ensure-InnoSetup
    $outputDirectory = Join-Path (Join-Path $script:InstallerRoot 'Output') $runId
    if (Test-Path -LiteralPath $outputDirectory) { throw ('Run-scoped output already exists: {0}' -f $outputDirectory) }
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    $compileStarted = [DateTime]::UtcNow
    $iss = Join-Path $script:InstallerRoot 'Sandboxie-Plus.iss'
    $isccArgs = @(('/O{0}' -f $outputDirectory), ('/DMyAppVersion={0}' -f $sourceVersion.Display), '/DMyAppArch=x64', ('/DMyAppSrc={0}' -f $stageLeaf), $iss)
    Invoke-External -FilePath $iscc -Arguments $isccArgs -Description 'Compiling the canonical permanently unsigned Inno Setup source.' -WorkingDirectory $script:InstallerRoot

    $installer = Select-ExactlyOne -Items @(Get-ChildItem -LiteralPath $outputDirectory -Filter '*.exe' -File) -Description 'current-run installer executable'
    $expectedName = 'Sandboxie-Plus-x64-v{0}.exe' -f $sourceVersion.Display
    if ($installer.Name -ne $expectedName) { throw ('Unexpected installer name: expected {0}, received {1}.' -f $expectedName, $installer.Name) }
    if ($installer.Length -lt 1048576) { throw ('Installer is unexpectedly small: {0} bytes.' -f $installer.Length) }
    if ($installer.LastWriteTimeUtc -lt $compileStarted.AddSeconds(-2)) { throw 'Installer output predates this compile invocation.' }
    $installerVersion = Assert-PeVersion -Path $installer.FullName -Version $sourceVersion
    $authenticode = Get-AuthenticodeSignature -LiteralPath $installer.FullName
    if ($authenticode.Status.ToString() -ne 'NotSigned' -or $null -ne $authenticode.SignerCertificate -or $null -ne $authenticode.TimeStamperCertificate) {
        throw ('Installer violates the permanent unsigned policy: status={0}.' -f $authenticode.Status)
    }
    $sha256 = Get-Sha256 -Path $installer.FullName
    if ($sha256 -notmatch '^[0-9a-f]{64}$') { throw 'Installer SHA-256 was not a 64-character hexadecimal digest.' }

    $completed = [DateTime]::UtcNow
    $manifest = [ordered]@{
        schema = 1
        artifact = [ordered]@{ path = $installer.FullName; bytes = [long]$installer.Length; sha256 = $sha256; authenticodeStatus = 'NotSigned'; fileVersion = $installerVersion }
        source = [ordered]@{ commit = $finalProvenance.Commit; clean = $finalProvenance.Clean; version = $sourceVersion.Display; versionBinary = $sourceVersion.Binary }
        build = [ordered]@{ mode = $Mode; architecture = $Architecture; startedAt = $overallStarted.ToString('o'); compileStartedAt = $compileStarted.ToString('o'); completedAt = $completed.ToString('o'); stage = $stage; outputDirectory = $outputDirectory }
        toolchain = [ordered]@{ source = $toolchain.Source; version = $toolchain.Version; windowsKit = $script:WindowsKitVersion; platformToolset = $script:Toolset; qt = $script:QtVersion; innoSetup = $script:Pins.InnoSetup.Version }
        dependencies = @($script:Pins.GetEnumerator() | ForEach-Object { [ordered]@{ name = $_.Value.Name; version = $_.Value.Version; sha256 = $_.Value.Sha256; source = $_.Value.Uri } })
    }
    $manifestPath = Join-Path $outputDirectory ('{0}.build-manifest.json' -f [IO.Path]::GetFileNameWithoutExtension($installer.Name))
    [IO.File]::WriteAllText($manifestPath, (($manifest | ConvertTo-Json -Depth 8) + [Environment]::NewLine), (New-Object Text.UTF8Encoding($false)))

    Write-Phase 'complete' ('Verified unsigned installer: {0}' -f $installer.FullName)
    Write-Phase 'complete' ('SHA-256: {0}' -f $sha256)
    Write-Phase 'complete' ('Version: {0}; commit: {1}; Authenticode: NotSigned.' -f $installerVersion, $finalProvenance.Commit)
    Write-Phase 'complete' ('Provenance manifest: {0}' -f $manifestPath)
} catch {
    Write-Error ('Windows build bootstrap failed: {0}' -f $_.Exception.Message)
    exit 1
} finally {
    if ($null -ne $script:MountedEwdkImage) {
        Write-Phase 'bootstrap' ('Dismounting Enterprise WDK image {0}.' -f $script:MountedEwdkImage)
        Dismount-DiskImage -ImagePath $script:MountedEwdkImage -ErrorAction SilentlyContinue | Out-Null
    }
}
