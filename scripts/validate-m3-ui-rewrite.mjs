#!/usr/bin/env node
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';

const args = process.argv.slice(2);
let root = process.cwd();
for (let i = 0; i < args.length; i += 1) {
  if (args[i] === '--root') root = path.resolve(args[++i]);
}

const failures = [];
const passes = [];
const read = relative => {
  const absolute = path.join(root, relative);
  if (!fs.existsSync(absolute)) {
    failures.push(`missing: ${relative}`);
    return '';
  }
  return fs.readFileSync(absolute, 'utf8');
};
const expect = (condition, message) => {
  (condition ? passes : failures).push(message);
};

const base = 'SandboxiePlus/SandMan';
const common = 'SandboxiePlus/MiscHelpers/Common';
const required = [
  `${common}/M3Tokens.h`, `${common}/M3Tokens.cpp`, `${common}/MaterialTheme.cpp`,
  `${base}/Windows/M3ShellHost.h`, `${base}/Windows/M3ShellHost.cpp`,
  `${base}/Windows/M3SearchField.h`, `${base}/Windows/M3SearchField.cpp`,
  `${base}/Windows/RegexBuilderDialog.h`, `${base}/Windows/RegexBuilderDialog.cpp`,
  `${base}/Windows/M3Menu.h`, `${base}/Windows/M3Menu.cpp`,
  `${base}/Windows/M3NavigationRail.h`, `${base}/Windows/M3NavigationRail.cpp`,
  `${base}/Windows/M3TabStrip.h`, `${base}/Windows/M3TabStrip.cpp`,
  `${base}/Windows/M3WorkspaceHost.h`, `${base}/Windows/M3WorkspaceHost.cpp`,
  `${base}/Windows/M3PageNavigationHost.h`, `${base}/Windows/M3PageNavigationHost.cpp`,
  `${base}/Tests/M3PageNavigationHostTests.cpp`, `${base}/Tests/M3PageNavigationHostTests.pro`,
  `${base}/Tests/M3PageNavigationHostTests.vcxproj`,
  `${base}/Windows/SnackBar.h`, `${base}/Windows/SnackBar.cpp`,
  `${base}/Views/LocalMemoryRepository.h`, `${base}/Views/LocalMemoryRepository.cpp`,
  `${base}/Views/MemorySyncView.h`, `${base}/Views/MemorySyncView.cpp`,
  `${base}/Views/SkillsView.h`, `${base}/Views/SkillsView.cpp`,
  `${base}/Views/MemoryInventoryView.h`, `${base}/Views/MemoryInventoryView.cpp`,
  `${base}/Views/OperationsView.h`, `${base}/Views/OperationsView.cpp`,
  `${base}/Views/StatusHubView.h`, `${base}/Views/StatusHubView.cpp`,
];
for (const file of required) read(file);

const tokens = read(`${common}/M3Tokens.h`);
const theme = read(`${common}/MaterialTheme.cpp`);
expect(theme.includes('densityFromSettingIndex(density)'), 'theme preserves persisted 0/1/2 density semantics');
for (const [name, value] of Object.entries({
  AppBarHeight: 64, TabStripHeight: 48, NavigationRailWidth: 80,
  RailIndicatorWidth: 56, RailIndicatorHeight: 32, StatusBarHeight: 40,
  MenuRowHeight: 48, SearchHeight: 56, MinimumTarget: 40,
})) {
  expect(new RegExp(`${name}\\s*=\\s*${value}`).test(tokens), `token ${name}=${value}`);
}

const rail = read(`${base}/Windows/M3NavigationRail.cpp`);
const destinationIds = ['boxes','recovery','trace','snapshots','docs','sync','skills','memory','ops','status','settings'];
for (const id of destinationIds) expect(rail.includes(`QStringLiteral("${id}")`), `rail destination ${id}`);
expect(!rail.includes('#9EF2E6'), 'rail uses shared memory tokens instead of light-only hardcode');

const menu = read(`${base}/Windows/M3Menu.cpp`);
expect(menu.includes('Nothing in this menu matches that search.'), 'exact menu empty state');
expect(menu.includes('separator->setObjectName'), 'searchable menu retains explicit separators');
expect(menu.includes('row.widget->setVisible(!m_filterActive)'), 'custom menu hides separators while filtering');
expect(menu.includes('NativeMenuSearchInstaller'), 'native QMenu search preserves synchronous context-menu paths');
expect(menu.includes('m3OriginalVisible'), 'native menu restores original action visibility');
expect(menu.includes('button->setVisible(action->isVisible())'), 'custom menu respects live QAction visibility');
expect(menu.includes('item->widget()->hide()'), 'menu rebuild hides retired rows before deferred deletion');

