# Local settings history

Sandboxie keeps a bounded, append-only local record of setting changes under the profile's `history` directory. The Settings window's **Open settings history** panel filters revisions, creates full settings checkpoints, and restores either a selected setting's previous value or a complete checkpoint. A restore first captures the live state as a new checkpoint, so recovery remains reversible without generating one noisy delta per key.

Values are type-preserving `QDataStream` snapshots encoded inside JSONL. Records have stable IDs, UTC timestamps, and explicit before/after presence flags; full checkpoints carry an `isSnapshot` payload with deterministic key ordering. Delta lines remain bounded to 64 KiB, while checkpoints are bounded to 1 MiB and 10,000 keys. Malformed or oversized records are ignored; a history write never blocks the live settings operation. Checkpoints preserve ciphertext as stored and never export values, decrypt them, or send them anywhere.

Suggested articles: [Material Design](qrc:/Docs/material-design.md), [Contributor build](qrc:/Docs/contributor-build.md).
