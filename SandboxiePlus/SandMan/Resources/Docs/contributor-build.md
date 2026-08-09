# Contributor capability mode

Contributor builds normalize the driver capability state to the highest feature profile and suppress supporter, purchase, expiry, and certificate reminder interruptions. Settings keeps the updater child tab (renamed **Updates**) while removing the certificate child tab. Setup Wizard hides the commercial/personal license choice, clears stale usage markers, opens the Insider channel without certificate gating, and routes directly to UI setup. Legacy support-dialog callers receive an allowed no-op result, so a silent reminder cannot terminate the manager.

Both qmake projects and all six SandMan Visual Studio configurations define the contributor profile. The MSVC matrix covers Debug and Release for Win32, x64, and ARM64, and every configuration preserves inherited preprocessor definitions.

## Security boundary

This is a build-time profile. Copyright, GPL, LGPL, Qt, and third-party notices remain unchanged.

## Verification

Run `node scripts/validate-contributor-build.mjs`, build the driver, service, and SandMan together, start without Certificate.dat, and exercise security, encryption, network, desktop, image, and breakout paths. The validator's six-configuration source result is not a substitute for the runtime exercise.

Suggested articles: [Material Design 3 desktop shell](material-design.md).
