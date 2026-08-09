# School mode

The Global Settings presentation group provides a persisted, user-renamable School mode. Enabling it forces app-authored messages to English, suppresses playful presentation controls, and keeps the user's prior choices stored for restoration after the mode is disabled. The mode name is a display label only and does not change application identity or data locations.

The current implementation intentionally does not store an unlock credential. Adding a shared PIN or passkey requires an operating-system credential-vault contract and is tracked separately rather than placing secrets in profile configuration.

Verification: `node scripts/validate-school-mode.mjs`; CI also bundles `Docs/school-mode.md` through the SandMan resource file.

Suggested articles: [Material design](material-design.md), [Settings history](settings-history.md), [Notifications](notifications.md).
