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
const featureSlugs = [
  'material-design.md', 'features/appearance-editor.md', 'features/scheduled-settings.md',
  'features/school-mode.md', 'features/tab-discovery.md', 'features/settings-history.md',
  'features/notifications.md', 'features/external-editor.md', 'features/command-palette.md',
  'features/dim-sum-surprise.md', 'features/color-translator.md', 'features/contributor-build-audit.md',
  'features/settings-provenance.md', 'features/build-entrypoints.md', 'features/destructive-confirmation.md',
  'features/native-ci-evidence.md', 'features/ui/m3-shell-boundary.md', 'features/editor-settings.md',
  'features/changelog-viewer.md', 'features/pages-language-tone.md', 'features/pages-a11y-boundary.md',
];
const featureCount = (source.match(/class="card feature-card"/g) || []).length;
for (const slug of featureSlugs) {
  if (!source.includes(`href="${slug}"`)) failures.push(['feature inventory article', slug]);
}
if (featureCount !== featureSlugs.length) failures.push(['feature inventory count', `exactly ${featureSlugs.length} feature cards`]);
if (failures.length) {
  console.error('Pages Material contract failed');
  for (const [name, token] of failures) console.error(`- ${name}: ${token}`);
  process.exit(1);
}
console.log(`pages-material-contract checks=${required.length + 1} features=${featureCount}`);
