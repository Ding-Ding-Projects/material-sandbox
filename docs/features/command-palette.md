# Command palette

The desktop application exposes a command palette from **Ctrl+Shift+F**. It contains real commands and feature destinations, including direct links into the General, UI/appearance, scheduled-settings, and compatibility settings tabs.

Plain text is the default. The anchored **Regex…** builder beside the search field supports bounded Qt `QRegularExpression` patterns and case-insensitive matching. Invalid patterns remain unapplied. Enter activates the selected result, and the search field also activates its current result with Enter.

The palette's **Card** and **Full window** layouts are persisted under `UIConfig/CommandPaletteSizeMode`. Both layouts preserve the same result order and accessible keyboard path.

## Failure and security notes

The palette only calls existing application routes; it does not create fake commands or network requests. Regex input is bounded to 512 characters and compiled locally. Invalid patterns do not change the active search.

## Verification

Run `node scripts/validate-command-palette.mjs` for the source contract. The Qt resource `:/Docs/command-palette.md` bundles this behavior for offline documentation.

Suggested articles: [Material Design](../material-design.md), [Scheduled settings](scheduled-settings.md), [Settings history](settings-history.md).
