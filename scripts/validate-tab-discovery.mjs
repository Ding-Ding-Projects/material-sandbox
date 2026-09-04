import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const manager = fs.readFileSync(path.join(root, 'SandboxiePlus/MiscHelpers/Common/TabStateManager.cpp'), 'utf8');
const header = fs.readFileSync(path.join(root, 'SandboxiePlus/MiscHelpers/Common/TabStateManager.h'), 'utf8');
const settings = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Windows/SettingsWindow.cpp'), 'utf8');
const options = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Windows/OptionsWindow.cpp'), 'utf8');
const navigationHost = fs.readFileSync(path.join(root, 'SandboxiePlus/SandMan/Windows/M3PageNavigationHost.cpp'), 'utf8');
const article = fs.readFileSync(path.join(root, 'docs/features/tab-discovery.md'), 'utf8');
const pages = fs.readFileSync(path.join(root, '.github/workflows/pages.yml'), 'utf8');
const checks = [
  ['manager class', header.includes('CTabStateManager')],
  ['search action', manager.includes('Search all open tabs') && manager.includes('showTabSearch')],
  ['four search scopes', manager.includes('Search current tab strip') && manager.includes('Search current tab group') && manager.includes('Search tab groups') && manager.includes('Search all open tabs')],
  ['scoped search model', header.includes('SearchScope') && manager.includes('showScopedTabSearch') && manager.includes('CurrentStrip') && manager.includes('CurrentGroup') && manager.includes('GroupNames') && manager.includes('MasterTabs')],
  ['keyboard activation', manager.includes('Key_T') && manager.includes('Key_G') && manager.includes('Key_N') && manager.includes('Key_O') && manager.includes('WidgetWithChildrenShortcut')],
  ['group picker', manager.includes('showGroupPicker') && manager.includes('Create new group') && manager.includes('Group search')],
  ['group member counts', manager.includes('%2 tabs') && manager.includes('setBackground')],
  ['regex builder', manager.includes('Regex builder') && manager.includes('QRegularExpression')],
  ['bounded input', manager.includes('left(4096)')],
  ['active schema migration', manager.includes('if (schema >= 2)') && manager.includes('m_active') && manager.includes('schema"), 3')],
  ['settings consumer', settings.includes('CM3PageNavigationHost::adapt') && navigationHost.includes('CTabStateManager')],
  ['options consumer', options.includes('CM3PageNavigationHost::adapt') && navigationHost.includes('CTabStateManager')],
  ['article', article.includes('Search current tab strip') && article.includes('Search all open tabs') && article.includes('Suggested articles')],
  ['Pages gate', pages.includes('validate-tab-discovery.mjs')],
];
const failures = checks.filter(([, ok]) => !ok);
if (failures.length) {
  console.error(`tab-discovery-contract failed: ${failures.map(([name]) => name).join(', ')}`);
  process.exit(1);
}
console.log(`tab-discovery-contract checks=${checks.length}`);
