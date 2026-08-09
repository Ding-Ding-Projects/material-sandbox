# Changelog

## 2026-08-09 · Native Settings App Compatibility tab

Commit: [dab079866c9160de106328cc5dba37c69b68c585](https://github.com/Ding-Ding-Projects/material-sandbox/commit/dab079866c9160de106328cc5dba37c69b68c585)

- Rebuilt the App Compatibility child tab with native Material 3 detected-app tree, enable/disable actions, and preference toggle.
- Preserved compatibility-template handlers and preference persistence.

## 2026-08-09 · Native Settings Local Templates tab

Commit: [4b6ec4969a0c417560d905a7566d2161b4e3951d](https://github.com/Ding-Ding-Projects/material-sandbox/commit/4b6ec4969a0c417560d905a7566d2161b4e3951d)

- Rebuilt the Local Templates child tab with native Material 3 filter, multi-select tree, and template actions.
- Preserved pending-change tracking, template menus, and existing handlers.

## 2026-08-09 · Native Settings USB sandbox tab

Commit: [aacf96e5be483ba0ec9f8adf8912225b544e793d](https://github.com/Ding-Ding-Projects/material-sandbox/commit/aacf96e5be483ba0ec9f8adf8912225b544e793d)

- Rebuilt the USB Drive Sandboxing child tab with native Material 3 toggle, sandbox selector, and volume tree.
- Preserved sandbox-model population and enablement handlers.

## 2026-08-09 · Native Settings Force Process tab

Commit: [b5b3cdc6689a0cd467bec058f775246a050aaff6](https://github.com/Ding-Ding-Projects/material-sandbox/commit/b5b3cdc6689a0cd467bec058f775246a050aaff6)

- Rebuilt the Force Process Options child tab with native Material 3 Mark of The Web selector and force controls.
- Preserved model-populated sandbox choices and global settings handlers.

## 2026-08-09 · Native Settings Program Alerts tab

Commit: [55213da2d0ec319baae5ef961d66929434bed47d](https://github.com/Ding-Ding-Projects/material-sandbox/commit/55213da2d0ec319baae5ef961d66929434bed47d)

- Rebuilt the Program Alerts child tab with native Material 3 warning tree, actions, and message toggles.
- Preserved global settings, warning-list editing, and existing signal handlers.

## 2026-08-09 · Native Settings Optional Add-Ons list

Commit: [dddeb77dad72ef484f58dae938edd07e94b7d0a4](https://github.com/Ding-Ding-Projects/material-sandbox/commit/dddeb77dad72ef484f58dae938edd07e94b7d0a4)

- Rebuilt the Optional Add-Ons child tab with native Material 3 add-on tree, install/remove actions, and update link.
- Preserved the existing add-on model and selection behavior.

## 2026-08-09 · Complete Day Teet Hui article inventory

Commit: [afffeccc09c69f76651d635f510491cfb26b530f](https://github.com/Ding-Ding-Projects/material-sandbox/commit/afffeccc09c69f76651d635f510491cfb26b530f)

- The landing page now exposes all 21 bundled feature articles, including the changelog viewer and Pages language/accessibility contracts.
- The validator requires that exact indexed list, preventing new articles from disappearing from the first surface users see.

## 2026-08-09 · Native Settings Add-On Configuration tab

Commit: [27525843940b9acb2efd9af6597bb0c97d8ceeef](https://github.com/Ding-Ding-Projects/material-sandbox/commit/27525843940b9acb2efd9af6597bb0c97d8ceeef)

- Rebuilt the Add-On Configuration child tab with native Material 3 Ram Disk controls.
- Preserved installation checks, enablement rules, drive-letter selection, and persistence.

## 2026-08-09 · Correct native shell tab ownership

Commit: [8c1f71ff1f527f8a25b3a65a1fabc838a72a9e77](https://github.com/Ding-Ding-Projects/material-sandbox/commit/8c1f71ff1f527f8a25b3a65a1fabc838a72a9e77)

- Tray and Run Menu native replacement widgets now use `tabsShell` as their owner, matching their actual tab container and preventing a parent/lifetime mismatch.

## 2026-08-09 · Activate native Shell child tabs

Commit: [cf86c95a182f4a9f49db9dd58191c732d043fa7e](https://github.com/Ding-Ding-Projects/material-sandbox/commit/cf86c95a182f4a9f49db9dd58191c732d043fa7e)

- Corrected the Tray and Run Menu native replacements to target their actual `tabsShell` owner, activating the migrated controls at runtime.

## 2026-08-09 · Native Settings Windows Shell tab

Commit: [5c4d93534be1115adf2bc15b1c0e0efaa2d9a02f](https://github.com/Ding-Ding-Projects/material-sandbox/commit/5c4d93534be1115adf2bc15b1c0e0efaa2d9a02f)

- Rebuilt the Settings Windows Shell child tab with native Material 3 startup, context-menu, integration, and desktop controls.
- Preserved contributor-sensitive context-menu replacement, settings persistence, and existing handlers.

## 2026-08-09 · Native Settings Run Menu tab

Commit: [00810801287f84a2f228b3d222c763ea7a71ab7d](https://github.com/Ding-Ding-Projects/material-sandbox/commit/00810801287f84a2f228b3d222c763ea7a71ab7d)

- Rebuilt the Settings Run Menu child tab with native Material 3 command tree and reorder actions.
- Preserved custom command serialization, selection behavior, and existing signal handlers.

## 2026-08-09 · Native Settings System Tray tab

Commit: [00b68f7d93e602816d42db6376f3d5a5de834adf](https://github.com/Ding-Ding-Projects/material-sandbox/commit/00b68f7d93e602816d42db6376f3d5a5de834adf)

- Rebuilt the Settings System Tray child tab with native Material 3 selectors, alias limits, tri-state status tips, and close behavior.
- Preserved tray settings persistence, live update handlers, and existing control semantics.

## 2026-08-09 · Native Settings General tab

Commit: [454ff6e40ebb33bdddc5c77d6860b175d6de68e2](https://github.com/Ding-Ding-Projects/material-sandbox/commit/454ff6e40ebb33bdddc5c77d6860b175d6de68e2)

- Rebuilt the Settings General Options child tab with native Material 3 language, checkbox, hotkey, and recovery controls.
- Preserved the existing settings keys, shortcut editors, signal handlers, and elevation behavior.

## 2026-08-09 · Day Teet Hui keyboard and overlay semantics

Commit: [693e09d16f8d1f86a2ecc144eb050a657131ea76](https://github.com/Ding-Ding-Projects/material-sandbox/commit/693e09d16f8d1f86a2ecc144eb050a657131ea76)

- Tab navigation now exposes explicit horizontal semantics, roving focus, Home/End movement, and semantically hidden inactive panels.
- The command palette now exposes its open state, traps focus, locks background scrolling, and restores focus after close.

## 2026-08-09 · Complete Day Teet Hui feature inventory

Commit: [c84586d554191d4d7a0bcfc402215375e915665c](https://github.com/Ding-Ding-Projects/material-sandbox/commit/c84586d554191d4d7a0bcfc402215375e915665c)

- The landing page now exposes all 18 feature articles from the hand-written index, including destructive confirmation, native CI evidence, and editor settings.
- The Pages validator requires that exact article list and rejects missing or extra feature cards.

## 2026-08-09 · Contributor capability initialization and Pages language/tone wiring

Commits: [33202613770e9bb4c1329827fb88b395cb00e26e](https://github.com/Ding-Ding-Projects/material-sandbox/commit/33202613770e9bb4c1329827fb88b395cb00e26e), [4730fbad267a86686cd5fdff6985b050b1f64d95](https://github.com/Ding-Ding-Projects/material-sandbox/commit/4730fbad267a86686cd5fdff6985b050b1f64d95)

- Contributor builds initialize full capability flags before certificate I/O, so missing or malformed certificate data cannot disable security, encryption, networking, image, USB, update, or breakout features.
- The Day Teet Hui language mode and independent English/Cantonese funny levels now change real rendered hero and status copy with bounded, safe text updates.

## 2026-08-09 · Native Settings notifications tab

Commit: [2b3cef9e573317efe833a9f72b1cff5a77835ebb](https://github.com/Ding-Ding-Projects/material-sandbox/commit/2b3cef9e573317efe833a9f72b1cff5a77835ebb)

- Rebuilt the Settings notifications child tab with native Material 3 notification options, message tree, and actions.
- Preserved message filters, tree edits, settings persistence, and existing signal handlers.

## 2026-08-09 · Native Settings GUI tab

Commit: [0603d8fc11912a045251602bca639cd26ad1fdb9](https://github.com/Ding-Ding-Projects/material-sandbox/commit/0603d8fc11912a045251602bca639cd26ad1fdb9)

- Rebuilt the Settings GUI child tab with native Material 3 DPI, font scaling, UI font, external editor, and capture controls.
- Retained the existing generated member pointers, persistence paths, and signal handlers while leaving unrelated tabs staged.

## 2026-08-09 · Native Material 3 surface token consumption

Commit: [69a8c1a0fe310bb26d0f44f345395150e67d7ecb](https://github.com/Ding-Ding-Projects/material-sandbox/commit/69a8c1a0fe310bb26d0f44f345395150e67d7ecb)

- MaterialTheme now consumes the native settings-surface marker with a dedicated M3 group treatment, making appearance metadata affect rendered chrome.

## 2026-08-09 · Retired legacy theme controls

Commit: [2081b5d9ab793667acf710066a81ba62dceda6e6](https://github.com/Ding-Ding-Projects/material-sandbox/commit/2081b5d9ab793667acf710066a81ba62dceda6e6)

- Removed the obsolete Fusion and Windows 11 style controls from SettingsWindow.
- Preserved one-way migration keys while keeping Material 3 as the only rendered chrome path.

## 2026-08-09 · Native settings window-options tab

Commit: [3e2b189577bb18a629c72f1d0e9c4e95db01b1ad](https://github.com/Ding-Ding-Projects/material-sandbox/commit/3e2b189577bb18a629c72f1d0e9c4e95db01b1ad)

- Rebuilt the Settings window's Window Options tab with six native Material 3 monitor selectors and a wrapped explanatory hint.
- Kept generated object names, monitor option persistence, and existing change handlers; the other Settings tabs remain an explicit staged boundary.

## 2026-08-09 · Native M3 settings dialog shell

Commit: [dcd3f5f45ad2e6fc61cbb9f83803ee590feea6a0](https://github.com/Ding-Ding-Projects/material-sandbox/commit/dcd3f5f45ad2e6fc61cbb9f83803ee590feea6a0)

- Applied the shared frameless Material 3 host to the nine-tab global settings surface.
- Retired the visible Fusion switch and preserved the existing settings content as an explicit staged migration boundary; no controls were dropped.

## 2026-08-09 · Native M3 options dialog shell

Commit: [257440d19e43b025af0f0b1cc5ec5e79a4fbef4c](https://github.com/Ding-Ding-Projects/material-sandbox/commit/257440d19e43b025af0f0b1cc5ec5e79a4fbef4c)

- Applied the shared frameless Material 3 dialog host to the large options surface.
- Kept its 176 behavior-rich controls and `OptionsWindow.ui` intact as an explicit staged migration boundary; no controls were silently dropped.

## 2026-08-09 · Native notification popup

Commit: [5424988941371677e9a983c859aadab01703602e](https://github.com/Ding-Ding-Projects/material-sandbox/commit/5424988941371677e9a983c859aadab01703602e)

- Replaced `PopUpWindow.ui` with a native Material 3 notification table and frameless shell.
- Preserved batched messages, prompts, quick-recovery entries, progress indicators, copy, and geometry behavior.

## 2026-08-09 · Native snapshots dialog

Commit: [dd8d085ba7e51d405b59fe4c22acc863fb611ec9](https://github.com/Ding-Ding-Projects/material-sandbox/commit/dd8d085ba7e51d405b59fe4c22acc863fb611ec9)

- Replaced `SnapshotsWindow.ui` with native Material 3 snapshot tree, details editor, and action controls.
- Preserved snapshot selection, default toggling, info autosave, take/remove operations, and geometry persistence.

## 2026-08-09 · Native recovery dialog

Commit: [2731f5c79014350fae265fa58fe3bb9d588a2996](https://github.com/Ding-Ding-Projects/material-sandbox/commit/2731f5c79014350fae265fa58fe3bb9d588a2996)

- Replaced `RecoveryWindow.ui` with native Material 3 recovery controls for the file tree, target picker, filters, and actions.
- Preserved grouped file recovery, refresh/add-folder flows, delete menus, remembered targets, and geometry persistence.

## 2026-08-09 · Native box image password dialog

Commit: [cabbef7aae1019c95e81e10e4997a9b25a3f0155](https://github.com/Ding-Ding-Projects/material-sandbox/commit/cabbef7aae1019c95e81e10e4997a9b25a3f0155)

- Replaced `BoxImageWindow.ui` with native Material 3 password, image-size, cipher, and protection controls.
- Preserved action-specific visibility, password validation, image-size checks, and force-protection behavior.

## 2026-08-09 · Native sandbox picker dialog

Commit: [032a3b0b71d6a949e55f17fe51ff55342a123609](https://github.com/Ding-Ding-Projects/material-sandbox/commit/032a3b0b71d6a949e55f17fe51ff55342a123609)

- Replaced `SelectBoxWindow.ui` with native Material 3 controls for sandbox selection, new-box, outside-sandbox, force-child, and administrator actions.
- Preserved grouped tree selection, double-click launch, geometry persistence, and the existing run semantics.

## 2026-08-09 · Native proxy and image dialogs

Commit: [3f2dfb141f76a3acce2ff9c51ea210455d9d23b4](https://github.com/Ding-Ding-Projects/material-sandbox/commit/3f2dfb141f76a3acce2ff9c51ea210455d9d23b4)

- Rebuilt the proxy test dialog in native Material 3 controls and removed its Designer form.
- Routed the image dialog through shared frameless M3 chrome and kept accessible close/focus behavior.
- Removed the remaining pending-change legacy style factory so highlights use the live Material palette.

## 2026-08-09 · Native editor settings dialog

Commit: [a371d3d674db6d49c8928ef98bd8b91d5e6dc696](https://github.com/Ding-Ding-Projects/material-sandbox/commit/a371d3d674db6d49c8928ef98bd8b91d5e6dc696)

- Replaced `EditorSettingsWindow.ui` with a native Material 3 settings table.
- Preserved tri-state editor options, dependency handling, reset semantics, and cancel restoration.

## 2026-08-09 · Native archive export dialog

Commit: [88181375fdc0d38478aaa3b7d19b75121d1ab760](https://github.com/Ding-Ding-Projects/material-sandbox/commit/88181375fdc0d38478aaa3b7d19b75121d1ab760)

- Replaced `CompressDialog.ui` with a native Material 3 export form.
- Preserved format-dependent compression, solid archive, encryption, and export settings behavior.

## 2026-08-09 · Native rename dialog

Commit: [2251972eb131533c38532531fba767fb5e2f36c3](https://github.com/Ding-Ding-Projects/material-sandbox/commit/2251972eb131533c38532531fba767fb5e2f36c3)

- Replaced `RenameSandboxDialog.ui` with a native Material 3 form.
- Preserved alias visibility and remembered hide-alias behavior.

## 2026-08-09 · Native sandbox import dialog

Commit: [f2c3e874db5a1673a689b310e55b5cc2a752f935](https://github.com/Ding-Ding-Projects/material-sandbox/commit/f2c3e874db5a1673a689b310e55b5cc2a752f935)

- Replaced `ExtractDialog.ui` with a native Material 3 layout and accessible controls.
- Removed the retired Qt Designer resource and project registrations.

## 2026-08-09 · Application-wide dialog chrome

Commit: [d284a99552e7df6ef6c38e129db9c72018764cce](https://github.com/Ding-Ding-Projects/material-sandbox/commit/d284a99552e7df6ef6c38e129db9c72018764cce)

- Routed every shown application-owned `QDialog` through the reusable Material 3 host.
- Kept the dialog migration idempotent so rebuilt surfaces cannot stack title bars.

## 2026-08-09 · M3 modal dialog host

Commit: [fefd451cec268a46f507f6bdd6ce790d70b659b4](https://github.com/Ding-Ding-Projects/material-sandbox/commit/fefd451cec268a46f507f6bdd6ce790d70b659b4)

- Added a reusable frameless Material 3 dialog host.
- Migrated appearance, color, offline documentation, and destructive-confirmation dialogs to the shared title and close surface.

## 2026-08-09 · Obsolete proxy chrome removed

Commit: [f5a66a18a49a3482a757c5b2dce3c4462593e1fa](https://github.com/Ding-Ding-Projects/material-sandbox/commit/f5a66a18a49a3482a757c5b2dce3c4462593e1fa)

- Removed the unused `CustomStyles.h` proxy chrome classes and their qmake/MSVC registrations.
- Kept pending-value editor decoration separate from product chrome.

## 2026-08-09 · Legacy chrome switch retirement

Commit: [c0485166784f94d2f6969ab234e0b1aff29889ce](https://github.com/Ding-Ding-Projects/material-sandbox/commit/c0485166784f94d2f6969ab234e0b1aff29889ce)

- Removed startup Windows Vista/11 style selection and migrated the old preference to `false`.
- Box Assistant controls now inherit the shared Material 3 styling without a proxy chrome override.

## 2026-08-09 · Ground-up Material 3 shell migration slice

Commit: [babc125bae47c8f8a95fc283fd4936be02731a37](https://github.com/Ding-Ding-Projects/material-sandbox/commit/babc125bae47c8f8a95fc283fd4936be02731a37)

- Added one frameless M3 title-bar/menu host with accessible window controls and idempotent installation.
- Removed central Fusion and proxy-style resets; remaining data-view factories are documented as the next migration boundary.

## 2026-08-09 · Windows entrypoints and contributor capability cleanup

Commit: [d7012a106615c8b1033580d7500e92805a3c5b04](https://github.com/Ding-Ding-Projects/material-sandbox/commit/d7012a106615c8b1033580d7500e92805a3c5b04)

- Contributor builds now keep USB automation enabled and omit the non-commercial title restriction.
- Added silent Windows build and unsigned-installer entrypoints with explicit missing-tool reporting.

## 2026-08-09 · Pure Material 3 chrome boundary

Commit: [62639ad9ba5ef2a9a7f3e24f43b0573990c0618a](https://github.com/Ding-Ding-Projects/material-sandbox/commit/62639ad9ba5ef2a9a7f3e24f43b0573990c0618a)

- Retired the exposed Fusion-theme switch so stale profiles cannot re-enable a competing style path.

## Unreleased — scheduled-source status

- Scheduled-rules rows now show a localized active/inactive source status and a
  recovery hint for retained external metadata, without requesting credentials
  or performing network I/O.

## 2026-08-09 · Native CI stale-run control

Commit: [731a5a336fde7e6e3ce1a341a02d4e5b0e5dc995](https://github.com/Ding-Ding-Projects/material-sandbox/commit/731a5a336fde7e6e3ce1a341a02d4e5b0e5dc995)

- Added an explicit concurrency contract for validation-only Windows builds.
- Added architecture-specific artifact completeness checks and SHA-256 manifests before upload.
- Kept Pages and release side effects outside cancellation so external state is not stranded.

## Unreleased — dim-sum startup contract

- Added a delayed, non-blocking 10% startup draw that can display a bilingual dim-sum photo from a validated application-data cache sourced from the public catalog; offline or unsafe caches are skipped without interrupting startup. Commit: [7b41728bfcd85f058ea64e19f05e7fde51e5a922](https://github.com/Ding-Ding-Projects/material-sandbox/commit/7b41728bfcd85f058ea64e19f05e7fde51e5a922) (summary completion fix).

## Unreleased — tab discovery

- Added an anchored open-tab search with plain text and bounded regex-builder modes, result activation, persisted active tab metadata, and the searchable group picker. Commit: [5672418386d6d22d878d9fa92adf70914402bcb4](https://github.com/Ding-Ding-Projects/material-sandbox/commit/5672418386d6d22d878d9fa92adf70914402bcb4) (summary completion fix).

## 2026-08-09 · Filtered history exports

Commit: [6a19a27803a08397908316e6c2d3b8f371268450](https://github.com/Ding-Ding-Projects/material-sandbox/commit/6a19a27803a08397908316e6c2d3b8f371268450)

- Added atomic JSON/Markdown exports for filtered notification and settings-history views.

## 2026-08-09 · Material color translator

Commit: [f144d7fbfb9b09010311772d0397339b1fd0472a](https://github.com/Ding-Ding-Projects/material-sandbox/commit/f144d7fbfb9b09010311772d0397339b1fd0472a)

- Added bidirectional HEX/RGB/HSL accent editing with alpha, contrast feedback, validation, and keyboard focus.

## 2026-08-09 · Notifications and screenshot gallery

Commit: [062426ff87594ccd170bee2fba74dbf80d55d8bf](https://github.com/Ding-Ding-Projects/material-sandbox/commit/062426ff87594ccd170bee2fba74dbf80d55d8bf)

- Added the non-blocking notification center with local history, search, regex mode, and bulk dismissal.
- Added the README screenshot gallery and the bundled notification article.

## 2026-08-09 · Local settings history

Commit: [7c8f9d9e55e69fedd8e0d078145604fe6c483c10](https://github.com/Ding-Ding-Projects/material-sandbox/commit/7c8f9d9e55e69fedd8e0d078145604fe6c483c10)

- Added bounded, type-preserving local settings history with stable revision IDs and restore support.
- Added a Settings-window history panel and documented the local-only recovery boundary.

## 2026-08-08 · Material contributor baseline

Commit: [bd9aea138f9a401cebea1b854cd84f69cb9494b6](https://github.com/Ding-Ding-Projects/material-sandbox/commit/bd9aea138f9a401cebea1b854cd84f69cb9494b6)

- Added the Material 3 desktop baseline.
- Added contributor capability mode and quiet support handling.
- Published the Material 3 Pages landing site.


