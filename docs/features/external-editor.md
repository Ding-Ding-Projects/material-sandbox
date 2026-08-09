# External editor integration

Settings provides a persisted, user-controlled **VS Code command** field and
an **Open profile folder in VS Code** action. An empty command automatically
checks `code`/`code.cmd` on `PATH`, the per-user VS Code `bin` folder, and the
common `Program Files` installations. A selected executable or command file is
stored as a single path; arguments, tokens, and other secrets are never saved.

The action opens the app's existing profile directory (the same directory that
stores its settings and local history) through `QProcess::startDetached`, with
the folder passed as a separate argument. It does not copy profile data or send
it to a service. The command can be detected, browsed for, or reset to automatic
detection at any time.

## Failure and security boundaries

If VS Code is unavailable, the profile directory is missing, or the child
process cannot start, the settings surface reports the concrete reason in a
non-blocking notification and leaves the profile untouched. The configured
value is treated as one executable path or command name, not as a shell command;
this prevents an editor setting from becoming an argument-injection path.

## Verification

Run `node scripts/validate-external-editor.mjs`. The static contract checks
path detection, persisted configuration, separated process arguments, bundled
offline documentation, failure/security wording, and suggested article links.
A Qt/MSVC build and hidden-desktop launch remain separate runtime evidence.

Suggested articles: [Material appearance editor](appearance-editor.md),
[Settings history](settings-history.md), and [Scheduled settings](scheduled-settings.md).
