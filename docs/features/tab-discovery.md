# Tab discovery

The Settings and Options tab strips expose four independent tab-discovery searches from their tab context menu and keyboard paths:

1. **Search current tab strip…** (`Ctrl+Shift+T`) searches the tabs owned by the originating strip.
2. **Search current tab group…** (`Ctrl+Shift+G`) searches only the selected tab's persisted group.
3. **Search tab groups…** (`Ctrl+Shift+N`) searches group names and returns one result per group.
4. **Search all open tabs…** (`Ctrl+Shift+O`) is the master search across every tab in the manager.

Each invocation creates an anchored dialog with its own query, raw pattern, sample text, validation, capture preview, and case-sensitivity state. Plain text is the default; enabling **Regex** exposes the full local builder. Inputs are bounded to 4096 characters, invalid patterns produce no results, and activating a result focuses the real tab and records its stable ID as active. The current strip and master scopes intentionally share the manager's tab collection because each `CTabStateManager` owns one actual strip; they remain separate entry points and stateful dialogs.

## Moving tabs into groups

**Move… into group…** opens an anchored picker beside the originating tab. It lists every stored group, including empty groups, in saved order with its ARGB color, member count, collapsed/expanded state, and accessible description; the picker has its own plain-text search and an explicit Regex builder. **Create new group…** uses the current search value as the proposed name, so a group can be created without leaving the picker. Enter moves the selected tab, Escape cancels, and focus returns to the tab menu.

## Manage tab groups

**Manage tab groups…** is available from every tab context menu on the Settings and Options surfaces. The anchored management panel supports creating empty groups, renaming, choosing an ARGB color, moving a group up or down, collapsing or expanding it, and removing it. Names are trimmed to 1–80 UTF-16 code units, reject control characters, and are unique case-insensitively. Renaming updates every member; removing a group only clears membership and never closes a tab.

Pinned tabs remain first and visible. Unpinned groups follow the saved group order, members retain their existing order, and ungrouped tabs remain last. A collapsed group hides non-active members while the active tab and a tab reached through search remain visible; the collapsed preference is not cleared by navigation or search. Group color is applied to the tab and navigation presentation and is included in accessible descriptions.

## Persistence

`CTabStateManager` stores order, pinned membership, the existing tab-key→group mapping, group metadata, and the active stable tab ID in the existing settings blob. Schema 1 and 2 blobs remain readable and migrate deterministically to schema 3: group names are case-insensitively normalized, groups are ordered by their stored order or a stable case-insensitive fallback, colors are derived from a stable name hash, and collapsed defaults are `false`. Schema 3 stores `groupOrder` plus `groupMetadata` entries containing ARGB color, order, and collapsed state. Stable IDs come from each page's object name and fall back to its title, so callers should provide object names for long-lived tabs.

## Failure modes and security

Patterns and sample text are bounded to 4096 characters and evaluated locally with Qt's `QRegularExpression`; invalid patterns produce an honest validation message and no results. Search does not access network data or hidden page content. A missing or malformed settings blob is ignored, preserving the live tab layout.

## Verification

Run `node scripts/validate-tab-discovery.mjs` to verify the manager, docs article, and both consumer windows retain all four search entry points, keyboard shortcuts, anchored group picker, regex builder, bounded inputs, member counts, and the schema 3 group metadata contract. Runtime capture remains a separate headless-app gate; this lane does not claim full visual capture of every overlay.

## Suggested articles

- [Material appearance editor](appearance-editor.md)
- [Local settings history](settings-history.md)
- [Scheduled settings](scheduled-settings.md)
