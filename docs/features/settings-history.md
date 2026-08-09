# Local settings history

## Behavior

Sandboxie records bounded, append-only setting changes in a local JSONL file under the application profile's `history` directory. The history never enters a user's sandbox or project and is not uploaded. Each record has a stable UUID, UTC timestamp, setting key, action, and type-preserving before/after values. The Settings window can also create a full-state checkpoint: one deterministic, type-preserving map of every non-reserved setting key.

The Settings window exposes **Open settings history** and **Create full settings checkpoint**. The panel filters revisions by key, action, or date, labels checkpoints distinctly, and restores the selected setting or complete checkpoint. A restore first records the live state as a new checkpoint, then applies the selected state without emitting one noisy delta per key; recovery therefore remains reversible through a later revision.

The panel exports the active filtered view as JSON metadata or Markdown. Before/after values are deliberately excluded from exports because settings may contain sensitive data.

## Configuration

- File: `<profile>/history/settings-history.jsonl`
- Retention: 500 entries by default, bounded to 50–5000 by the implementation.
- Record size: delta records retain the 64 KiB JSONL contract; full checkpoints are independently bounded to 1 MiB of serialized values and 10,000 keys. Oversized or malformed lines are ignored during load.
- Values: serialized with `QDataStream` and base64 encoded inside JSON so integers, lists, byte arrays, and strings retain their Qt type.
- Checkpoint schema: `isSnapshot` plus an explicit Qt 5.15-compatible `QDataStream` payload. Keys are sorted by `QVariantMap`, and reserved `History/` keys are excluded.

## Failure modes and security

If the history file is absent, unreadable, malformed, or cannot be atomically rewritten, the live settings operation still proceeds; history is best-effort recovery and never a reason to block configuration changes. History is local-only, uses `QSaveFile` for replacement, and excludes reserved `History/` keys to avoid self-recording loops. Checkpoints preserve values exactly as stored, including ciphertext; they do not decrypt, log, or export values. Credential-bearing settings should continue to use the application's existing protected storage.

## Verification

Static verification checks source/project parity, schema validation, stable-ID restore calls, bounded line reads, and qmake/MSVC registration. Qt runtime verification remains pending until a Qt/MSVC toolchain is available.

Suggested articles: [Material Design](../material-design.md), [Contributor build](../contributor-build.md).
