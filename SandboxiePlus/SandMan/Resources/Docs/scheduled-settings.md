# Scheduled settings

Scheduled settings are local, offline rules for language, theme, density, and
accent. Rules are stored as schema version 1 in `UIConfig/ScheduledRules` and
resolve by priority, then stable id. Dates use ISO format; times use local time;
empty weekdays means every day. Cross-midnight windows include the next day.

School mode remains the final presentation gate and keeps its English-only
behavior while schedules continue to retain the user's ordinary choices.

The settings editor validates labels, dates, times, priorities, weekdays, and
allowlisted values before saving. No network or credential source is involved.
