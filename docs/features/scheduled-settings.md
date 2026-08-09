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
rejected on save. Schema-v1 rules carry a `source` object. `local` is applied
directly. An external source requires a bounded HTTPS URL without embedded
credentials, query, fragment, redirects, or a non-standard port; a
15-second-to-24-hour refresh bound; and an opaque
`os-vault://scheduled-settings/<id>` credential reference. Tokens are never
stored in the schedule. A future schema version is ignored rather than guessed.

The **Refresh external sources** control is an explicit activation path. It
uses only a matching, enabled external rule and a Windows Credential Manager
generic credential that already exists. It never requests, creates, displays,
or logs a token. Requests reject TLS errors and redirects, abort after 10
seconds, and accept at most 64 KiB of `application/json`. A Home Assistant
source is limited to `/api/states/<entity>` and its `on`/`off` state. An HTTPS
API source accepts exactly `{"active": true}` or `{"active": false}`. An `on`
or `true` response temporarily gates the rule's existing local allowlisted
value. A missing credential, `off`, timeout, redirect, TLS/network failure,
oversized, or malformed response keeps the base settings unchanged. No remote
response is persisted as the user's base setting.

The scheduled-rules list shows a localized source status beside each rule and
in its tooltip. Local rules are labelled active. External rules identify an
invalid reference, a missing Credential Manager item, a safe refresh in
progress, active/inactive last result, or a fail-safe refresh failure without
exposing URLs or credentials.

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

Suggested articles: [appearance](appearance-editor.md), [settings history](settings-history.md), and [language and tone](pages-language-tone.md).
