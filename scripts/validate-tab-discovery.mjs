import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const manager = fs.readFileSync(path.join(root, 'SandboxiePlus/MiscHelpers/Common/TabStateManager.cpp'), 'utf8');
const header = fs.readFileSync(path.join(root, 'SandboxiePlus/MiscHelpers/Common/TabStateManager.h'), 'utf8');
const settings = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Windows/SettingsWindow.cpp'), 'utf8');
const options = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Windows/OptionsWindow.cpp'), 'utf8');
const article = fs.readFileSync(path.join(root, 'docs/features/tab-discovery.md'), 'utf8');
const pages = fs.readFileSync(path.join(root, '.github/workflows/pages.yml'), 'utf8');
const checks = [
  ['manager class', header.includes('CTabStateManager')],
  ['search action', manager.includes('Search open tabs') && manager.includes('showTabSearch')],
  ['regex builder', manager.includes('Regex builder') && manager.includes('QRegularExpression')],
  ['active schema', manager.includes('schema"), 2') && manager.includes('active')],
  ['settings consumer', settings.includes('CTabStateManager')],
  ['options consumer', options.includes('CTabStateManager')],
  ['article', article.includes('Search open tabs') && article.includes('Suggested articles')],
  ['Pages gate', pages.includes('validate-tab-discovery.mjs')],
];
const failures = checks.filter(([, ok]) => !ok);
if (failures.length) {
  console.error(`tab-discovery-contract failed: ${failures.map(([name]) => name).join(', ')}`);
  process.exit(1);
}
console.log(`tab-discovery-contract checks=${checks.length}`);
