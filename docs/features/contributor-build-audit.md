# Contributor capability and quiet certificate boundary

Contributor builds define `SANDBOXIE_CONTRIBUTOR_BUILD` for the native driver,
service, SandMan, and start-helper targets. The driver initializes the highest capability shape
and makes certificate checks succeed without asking the user to purchase,
renew, donate, or wait through a supporter reminder.

Capability initialization happens before optional `Certificate.dat` I/O. Missing,
malformed, or unreadable certificate data therefore leaves `active=1`, clears
expiry/outdated/grace/lock state, selects `eCertContributor` at
`eCertMaxLevel`, and enables security, encryption, network, and desktop options.
The native driver, service, and UI consume the same record; contributor builds
also compile out debug certificate simulation so it cannot reintroduce a split
state.

## Build configuration coverage

The SandMan Visual Studio project carries the contributor boundary in all six
declared configurations: Debug and Release for Win32, x64, and ARM64. Each
configuration preserves `%(PreprocessorDefinitions)` after adding the profile
macro, so existing Qt, project, and toolchain definitions are not replaced.
The Qt 5 and Qt 6 qmake projects define the same profile. The validator keeps a
hand-maintained six-configuration inventory, compares it with the project file,
and rejects a missing, duplicated, unexpected, or non-inheriting definition.

The Settings certificate child page is removed from the contributor build while
the updater child remains available under an **Updates** tab with
certificate-independent access. Its certificate-refresh slot is inert. Options and troubleshooting surfaces compile
out supporter badges and certificate footers, and the Setup Wizard's update
footer is hidden at compile time. Setup Wizard skips the certificate page and
the personal/commercial license choice, clearing stale usage markers;
certificate-error popups, expiry logs, and support dialogs are all enclosed by
the contributor preprocessor boundary. The normal certificate workflow remains
available in non-contributor builds.

This change does not delete legal material. GPL, LGPL, Qt, and third-party
copyright/license notices remain in the source and distribution. “License-free”
means only that the contributor capability profile has no runtime supporter
gate; it does not remove copyright obligations.

## Failure and security boundaries

The profile is compile-time, not a hidden runtime switch. A build that omits
the define keeps upstream certificate behavior. Certificate files from an old
profile are not treated as a reason to show a purchase prompt. Driver and
service capability agreement still requires the matching contributor build;
runtime testing must exercise encrypted/file-image, security/privacy, network,
desktop/USB, and breakout paths.

## Verification

Run:

```text
node scripts/validate-contributor-build.mjs
```

The validator enumerates all six SandMan MSVC configurations, checks both Qt
project definitions, and checks the early native capability contract, every
reminder boundary, certificate feature gate, Setup Wizard route, support-tab
removal, Options and troubleshooting footer guards, and the legal-notice boundary. A Qt/MSVC
runtime build remains a separate gate; static source evidence must not be
reported as runtime proof.

Suggested articles: [School mode](school-mode.md), [Scheduled settings](scheduled-settings.md), and [Material appearance editor](appearance-editor.md).
