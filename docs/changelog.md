# Changelog

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
