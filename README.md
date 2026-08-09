# Sandboxie Material Desktop

Sandboxie is a Windows desktop sandbox manager built with Qt. This checkout carries the Material Design 3 desktop shell and the contributor build profile.

## Current delivery

- Shared Material 3 color roles, typography baseline, shape, elevation, focus, and control styling are applied by `MaterialTheme`.
- Light/dark selection continues to use the existing persisted settings and is applied consistently to the widget tree.
- Contributor builds unlock the feature capability state in one driver boundary and suppress supporter, purchase, expiry, and certificate reminder interruptions.
- Settings changes have bounded, type-preserving local history with stable IDs, restore support, and filtered JSON/Markdown export that excludes setting payloads.
- Informational events have a non-blocking Notifications tab with local history, extended selection, bulk dismissal, plain search, opt-in regex search, and filtered export.
- Material appearance includes a keyboard-accessible HEX/HEX8, RGB/RGBA, and HSL/HSLA translator with alpha preservation, preview, validation, contrast feedback, and live persistence.
- Settings and Options tabs persist order, active tab, pin/group metadata, and per-tab appearance edits; the full tab-discovery and group-editor expansion remains in progress.
- Scheduled presentation and appearance rules are persisted locally as schema-versioned JSON with native date/time/weekday editing, cross-midnight semantics, deterministic priority/id precedence, and School mode as the final language gate.
- GPL, LGPL, Qt, and upstream license notices remain intact; “remove license checks” refers only to runtime supporter gating, not copyright notices.

See [`docs/material-design.md`](docs/material-design.md) and [`docs/contributor-build.md`](docs/contributor-build.md) for behavior, boundaries, and verification.

## Screenshots

The [Material desktop screenshot gallery](docs/screenshots.md) contains a large catalog of real bundled visual assets, organized by surface and labeled honestly. Built-window captures will be appended when the Qt/MSVC headless capture lane produces them.

![Sandboxie desktop mark](SandboxiePlus/SandMan/Resources/SandMan.png)
![Simple theme asset](SandboxiePlus/SandMan/Resources/Simple.png)
![Configuration action](SandboxiePlus/SandMan/Resources/Actions/Config.png)
![Material design action](SandboxiePlus/SandMan/Resources/Actions/Design.png)
![Notification action](SandboxiePlus/SandMan/Resources/Actions/Notification.png)
![Recovery action](SandboxiePlus/SandMan/Resources/Actions/recover.png)

## Shared-instructions mirror

This repository also follows the sanitized shared agent instructions: preserve user work, use isolated checkouts, verify builds and runtime evidence separately, keep user-facing surfaces accessible and localized, and never expose credentials. The canonical instructions remain in the private agent-global-memory repository.

## Build boundary

The Qt desktop build requires the repository's documented Qt/MSVC toolchain. On hosts without `qmake`, use `SandboxiePlus/qmake_plus.cmd` from a configured Qt developer prompt. Static checks remain useful but do not replace a built-app or driver/runtime proof.
