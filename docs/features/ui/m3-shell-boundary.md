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
