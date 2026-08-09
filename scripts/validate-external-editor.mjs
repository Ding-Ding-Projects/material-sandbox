import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const sourcePath = path.join(root, 'SandboxiePlus', 'SandMan', 'Windows', 'SettingsWindow.cpp');
const qrcPath = path.join(root, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
const featurePath = path.join(root, 'docs', 'features', 'external-editor.md');
const offlinePath = path.join(root, 'SandboxiePlus', 'SandMan', 'Resources', 'Docs', 'external-editor.md');
for (const file of [sourcePath, qrcPath, featurePath, offlinePath]) {
  if (!fs.existsSync(file)) throw new Error(`missing external-editor artifact: ${path.relative(root, file)}`);
}
const source = fs.readFileSync(sourcePath, 'utf8');
const qrc = fs.readFileSync(qrcPath, 'utf8');
const feature = fs.readFileSync(featurePath, 'utf8');
const offline = fs.readFileSync(offlinePath, 'utf8');
const requiredSource = [
  'CFindExternalEditor', 'COpenFolderInExternalEditor', 'QStandardPaths::findExecutable',
  'LOCALAPPDATA', 'ProgramFiles(x86)', 'UIConfig/ExternalEditorCommand',
  'Open profile folder in VS Code', 'QProcess::startDetached'
];
for (const token of requiredSource) if (!source.includes(token)) throw new Error(`missing source contract: ${token}`);
if (!qrc.includes('Docs/external-editor.md')) throw new Error('offline documentation is not bundled in SandMan.qrc');
for (const text of [feature, offline]) {
  if (!text.startsWith('# ')) throw new Error('external-editor article has no title');
  if (!/Suggested articles:/i.test(text)) throw new Error('external-editor article has no suggested articles');
  if (!/failure|security|verification/i.test(text)) throw new Error('external-editor article omits failure/security/verification coverage');
}
console.log('external-editor-contract checks=12');
