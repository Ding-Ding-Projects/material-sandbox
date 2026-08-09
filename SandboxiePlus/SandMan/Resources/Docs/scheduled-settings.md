# Scheduled settings

Sandboxie Plus stores local, offline presentation rules in
`UIConfig/ScheduledRules`. The payload is a schema version 1 JSON document with
stable rule ids, labels, enabled flags, priorities, optional ISO start/end
dates, local start/end times, optional weekday lists, and allowlisted values for
language, theme, density, and accent.

Theme rules apply the real `Options/UseDarkTheme` setting and use `light`,
`dark`, or `system`. Cross-midnight windows are allowed, empty weekdays means
every day, and the highest priority matching rule wins with stable rule id as
the tie-breaker.

School mode remains the final presentation gate and keeps its English-only
behavior while schedules continue to retain the user's ordinary choices. The
settings editor validates labels, dates, times, priorities, weekdays, and
allowlisted values before saving. Schema-v1 also records bounded `source`
metadata: `local`, `https-api`, or `home-assistant`. External sources require
an opaque `os-vault://scheduled-settings/<id>` Windows Credential Manager
reference; tokens are rejected and never stored. HTTPS must not carry embedded
credentials, a query, fragment, redirects, or a non-standard port. Home
Assistant is limited to boolean entities (`binary_sensor.*` and
`input_boolean.*`).

**Refresh external sources** is an explicit activation path. It uses an
existing generic Credential Manager credential without requesting, creating,
displaying, or logging it. Requests reject TLS errors and redirects, abort
after 10 seconds, and accept no more than 64 KiB of `application/json`. Home
Assistant calls only `/api/states/<entity>` and reads `on` or `off`; an HTTPS
API must return exactly `{"active": true}` or `{"active": false}`. Only an
`on`/`true` result temporarily gates the rule's already-local allowlisted
value. Missing credentials, off responses, timeout, redirect, TLS/network
failure, oversized, or malformed data retain the local base setting. Remote
responses are not persisted.

The settings list and each rule tooltip show this state explicitly: local
sources are active; external metadata identifies invalid references, missing
credentials, safe refresh in progress, active/inactive last result, or a
fail-safe failure without exposing URLs or credentials.
