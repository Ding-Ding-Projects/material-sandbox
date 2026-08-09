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
HTTPS, bounded refresh intervals, and an opaque `os-vault://` credential
reference; tokens are rejected and never stored. Home Assistant is limited to
boolean entities (`binary_sensor.*` and `input_boolean.*`). The native
credential-vault/network adapter is not yet available, so external sources are
explicitly marked unsupported and remain inert; no network request or blocking
UI path is claimed.
