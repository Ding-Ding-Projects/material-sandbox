# Sandboxie Material Desktop

Sandboxie is a Windows desktop sandbox manager built with Qt. This checkout carries the Material Design 3 desktop shell and the contributor build profile.

## Current delivery

- Shared Material 3 color roles, typography baseline, shape, elevation, focus, and control styling are applied by `MaterialTheme`.
- Light/dark selection continues to use the existing persisted settings and is applied consistently to the widget tree.
- Contributor builds unlock the feature capability state in one driver boundary and suppress supporter, purchase, expiry, and certificate reminder interruptions.
- GPL, LGPL, Qt, and upstream license notices remain intact; “remove license checks” refers only to runtime supporter gating, not copyright notices.

See [`docs/material-design.md`](docs/material-design.md) and [`docs/contributor-build.md`](docs/contributor-build.md) for behavior, boundaries, and verification.

## Shared-instructions mirror

This repository also follows the sanitized shared agent instructions: preserve user work, use isolated checkouts, verify builds and runtime evidence separately, keep user-facing surfaces accessible and localized, and never expose credentials. The canonical instructions remain in the private agent-global-memory repository.

## Build boundary

The Qt desktop build requires the repository's documented Qt/MSVC toolchain. On hosts without `qmake`, use `SandboxiePlus/qmake_plus.cmd` from a configured Qt developer prompt. Static checks remain useful but do not replace a built-app or driver/runtime proof.
