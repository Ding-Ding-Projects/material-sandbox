# Command palette

The Windows desktop app opens the command palette with **Ctrl+Shift+F**. It is a keyboard-first Material 3 surface for real commands and destinations already owned by the application; it never invents sample actions.

## Search and navigation

Plain-text matching is the default and includes each destination's label and documented keywords. Press Enter in the results list (or in the search field) to activate the selected item. Settings destinations open the requested settings tab directly, while documentation and update destinations open their existing native surfaces.

The **Regex…** control is anchored beside the search field. It accepts a bounded Qt `QRegularExpression`, case-insensitive matching, and reports invalid patterns without applying them. Applying a pattern synchronizes the search field, pattern, flags, and regex mode; clearing it returns to plain-text search.

## Size and persistence

The palette offers a bounded **Card** view and a **Full window** view. The selected mode is persisted in `UIConfig/CommandPaletteSizeMode` and is restored on the next invocation. Both modes keep the same keyboard order, accessible names, and actions.

## Failure modes and verification

An invalid regular expression leaves the previous search active. A destination that cannot be opened follows its existing application route and does not claim success. The static contract `node scripts/validate-command-palette.mjs` checks the shortcut, anchored builder copy, persisted size key, direct settings destinations, and this article.

Suggested articles: [Material design](material-design.md), [Settings history](settings-history.md), [Scheduled settings](scheduled-settings.md).
