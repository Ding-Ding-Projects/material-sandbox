# In-app changelog viewer

The Help → Offline Documentation window keeps the bundled changelog available without network access. The Changelog tab renders each dated entry and preserves the full commit URL when one is recorded, so selecting a link opens the exact implementation commit on the project's forge.

## Search and date filtering

Plain-text search is the default. The adjacent **Regex…** control opens a bounded builder (512-character pattern limit, selectable case-insensitive flag, sample match feedback) attached to this changelog search. A start and end field accept ISO `YYYY-MM-DD` values; invalid or reversed ranges stay visible with inline validation and never silently discard the typed text.

## Copy and export

**Copy filtered** places the currently rendered result set on the clipboard. **Export filtered…** writes the same result set as UTF-8 Markdown/text through the native save picker. Empty results are reported honestly, and no network request is required.

## Failure and security boundaries

The viewer reads only the signed-in application's bundled `:/Docs/changelog.md` resource. Missing or malformed dates fail closed to an explanatory empty state. Commit links remain provider-authored URLs; they are not rewritten or guessed. Export paths are selected by the user and use Qt's native file dialog.

## Verification

`node scripts/validate-docs.mjs` checks the changelog resource, full SHA links, and the viewer's search, regex, date, copy, and export contracts. A native Qt build remains required for runtime visual verification when a Qt/MSVC toolchain is available.

Suggested articles: [Tab discovery](tab-discovery.md), [Settings history](settings-history.md), [Notifications](notifications.md).
