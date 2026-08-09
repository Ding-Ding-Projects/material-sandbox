$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$page = Get-Content (Join-Path $root 'docs/index.html') -Raw
$article = Get-Content (Join-Path $root 'docs/features/pages-language-tone.md') -Raw
$checks = @(
    [pscustomobject]@{ Pass = $page -match 'const copy ='; Message = 'page has a bounded language copy table' },
    [pscustomobject]@{ Pass = $page -match 'function applyPresentation'; Message = 'page applies language and tone to rendered copy' },
    [pscustomobject]@{ Pass = $page -match "funnyEnglish.*addEventListener\('input', applyPresentation\)"; Message = 'English funny-level slider is wired' },
    [pscustomobject]@{ Pass = $page -match "funnyCantonese.*addEventListener\('input', applyPresentation\)"; Message = 'Cantonese funny-level slider is wired' },
    [pscustomobject]@{ Pass = $article -match 'remaining work'; Message = 'remaining page-contract gaps are documented' }
)
foreach ($check in $checks) {
    if (-not $check.Pass) { throw "Pages contract validation failed: $($check.Message)" }
    Write-Output "PASS: $($check.Message)"
}
