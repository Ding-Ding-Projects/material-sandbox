$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$page = Get-Content (Join-Path $root 'docs/index.html') -Raw
$checks = @(
    [pscustomobject]@{ Pass = $page -match 'role="tablist" aria-orientation="horizontal"'; Message = 'tablist exposes horizontal keyboard orientation' },
    [pscustomobject]@{ Pass = $page -match 'item\.hidden = !active'; Message = 'inactive tab panels are hidden semantically' },
    [pscustomobject]@{ Pass = $page -match "event\.key === 'Home'" -and $page -match "event\.key === 'End'"; Message = 'tab keyboard navigation supports Home and End' },
    [pscustomobject]@{ Pass = $page -match 'aria-hidden="true"' -and $page -match "setAttribute\('aria-hidden', 'false'\)"; Message = 'palette overlay exposes open state to assistive technology' },
    [pscustomobject]@{ Pass = $page -match 'body\.classList\.add\(''dialog-open''\)' -and $page -match 'body\.classList\.remove\(''dialog-open''\)'; Message = 'open palette locks background scrolling' },
    [pscustomobject]@{ Pass = $page -match "paletteSurface.*addEventListener\('keydown'"; Message = 'palette traps keyboard focus' },
    [pscustomobject]@{ Pass = $page -match 'prefers-reduced-motion: reduce'; Message = 'reduced-motion CSS remains present' }
)
foreach ($check in $checks) {
    if (-not $check.Pass) { throw "Pages accessibility validation failed: $($check.Message)" }
    Write-Output "PASS: $($check.Message)"
}
