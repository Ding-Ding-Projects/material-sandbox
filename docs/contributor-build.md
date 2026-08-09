# Contributor capability mode

## Behavior

`SANDBOXIE_CONTRIBUTOR_BUILD` is defined by the Qt SandMan projects, the start helper, and the four native Sandboxie property sheets. The driver initializes the contributor capability record before any optional `Certificate.dat` I/O, so missing, malformed, or unreadable certificate files cannot disable the runtime contract. It sets `active=1`, clears expiry/outdated/grace/lock state, selects `eCertContributor`/`eCertMaxLevel`, and enables `opt_sec`, `opt_enc`, `opt_net`, and `opt_desk`. The service and driver feature gates therefore agree with the desktop UI from startup.

The desktop manager also makes `CheckCertificate()` succeed without a modal purchase prompt, skips support/expiry reminder flows, and suppresses certificate-error popups. The normal Help/support entry points remain available when a contributor explicitly asks for them.

## Configuration

This is a build-time profile, not a hidden runtime switch. Distribution builds that omit the define retain their upstream certificate behavior. Copyright and third-party license notices are not removed.

## Failure modes and security

Because the profile changes driver capability policy, it must only be used for an authorized contributor build. Debug certificate simulation is excluded from contributor builds so a developer-only fake state cannot reintroduce a UI/native mismatch. A static compile check cannot prove that a loaded driver and service agree; runtime testing must cover encrypted/file-image, security/privacy, network, desktop/USB, and breakout paths and confirm no delayed termination or 6004/6008/6009 certificate messages.

## Verification

Build the matching driver, service, SandMan, and start-helper targets with the define enabled. Start without `Certificate.dat`, query driver feature flags, exercise the gated paths, and record the exact build SHA. Hosted CI verifies x64 and ARM64 compilation plus a bounded x64 SandMan startup smoke; driver loading, feature-toggle exercise, and visual capture remain separate runtime gates. The local checkout still does not provide the Qt/MSVC toolchain.

## Suggested articles

- [Material Design 3 desktop shell](material-design.md)
