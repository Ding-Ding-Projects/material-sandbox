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

The Immediate Recovery child tab is now a focused native slice: its prompt
check box, ignore tree, and add/remove actions are constructed in code with
Material 3 surface metadata. The existing object names, recovery handlers,
template persistence, and recovery-tab merge remain unchanged, so the
Designer layout can be retired incrementally without changing user data.

Quick Recovery now follows the same path for its folder tree, template toggle,
and add/remove actions. The recovery list still feeds the existing model and
event filters; only the visible controls are native M3, leaving the surrounding
tab merge as a clearly bounded follow-up.

The Dlls & Extensions child tab now constructs its DLL tree and host-image
protection toggles in code as well. Existing enablement signals, explanatory
tooltips, sorting, and persisted protection values are retained while the
Designer controls are retired incrementally.

Advanced Options > Miscellaneous now does the same for its option tree,
add/remove actions, and template toggle. Embedded per-row editors, selection
filters, and template refresh behavior keep their existing object names and
signals; the generated grid remains only as a temporary migration seam.

Advanced Options > Users now uses a native M3 account list, add/remove actions,
and administrator-only monitor toggle. Account selection and monitor settings
continue to load and save through the existing handlers, with the surrounding
Designer grid retained only as a bounded migration seam.

Advanced Options > Tracing now rebuilds every trace switch as a native M3
checkbox, copying the shipped labels and tooltips. Trace signal wiring and
configuration persistence remain unchanged while the generated grid stays a
temporary, explicit migration seam.

The Config Dump child surface now uses native M3 filter switches and Refresh
action while retaining its dynamically supplied configuration tree and exact
dump semantics. The tree remains a data view; only its surrounding controls
are part of this bounded migration slice.

App Templates > Templates now uses native M3 category/text filters, template
tree, and actions. Extended selection, popup menus, filtering, and template
persistence continue through the same object names and handlers while the
generated grid remains an explicit migration seam.

App Templates > Template Folders now hosts its editable path delegates inside
a native M3 tree. Folder loading, in-cell path editors, sorting, and global
persistence continue unchanged; only the tree host is part of this bounded
slice.

App Templates > Accessibility now uses a native M3 screen-reader switch while
retaining the `ScreenReader` template setting, tooltip, and handler wiring.

The Edit ini Section surface now builds its validation, completion, tooltip,
editor, and save/cancel controls as native M3 widgets. The later code-editor
upgrade, INI validation, and persistence workflow continue using the same
object names and pointers.

Advanced Options > Processes now uses native M3 process-tree, hiding-switch,
WMI guard, and add/remove controls. Templates, tooltips, event filters, and
persisted process rules remain on the existing handlers and object names.

Advanced Options > Privacy now uses native M3 firmware, identifier, serial,
MAC, locale, and dump controls. Privacy persistence and firmware-table export
remain unchanged with the same object-name and signal wiring.

Various Options > Compatibility now rebuilds every compatibility workaround
switch as a native M3 control while preserving labels, tooltips, and box-setting
persistence.

Advanced Options > Triggers now uses a native M3 event tree, trigger actions,
remove action, and template toggle. Popup menus, template filtering, event
signals, and persistence continue through the existing object names.

Advanced Options > Debug now wraps its dynamically generated debug switches in
a native M3 scroll host while preserving the existing `dbgLayout` and runtime
checkboxes.

The Debug Options and Config Dump pages now share a native M3 tab host. Current
page state, dynamic controls, and Config Dump refresh-on-tab-change behavior
remain unchanged.

The App Templates surface now uses a native M3 tab host for Templates, Template
Folders, and Accessibility, preserving page order and each page's controls.

Various Options now uses a native M3 tab host for Compatibility and Dlls &
Extensions, preserving page state and the migrated child controls.

Advanced Options now uses a native M3 tab host for Miscellaneous, Triggers,
Processes, Privacy, Users, Tracing, and Debug. Dynamic Debug removal, page
order, icons, and migrated child controls remain intact.

General Options now uses a native M3 tab host for Box, File, Migration,
Restrictions, Isolation, and Run pages while preserving page order, icons, and
child behavior.

Security Options now uses a native M3 tab host for Security, Isolation,
Protection, Job, and Advanced Security pages while preserving state, order,
icons, and migrated child behavior.

Program Control now uses a native M3 tab host for Force and Breakout pages;
later dynamic Group, Stop, and Start pages continue to append through the same
host.

Resource Access now uses a native M3 tab host for Files, Registry, IPC, Wnd,
COM, and Access Policies pages while preserving page order, state, icons, and
migrated child behavior.

Stop Options now uses native M3 controls for lingering-window and leniency
switches while preserving stop-change handlers, persistence, and the later
stop-tab merge.

Leader Programs now uses native M3 controls for Add, Remove, and Show
Templates while preserving the program tree, handlers, and template state.

Start Restrictions now uses native M3 policy radios, actions, and warning
switches while preserving start handlers, tree state, and the hidden template
toggle.

Lingering Programs now uses native M3 Add, Remove, and Show Templates controls
while preserving the editable program tree, delegate, and stop handlers.

Leader Programs now also uses a native QTreeWidget host configured from the
Designer tree's columns, sorting, selection, and edit behavior; its existing
model class and handlers remain unchanged.

Lingering Programs now uses the same native QTreeWidget host treatment,
preserving its columns, sorting, selection, edit behavior, delegate, and stop
event flow.

