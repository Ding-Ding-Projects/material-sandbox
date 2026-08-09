# Material 3 shell boundary

## Behaviour

`M3ShellHost::Install` is the single chrome boundary for the SandMan desktop
window. It replaces the platform title bar with a frameless Material 3 title
bar, keeps the existing menu bar routable, and provides keyboard/touch-sized
minimize, maximize/restore, and close controls. Dragging the title surface
moves the window while maximized windows remain stable.

The host is idempotent (`m3ShellInstalled`) so `RebuildUI()` can recreate
content without stacking title bars. Existing views remain children beneath the
host and can be migrated one at a time; new screens should attach to the host
instead of creating another `QMainWindow` or custom title bar.

## Migration boundary

The legacy seams are `CSandMan` (`QMainWindow`), `SettingsWindow.ui`,
`OptionsWindow.ui`, and the dialogs under `SandMan/Windows`. `CustomStyles.h`
continues to style data controls but is no longer allowed to own product
chrome. Follow-up migrations should move one view at a time behind this host,
then remove its legacy form and proxy-style dependency. The central theme path
no longer constructs `Fusion` or wraps the application in the old proxy chrome;
data-view factories remain an explicitly tracked follow-up until each view is
rebuilt as a native M3 component.

The old Windows 11 style preference is hidden and written as `false` during
migration. Existing profiles therefore cannot bring a second product chrome
back after a restart.

## Dialog migration

Appearance, color translation, offline documentation, and destructive-action
dialogs now use the reusable `M3DialogHost`. It supplies a frameless title
surface, accessible close action, keyboard-sized target, and drag behavior while
preserving each dialog's existing content and decision semantics.

`M3ShellHost::InstallDialog` is the first dialog migration slice. `BoxImageWindow`
and `TestProxyDialog` retain their tested controls and async behavior while
their native dialog chrome is replaced by the same frameless M3 header, close
control, focus styling, and 40×32px target contract. `SelectBoxWindow` now also
owns its sandbox-selection controls in code, preserving grouped tree selection,
double-click launch, force-child/admin actions, and geometry persistence without
the retired Designer form. `BoxImageWindow` likewise owns its password, image
size, cipher, and protection controls in code while retaining action-specific
visibility and validation behavior.
`RecoveryWindow` now also owns its tree, target picker, filters, and recovery
actions in code while retaining the existing model, remembered targets, and
delete/recovery menu semantics.
`SnapshotsWindow` now likewise owns its snapshot tree, editable details, and
action group in code while retaining selection, defaults, autosave, and
snapshot-operation behavior.
`PopUpWindow` now owns its notification table in code and installs the shared
M3 main-window shell while retaining message batching, prompts, recovery, and
progress-entry behavior.
`OptionsWindow` now uses the same native M3 dialog host around its large
behavior-rich content. Its Designer form remains deliberately tracked as a
staged migration boundary until all controls have programmatic replacements;
this slice does not claim the old form is retired.
`SettingsWindow` now follows the same boundary: its nine-tab content keeps its
existing behavior while the native M3 host owns dialog chrome, and the exposed
Fusion style switch remains retired. The Designer form is still tracked until
all of its controls have safe programmatic replacements.
The Window Options child tab is now a concrete native slice: six monitor
selectors and the fallback hint are created in code and wired to the existing
settings handlers; the remaining child tabs stay staged rather than being
claimed as complete.
The shared MaterialTheme consumes the native-surface marker with a dedicated
group treatment, so the appearance slice changes rendered chrome rather than
only carrying validator metadata.
The GUI child tab is now another concrete native slice: DPI, font scaling, UI
font, external editor, and capture controls are created in code and reattached
to the existing persistence and signal paths; the remaining Settings tabs stay
explicitly staged.

The General Options child tab is now another concrete native slice: language,
option checkboxes, hotkey editors, and recovery controls are created in code
and retain the existing settings keys and signal paths; the remaining Settings
tabs stay explicitly staged.

The System Tray child tab is now another concrete native slice: tray selectors,
alias limits, tri-state status tips, and close behavior are created in code and
retain the existing persistence and update handlers; the remaining Settings
tabs stay explicitly staged.

The Run Menu child tab is now another concrete native slice: its editable
command tree and add/move/remove actions are created in code and retain the
existing command serialization and signal handlers; the remaining Settings
tabs stay explicitly staged.

The Windows Shell child tab is now another concrete native slice: startup,
context-menu, integration, and desktop controls are created in code while the
contributor-sensitive context-menu replacement and handlers remain intact; the
remaining Settings tabs stay explicitly staged.

The Tray and Run Menu replacements are owned by `tabsShell`; their native
install guards and validator coverage now target that owner, so both slices are
active at runtime rather than merely present in source.

The Add-On Configuration child tab is now another concrete native slice: Ram
Disk enablement, limit, drive-letter selection, and explanatory copy are
created in code while installation checks and persistence remain intact.

The Optional Add-Ons child tab is now another concrete native slice: its
add-on tree, install/remove actions, and update link are created in code while
the existing model and selection behavior remain intact.

