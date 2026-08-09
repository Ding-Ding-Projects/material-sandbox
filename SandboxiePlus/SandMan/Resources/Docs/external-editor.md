# External editor integration

The Settings appearance surface stores one optional VS Code executable or
`code` command and offers **Open profile folder in VS Code**. Automatic
detection checks `PATH`, the user installation, and common Windows `Program
Files` locations. The profile folder is the app's own settings directory and
is passed as a separate process argument.

No arguments, credentials, or profile contents are persisted or transmitted.
When detection or launch fails, Settings shows a non-blocking explanation and
leaves the profile unchanged.

Verification: `node scripts/validate-external-editor.mjs` checks the source
contract, Qt resource entry, and this article. A built Qt/MSVC runtime launch
is still required for UI proof.

Suggested articles: [Material appearance editor](appearance-editor.md),
[Settings history](settings-history.md), and [Scheduled settings](scheduled-settings.md).
