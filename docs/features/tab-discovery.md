# Tab discovery

The Settings and Options tab strips expose **Search open tabs…** from their tab context menu. The anchored Material dialog searches visible tab labels, stable tab IDs, and persisted group names. Plain text is the default; enabling **Regex** exposes the adjacent builder fields for a bounded raw pattern, sample text, validation, and capture preview. Case sensitivity is explicit, and results identify pinned and grouped tabs. Activating a result focuses the real tab and records it as the active tab.

## Persistence

`CTabStateManager` stores order, pinned membership, group names, and the active stable tab ID in the existing settings blob. Schema 1 blobs remain readable; saves migrate them to schema 2. Stable IDs come from each page's object name and fall back to its title, so callers should provide object names for long-lived tabs.

## Failure modes and security

Patterns and sample text are bounded to 4096 characters and evaluated locally with Qt's `QRegularExpression`; invalid patterns produce an honest validation message and no results. Search does not access network data or hidden page content. A missing or malformed settings blob is ignored, preserving the live tab layout.

## Verification

Run `node scripts/validate-tab-discovery.mjs` to verify the manager, docs article, and both consumer windows retain the search entry point, regex builder, and schema 2 active-state contract. Runtime capture remains a separate headless-app gate; this lane does not claim the full four-search or complete group-appearance contract.

## Suggested articles

- [Material appearance editor](appearance-editor.md)
- [Local settings history](settings-history.md)
- [Scheduled settings](scheduled-settings.md)
