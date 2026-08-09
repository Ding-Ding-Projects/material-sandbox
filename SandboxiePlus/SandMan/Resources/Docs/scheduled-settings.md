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
allowlisted values before saving. No network or credential source is involved.
