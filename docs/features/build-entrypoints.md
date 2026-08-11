# Windows build entrypoints

The repository exposes `build.bat` for the supported Windows desktop build and
`build-installer.bat` for the staged unsigned installer. Both accept `/s`,
`--silent`, or `SILENT=1`; silent mode never opens a prompt and exits non-zero
when the pinned Qt/MSVC toolchain or a build step is unavailable.

`build.bat` calls the supported `SandboxiePlus/qmake_plus.cmd` path and verifies
that `SandMan.exe` exists for the selected architecture. `build-installer.bat`
stages the x64 output through `Installer/copy_build.cmd`, compiles the canonical
Inno Setup script with no signing route, rejects any installer that is not
reported as `NotSigned`, and prints its SHA-256 digest. CI staging derives the
Qt root from the installer action's target package when that package lives
beside the checkout, and artifact verification discovers the installed VC143
runtime version instead of assuming the `v143` alias exists.

The ARM64 Sandboxie-Tools solution maps the `ImBox` Release configuration to
`Release|ARM64` and includes its build mapping, so the ARM64 staging step cannot
silently omit the executable. Archive membership checks normalize 7-Zip's
separator and `./` presentation before matching required troubleshooting files.
The local entrypoint contract checks both the solution mapping and these listing
normalizations before a build is considered ready for hosted verification.

Every `SboxDrv.vcxproj` debug and release configuration for Win32, x64, and
ARM64 sets the scalar WDK `SignMode` property to `Off`, so staging cannot inherit a
freshly signed driver from the supported build. The unsigned-packaging contract
enumerates all six configurations rather than trusting whichever one happened
to build locally. A global `SignMode=Off` default and an evaluated pre-build
target fail closed if a command-line or imported property tries to reactivate
driver signing.

Signing is permanently disabled. The obsolete `Sandboxie/install/build.bat`
route fails closed and points callers to the repository-root installer entry
point; it no longer contains or invokes the historical signing machinery. The
supported scripts do not collect credentials, install signing material,
publish, or create releases. If the pinned Qt kit or Visual Studio environment
is absent, the scripts report the exact path and stop. This checkout currently
has no local Qt qmake kit, so hosted Windows CI remains the build proof.

The bootstrap uses the Qt 6.8.3 base desktop package set for both the x64 host
and ARM64 target. Qt 6.8.3 already includes the `qtdeclarative` and `qttools`
archives in that set; passing them again as `--modules` makes `aqtinstall`
reject the official metadata before installation, so the helper deliberately
does not duplicate those selectors.

Run `node scripts/validate-unsigned-packaging.mjs --self-test` to verify the
permanent unsigned invariant and prove that representative signing directives,
executables, inputs, included Inno scripts, signed-uninstaller and file-signing
routes, environment/preprocessor construction, and enabled driver-signing modes
are rejected. `node scripts/validate-build-entrypoints.mjs` also invokes that
contract.

The `Windows Release` workflow is the publication owner. It prepares its hosted
toolchain and then calls the two root scripts rather than recreating their build
commands in YAML. Only after `build-installer.bat /s` succeeds does the workflow
verify exactly one `NotSigned` installer, its product version, size, SHA-256,
source commit/tree, and deep-recomputed line-count evidence. The root scripts themselves never tag or
publish, so running either one locally cannot create external release state.

No local installer was produced while the release contract was added because
this checkout does not contain the pinned native Qt/MSVC toolchain. The static
workflow and counter checks are reproducible locally, but they are not a
substitute for a successful hosted build and verified downloadable release.

## Suggested articles

- [Material Design 3](../material-design.md)
- [Native CI evidence](native-ci-evidence.md)
- [Contributor capability](contributor-build-audit.md)
