# Implementation status

## Implemented in this source package

| Contract area | Implementation |
|---|---|
| App and memory color roles | `M3Tokens.h/.cpp`, consumed by `MaterialTheme.cpp` |
| Fixed shell geometry | 64 px app bar, 48 px tabs, 80 px rail, 40 px status, 56 px search, 40 px minimum targets |
| Shared search | `CM3SearchField`, plain text by default, optional bounded regex |
| Regex builder | Native Qt dialog, flags `i m s x U`, 500-character inputs, bounded matches/captures |
| Top menus | Live source QActions rendered by `CM3Menu` |
| Context menus | Global in-place search treatment preserves native `QMenu::exec()` behavior |
| Workspace shell | Existing central Boxes widget is taken and rehosted, not copied |
| Tabs | Stable IDs, close/reopen, move, context menu, search shortcut |
| Rail | Exact 11 destinations and contract grouping |
| Settings/Options presentation | Existing QTabWidget stays authoritative and is visually recomposed into a two-pane host |
| Memory surfaces | Five local/read-only pages with explicit unavailable states |
| Local file safety | Canonical root, traversal denial, UTF-8/text bounds, entry bounds |
| Integration | Idempotent qmake/MSVC/SandMan patcher, RebuildUI lifecycle refresh, and package materializer |

## Deliberately preserved rather than rewritten

- Sandbox/process/tree models.
- Existing Recovery, Trace, Snapshots, Docs, and Settings actions.
- Existing generated Settings/Options page objects, slots, option keys, and CTabStateManager ownership.
- Service, driver, IPC, certificate, installer, and security code.

Core destinations that cannot be safely instantiated without their application-specific constructor/controller context are represented by Material destination pages that trigger the existing live QAction. This is functional and avoids shadow models. A Windows-native follow-up may replace an adapter with an embedded presentation adapter only after exercising the real constructor/lifetime path.

## Not proven in this environment

- Qt/qmake or MSVC compilation.
- x64/ARM64 linking.
- SandMan startup.
- Loaded service/driver behavior.
- Real high-DPI, narrow-width, bilingual, light/dark screenshots.
- Exact runtime constructor compatibility for every downstream branch revision.

These are evidence gaps, not hidden claims. The exact verification sequence is in `NATIVE_VERIFICATION.md`.
