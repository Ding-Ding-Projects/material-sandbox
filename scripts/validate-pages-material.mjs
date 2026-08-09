import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const source = fs.readFileSync(path.join(root, 'docs', 'index.html'), 'utf8');

const required = [
  ['M3 color roles', '--md-sys-color-primary'],
  ['M3 shape tokens', '--md-sys-shape-extra-large'],
  ['M3 elevation', '--md-sys-elevation-1'],
  ['reduced motion', 'prefers-reduced-motion'],
  ['light/dark theme', 'data-theme="dark"'],
  ['tablist', 'role="tablist"'],
  ['regex builder', 'Anchored regex builder'],
  ['command palette shortcut', 'Ctrl+Shift+F'],
  ['persisted preferences', 'localStorage'],
  ['non-blocking notification', 'role="status"'],
  ['conditional installer status', 'No installer is advertised'],
  ['feature inventory', 'feature-card'],
  ['settings search', 'id="settingsSearch"'],
  ['settings regex builder', 'id="settingsRegexBuilder"'],
  ['settings search wiring', 'filterSettings'],
];
const failures = required.filter(([, token]) => !source.includes(token));
const externalAsset = /<(?:script|link|img)[^>]+(?:https?:|fonts\.googleapis|googletagmanager|analytics)/i.test(source);
if (externalAsset) failures.push(['local asset policy', 'no remote scripts, styles, images, or trackers']);
const featureCount = (source.match(/class="card feature-card"/g) || []).length;
if (featureCount < 10) failures.push(['feature inventory count', 'at least 10 feature cards']);
if (failures.length) {
  console.error('Pages Material contract failed');
  for (const [name, token] of failures) console.error(`- ${name}: ${token}`);
  process.exit(1);
}
console.log(`pages-material-contract checks=${required.length + 1} features=${featureCount}`);
