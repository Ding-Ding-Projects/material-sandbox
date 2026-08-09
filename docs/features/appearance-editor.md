# Material appearance editor

Settings → Material appearance identity provides a native Material 3 editor for installed font family, point size, weight, style, and the accent seed. Controls update a live preview and are persisted only after the user accepts the dialog; reset returns to the shipped platform font and Material purple seed.

## Configuration

The profile keys are `UIConfig/UIFontFamily`, `UIConfig/UIFontPointSize`, `UIConfig/UIFontWeight`, `UIConfig/UIFontStyle`, and `UIConfig/AccentSeed`. A zero or missing point size means the normal application scaling path supplies the size. The editor never changes package identifiers or data directories.

## Failure modes and security

If a stored family is unavailable, Qt falls back to its current application family. The accent translator rejects malformed or out-of-range values without replacing the last valid color. The editor is local-only and has no license, network, credential, or signing dependency.

## Verification

Run `node scripts/validate-appearance-editor.mjs`, then open the editor, change each supported control, accept, reopen to confirm persistence, and activate **Reset to shipped defaults**. The dialog explicitly lists unsupported Word-style properties; this slice does not claim variable axes, text effects, spacing, or per-element editing.

Suggested articles: [Material Design](../material-design.md), [Color translator](color-translator.md), [Settings history](settings-history.md).
