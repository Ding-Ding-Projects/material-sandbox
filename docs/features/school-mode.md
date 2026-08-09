# School mode

The Global Settings presentation group provides a persisted, user-renamable School mode. Enabling it forces app-authored messages to English, suppresses playful presentation controls, and keeps the user's prior choices stored for restoration after the mode is disabled. The mode name is a display label only and does not change application identity or data locations.

The current implementation intentionally does not store an unlock credential. Adding a shared PIN or passkey requires an operating-system credential-vault contract and is tracked separately rather than placing secrets in profile configuration.

Verification: `node scripts/validate-school-mode.mjs`; CI also bundles `Docs/school-mode.md` through the SandMan resource file.

## GitHub Pages workspace

The GitHub Pages workspace keeps a separate browser-local focused mode with a
visitor-chosen name and a browser-local hash for its unlock code. It is an
experience lock rather than an account-security boundary: clearing the site’s
local data intentionally resets it. While enabled, the workspace forces English
and omits language, bilingual, funny-level, dim-sum, and related documentation
routes from cards, search, palette results, direct hash routes, notifications,
and active toasts. A renamed mode keeps the exact chosen name in its dialog,
search, close action, notifications, and accessible labels.

The Pages static site never exports the unlock hash; importing preferences
explicitly leaves focused mode off. Run `node scripts/validate-pages-material.mjs`
and verify enable, renamed unlock labels, direct blocked article routes, and
hidden discovery results in a browser.

Suggested articles: [Material design](../material-design.md), [Settings history](settings-history.md), [Notifications](notifications.md).
