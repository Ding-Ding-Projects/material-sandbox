$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$pri = Get-Content (Join-Path $root 'SandboxiePlus/SandMan/SandMan.pri') -Raw
$cpp = Get-Content (Join-Path $root 'SandboxiePlus/SandMan/SandMan.cpp') -Raw
$hostCpp = Get-Content (Join-Path $root 'SandboxiePlus/SandMan/Windows/M3ShellHost.cpp') -Raw
$boxCpp = Get-Content (Join-Path $root 'SandboxiePlus/SandMan/Windows/BoxImageWindow.cpp') -Raw
$proxyCpp = Get-Content (Join-Path $root 'SandboxiePlus/SandMan/Windows/TestProxyDialog.cpp') -Raw
$checks = @(
    [pscustomobject]@{ Pass = $pri -match 'Windows/M3ShellHost\.h'; Message = 'header is listed in SandMan.pri' },
    [pscustomobject]@{ Pass = $pri -match 'Windows/M3ShellHost\.cpp'; Message = 'source is listed in SandMan.pri' },
    [pscustomobject]@{ Pass = $cpp -match 'M3ShellHost::Install\(this, m_pMenuBar\)'; Message = 'CSandMan installs the M3 shell' },
    [pscustomobject]@{ Pass = $hostCpp -match 'm3ShellInstalled'; Message = 'installation is idempotent' },
    [pscustomobject]@{ Pass = $hostCpp -match 'FramelessWindowHint'; Message = 'native chrome is replaced' }
    , [pscustomobject]@{ Pass = ($boxCpp -match 'M3ShellHost::InstallDialog' -or $boxCpp -match 'M3DialogHost::Install\(this\)'); Message = 'BoxImageWindow uses the M3 dialog surface' }
    , [pscustomobject]@{ Pass = ($proxyCpp -match 'M3ShellHost::InstallDialog' -or $proxyCpp -match 'M3DialogHost::Install\(this\)'); Message = 'TestProxyDialog uses the M3 dialog surface' }
)
foreach ($check in $checks) {
    if (-not $check.Pass) { throw "M3 shell validation failed: $($check.Message)" }
    Write-Output "PASS: $($check.Message)"
}
