# Editor settings

The editor-settings surface is a native Material 3 dialog rather than a generated Qt Designer form. It presents the six editor preferences in an accessible table, keeps tri-state values explicit, and provides per-row and all-settings reset actions.

## Behaviour

- INI-key validation, INI and popup tooltips, completion consent, completion mode, and fuzzy matching retain their existing `Options/*` keys.
- Completion remains dependency-aware: consent enables completion, and fuzzy matching is available only when completion is active.
- Cancel restores the values captured when the dialog opened; Apply/OK records edits and reset operations through the existing settings path.

## Failure modes and security

Malformed or missing values fall back to the documented defaults. Reset removes the user override instead of writing a guessed replacement. The dialog does not collect credentials, contact a network service, or alter sandbox security policy.

## Verification

The static M3 contract validates the native layout, project registration, and removal of `Forms/EditorSettingsWindow.ui`. Native MSVC/Qt compilation remains covered by the hosted CI run for the exact commit; local qmake is intentionally unavailable in this checkout.

## Suggested articles

- [Material 3 shell boundary](ui/m3-shell-boundary.md)
- [Settings history](settings-history.md)
- [Appearance editor](appearance-editor.md)
