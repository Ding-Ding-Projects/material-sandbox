import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const settings = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Windows/SettingsWindow.cpp'), 'utf8');
const theme = fs.readFileSync(path.join(root, 'SandboxiePlus/MiscHelpers/Common/MaterialTheme.cpp'), 'utf8');
const docs = fs.readFileSync(path.join(root, 'docs/material-design.md'), 'utf8');
const checks = [
  ['legacy Fusion control removed', !settings.includes('ui.chkFusionTheme')],
  ['legacy Fusion preference cannot re-enable style', settings.includes('theConf->SetValue("Options/UseFusionTheme", 0);')],
  ['shared Material theme remains applied', theme.includes('MaterialTheme::Apply') || theme.includes('BuildStyleSheet')],
  ['migration boundary documented', docs.includes('Fusion') && docs.includes('legacy') && docs.includes('Material 3')],
];
const failed = checks.filter(([, ok]) => !ok).map(([name]) => name);
if (failed.length) throw new Error(`m3-chrome-contract failed: ${failed.join(', ')}`);
console.log(`m3-chrome-contract checks=${checks.length}`);
