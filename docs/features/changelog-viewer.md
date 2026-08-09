# In-app changelog viewer

The Help → Offline Documentation window keeps the bundled changelog available without network access. The Changelog tab renders each dated entry and preserves the full commit URL when one is recorded, so selecting a link opens the exact implementation commit on the project's forge.

## Article inventory and navigation

The Articles tab reads the ordered slug, title, canonical source, and related-article records from `docs/articles/index.json` through the bundled `:/Docs/articles/index.json` resource. Its inventory contains exactly 22 feature articles plus the separately labelled screenshot supplement. Qt resource aliases point at those canonical UTF-8 files directly, so the desktop browser and documentation site cannot quietly publish different copies.

Relative Markdown links resolve against the current article's canonical source path, select and render the destination in the same browser, and retain heading fragments. The command palette creates one destination per manifest slug and opens that exact article instead of stopping at the generic documentation window.

## Search and date filtering

Plain-text search is the default. The adjacent **Regex…** control opens a bounded builder (512-character pattern limit, selectable case-insensitive flag, sample match feedback) attached to this changelog search. A start and end field accept ISO `YYYY-MM-DD` values; invalid or reversed ranges stay visible with inline validation and never silently discard the typed text.

## Copy and export

**Copy filtered** places the currently rendered result set on the clipboard. **Export filtered…** writes the same result set as UTF-8 Markdown/text through the native save picker. Empty results are reported honestly, and no network request is required.

## Failure and security boundaries

The viewer reads only the application's bundled `:/Docs/changelog.md` resource. Missing or malformed inventories, articles, titles, or dates fail closed to an explanatory state. Relative destinations must map to the bundled manifest; traversal and unknown paths remain inside the application and are rejected. Only valid HTTPS links without embedded credentials can open through the operating system. HTTP, file, script, and other schemes are blocked. Export paths are selected by the user and use Qt's native file dialog.

## Verification

`node scripts/validate-docs.mjs` checks the exact ordered manifest, canonical real paths, no-link boundary, byte-identical Qt resources, project registration, internal destinations, safe-link code, full SHA links, palette routing, and accessible search contracts. `node --test scripts/test-offline-documentation-contract.mjs` proves the complete fixture and mutations that remove a resource, remove the runtime manifest hook, introduce a copied resource, break an internal link, or substitute a manifest identity. The changed browser, MOC, QRC, and palette translation units compile with Qt 6.8.3/MSVC; runtime visual verification remains a separate release gate.

Suggested articles: [Tab discovery](tab-discovery.md), [Settings history](settings-history.md), [Notifications](notifications.md).
