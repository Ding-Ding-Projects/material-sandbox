# Local settings history

Sandboxie keeps a bounded, append-only local record of setting changes under the profile's `history` directory. The Settings window's **Open settings history** panel filters revisions and restores a selected setting's previous value. Restoration writes a new revision, so recovery remains reversible.

Values are type-preserving `QDataStream` snapshots encoded inside JSONL. Records have stable IDs, UTC timestamps, and explicit before/after presence flags. Malformed or oversized records are ignored; a history write never blocks the live settings operation. History is local-only and never enters a sandbox or project.

Suggested articles: [Material Design](qrc:/Docs/material-design.md), [Contributor build](qrc:/Docs/contributor-build.md).
