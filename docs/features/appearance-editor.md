# Material appearance editor

Settings → Material appearance identity provides a native Material 3 editor for installed font family, point size, weight, style, decorations, capitalization, letter/word spacing, and the accent seed. Controls update a live preview and are persisted only after the user accepts the dialog; reset returns to the shipped platform font and Material purple seed.

## Configuration

The profile keys are `UIConfig/UIFontFamily`, `UIConfig/UIFontPointSize`, `UIConfig/UIFontWeight`, `UIConfig/UIFontStyle`, `UIConfig/UIFontUnderline`, `UIConfig/UIFontStrikeOut`, `UIConfig/UIFontOverline`, `UIConfig/UIFontCapitalization`, `UIConfig/UIFontLetterSpacing`, `UIConfig/UIFontWordSpacing`, and `UIConfig/AccentSeed`. A zero or missing point size means the normal application scaling path supplies the size. The editor never changes package identifiers or data directories. Per-tab overrides are keyed by each tab's stable tab-state key.

## Failure modes and security

If a stored family is unavailable, Qt falls back to its current application family. The accent translator rejects malformed or out-of-range values without replacing the last valid color. The editor is local-only and has no license, network, credential, or signing dependency.

## Verification

Run `node scripts/validate-appearance-editor.mjs`, then open the editor, change each supported control, accept, reopen to confirm persistence, and activate **Reset to shipped defaults**. The dialog and page-target editor explicitly disclose that line-height, baseline offset, superscript, subscript, underline variants, and text effects are unsupported. Qt 6.8 can represent variable axes, but this editor does not expose or persist them until global widget-style behavior is verified. The stable-keyed per-tab override styles the tab page content, not its `QTabBar` label, and uses only safe `QFont` properties rather than fabricating rich-text layout controls.

Suggested articles: [Material Design](../material-design.md), [Color translator](color-translator.md), [Settings history](settings-history.md).
