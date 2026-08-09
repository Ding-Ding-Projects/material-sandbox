# Changelog

## 2026-08-09 · M3 modal dialog host

Commit: [fefd451cef268a46f507f6bdd6ce790d70b659b4](https://github.com/Ding-Ding-Projects/material-sandbox/commit/fefd451cef268a46f507f6bdd6ce790d70b659b4)

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
