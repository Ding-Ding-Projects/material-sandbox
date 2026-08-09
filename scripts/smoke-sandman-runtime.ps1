[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$ArtifactDirectory,

    [ValidateRange(1, 60)]
    [int]$StartupSeconds = 8
)

$ErrorActionPreference = 'Stop'
$evidencePath = $null

function Write-SmokeEvidence {
    param(
        [Parameter(Mandatory = $true)] [string]$Status,
        [string]$Detail,
        [Nullable[int]]$ExitCode
    )

    $evidence = [ordered]@{
        schema = 1
        status = $Status
        observedAt = [DateTime]::UtcNow.ToString('o')
        executable = 'SandMan.exe'
        arguments = @('-autorun')
        platformPlugin = 'minimal'
        startupSeconds = $StartupSeconds
        exitCode = $ExitCode
        detail = $Detail
        evidence = 'Process-level startup only: SandMan.exe remained running for the bounded interval under the packaged Qt minimal platform plugin, then CI terminated that exact process. This does not prove visible rendering, UI interaction, qwindows behavior, service or driver installation/loading, sandboxing, installer behavior, or signing.'
    }
    $evidence | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $evidencePath -Encoding utf8
}

$artifactRoot = (Resolve-Path -LiteralPath $ArtifactDirectory -ErrorAction Stop).Path
$evidencePath = Join-Path $artifactRoot 'ci-runtime-smoke.json'
$sandMan = Join-Path $artifactRoot 'SandMan.exe'
$minimalPlugin = Join-Path $artifactRoot 'platforms\qminimal.dll'
if (-not (Test-Path -LiteralPath $sandMan -PathType Leaf)) {
    throw "SandMan runtime smoke cannot start: missing executable $sandMan"
}
if (-not (Test-Path -LiteralPath $minimalPlugin -PathType Leaf)) {
    throw "SandMan runtime smoke cannot start: missing minimal Qt platform plugin $minimalPlugin"
}

$smokeRoot = Join-Path ([IO.Path]::GetTempPath()) ("sandboxie-sandman-smoke-" + [Guid]::NewGuid().ToString('N'))
$copyRoot = Join-Path $smokeRoot 'artifact'
$appData = Join-Path $smokeRoot 'appdata'
$localAppData = Join-Path $smokeRoot 'localappdata'
$tempDirectory = Join-Path $smokeRoot 'temp'
$process = $null

try {
    New-Item -ItemType Directory -Force $copyRoot, $appData, $localAppData, $tempDirectory | Out-Null
    Copy-Item -Path (Join-Path $artifactRoot '*') -Destination $copyRoot -Recurse -Force

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = Join-Path $copyRoot 'SandMan.exe'
    $startInfo.Arguments = '-autorun'
    $startInfo.WorkingDirectory = $copyRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.EnvironmentVariables['QT_QPA_PLATFORM'] = 'minimal'
    $startInfo.EnvironmentVariables['APPDATA'] = $appData
    $startInfo.EnvironmentVariables['LOCALAPPDATA'] = $localAppData
    $startInfo.EnvironmentVariables['TEMP'] = $tempDirectory
    $startInfo.EnvironmentVariables['TMP'] = $tempDirectory

    $process = [Diagnostics.Process]::Start($startInfo)
    Start-Sleep -Seconds $StartupSeconds

    if ($process.HasExited) {
        $stdout = $process.StandardOutput.ReadToEnd().Trim()
        $stderr = $process.StandardError.ReadToEnd().Trim()
        $detail = "SandMan exited during the $StartupSeconds-second startup interval with exit code $($process.ExitCode)."
        if ($stdout) { $detail += " stdout: $stdout" }
        if ($stderr) { $detail += " stderr: $stderr" }
        Write-SmokeEvidence -Status 'failed' -Detail $detail -ExitCode $process.ExitCode
        throw $detail
    }

    $process.Kill()
    if (-not $process.WaitForExit(5000)) {
        $detail = 'SandMan remained running after the bounded startup interval but did not terminate within 5 seconds of CI cleanup.'
        Write-SmokeEvidence -Status 'failed' -Detail $detail -ExitCode $null
        throw $detail
    }

    Write-SmokeEvidence -Status 'passed' -Detail "SandMan remained running for $StartupSeconds seconds and was then terminated by CI cleanup." -ExitCode $process.ExitCode
    Write-Output "sandman-runtime-smoke status=passed startupSeconds=$StartupSeconds"
}
catch {
    if ($null -ne $evidencePath -and -not (Test-Path -LiteralPath $evidencePath)) {
        Write-SmokeEvidence -Status 'failed' -Detail $_.Exception.Message -ExitCode $null
    }
    throw
}
finally {
    if ($null -ne $process -and -not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit(5000) | Out-Null
    }
    if (Test-Path -LiteralPath $smokeRoot) {
        Remove-Item -LiteralPath $smokeRoot -Recurse -Force
    }
}
