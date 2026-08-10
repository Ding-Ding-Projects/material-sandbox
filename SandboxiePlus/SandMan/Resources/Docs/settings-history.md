# Local Git settings history

## Behavior

Sandboxie stores settings history in an isolated local Git working repository beside the application profile. It never places `.git` inside a user sandbox, project, or document folder, and it configures no remote. The repository records a complete, type-preserving settings snapshot for every real setting creation, edit, or deletion. Commit messages contain only bounded action and key metadata; values remain in the snapshot object.

The Settings window exposes **Open settings history** and **Create full settings checkpoint**. Each row shows the local Git commit ID, timestamp, action, key, and checkpoint state. Selecting a revision restores the complete snapshot stored by that commit. Restore is append-only: the application first commits the live state, applies the selected snapshot, and then commits the restored state. A restore never checks out, resets, rebases, amends, deletes, or rewrites an earlier commit, so the restore can itself be undone later.

The active filtered list can be exported as JSON metadata or Markdown. **Export Git bundle** runs a complete Git bundle export for a re-importable copy of every reachable local revision.

## Configuration

- Repository: `<profile>/history/settings-history/`
- Snapshot: `settings.snapshot`, serialized with the Qt 5.15 `QDataStream` format so integers, lists, byte arrays, and strings keep their original type.
- Display limit: the newest 500 commits by default, bounded to 50–5000. This limits the panel query only; reachable Git commits are not truncated or rewritten.
- Snapshot bounds: 1 MiB and 10,000 non-reserved keys per commit.
- Legacy migration: an existing bounded `<profile>/history/settings-history.jsonl` is copied once to `legacy/settings-history.jsonl` in the first Git commit. The original file is left untouched.
- Local identity: repository-local `user.name` and `user.email` values identify application history commits without reading or modifying a person's global Git identity.

## Git command contract

The implementation finds `git` on `PATH`, disables terminal prompting, hides the child process on Windows, applies a bounded timeout, and invokes Git with separate arguments rather than a command shell. The lifecycle uses these real commands:

```text
git init --quiet
git symbolic-ref HEAD refs/heads/main
git config --local ...
git remote
git add -- settings.snapshot
git commit --allow-empty --no-gpg-sign --no-verify ...
git rev-parse HEAD
git log ...
git show <commit>:settings.snapshot
git bundle create <destination> --all
```

The application refuses to write when the history directory resolves outside its expected application-data location, when `.git` is a link or non-directory, or when `git remote` reports any configured remote.

## Failure modes and security

A setting change remains successful if Git is unavailable, times out, or cannot record history. The history panel reports that failure without claiming a revision exists. Git commands never open an interactive credential prompt and never contact a network location because the repository has no remote. The application passes arguments directly to `QProcess`, validates 40-character commit IDs before `git show`, and bounds subprocess output and snapshot size.

Snapshots preserve the live store's representation: ciphertext stays ciphertext, and the history layer does not decrypt or print it. Metadata exports omit values. A Git bundle contains the complete snapshot history and should therefore be handled with the same care as the profile itself. The permanent code-signing policy is unaffected; local history commits use `--no-gpg-sign` and are not software signatures.

## Verification

The committed contract test checks every required production Git command, rejects a removed commit path through a deliberate negative mutation, and exercises a disposable real repository with `git init`, four append-only commits, `git show`, no remote, `git bundle create`, and `git bundle verify`. Native CI separately compiles the QProcess-backed implementation for the supported Windows configurations.

Suggested articles: [Material Design](../material-design.md), [Contributor build](../contributor-build.md).