Start Restrictions now uses the same native QTreeWidget host treatment,
preserving its columns, sorting, selection, edit behavior, delegate, and policy
event flow.

Settings GUI now uses a native M3 tab host around its native child pages,
preserving page order, labels, current selection, and icon/state wiring.

Settings Shell now uses a native M3 tab host around Windows Shell, System Tray,
and Run Menu pages, preserving page order, labels, current selection, and
settings pointer wiring.

Settings General now uses a native M3 tab host around General Options and
Notifications pages, preserving page order, labels, current selection, and
settings wiring.

Settings Add-Ons now uses a native M3 tab host around Optional Add-Ons and
Add-On Configuration pages, preserving page order, labels, current selection,
and add-on model wiring.

Settings Control now uses a native M3 tab host around Program Alerts, Force
Process Options, and USB Drive Sandboxing pages, preserving page order, labels,
current selection, and control wiring.

Settings Templates now uses a native M3 tab host around Local Templates and App
Compatibility pages, preserving page order, labels, current selection, and
template model wiring.

Settings Advanced now uses a native M3 tab host around Sandbox Options and
Sandboxie.ini pages, preserving page order, labels, current selection, and
settings wiring.

The Settings outer tab host now uses native M3 controls around every migrated
page, preserving labels, icons, order, current selection, and tab-state wiring.

Settings Support now uses a native M3 tab host around the Sandboxie Updater
page, preserving its label, icon, current selection, and update handlers.

The `OptionsWindow.ui` and `SettingsWindow.ui` form registrations remain
intentional: live generated `ui.*` pointers still supply model-backed controls
and persistence paths that have not yet been replaced. Removing either `.ui`
from `SandMan.pri` before those controls move would break compilation, so the
remaining boundary is limited to those documented control clusters.

Stop, Leader, and Start guidance labels (`label_2`, `label_71`, and `label_4`) are now rebuilt by
object name with native M3 metadata while preserving text, wrapping, and
formatting; model-backed trees and controls remain the deliberate form boundary.

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

The Debug tab host now keeps its tabs above the content in a predictable focus
order, and both the tab host and scroll surface expose explicit accessible
names. The M3 contract validator guards these choices so a bottom-only strip or
unnamed surface cannot silently return.

All nested Options tab hosts now expose localized accessible names and left-dock
their strips consistently with the app-level tab contract. This keeps keyboard
navigation and tab discovery predictable across Templates, Various, Advanced,
General, Security, and Program Control pages.

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

OptionsWindow Start Restrictions is now another native slice: start-policy
radios, the editable program list, and warning controls are created in code
while the existing radio semantics, delegate, and persistence remain intact;
the intentionally hidden template toggle and other OptionsWindow tabs remain
staged.

OptionsWindow Resource Access > Files is now another native slice: its
four-column access-rule table and add/remove/template action rail are created
in code while the existing access model, add-menu semantics, templates, and
deletion handlers remain intact; other OptionsWindow tabs remain staged.

OptionsWindow Resource Access > Registry is now another native slice: its
four-column registry-rule table and add/remove/template action rail are created
in code while the existing access model, add-menu semantics, templates, and
deletion handlers remain intact; other OptionsWindow tabs remain staged.

OptionsWindow Resource Access > IPC is now another native slice: its
four-column IPC-rule table and add/remove/template action rail are created in
code while the existing process targeting, access model, add-menu semantics,
templates, and deletion handlers remain intact; other OptionsWindow tabs remain
staged.

OptionsWindow Resource Access > Wnd is now another native slice: its
four-column window-class table, add/remove/template rail, and no-rename policy
control are created in code while the existing access model and handlers remain
intact; other OptionsWindow tabs remain staged.

OptionsWindow Resource Access > COM is now another native slice: its
four-column COM-object table, add/remove/template rail, and virtualization
policy control are created in code while the existing access model, templates,
deletion handlers, and persistence remain intact; other OptionsWindow tabs
remain staged.

OptionsWindow Resource Access > Access Policies is now another native slice:
privacy, rule-specificity, and boxed-access policy controls plus their concise
guidance are created in code while the existing dependency and persistence
rules remain intact; other OptionsWindow tabs remain staged.

OptionsWindow Network > Process Restrictions is now another native slice: its
network mode row, prompt/message toggles, editable per-program table, and
action rail are created in code while OptionsNetwork keeps embedded editors,
dependencies, and persistence intact; other OptionsWindow tabs remain staged.

OptionsWindow Network > Firewall is now another native slice: its five-column
rule table, add/remove/template rail, test fields, and WFP caution are created
in code while OptionsNetwork keeps embedded rule editors, testing behavior, and
persistence intact; other OptionsWindow tabs remain staged.

OptionsWindow Network > DNS Filter is now another native slice: its
per-process domain/IP table, add/remove action rail, and explanatory guidance
are created in code while OptionsNetwork keeps delegates, editing, and
delete/persistence behavior intact; other OptionsWindow tabs remain staged.

OptionsWindow Network > Internet Proxy is now another native slice: its
seven-column proxy editor table and add/test/reorder/remove rail are created in
code while OptionsNetwork keeps embedded editors, proxy testing, ordering,
hidden legacy switches, and persistence intact; other OptionsWindow tabs remain
staged.

OptionsWindow Network > Other Options is now another native slice: port and
network restriction toggles, adapter binding, and IPv4/IPv6 fields are created
in code while OptionsNetwork keeps adapter population, read-only rules, and
persistence intact; other OptionsWindow tabs remain staged.

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
