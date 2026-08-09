param(
    [switch]$SelfTest
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$page = Get-Content (Join-Path $root 'docs/index.html') -Raw

function Get-PagesAccessibilityChecks {
    param([Parameter(Mandatory = $true)][string]$Source)

    $notificationZ = [regex]::Match($Source, '(?s)\.notification-stack\s*\{[^}]*?z-index:\s*(\d+)').Groups[1].Value
    $dialogZ = [regex]::Match($Source, '(?s)\.dialog-surface\s*\{[^}]*?z-index:\s*(\d+)').Groups[1].Value
    $modalLayeringIsSafe = $notificationZ -and $dialogZ -and ([int]$dialogZ -gt [int]$notificationZ)

    return @(
        [pscustomobject]@{ Pass = $Source -match 'role="tablist" aria-orientation="vertical"' -and $Source -match 'effectiveOrientation' -and $Source -match "'ArrowDown'.*'ArrowRight'"; Message = 'tablist defaults left and follows its effective keyboard axis' },
        [pscustomobject]@{ Pass = $Source -match 'panel\.hidden = !active'; Message = 'inactive tab panels are hidden semantically' },
        [pscustomobject]@{ Pass = $Source -match "event\.key === 'Home'" -and $Source -match "event\.key === 'End'" -and $Source -match "event\.key === 'Enter'"; Message = 'tab keyboard navigation supports movement and explicit activation' },
        [pscustomobject]@{ Pass = $Source -match 'aria-hidden="true"' -and $Source -match "setAttribute\('aria-hidden', 'false'\)"; Message = 'palette overlay exposes open state to assistive technology' },
        [pscustomobject]@{ Pass = $Source -match 'body\.classList\.add\(''dialog-open''\)' -and $Source -match 'body\.classList\.remove\(''dialog-open''\)'; Message = 'open palette locks background scrolling' },
        [pscustomobject]@{ Pass = $Source -match "paletteSurface.*addEventListener\('keydown'" -and $Source -match 'focusable\[0\]' -and $Source -match 'focusable\.length - 1'; Message = 'palette owns an explicit keyboard focus loop' },
        [pscustomobject]@{ Pass = $Source -match 'closePalette\s*\(\s*(?:false|\{\s*restoreFocus\s*:\s*false\s*\})\s*\)' -and $Source -match 'teleport-target'; Message = 'palette setting teleport retains focus on its destination' },
        [pscustomobject]@{ Pass = $Source -match 'querySelectorAll\(''\.feature-card h3''\)' -or $Source -match 'querySelectorAll\("\.feature-card h3"\)'; Message = 'palette indexes feature names instead of article links alone' },
        [pscustomobject]@{ Pass = $Source -match 'prefers-reduced-motion: reduce'; Message = 'reduced-motion CSS remains present' },
        [pscustomobject]@{ Pass = $Source -match '\.notification button \{ min-width: 48px; min-height: 48px;'; Message = 'notification dismissal keeps a 48-pixel target' },
        [pscustomobject]@{ Pass = $Source -match '\.panel:focus-visible' -or $Source -match '\[role="tabpanel"\]:focus-visible'; Message = 'focused tab panels have a visible focus treatment' },
        [pscustomobject]@{ Pass = $modalLayeringIsSafe; Message = 'modal palette paints above the independently focusable notification' },
        [pscustomobject]@{ Pass = $Source -match '(?s)severity\s*===\s*[''\"]error[''\"].*?[''\"]alert[''\"].*?[''\"]assertive[''\"]' -and $Source -match 'const\s+persistent\s*=\s*options\.persistent\s*\?\?\s*severity\s*!==\s*[''\"]info[''\"]' -and $Source -match 'if\s*\(\s*!persistent\s*\)\s*notify\.timer\s*=\s*window\.setTimeout'; Message = 'error notifications stay persistent and use assertive semantics' }
    )
}

function Assert-PagesAccessibility {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [switch]$Quiet
    )

    $failed = @()
    foreach ($check in Get-PagesAccessibilityChecks -Source $Source) {
        if (-not $check.Pass) {
            $failed += $check.Message
        }
        elseif (-not $Quiet) {
            Write-Host "PASS: $($check.Message)"
        }
    }
    return $failed
}

$failures = @(Assert-PagesAccessibility -Source $page)
if ($failures.Count -gt 0) {
    throw "Pages accessibility validation failed: $($failures -join '; ')"
}

if ($SelfTest) {
    $mutations = @(
        [pscustomobject]@{
            Name = 'palette destination focus removal is rejected'
            Source = [regex]::Replace($page, 'closePalette\s*\(\s*(?:false|\{\s*restoreFocus\s*:\s*false\s*\})\s*\)', 'closePalette()', 1)
            Expected = 'palette setting teleport retains focus on its destination'
        },
        [pscustomobject]@{
            Name = 'feature-name index removal is rejected'
            Source = $page.Replace('.feature-card h3', '.feature-card a')
            Expected = 'palette indexes feature names instead of article links alone'
        }
    )

    foreach ($mutation in $mutations) {
        $mutationFailures = @(Assert-PagesAccessibility -Source $mutation.Source -Quiet)
        if ($mutationFailures -notcontains $mutation.Expected) {
            throw "Pages accessibility self-test failed: $($mutation.Name)"
        }
        Write-Output "PASS mutation: $($mutation.Name)"
    }
    Write-Output "pages-accessibility-self-test mutations=$($mutations.Count)"
}
