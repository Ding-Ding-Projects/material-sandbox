import fs from 'node:fs';

const source = fs.readFileSync('SandboxiePlus/SandMan/SandMan.cpp', 'utf8');
const article = fs.readFileSync('SandboxiePlus/SandMan/Resources/Docs/command-palette.md', 'utf8');
const checks = [
  ['Ctrl+Shift+F shortcut', source.includes('Ctrl+Shift+F')],
  ['anchored regex builder', source.includes('Command palette regex builder') && source.includes('mapToGlobal(QPoint(0, regex->height()))')],
  ['persisted size mode', source.includes('UIConfig/CommandPaletteSizeMode')],
  ['direct settings destinations', source.includes('OpenSettings("General")') && source.includes('OpenSettings("UI")') && source.includes('OpenSettings("Compat")')],
  ['keyboard activation', source.includes('query, &QLineEdit::returnPressed') && source.includes('itemActivated')],
  ['documentation article', article.includes('# Command palette') && article.includes('Suggested articles')],
];
const failed = checks.filter(([, ok]) => !ok);
if (failed.length) {
  console.error(`command-palette-contract failed: ${failed.map(([name]) => name).join(', ')}`);
  process.exit(1);
}
console.log(`command-palette-contract checks=${checks.length}`);
