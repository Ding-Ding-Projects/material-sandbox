# Sandboxie Material Desktop

![Sandboxie](SandboxiePlus/SandMan/Resources/SandMan.png)

Sandboxie is a Windows desktop sandbox manager. This repository contains the Qt desktop application, the Sandboxie service and driver projects, installer assets, and the Material Design 3 (M3) presentation work for the desktop manager.

[![Build](https://github.com/Ding-Ding-Projects/material-sandbox/actions/workflows/main.yml/badge.svg)](https://github.com/Ding-Ding-Projects/material-sandbox/actions/workflows/main.yml) [![Pages](https://github.com/Ding-Ding-Projects/material-sandbox/actions/workflows/pages.yml/badge.svg)](https://github.com/Ding-Ding-Projects/material-sandbox/actions/workflows/pages.yml) [![Documentation](https://img.shields.io/badge/docs-Material%20desktop-6750A4)](https://ding-ding-projects.github.io/material-sandbox/)

**Project site:** [ding-ding-projects.github.io/material-sandbox](https://ding-ding-projects.github.io/material-sandbox/)

**Source baseline documented here:** `c4ac7bb4` (the current contributor-safe Material desktop integration baseline).

## Quick index

- [What this checkout provides](#what-this-checkout-provides)
- [Install and build](#install-and-build)
- [Contributor capability](#contributor-capability)
- [Feature inventory](#feature-inventory)
- [Screenshots and visual catalog](#screenshots-and-visual-catalog)
- [Verification and evidence](#verification-and-evidence)
- [Documentation](#documentation)
- [Security, privacy, and licensing](#security-privacy-and-licensing)
- [Contributing](#contributing)
- [Known boundaries](#known-boundaries)

## What this checkout provides

The desktop manager is being presented as a single M3 system: shared color roles, typography, shape, elevation, focus treatment, tabs, menus, fields, notifications, and responsive spacing are owned by `MaterialTheme` rather than by a collection of unrelated widget overrides. The existing Sandboxie isolation model, service communication, driver capability checks, encryption, and upstream attribution remain part of the product.

The current delivery also includes:

- a contributor build profile for authorized contributors;
- quiet, non-blocking informational notifications;
- local, append-only settings history with restore and filtered export;
- persisted presentation, appearance, tab, scheduled-settings, and School-mode state;
- an in-app offline documentation browser and `Ctrl+Shift+F` command palette;
- a Material color translator and appearance editor;
- a 10% non-blocking dim-sum startup surprise using the public catalog contract;
- a persisted VS Code profile-folder opener;
- a GitHub Pages documentation site with a feature index and visual catalog.

The feature list below deliberately distinguishes implemented behavior from evidence still pending. A static contract is not presented as proof that a Qt window, loaded driver, or release artifact was executed.

## Install and build

### Prerequisites

The supported desktop build uses Windows, Visual Studio/MSVC, Qt, `qmake`, and the repository's normal Qt development prompt. The checkout contains helper scripts for the expected toolchain:

```powershell
.\SandboxiePlus\install_qt.cmd
.\SandboxiePlus\install_jom.cmd
```

Open a configured Qt/MSVC developer prompt, then generate and build the solution with the repository's normal project files. `SandboxiePlus\qmake_plus.cmd` is the project-specific qmake entry point. Build the desktop manager together with the matching service and driver targets when verifying contributor capability; a manager-only build cannot prove service/driver agreement.

### A reproducible local sequence

```powershell
git clone https://github.com/Ding-Ding-Projects/material-sandbox.git
cd material-sandbox
.\SandboxiePlus\qmake_plus.cmd
```

The exact Qt and MSVC versions are determined by the configured developer environment and project files. Do not copy an installer or a signing key into the checkout. Release artifacts are unsigned under the repository's permanent no-signing policy and may receive the normal Windows unknown-publisher warning.

## Contributor capability

Contributor capability is an explicit build-time profile, not a hidden runtime switch. The Qt manager, service, and driver property sheets use the same `SANDBOXIE_CONTRIBUTOR_BUILD` boundary. In that profile, the driver normalizes certificate capability after parsing, the manager's certificate gate succeeds without a purchase prompt, and support/expiry/certificate reminder interruptions are suppressed. A contributor can still open the normal support/help routes deliberately.

This profile does **not** remove copyright notices, third-party attributions, or source licenses; license notices remain intact. It changes the authorized capability policy used by this contributor build; it does not authorize using another person's data, weakening Windows security, or bypassing an unrelated access control. See [`docs/contributor-build.md`](docs/contributor-build.md) and [`docs/features/contributor-build-audit.md`](docs/features/contributor-build-audit.md) for the exact boundary and required runtime checks.

## Feature inventory

| Area | Shipped behavior | Evidence and details |
| --- | --- | --- |
| Material Design 3 shell | Shared light/dark roles, typography baseline, shape, elevation, focus, and responsive control styling | [`docs/material-design.md`](docs/material-design.md) |
| Appearance editor | Persisted theme, density, accent, display name, font family/size/weight/style, live preview, and reset; unsupported properties are disclosed | [`docs/features/appearance-editor.md`](docs/features/appearance-editor.md) |
| Color translator | Validated HEX/HEX8, RGB/RGBA, HSL/HSLA, alpha, preview, contrast feedback, and keyboard-safe editing | [`docs/features/color-translator.md`](docs/features/color-translator.md) |
| Presentation preferences | English, playful Hong Kong Cantonese, bilingual mode, independent funny-level controls, and dialog/message emoji preference | [`docs/features/scheduled-settings.md`](docs/features/scheduled-settings.md) |
| Scheduled settings | Versioned local rules for language, theme, density, accent, fonts, motion, and display name; native date/time/weekday editing and deterministic precedence | [`docs/features/scheduled-settings.md`](docs/features/scheduled-settings.md) |
| School mode | User-renamable language gate with persisted unlock state and a documented local reset path | [`docs/features/school-mode.md`](docs/features/school-mode.md) |
| Dim-sum surprise | One fresh 10% startup draw, delayed and non-blocking, with bilingual name/alt text and a validated public-photo cache; no opt-out switch | [`docs/features/dim-sum-surprise.md`](docs/features/dim-sum-surprise.md) |
| Notifications | Non-blocking informational center, persistent local history, keyboard selection, bulk dismissal, plain search, opt-in bounded regex, and filtered JSON/Markdown export | [`docs/features/notifications.md`](docs/features/notifications.md) |
| Settings history | Type-preserving append-only local revisions, stable IDs, restore, date/action/text filtering, and filtered JSON/Markdown export | [`docs/features/settings-history.md`](docs/features/settings-history.md) |
| Browser-style tabs | Persisted order, active tab, pin/group metadata, tab appearance entry point, overflow/discovery foundations, and keyboard paths | [`docs/features/tab-discovery.md`](docs/features/tab-discovery.md) |
| Command palette | `Ctrl+Shift+F` discovery route for destinations, features, and settings | [`docs/features/command-palette.md`](docs/features/command-palette.md) |
| Offline documentation | Bundled feature articles rendered inside the application with local search and completeness validation | [`docs/features/README.md`](docs/features/README.md) |
| External editor | Detects/configures VS Code and opens the persisted profile folder as a workspace root | [`docs/features/external-editor.md`](docs/features/external-editor.md) |
| Destructive confirmation | Remove Sandbox uses a two-key, full-range destructive-action confirmation with reduced-motion-aware progress, Emergency exit, and focus return | [`docs/features/destructive-confirmation.md`](docs/features/destructive-confirmation.md) |
| Native CI evidence | [Run 31300538500](https://github.com/Ding-Ding-Projects/material-sandbox/actions/runs/31300538500) verifies x64 and ARM64 SandMan/service/driver/Qt artifacts, SHA-256 manifests, and unsigned outputs; runtime evidence stays separately labelled | [`docs/features/native-ci-evidence.md`](docs/features/native-ci-evidence.md) |
| Screenshots | A large, honest catalog of tracked visual assets, with runtime-capture boundaries called out | [`docs/screenshots.md`](docs/screenshots.md) |

### Product behavior that remains unchanged

Sandbox creation and management, file/image/network/security controls, service and driver communication, encrypted storage, and upstream integration remain the core product. The M3 work changes presentation and contributor capability handling; it does not silently remove sandbox isolation or security features.

## Screenshots and visual catalog

The following images are real bundled assets from this checkout. They are included so the README is useful as a visual index, not as a claim that every image is a runtime capture of the current Qt build. The [complete screenshot catalog](docs/screenshots.md) has the same distinction and links every local asset through the screenshot validator.

<details open>
<summary>Brand, sandbox, and theme surfaces</summary>

| Sandboxie | Logo | Backplate |
| --- | --- | --- |
| ![Sandboxie desktop mark](SandboxiePlus/SandMan/Resources/SandMan.png) | ![Sandboxie logo](SandboxiePlus/SandMan/Resources/sandboxie-logo.png) | ![Sandboxie backplate](SandboxiePlus/SandMan/Resources/sandboxie-back.png) |

| Empty sandbox | Full sandbox | Simple theme |
| --- | --- | --- |
| ![Empty sandbox](SandboxiePlus/SandMan/Resources/sandbox-empty.png) | ![Full sandbox](SandboxiePlus/SandMan/Resources/sandbox-full.png) | ![Simple theme](SandboxiePlus/SandMan/Resources/Simple.png) |

| Simple dark theme | Classic theme | Classic dark theme |
| --- | --- | --- |
| ![Simple dark theme](SandboxiePlus/SandMan/Resources/SimpleD.png) | ![Classic theme](SandboxiePlus/SandMan/Resources/Classic.png) | ![Classic dark theme](SandboxiePlus/SandMan/Resources/ClassicD.png) |
</details>

<details>
<summary>Settings, security, recovery, and editor actions</summary>

| Configuration | Settings | Material design |
| --- | --- | --- |
| ![Configuration action](SandboxiePlus/SandMan/Resources/Actions/Config.png) | ![Settings action](SandboxiePlus/SandMan/Resources/Actions/Interface.png) | ![Material design action](SandboxiePlus/SandMan/Resources/Actions/Design.png) |

| Security | Network | Recovery |
| --- | --- | --- |
| ![Security action](SandboxiePlus/SandMan/Resources/Actions/Security.png) | ![Network action](SandboxiePlus/SandMan/Resources/Actions/Network.png) | ![Recovery action](SandboxiePlus/SandMan/Resources/Actions/recover.png) |

| Box | Folder | Editor |
| --- | --- | --- |
| ![Box action](SandboxiePlus/SandMan/Resources/Actions/box.png) | ![Folder action](SandboxiePlus/SandMan/Resources/Actions/Folder.png) | ![Editor action](SandboxiePlus/SandMan/Resources/Actions/Editor.png) |
</details>

<details>
<summary>Accessibility, notifications, help, and connectivity</summary>

| Accessibility | Notification | Help |
| --- | --- | --- |
| ![Accessibility action](SandboxiePlus/SandMan/Resources/Actions/Accessibility.png) | ![Notification action](SandboxiePlus/SandMan/Resources/Actions/Notification.png) | ![Help action](SandboxiePlus/SandMan/Resources/Actions/Help.png) |

| Connect | Disconnect | Cleanup |
| --- | --- | --- |
| ![Connect action](SandboxiePlus/SandMan/Resources/Actions/Connect.png) | ![Disconnect action](SandboxiePlus/SandMan/Resources/Actions/Disconnect.png) | ![Cleanup action](SandboxiePlus/SandMan/Resources/Actions/clean.png) |
</details>

## Verification and evidence

Static contracts are committed under [`scripts/`](scripts/). Run them from the repository root:

```powershell
node scripts/validate-docs.mjs
node scripts/validate-screenshot-links.mjs
node scripts/validate-contributor-build.mjs
node scripts/validate-appearance-editor.mjs
node scripts/validate-color-translator.mjs
node scripts/validate-command-palette.mjs
node scripts/validate-dim-sum-surprise.mjs
node scripts/validate-external-editor.mjs
node scripts/validate-notifications.mjs
node scripts/validate-scheduled-settings.mjs
node scripts/validate-school-mode.mjs
node scripts/validate-settings-history.mjs
node scripts/validate-tab-discovery.mjs
node scripts/validate-ci-concurrency.mjs
node scripts/validate-ci-artifacts.mjs --help
git diff --check
```

The Pages workflow runs the documentation and feature contracts on every `master` update. A successful static contract proves source structure and local asset references; it does not prove that Qt/MSVC produced a fresh binary, that the service and driver loaded, or that a hidden-desktop screenshot was captured. Those boundaries are intentionally recorded in the feature articles and the Pages status panel.

| Evidence class | What it proves | What it does not prove |
| --- | --- | --- |
| Documentation and contract scripts | Articles, links, feature registrations, and static invariants are present | Native rendering, driver loading, or live network behavior |
| GitHub Actions build | The hosted job reached its recorded build/test result | A release asset or runtime capture unless the job explicitly publishes one |
| Pages deployment | The static documentation site was published | The native desktop application was built |
| Headless desktop capture | A specific built surface rendered at the captured commit | Uncaptured surfaces or unrelated driver/service paths |
| Contributor runtime exercise | Manager/service/driver capability agreement and gated-path behavior | Copyright ownership or authorization outside the contributor build |

## Documentation

The [Material Design 3 landing page](https://ding-ding-projects.github.io/material-sandbox/) is the public, tabbed entry point. The [feature index](docs/features/README.md) links one article per major behavior; each article records configuration, failure behavior, security considerations, and verification boundaries. The [changelog](docs/changelog.md) links released work to its implementing commit where that commit is known.

Useful entry points:

- [`docs/material-design.md`](docs/material-design.md) — M3 tokens and desktop shell anatomy.
- [`docs/contributor-build.md`](docs/contributor-build.md) — contributor build configuration and runtime checklist.
- [`docs/screenshots.md`](docs/screenshots.md) — the full local visual catalog.
- [`docs/changelog.md`](docs/changelog.md) — dated implementation milestones.
- [`AGENTS.md`](AGENTS.md) — the sanitized repository guidance for contributors and agents.

## Security, privacy, and licensing

Sandboxie is security-sensitive software. Do not treat the contributor profile, a static contract, or an unsigned installer as a substitute for reviewing the source, Windows isolation behavior, or your organization's authorization. Report security issues through the repository's configured private reporting path rather than publishing exploit details in a feature request.

This checkout retains the upstream and third-party notices shipped with the project. Relevant notices include [`Installer/license.txt`](Installer/license.txt), [`Sandboxie/install/LICENSE.TXT`](Sandboxie/install/LICENSE.TXT), [`SandboxiePlus/SandMan/LICENSE`](SandboxiePlus/SandMan/LICENSE), [`SandboxiePlus/MiscHelpers/LICENSE`](SandboxiePlus/MiscHelpers/LICENSE), and [`SandboxiePlus/QSbieAPI/LICENSE`](SandboxiePlus/QSbieAPI/LICENSE). Qt and other dependencies retain their own terms. The contributor capability change removes runtime supporter/certificate gating for the authorized build; it does not remove or alter copyright and license obligations.

The application does not require a payment, subscription, or recurring support prompt. Informational notifications are non-blocking; destructive actions remain explicit decisions. Network-backed scheduled settings and the dim-sum cache are bounded and fail safe, and secrets must stay in the operating system credential store or GitHub's secret store rather than in source, issues, screenshots, or exports.

## Contributing

1. Create a focused change in a fresh checkout.
2. Read the relevant feature article and local [`AGENTS.md`](AGENTS.md).
3. Keep user-facing changes keyboard-accessible, localized, responsive, and consistent with M3 roles.
4. Add or update the feature article, validator, changelog entry, Pages source, and screenshot catalog when the change affects a visible surface.
5. Run the applicable static contracts and `git diff --check`.
6. Describe native build/runtime evidence separately from static checks in the commit or pull request.

Commit subjects are concise and commit bodies explain the real behavior in English and Hong Kong-style Cantonese. Do not commit credentials, generated dependencies, private instructions, or fabricated screenshots.

## Known boundaries

The full global feature contract is broader than the currently verified native surface. In particular, complete Word-depth per-element appearance editing, every tab-discovery/group action, full-state local snapshots for every account and record, scheduled external-source integration, dim-sum public-asset availability, native Qt/MSVC builds, loaded-driver smoke tests, and runtime screenshot capture require additional evidence or implementation. They are documented as pending rather than described as complete.

The README is intentionally a navigable index: long reference material stays in collapsible sections and dedicated articles, while the compact links above keep the first screen useful. The project site provides the same content as browser-style tabs for readers who prefer a full documentation surface.