const search = read(`${base}/Windows/M3SearchField.cpp`);
expect(search.includes('kMaximumPatternLength = 500'), 'search input is bounded');
expect(search.includes('QRegularExpression::escape'), 'plain search remains escaped');
expect(search.includes("Use i, m, s, x, or U"), 'regex flags are allow-listed');

const workspace = read(`${base}/Windows/M3WorkspaceHost.cpp`);
expect(workspace.includes('takeCentralWidget'), 'workspace preserves and rehosts existing central view');
expect(workspace.includes('qobject_cast<CM3WorkspaceHost*>(current)'), 'workspace reuses only the active central host');
expect(workspace.includes('staleHosts'), 'workspace retires stale hosts left by RebuildUI');
expect(workspace.includes('findExistingAction'), 'core destination adapters reuse existing actions');
expect(workspace.includes('new CMemorySyncView'), 'memory views are registered');
expect(!workspace.includes('QStringLiteral("settings"), QStringLiteral("options")'), 'Settings adapter does not select broad Box Options actions');

const pageHost = read(`${base}/Windows/M3PageNavigationHost.cpp`);
const pageHostHeader = read(`${base}/Windows/M3PageNavigationHost.h`);
expect(pageHost.includes('replaceWidget(tabs, host'), 'Settings/Options are adapted in place');
expect(pageHost.includes('setData(Qt::UserRole'), 'page identity is retained');
expect(pageHost.includes('m_adaptedTabs->setCurrentIndex'), 'original QTabWidget remains authoritative');
expect(pageHostHeader.includes('void rebind(QTabWidget* tabs)'), 'page host can rebind a final tab container');
expect(pageHostHeader.includes('QStackedLayout* pages'), 'page host can bind a final tree-converted stack');
expect(pageHost.includes('m_adaptedStack->setCurrentIndex'), 'M3 navigation drives tree-converted pages');
expect(!pageHost.includes('removeTab('), 'Settings/Options adapter does not detach live pages');

const pageHostTests = read(`${base}/Tests/M3PageNavigationHostTests.cpp`);
const pageHostTestProject = read(`${base}/Tests/M3PageNavigationHostTests.pro`);
const pageHostVisualStudioProject = read(`${base}/Tests/M3PageNavigationHostTests.vcxproj`);
const visualStudioSolution = read('SandboxiePlus/SandboxiePlus.sln');
const qmakeBuild = read('SandboxiePlus/qmake_plus.cmd');
expect((pageHostTests.match(/QEvent::DeferredDelete/g) || []).length >= 1, 'page-host tests process deferred deletes');
for (const scenario of [
  'settingsNormalRebindSurvivesDeferredDelete',
  'settingsOptionTreeRebindSurvivesDeferredDeletes',
  'optionsNormalRefreshSurvivesDeferredDelete',
  'optionsOptionTreeRebindSurvivesDeferredDelete',
]) {
  expect(pageHostTests.includes(scenario), `page-host lifecycle test covers ${scenario}`);
}
expect(pageHostTests.includes('navigation->setCurrentRow(index)'), 'page-host tests drive every page through visible M3 navigation');
expect(pageHostTests.includes('QCOMPARE(host->currentPage(), expectedPages.at(index))'), 'page-host tests assert each current widget');
expect(pageHostTestProject.includes('QT += core gui network widgets testlib'), 'page-host QtTest target links Qt Test and Widgets');
expect(pageHostVisualStudioProject.includes('<QtModules>core;gui;network;widgets;testlib</QtModules>'), 'page-host QtTest target is wired for Visual Studio');
expect(visualStudioSolution.includes('SandMan\\Tests\\M3PageNavigationHostTests.vcxproj'), 'page-host QtTest target is present in the Visual Studio solution');
expect(qmakeBuild.includes('SandMan\\Tests\\M3PageNavigationHostTests.pro'), 'x64 qmake build compiles the page-host QtTest target');
expect(qmakeBuild.includes('M3PageNavigationHostTests.exe -o M3PageNavigationHostTests.txt,txt'), 'x64 qmake build runs the page-host lifecycle suite');

