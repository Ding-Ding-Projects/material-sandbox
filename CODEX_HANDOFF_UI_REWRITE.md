# Codex handoff — land and verify the Material Sandbox UI rewrite

## Mission

Apply this overlay to current `master`, keep `design/` as the UI contract, fix only concrete integration/build/runtime defects, and produce native evidence. Do not redesign the design, add another UI framework, or duplicate SandMan domain state.

## First actions

```powershell
git fetch --all --prune
git switch -c codex/complete-material-3-ui origin/master
py -3 <kit>\tools\apply_ui_rewrite.py . --report ui-rewrite-apply.json
node scripts/validate-m3-ui-rewrite.mjs --root .
git diff --check
```

Review every changed file before building.

## Required integration review

1. Confirm `CM3WorkspaceHost::install(this)` runs once after initial shell installation and again after `CSandMan::RebuildUI()` creates a replacement central widget; repeated calls while the current host is active must be no-ops.
2. Confirm `M3ShellHost::Refresh(this, m_pMenuBar)` runs after `CreateUI()` in `RebuildUI()` and the app bar points at the newly rebuilt QAction graph.
3. Confirm `M3ShellHost::InstallDialog` is reached by Settings and Box Options, then verify their original `QTabWidget` page count, object names, current index, and CTabStateManager persistence before/after adaptation.
4. Verify global native QMenu search does not change synchronous `exec()` return values or action visibility after close.
5. Map Recovery, Trace, Snapshots, Docs, and Settings destination adapters to embedded presentation adapters only when their real controllers can be shared safely. Never instantiate a second model/controller to make the page look embedded.
6. Connect snackbar calls only for transient feedback; persistent failures remain persistent.
7. Confirm toolbar actions retain configured order/state in overflow.

## Bug hunt

- Missing qmake/MSVC source registrations.
- MOC headers registered as plain headers.
- Duplicate shell/workspace installation.
- Duplicate QAction connections.
- QMenu actions left hidden after a filter closes.
- Focus not returned after menu/regex/dialog close.
- CTabStateManager index/order persistence regression.
- Frameless drag, maximize/restore, snap-layout, mixed-DPI hit-region failures.
- Hidden memory pages doing work; they must refresh only on construction/user action.
- Path traversal or symlink escape from the local memory root.
- Repeated navigation causing widget growth or duplicate timers.

## Stop condition

Do not call the rewrite release-ready until:

- repository validators pass;
- x64 and ARM64 Qt builds pass;
- bounded startup smoke passes;
- Settings and Options open, apply, cancel, reset, persist, and reopen;
- every top/context menu filters and restores correctly;
- light/dark, 100/150/200%, narrow width, and bilingual screens have real built-window evidence;
- no service/driver/security behavior changed;
- documentation states exact observed evidence only.

Keep fixes focused. Do not overengineer.
