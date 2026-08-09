# School mode

School mode is a persisted presentation choice for shared or classroom profiles. The user may rename its display label, enable it from Global Settings, and reset the label to **School mode** at any time.

When enabled, app-authored messages use English and serious level-1 wording. Cantonese, bilingual, funny-level, and dialog-emoji presentation controls are omitted from the settings surface while the mode is active. Their previous values remain stored and return when the mode is disabled. This mode changes presentation only; it is not a security boundary and does not alter package identity, data paths, credentials, or sandbox policy.

The setting is stored under `Options/SchoolModeEnabled` and `Options/SchoolModeName` in the existing profile configuration. Empty or overlong labels are normalized to the shipped name or an 80-character bound. No credential is stored by this slice; unlock credentials remain a follow-up requiring an operating-system credential-vault design.

Verification: `node scripts/validate-school-mode.mjs` checks the persisted API, English-forcing path, settings controls, bundled article, and documentation index.

Suggested articles: [Material design](material-design.md), [Settings history](settings-history.md), [Notifications](notifications.md).
