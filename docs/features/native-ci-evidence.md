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

## Windows release contract

The separate `Windows Release` workflow runs on `master` pushes and manual
dispatches. It deliberately has no workflow concurrency group: once a release
run starts, a newer run cannot cancel it halfway through tag or release
publication. The job checks out full history, resolves API access through
`RELEASE_TOKEN || ORG_TOKEN || GITHUB_TOKEN`, bootstraps the declared hosted
toolchain, and invokes `build.bat /s` and `build-installer.bat /s` from the
repository root.

Publication cannot begin until the job finds exactly one x64 installer, checks
its size and product version, calculates SHA-256, and verifies Authenticode
status `NotSigned`. It then creates one unique run-number tag as a draft,
verifies the target commit, publishes the same record as non-draft, and checks
the downloaded installer again. The notes record the exact commit, installer
name and hash, authoritative workflow start and publication timestamps, stable
`HH:mm:ss` duration, an explicit unsigned-installer and unknown-publisher
warning, and the complete
line-count table. The generated Markdown is inserted between stable
`line-count:start` / `line-count:end` markers and compared exactly after newline
normalization before the run accepts publication. The only dim-sum material is a link to the public
[`Ding-Ding-Projects/dim-sum-photos`](https://github.com/Ding-Ding-Projects/dim-sum-photos/releases)
catalog; no image is copied into this repository or attached to its release.

`scripts/count-lines.mjs` classifies every immutable tracked regular blob as
source, tests, styles/markup/docs, generated, or excluded. Its ordered audited
allowlists keep vendor, historical design, localization, and binary families
visible in the excluded row; unrecognized paths, unsupported Git modes,
ambiguous encodings, and new generated markers fail as review-required instead
of disappearing into a broad fallback. The evidence reports files, bytes,
total/nonblank/blank lines, project and grand totals, and surviving-line
attribution from `git blame`, including agent/people blank and nonblank splits.
A commit counts as agent-authored only when its exact author identity or a
canonical final `Co-Authored-By` trailer names an approved agent or verified
GitHub bot; all other committed lines count as people-authored.

The script refuses shallow history and non-blob modes, disables replacement
objects and ignored-revision configuration, pins the full commit and tree object
IDs, and keeps Git LF-record semantics so attribution and line totals reconcile.
Its deterministic JSON has no generated timestamp or absolute path. `--verify`
recomputes the report from the immutable revision and deep-compares it instead
of accepting internally balanced but stale evidence. Focused tests mutate every
arithmetic family, a balanced forged report, dirty state, UTF-16 and legacy
encodings, shallow history, misleading identities, and unclassified text and
binary paths.

Safe logs, provenance, checksums, line evidence, job outcome, and any exact
installer candidate are uploaded with `always()` handling even when an earlier
step fails. Evidence collection is non-masking and never makes a failed build
green. This committed contract does not prove that a release has run. No local
installer, tag, or release was created while adding it; an actual successful
workflow and downloadable asset remain the required publication evidence.

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

Run `node scripts/validate-ci-concurrency.mjs`,
`node scripts/validate-release-contract.mjs`,
`node scripts/validate-release-contract.mjs --self-test`,
`node --test scripts/test-count-lines.mjs`, `node scripts/count-lines.mjs`,
`node scripts/validate-ci-artifacts.mjs --help`, and
`node scripts/validate-ci-runtime-smoke.mjs`. The Pages validation job runs
the workflow-shape contracts; artifact verification runs only after the hosted
Windows build creates each architecture directory. Inspect the exact CI run for the target commit with `gh run view`;
do not infer a native result from a successful Pages deployment.

Suggested articles: [Contributor capability](contributor-build-audit.md),
[Material design](../material-design.md), and [Screenshot gallery](../screenshots.md).
