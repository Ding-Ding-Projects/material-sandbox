# Contributor capability and quiet certificate boundary

Contributor builds define `SANDBOXIE_CONTRIBUTOR_BUILD` for the native driver,
service, and SandMan targets. The profile enables the highest capability shape
and makes certificate checks succeed without asking the user to purchase,
renew, donate, or wait through a supporter reminder.

The Settings support page is removed from the contributor build and its
certificate-refresh slot is inert. Setup Wizard skips the certificate page;
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
desktop, and breakout paths.

## Verification

Run:

```text
node scripts/validate-contributor-build.mjs
```

The validator checks every reminder boundary, certificate feature gate, Setup
Wizard route, support-tab removal, and the legal-notice boundary. A Qt/MSVC
runtime build remains a separate gate; static source evidence must not be
reported as runtime proof.

Suggested articles: [School mode](school-mode.md), [Scheduled settings](scheduled-settings.md), and [Material appearance editor](appearance-editor.md).
