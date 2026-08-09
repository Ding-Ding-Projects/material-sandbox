param([switch]$SelfTest)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$index = Get-Content (Join-Path $root 'docs/index.html') -Raw
$app = Get-Content (Join-Path $root 'docs/assets/app.js') -Raw
$css = Get-Content (Join-Path $root 'docs/assets/app.css') -Raw
$checks = @(
    [pscustomobject]@{ Pass = $index -match 'id="app"'; Message = 'app shell has a semantic mount point' },
    [pscustomobject]@{ Pass = $app -match 'role="tablist"' -and $app -match 'role="tab"' -and $app -match 'role="tabpanel"'; Message = 'tablist, tab, and tabpanel roles are present' },
    [pscustomobject]@{ Pass = $app -match 'aria-orientation' -and $app -match "vertical \? 'ArrowUp' : 'ArrowLeft'" -and $app -match "vertical \? 'ArrowDown' : 'ArrowRight'"; Message = 'tab keyboard navigation follows its dock axis' },
    [pscustomobject]@{ Pass = $app -match "event\.key === 'Home'" -and $app -match "event\.key === 'End'"; Message = 'tab keyboard navigation supports Home and End' },
    [pscustomobject]@{ Pass = $app -match 'aria-controls' -and $app -match 'aria-selected'; Message = 'tabs expose controlled panels and selection state' },
    [pscustomobject]@{ Pass = $css -match ':focus-visible' -and $css -match 'outline: 3px'; Message = 'interactive controls have visible focus' },
    [pscustomobject]@{ Pass = $css -match 'min-height: 44px'; Message = 'controls meet the touch-target baseline' },
    [pscustomobject]@{ Pass = $css -match '\.text-button \{ min-height: 44px' -and $css -match '\.tab-more \{ min-width: 44px; min-height: 44px' -and $css -match '\.toast button \{ min-height: 44px; min-width: 44px' -and $css -match '\.tab-group > summary \{ display: flex; width: 100%; min-height: 44px'; Message = 'compact text, tab-overflow, group, and toast controls meet the target baseline' },
    [pscustomobject]@{ Pass = $css -notmatch '\.tab-more\s*\{[^}]*display:\s*none'; Message = 'narrow layouts do not remove per-tab overflow actions' },
    [pscustomobject]@{ Pass = $css -match 'prefers-reduced-motion: reduce' -and $css -match 'body\[data-motion="reduced"\]'; Message = 'reduced-motion system and user settings are honored' },
    [pscustomobject]@{ Pass = $app -match 'aria-live="polite"' -and $app -match 'toast-stack'; Message = 'non-blocking notifications are announced' },
    [pscustomobject]@{ Pass = $index -notmatch 'id="app"[^>]*aria-live'; Message = 'workspace rerenders do not announce the whole application' },
    [pscustomobject]@{ Pass = $app -match '<dialog id="palette-dialog"' -and $app -match 'data-action="close-dialog"'; Message = 'dialogs have a native modal surface and close controls' },
    [pscustomobject]@{ Pass = $app -match 'contextMenuHandler' -and $app -match 'data-appearance-trigger'; Message = 'appearance controls expose pointer and visible activation paths' },
    [pscustomobject]@{ Pass = $app -match 'class="tab-row"><button id="tab-\$\{id\}" class="tab-button"' -and $app -match '</button><button class="tab-more"'; Message = 'tab primary and overflow buttons are sibling controls' },
    [pscustomobject]@{ Pass = $app -match 'activeDialogId' -and $app -match "dialog\.addEventListener\('cancel'"; Message = 'rerenders preserve dialogs and Escape returns focus' },
    [pscustomobject]@{ Pass = $css -match '@media \(max-width: 620px\)' -and $css -match 'overflow-x: auto'; Message = 'narrow layouts retain a scrollable tab route' },
    [pscustomobject]@{ Pass = $app -match 'function tabRailIsVertical' -and $app -match "matchMedia\('\(max-width: 900px\)'\)"; Message = 'responsive tab semantics follow the physical tab axis' },
    [pscustomobject]@{ Pass = $app -match 'const SETTINGS_SECTIONS' -and $app -match 'function renderSettingsTabs' -and $app -match 'data-settings-tab' -and $app -match "role', 'tablist'" -and $app -match "role', 'tabpanel'" -and $app -match 'scrollIntoView\(\{ block: ''nearest'', inline: ''nearest'' \}\)' -and $css -match '\.settings-tab-strip'; Message = 'settings sections are a keyboard-operable, revealed nested tab surface' },
    [pscustomobject]@{ Pass = $app -match "renderSearch\('tab-menu'" -and $app -match "renderSearch\('appearance-menu'" -and $app -match "renderSearch\('group-picker'"; Message = 'context menus and group picker carry their own regex-enabled search' },
    [pscustomobject]@{ Pass = $app -match 'function attachDialogSearches' -and $app -match "'appearance-dialog-search'" -and $app -match "'school-dialog-search'"; Message = 'appearance and focused-mode dialogs carry their own regex-enabled search' },
    [pscustomobject]@{ Pass = $app -match 'function setReturnFocus' -and $app -match 'function restoreReturnFocus' -and $app -match 'returnFocusSelector' -and $app -match "'data-delete-preset'"; Message = 'rerendered dialogs preserve a durable return-focus path' },
    [pscustomobject]@{ Pass = $app -match 'floatingMenu\?\.setAttribute\(''role'', ''dialog''\)' -and $app -match '#search-tab-menu' -and $app -match '#search-appearance-menu'; Message = 'context menus have a semantic surface and move focus to their search' },
    [pscustomobject]@{ Pass = $app -match 'function closeMenu\(restoreFocus = false\)' -and $app -match 'closeMenu\(true\)' -and $app -match 'menuState = \{ id: `appearance-\$\{target\}`, origin \}'; Message = 'Escape closes both menu variants and returns focus to their origin' },
    [pscustomobject]@{ Pass = $app -match 'const refreshFrom = \(source\)' -and $app -match 'replacement\?\.focus\(\{ preventScroll: true \}\)' -and $app -match 'setSelectionRange\(start, end \?\? start\)'; Message = 'rerendered detached searches preserve keyboard focus and caret position' },
    [pscustomobject]@{ Pass = $app -match 'preserveDialogFocusId' -and $app -match 'presentDialog\(dialogToRestore, \{ preserveFocus: preserveDialogFocus \}\)' -and $app -match "input\.closest\('dialog'\)\?\.id"; Message = 'dialog-contained searches preserve focus while their dialog rerenders' },
    [pscustomobject]@{ Pass = $app -match 'function positionAnchoredDialog' -and $app -match 'dialogAnchorSelector' -and $app -match "dialog\.dataset\.anchored = 'true'"; Message = 'appearance and group overlays use a viewport-bounded anchor' },
    [pscustomobject]@{ Pass = $app -match 'groupChoices\.forEach' -and $app -match "\['ArrowUp', 'ArrowDown', 'Enter'\]"; Message = 'group picker has arrow and Enter keyboard movement' },
    [pscustomobject]@{ Pass = $app -match 'function renderDestructiveDialog' -and $app -match 'destructiveConfirmation\.range >= 100' -and $css -match 'confirmation-authorized' -and $css -match '--confirmation-progress'; Message = 'destructive collection actions require an animated in-page super confirmation' }
)
foreach ($check in $checks) {
    if (-not $check.Pass) { throw "Pages accessibility validation failed: $($check.Message)" }
    Write-Output "PASS: $($check.Message)"
}
if ($SelfTest) {
    $mutations = @(
        [pscustomobject]@{ Name = 'semantic app mount'; Detected = -not (($index -replace 'id="app"', 'id="missing-app"') -match 'id="app"') },
        [pscustomobject]@{ Name = 'visible focus rule'; Detected = -not (($css -replace ':focus-visible', ':focus-gone') -match ':focus-visible') },
        [pscustomobject]@{ Name = 'tab role source'; Detected = -not (($app -replace 'role="tablist"', 'role="list"') -match 'role="tablist"') }
    )
    foreach ($mutation in $mutations) {
        if (-not $mutation.Detected) { throw "Pages accessibility self-test failed: $($mutation.Name)" }
        Write-Output "PASS mutation: $($mutation.Name)"
    }
    Write-Output "pages-accessibility-self-test mutations=$($mutations.Count)"
}
