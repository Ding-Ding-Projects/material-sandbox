# Contributor capability mode

## Behavior

`SANDBOXIE_CONTRIBUTOR_BUILD` is defined by the Qt SandMan projects and the four native Sandboxie property sheets. In this profile, the driver normalizes certificate state to the contributor/highest-capability shape after certificate parsing, so service and driver feature gates agree with the desktop UI.

The desktop manager also makes `CheckCertificate()` succeed without a modal purchase prompt, skips support/expiry reminder flows, and suppresses certificate-error popups. The normal Help/support entry points remain available when a contributor explicitly asks for them.

## Configuration

This is a build-time profile, not a hidden runtime switch. Distribution builds that omit the define retain their upstream certificate behavior. Copyright and third-party license notices are not removed.

## Failure modes and security

Because the profile changes driver capability policy, it must only be used for an authorized contributor build. A static compile check cannot prove that a loaded driver and service agree; runtime testing must cover encrypted/file-image, security/privacy, network, and breakout paths and confirm no delayed termination or 6004/6008/6009 certificate messages.

## Verification

Build the matching driver, service, and SandMan targets with the define enabled. Start without `Certificate.dat`, query driver feature flags, exercise the gated paths, and record the exact build SHA. This checkout currently has no Qt toolchain available, so that runtime proof remains pending.

## Suggested articles

- [Material Design 3 desktop shell](material-design.md)
