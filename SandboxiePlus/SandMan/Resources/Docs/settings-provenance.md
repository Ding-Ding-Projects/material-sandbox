# Settings explanations and provenance

The Global Settings UI gives the language/tone and Material appearance controls a concrete explanation and a truthful provenance line. Each line names the current value and says whether it came from the profile or from the compiled-in shipped value. The line updates after a control is changed, after typography is edited, and after the appearance reset action.

The provenance indicator checks existing `CSettings::ListSettings()` keys and never writes or exports a new marker. A profile value means the profile contains that key; a compiled-in value means the application is using its documented shipped value because the key is absent. Unsupported typography properties remain disclosed by the native appearance editor; the provenance line does not claim that unsupported Word-style properties are implemented.

Verification: run `node scripts/validate-settings-provenance.mjs`. Native runtime verification should confirm that changing language, funny level, emoji, display name, density, accent, or typography updates the adjacent line without blocking the Settings window.

Suggested articles: [Material appearance editor](appearance-editor.md), [Settings history](settings-history.md), and [Scheduled settings](scheduled-settings.md).
