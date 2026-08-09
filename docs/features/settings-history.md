# Local settings history

## Behavior

Sandboxie records bounded, append-only setting changes in a local JSONL file under the application profile's `history` directory. The history never enters a user's sandbox or project and is not uploaded. Each record has a stable UUID, UTC timestamp, setting key, action, and type-preserving before/after values.

The Settings window exposes **Open settings history**. The panel filters revisions by key, action, or date, and restores the selected setting's previous value. Restoring is itself a new setting write, so the recovery action remains reversible through a later revision.

The panel exports the active filtered view as JSON metadata or Markdown. Before/after values are deliberately excluded from exports because settings may contain sensitive data.

## Configuration

- File: `<profile>/history/settings-history.jsonl`
- Retention: 500 entries by default, bounded to 50–5000 by the implementation.
- Record size: 64 KiB per JSONL line; oversized or malformed lines are ignored during load.
- Values: serialized with `QDataStream` and base64 encoded inside JSON so integers, lists, byte arrays, and strings retain their Qt type.

## Failure modes and security

If the history file is absent, unreadable, malformed, or cannot be atomically rewritten, the live settings operation still proceeds; history is best-effort recovery and never a reason to block configuration changes. History is local-only, uses `QSaveFile` for replacement, and excludes reserved `History/` keys to avoid self-recording loops. It does not contain credentials unless a setting itself already stores one; credential-bearing settings should continue to use the application's existing protected storage.

## Verification

Static verification checks source/project parity, schema validation, stable-ID restore calls, bounded line reads, and qmake/MSVC registration. Qt runtime verification remains pending until a Qt/MSVC toolchain is available.

Suggested articles: [Material Design](../material-design.md), [Contributor build](../contributor-build.md).
