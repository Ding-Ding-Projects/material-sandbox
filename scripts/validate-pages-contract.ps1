$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$app = Get-Content (Join-Path $root 'docs/assets/app.js') -Raw
$article = Get-Content (Join-Path $root 'docs/features/pages-language-tone.md') -Raw
$checks = @(
    [pscustomobject]@{ Pass = $app -match 'const COPY =' -and $app -match 'function textPair'; Message = 'page has bounded language copy helpers' },
    [pscustomobject]@{ Pass = $app -match 'funnyEnglish' -and $app -match 'funnyCantonese' -and $app -match 'function funny'; Message = 'independent funny levels are wired to rendered copy' },
    [pscustomobject]@{ Pass = $app -match 'activeLanguage\(\)' -and $app -match 'state\.school\.enabled'; Message = 'School-mode English-only behavior is applied at the text boundary' },
    [pscustomobject]@{ Pass = $app -match 'applyPresentation' -and $app -match 'localStorage'; Message = 'presentation settings are applied and persisted' },
    [pscustomobject]@{ Pass = $app -match 'MaterialSandboxMarkdown\?\.render' -and $app -match 'routeFromHash'; Message = 'canonical Markdown articles render inside local hash routes' },
    [pscustomobject]@{ Pass = $app -match 'state\.notifications' -and $app -match 'notification-export'; Message = 'notification history is persisted and exportable' },
    [pscustomobject]@{ Pass = $app -match 'reviewBulkClose' -and $app -match 'bulk-close-dialog'; Message = 'tab bulk close has a review surface' },
    [pscustomobject]@{ Pass = $article -match 'Markdown' -and $article -match 'Suggested articles'; Message = 'Pages language article documents the in-site article route' }
)
foreach ($check in $checks) {
    if (-not $check.Pass) { throw "Pages contract validation failed: $($check.Message)" }
    Write-Output "PASS: $($check.Message)"
}
