# Tab discovery

The Settings and Options tab strips expose **Search open tabs…** from their tab context menu. The anchored Material dialog searches visible tab labels, stable tab IDs, and persisted group names. Plain text is the default; enabling **Regex** exposes the adjacent builder fields for a bounded raw pattern, sample text, validation, and capture preview. Case sensitivity is explicit, and results identify pinned and grouped tabs. Activating a result focuses the real tab and records it as the active tab.

## Moving tabs into groups

**Move… into group…** opens an anchored picker beside the originating tab. It lists existing groups with a deterministic colour swatch, member count, and accessible description; the picker has its own plain-text search and an explicit Regex builder. **Create new group…** uses the current search value as the proposed name, so a group can be created without leaving the picker. Enter moves the selected tab, Escape cancels, and focus returns to the tab menu. Group membership is persisted in the same schema-2 state blob.

## Persistence

`CTabStateManager` stores order, pinned membership, group names, and the active stable tab ID in the existing settings blob. Schema 1 blobs remain readable; saves migrate them to schema 2. Stable IDs come from each page's object name and fall back to its title, so callers should provide object names for long-lived tabs.

## Failure modes and security

Patterns and sample text are bounded to 4096 characters and evaluated locally with Qt's `QRegularExpression`; invalid patterns produce an honest validation message and no results. Search does not access network data or hidden page content. A missing or malformed settings blob is ignored, preserving the live tab layout.

## Verification

Run `node scripts/validate-tab-discovery.mjs` to verify the manager, docs article, and both consumer windows retain the search entry point, anchored group picker, regex builder, member counts, and schema 2 active-state contract. Runtime capture remains a separate headless-app gate; this lane does not claim the full four-search or complete group-appearance contract.

## Suggested articles

- [Material appearance editor](appearance-editor.md)
- [Local settings history](settings-history.md)
- [Scheduled settings](scheduled-settings.md)
