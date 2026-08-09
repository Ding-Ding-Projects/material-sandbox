# Material appearance editor

The Settings → Material appearance identity surface opens a native Material 3 editor for the installed UI font family, point size, weight, style, and accent seed. Every change is previewed before acceptance, persisted in the local profile, and applied to the running desktop shell without changing package identity or data paths.

## Configuration and reset

The editor stores `UIConfig/UIFontFamily`, `UIConfig/UIFontPointSize`, `UIConfig/UIFontWeight`, `UIConfig/UIFontStyle`, `UIConfig/UIFontUnderline`, `UIConfig/UIFontStrikeOut`, `UIConfig/UIFontOverline`, `UIConfig/UIFontCapitalization`, `UIConfig/UIFontLetterSpacing`, `UIConfig/UIFontWordSpacing`, `UIConfig/UIFontTextColor`, `UIConfig/UIFontHighlight`, and `UIConfig/AccentSeed`. **Reset to shipped defaults** clears the custom typography values and uses the platform general font plus the Material purple seed. The global appearance reset in Settings restores display name, density, and accent; the typography editor is the focused path for font values.

## Supported and unsupported properties

This bounded native slice supports installed family selection, point size, weight, style, underline style, strikeout, overline, capitalization, letter/word spacing, accent HEX/RGB/HSL translation, preview text/highlight colors, persistence, and reset. It does not claim full Word-depth editing: variable-font axes, line-height, baseline offset, superscript, subscript, and text effects remain explicitly unsupported because Qt's application-font API cannot apply those rich-text semantics consistently to every widget. Per-element font overrides are supported for real tab targets through **Edit tab appearance…**; they are stored under the tab-state blob's `appearance` object keyed by the stable tab key and contain only Qt-supported `QFont` attributes. The global editor keeps per-element controls out rather than pretending a global value can target an individual widget.

## Failure modes and security

An unavailable family falls back to the current Qt application font. Invalid colors remain in the translator's validation state and cannot be accepted. Values are bounded to 8–72 points, spacing is bounded to -20–100 px, and families come from installed Qt fonts; no network, credential, licensing, or signing path is involved.

## Verification

Run `node scripts/validate-appearance-editor.mjs`. For runtime verification, open Settings → UI → Material appearance identity → **Edit Material typography and color**, change each supported control, confirm the preview changes, accept, reopen to confirm persistence, and use reset before checking the shell at 100% and 200% scale. On a tab strip, use **Edit tab appearance…**, change the target font, restart the window, and confirm the stable-keyed override returns; line-height, baseline offset, superscript, and subscript remain documented as unsupported rather than rendered by a fake control.

Suggested articles: [Material Design 3 shell](material-design.md), [Color translator](color-translator.md), [Settings history](settings-history.md).
