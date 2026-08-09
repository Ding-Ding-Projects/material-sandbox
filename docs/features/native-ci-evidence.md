# Native CI evidence and stale-run control

The Windows `CI` workflow builds the native Sandboxie manager, service, driver,
and installer inputs. It is a validation/build workflow, not a release or
deployment workflow. Each run still needs its hosted result before it is called
green, and an artifact upload does not imply that a desktop window or loaded
driver was exercised.

Validation runs use a workflow/ref concurrency group with
`cancel-in-progress: true`. A newer commit therefore cancels an obsolete queued
or running build instead of allowing several expensive Windows builds to hide
the verdict for the current commit. The group is deliberately not applied to
Pages deployment or release-announcement workflows, where cancellation could
leave external state half-written.

## Failure and evidence boundary

The cancellation status is not a test result. A cancelled run is recorded as
superseded, and the next uncancelled run must complete before native build
evidence is claimed. Static contracts and Pages publication remain separate from
Qt/MSVC compilation, service/driver loading, and cheap-headless screenshots.

After each architecture-specific merge, CI runs
`scripts/validate-ci-artifacts.mjs`. It checks the required manager, service,
driver, Qt, plugin, translation, and architecture sidecar files, rejects empty
files, and writes a SHA-256 manifest into the uploaded artifact. This proves
artifact completeness only; it does not prove that any executable launched or
that the service/driver loaded.

The x64 build then runs `scripts/smoke-sandman-runtime.ps1` against the merged
`Installer/SbiePlus_x64` directory. The check copies that directory to a
per-run temporary location, gives the process isolated application-data and
temporary directories, starts `SandMan.exe -autorun` with
`QT_QPA_PLATFORM=minimal`, and requires the process to remain alive for eight
seconds before terminating that exact process. `-autorun` suppresses the
ordinary main window, while the Qt minimal platform plugin avoids desktop
interaction. The check writes `ci-runtime-smoke.json` and uploads it even when
the smoke step fails, so the result records its observed status and boundary.

This is a bounded process-start proof for the packaged x64 manager and its Qt
runtime path. It does not prove visual rendering or interaction, the `qwindows`
platform plugin, service or driver installation/loading, sandbox operation,
installer behavior, ARM64 runtime behavior, or signing/trust status. The job
does not invoke `-assist`, install the artifact, or inspect or create any
signing material.

## Verification

Run `node scripts/validate-ci-concurrency.mjs` and
`node scripts/validate-ci-artifacts.mjs --help`, and
`node scripts/validate-ci-runtime-smoke.mjs`. The Pages validation job runs
the workflow-shape contracts; artifact verification runs only after the hosted
Windows build creates each architecture directory. Inspect the exact CI run for the target commit with `gh run view`;
do not infer a native result from a successful Pages deployment.

Suggested articles: [Contributor capability](contributor-build-audit.md),
[Material design](../material-design.md), and [Screenshot gallery](../screenshots.md).
