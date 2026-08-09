# Material appearance editor

The Settings → Material appearance identity surface opens a native Material 3 editor for the installed UI font family, point size, weight, style, and accent seed. Every change is previewed before acceptance, persisted in the local profile, and applied to the running desktop shell without changing package identity or data paths.

## Configuration and reset

The editor stores `UIConfig/UIFontFamily`, `UIConfig/UIFontPointSize`, `UIConfig/UIFontWeight`, `UIConfig/UIFontStyle`, and `UIConfig/AccentSeed`. **Reset to shipped defaults** clears the custom point size and uses the platform general font plus the Material purple seed. The global appearance reset in Settings restores display name, density, and accent; the typography editor is the focused path for font values.

## Supported and unsupported properties

This bounded native slice supports installed family selection, point size, weight, style, accent HEX/RGB/HSL translation, alpha, preview, persistence, and reset. It does not yet claim full Word-depth editing: variable-font axes, underline variants, strikethrough, overline, capitalization, small caps, superscript, subscript, text effects, character or word spacing, baseline offset, and per-element overrides remain explicitly listed as unsupported in the dialog.

## Failure modes and security

An unavailable family falls back to the current Qt application font. Invalid colors remain in the translator's validation state and cannot be accepted. Values are bounded to 8–72 points and installed Qt font families; no network, credential, licensing, or signing path is involved.

## Verification

Run `node scripts/validate-appearance-editor.mjs`. For runtime verification, open Settings → UI → Material appearance identity → **Edit Material typography and color**, change each supported control, confirm the preview changes, accept, reopen to confirm persistence, and use reset before checking the shell at 100% and 200% scale.

Suggested articles: [Material Design 3 shell](material-design.md), [Color translator](color-translator.md), [Settings history](settings-history.md).
