import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const source = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Windows/SettingsWindow.cpp'), 'utf8');
const docs = fs.readFileSync(path.join(root, 'docs/features/settings-provenance.md'), 'utf8');
const qrc = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Resources/SandMan.qrc'), 'utf8');
const checks = [
  ['presentation provenance reads profile keys', source.includes('Options/LanguageMode') && source.includes('Options/FunnyLevelEnglish') && source.includes('Options/ShowDialogEmojis')],
  ['appearance provenance reads profile keys', source.includes('UIConfig/AppDisplayName') && source.includes('UIConfig/Density') && source.includes('UIConfig/AccentSeed')],
  ['font provenance is disclosed', source.includes('UIConfig/UIFontFamily') && source.includes('typography')],
  ['provenance refreshes after edits', source.includes('refreshPresentationProvenance();') && source.includes('refreshAppearanceProvenance();')],
  ['docs describe fallback semantics', docs.includes('compiled-in value') && docs.includes('profile value') && docs.includes('does not claim')],
  ['offline article bundle', qrc.includes('Docs/settings-provenance.md')],
];
const failed = checks.filter(([, ok]) => !ok).map(([name]) => name);
if (failed.length) throw new Error(`settings-provenance-contract failed: ${failed.join(', ')}`);
console.log(`settings-provenance-contract checks=${checks.length}`);
