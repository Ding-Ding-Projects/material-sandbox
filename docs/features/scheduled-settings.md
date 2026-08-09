# Scheduled settings

Sandboxie Plus stores a small, offline schedule in `UIConfig/ScheduledRules`.
The value is a compact JSON document with `schemaVersion: 1` and a bounded list
of rules. Each rule has a stable id, label, enabled flag, priority, optional ISO
start/end dates, local start/end times, optional ISO weekday numbers (1 Monday
through 7 Sunday), and an allowlisted value (`language`, `theme`, `density`, or
`accent`). Empty weekdays means every day. A start time after the end time is a
cross-midnight window and includes the following day until the end time. Theme
rules use the real `Options/UseDarkTheme` setting with `light`, `dark`, or
`system` values so the schedule affects the actual app chrome.

Matching is deterministic: the highest priority wins; equal priorities use the
lexically smallest stable rule id. Invalid rules are ignored on load and are
rejected on save. Schema-v1 rules carry a `source` object. `local` is the only
source currently applied. `https-api` accepts only a bounded HTTPS URL without
embedded credentials; `home-assistant` accepts only `binary_sensor.*` or
`input_boolean.*`. Both require an opaque `os-vault://...` credential reference
and a 15-second-to-24-hour refresh bound. External metadata is retained but
reports `unsupported-external-source` and remains inert: no network I/O,
blocking wait, or partial setting change is claimed. Tokens are never stored in
the schedule. A future schema version is ignored rather than guessed.

School mode remains the final presentation gate. A matching language rule cannot
turn Cantonese or bilingual presentation back on while School mode is enabled;
the user's previous presentation choices remain stored and available after unlock.
The scheduler is applied at settings-window startup and can be refreshed by the
host UI without changing package identity or data paths. Editing a rule records
one local settings-history entry because the whole rule set is saved as one
canonical blob.

Verification: run `node scripts/validate-scheduled-settings.mjs`. Native runtime
verification still requires the supported Qt/MSVC build; the static contract does
not claim that hosted compilation or a rendered dialog has passed.

Suggested articles: [appearance](appearance.md), [settings history](settings-history.md), and [language and tone](language-and-tone.md).
