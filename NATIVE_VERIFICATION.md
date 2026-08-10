# Native Windows verification

Run from the exact materialized source revision.

## 1. Static contract

```powershell
node scripts/validate-m3-ui-rewrite.mjs --root .
node scripts/validate-docs.mjs
node scripts/validate-screenshot-links.mjs
node scripts/validate-contributor-build.mjs
node scripts/validate-appearance-editor.mjs
node scripts/validate-color-translator.mjs
node scripts/validate-command-palette.mjs
node scripts/validate-notifications.mjs
node scripts/validate-scheduled-settings.mjs
node scripts/validate-school-mode.mjs
node scripts/validate-settings-history.mjs
node scripts/validate-tab-discovery.mjs
git diff --check
```

Run every additional validator named by the current `README.md`; the
repository may have advanced since this overlay was generated.

## 2. Native build

Use the repository-supported qmake/MSVC entry point. Build at minimum Release
x64, Release ARM64, and Debug x64 when supported by current CI. Record the
exact commit, toolchain, Qt version, command, exit code, and artifact hashes.

## 3. Runtime smoke

Start SandMan with a bounded timeout. Confirm one workspace host and one app
bar; open Boxes, Recovery, Trace, Snapshots, Docs, Sync, Skills, Memory, Ops,
Status, Settings; open/close Settings and Box Options ten times; check for
duplicate signals or obvious widget/timer growth; and confirm the normal
service boundary still carries application communication.

## 4. Menu matrix

Test top-level, box, group, process, tab, and toolbar-overflow menus. Verify
focus on open, case-insensitive plain filtering, bounded invalid-regex
feedback, separator handling, exact empty copy, Escape focus return, complete
action-visibility restoration, and the existing action return path.

## 5. Settings/Options parity

Capture page/object-name/index inventories before and after adaptation. Verify
every control reads, applies, cancels, resets, validates, persists, localizes,
and respects contributor and policy gates exactly as before.

## 6. Visual evidence

Capture real built windows in light and dark themes at 100%, 150%, and 200%
scaling, narrow width, and bilingual/Cantonese presentation. Cover Boxes,
Settings, Box Options, Notifications, Docs, Recovery, Trace, Snapshots,
searchable menus, the regex builder, snackbar, and destructive confirmation.
Design HTML captures are reference material, not runtime evidence.