const memoryFiles = required.filter(file => file.includes('/Views/')).map(read).join('\n');
for (const forbidden of ['QProcess', 'QNetworkAccessManager', 'QTcpSocket', 'system(', 'ShellExecute', 'CreateProcess']) {
  expect(!memoryFiles.includes(forbidden), `memory surfaces forbid ${forbidden}`);
}
expect(memoryFiles.includes('1024 * 1024'), 'memory reader enforces 1 MiB bound');
expect(memoryFiles.includes('2000'), 'memory listing enforces 2,000-entry bound');
expect(memoryFiles.includes('canonicalFilePath'), 'memory paths are canonicalized');
expect(memoryFiles.includes('isWithinRoot'), 'memory path traversal is fail-closed');

const shell = read(`${base}/Windows/M3ShellHost.cpp`);
expect(shell.includes('setFixedHeight(64)'), 'top app bar is 64 px');
expect(shell.includes('setFixedHeight(40)'), 'status bar is 40 px');
expect(shell.includes('CM3Menu::popup'), 'top menus use searchable M3Menu');
expect(shell.includes('CM3PageNavigationHost::adapt'), 'fallback dialog host can adapt Settings/Options');
expect(shell.includes('InstallDialog(QDialog* dialog, const QString& title)'), 'legacy two-argument dialog API remains source-compatible');
expect(shell.includes('m3ShellRoot'), 'legacy shell root object name is retained');
expect(shell.includes('m3TitleBarLabel'), 'legacy title label object name is retained');
expect(shell.includes('QTimer::singleShot(0, window'), 'status widgets added after shell installation are restyled');
expect(shell.includes('void Refresh(QMainWindow* window, QMenuBar* menuBar)'), 'shell exposes a rebuild-safe refresh path');
expect(shell.includes('delete m_results'), 'window-level command results follow app-bar lifetime');
expect(shell.includes('m_optionalButtons'), 'app bar sheds optional controls at narrow widths');
expect(shell.includes('available < 620 ? 96 : 160'), 'global search has a bounded narrow-width mode');

const settingsWindow = read(`${base}/Windows/SettingsWindow.cpp`);
const optionsWindow = read(`${base}/Windows/OptionsWindow.cpp`);
expect(settingsWindow.includes('CM3PageNavigationHost::adapt(this, ui.tabs, tr("Search settings"))'), 'Settings uses live-tab two-pane adapter');
expect(optionsWindow.includes('CM3PageNavigationHost::adapt(this, ui.tabs, tr("Search sandbox options"))'), 'Box Options uses live-tab two-pane adapter');
expect(settingsWindow.includes('m_pPageNavigationHost->rebind(ui.tabs)'), 'Settings rebinds the final normal tab container');
expect(optionsWindow.includes('m_pPageNavigationHost->rebind(ui.tabs)'), 'Box Options refreshes the final normal tab topology');
expect(settingsWindow.includes('m_pPageNavigationHost->rebind(pAltView, m_pStack, m_pTree)'), 'Settings keeps M3 navigation in OptionTree mode');
expect(optionsWindow.includes('m_pPageNavigationHost->rebind(pAltView, m_pStack, m_pTree)'), 'Box Options keeps M3 navigation in OptionTree mode');

const tabStrip = read(`${base}/Windows/M3TabStrip.cpp`);
expect(!tabStrip.includes('m_tabBar->clear()'), 'tab strip avoids nonexistent QTabBar::clear API');
expect(tabStrip.includes('setFixedSize(40, 40)'), 'tab close target is at least 40 by 40');
const railSource = read(`${base}/Windows/M3NavigationRail.cpp`);
expect(railSource.includes('QScrollArea'), 'navigation rail remains usable in short windows');

const sandman = read(`${base}/SandMan.cpp`);
if (sandman) {
  expect(sandman.includes('CM3WorkspaceHost::install(this);'), 'SandMan installs workspace host');
  expect(sandman.includes('M3ShellHost::Refresh(this, m_pMenuBar);'), 'RebuildUI refreshes the app bar action graph');
  expect((sandman.match(/CM3WorkspaceHost::install\(this\);/g) || []).length >= 2, 'RebuildUI reinstalls the workspace around the new central widget');
}

if (failures.length) {
  console.error(`M3 UI rewrite validation failed (${failures.length})`);
  for (const failure of failures) console.error(`  - ${failure}`);
  process.exit(1);
}
console.log(`M3 UI rewrite validation passed (${passes.length} checks)`);
for (const pass of passes) console.log(`  ✓ ${pass}`);