The Program Alerts child tab is now another concrete native slice: warning-list
editing, alert actions, and message toggles are created in code while global
settings and signal handlers remain intact.

The Force Process Options child tab is now another concrete native slice: Mark
of The Web selection and force controls are created in code while sandbox model
population and global settings handlers remain intact.

The USB Drive Sandboxing child tab is now another concrete native slice: its
toggle, sandbox selector, and volume tree are created in code while sandbox
model population and enablement handlers remain intact.

The Local Templates child tab is now another concrete native slice: its filter,
multi-select tree, and add/open/remove actions are created in code while
pending-change tracking and template handlers remain intact.

The App Compatibility child tab is now another concrete native slice: its
detected-app tree, enable/disable actions, update link, and preference toggle
are created in code while compatibility-template handlers remain intact.

The Sandboxie.ini child tab is now another concrete native slice: portable-box
imports, config protection, and password actions are created in code while
protection persistence and handlers remain intact.

Its portable-box action row uses a two-column grid and its import-path column
stretches to the available width, preventing the final action or long bilingual
paths from clipping on compact windows.

The Notifications child tab is now a concrete native slice: message options,
message tree, and add/delete actions are created in code and retain the
existing persistence and handlers; the remaining Settings tabs stay staged.

The Sandboxie Updater child tab is now another concrete native slice: channel
radios, cadence, version links, and update toggles are created in code while
certificate gating, release state, and updater handlers remain intact.

App Compatibility actions now use a responsive vertical action stack with
explicit accessible names and a wrapping update link, keeping narrow bilingual
layouts usable.

The Edit ini Section child tab is now another concrete native slice: its
toolbar, validation/completion toggles, editor settings, and plain-text buffer
are created in code while edit/save/cancel handlers remain intact.

OptionsWindow File Options is now another native slice: virtualization,
encryption, deletion, and disk-access controls are created in code while the
OptionsGeneral pointers and handlers remain intact; other OptionsWindow tabs
remain an explicit staged boundary.

OptionsWindow File Migration is now another native slice: copy-limit settings,
prompts, wildcard rules, and actions are created in code while copy-rule
persistence and handlers remain intact; other OptionsWindow tabs remain staged.

OptionsWindow Restrictions is now another native slice: printing and
system-protection controls are created in code while the existing persistence
and handlers remain intact; other OptionsWindow tabs remain staged.

OptionsWindow Isolation is now another native slice: access-isolation toggles
and their safety guidance are created in code while OptionsAdvanced keeps the
existing persistence, enablement, and signal behavior; other OptionsWindow
tabs remain staged.

OptionsWindow Run Menu is now another native slice: its editable command tree
and add/remove/reorder action rail are created in code while the existing
command serialization, sorting, and handlers remain intact; other
OptionsWindow tabs remain staged.

OptionsWindow Advanced Security is now another native slice: privilege and
Sandboxie-token controls are created in code while OptionsAdvanced keeps the
existing persistence, tristate semantics, and dependency rules; other
OptionsWindow tabs remain staged.

OptionsWindow Security Isolation is now another native slice: compatibility
guidance, security-filtering switches, and desktop-isolation controls are
created in code while OptionsAdvanced keeps the existing safety dependencies
and persistence; other OptionsWindow tabs remain staged.

OptionsWindow Box Protection is now another native slice: confidentiality,
protected-window policy, host-process rules, and template actions are created
in code while OptionsAdvanced keeps the existing access-entry handlers and
persistence; other OptionsWindow tabs remain staged.

OptionsWindow Job Object is now another native slice: nested-job policy and
memory, process-count, and CPU limits are created in code with responsive
label/value rows while OptionsAdvanced keeps numeric validation, dynamic
enablement, and persistence; other OptionsWindow tabs remain staged.

OptionsWindow Program Groups is now another native slice: its group tree and
add/remove/template action rail are created in code while the existing group
delegates, model, and handlers remain intact; compact layout moves reuse the
same native page and other OptionsWindow tabs remain staged.

OptionsWindow Force Programs is now another native slice: its force-rule tree,
browse actions, template toggle, and disable switch are created in code while
the existing delegates, menus, handlers, and persistence remain intact; other
OptionsWindow tabs remain staged.

OptionsWindow Breakout Programs is now another native slice: its breakout-rule
tree, browse actions, template toggle, remove action, and security advisory are
created in code while the existing delegates, menus, handlers, and persistence
remain intact; other OptionsWindow tabs remain staged.

## Failure and accessibility notes

The host is deliberately compile-safe on Qt 5 and Qt 6. All window controls
have accessible names, tooltips, visible focus styling, and a minimum 40×32px
target. The host never blocks startup or opens a dialog. If a platform window
manager rejects frameless flags, Qt keeps the same central content and the
controls remain available through the host surface.

## Verification

The source validator checks that the shell and dialog hosts are present in
`SandMan.pri`, that the constructor installs the shell after
`setCentralWidget`, that migrated dialogs install the dialog host, and that both
hosts are guarded against duplicate installation. A native compile remains part
of the Windows CI gate because this checkout does not guarantee a Qt SDK on
every agent.
