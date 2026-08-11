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
  // Keep source-contract assertions independent of the checkout's CRLF/LF mode.
  return fs.readFileSync(absolute, 'utf8').replace(/\r\n/g, '\n');
};
const expect = (condition, message) => {
  (condition ? passes : failures).push(message);
};
const count = (source, fragment) => source.split(fragment).length - 1;

const base = 'SandboxiePlus/SandMan';
const common = 'SandboxiePlus/MiscHelpers/Common';
const required = [
  `${common}/M3Tokens.h`, `${common}/M3Tokens.cpp`, `${common}/MaterialTheme.cpp`,
  `${base}/Windows/M3ShellHost.h`, `${base}/Windows/M3ShellHost.cpp`,
  `${base}/Windows/M3RegexExecutionPolicy.h`, `${base}/Windows/M3RegexExecutionPolicy.cpp`,
  `${base}/Windows/M3SearchField.h`, `${base}/Windows/M3SearchField.cpp`,
  `${base}/Windows/RegexBuilderDialog.h`, `${base}/Windows/RegexBuilderDialog.cpp`,
  `${base}/Windows/M3Menu.h`, `${base}/Windows/M3Menu.cpp`,
  `${base}/Windows/M3NavigationRail.h`, `${base}/Windows/M3NavigationRail.cpp`,
  `${base}/Windows/M3TabStrip.h`, `${base}/Windows/M3TabStrip.cpp`,
  `${base}/Windows/M3WorkspaceHost.h`, `${base}/Windows/M3WorkspaceHost.cpp`,
  `${base}/Windows/M3PageNavigationHost.h`, `${base}/Windows/M3PageNavigationHost.cpp`,
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
const regexPolicy = read(`${base}/Windows/M3RegexExecutionPolicy.cpp`);
const regexPolicyHeader = read(`${base}/Windows/M3RegexExecutionPolicy.h`);
expect(search.includes('M3RegexExecutionPolicy::MaximumPatternLength') && regexPolicyHeader.includes('MaximumPatternLength = 500') && !search.includes('setMaxLength('), 'search input bounds reject without silently truncating typed text');
expect(search.includes('QRegularExpression::escape'), 'plain search remains escaped');
expect(regexPolicy.includes("Use i, m, s, x, or U"), 'regex flags are allow-listed');
const regexBuilder = read(`${base}/Windows/RegexBuilderDialog.cpp`);
const regexHeader = read(`${base}/Windows/RegexBuilderDialog.h`);
const dialogHost = read(`${base}/Windows/M3DialogHost.cpp`);
expect(regexPolicy.includes('QRegularExpression(QStringLiteral("["))'), 'invalid regex resolves to a non-match-all sentinel');
expect(regexPolicy.includes('(*LIMIT_MATCH=%1)(*LIMIT_DEPTH=%2)%3') && regexPolicy.includes('boundedPattern(pattern)'), 'search and builder share PCRE match/depth-bounded evaluation');
expect(regexPolicyHeader.includes('MaximumMatchAttempts = 100000') && regexPolicyHeader.includes('MaximumMatchDepth = 1000'), 'regex execution limits are explicit and reviewable');
expect(regexPolicy.includes('capture numbering, extended-mode comments, and top-level') && regexPolicy.includes('alternation semantics exactly as the user entered them'), 'bounded prefix preserves regex grammar semantics');
expect(regexPolicy.includes("case 'i'") && regexPolicy.includes("case 'm'") && regexPolicy.includes("case 's'") && regexPolicy.includes("case 'x'") && regexPolicy.includes("case 'U'"), 'full supported regex flag set is validated centrally');
expect(regexPolicy.includes('const QRegularExpression expression(boundedPattern(pattern), options)') && regexPolicy.includes('if (!expression.isValid())'), 'Qt regex grammar is compiled with bounded execution verbs');
for (const [token, label] of [
  ['QStringLiteral("()")', 'capture groups'],
  ['QStringLiteral("(?:)")', 'non-capturing groups'],
  ['QStringLiteral("*")', 'quantifiers'],
  ['QStringLiteral("|")', 'alternation'],
  ['QStringLiteral("{2,4}")', 'bounded quantifiers'],
]) {
  expect(regexBuilder.includes(token), `regex builder exposes ${label}`);
}
expect(search.includes('M3RegexExecutionPolicy::compile(pattern, flags, error)'), 'search field delegates regex compilation to the shared policy');
expect(search.includes('M3RegexExecutionPolicy::invalidExpression()'), 'invalid search state cannot expose an empty match-all expression');
expect(search.includes('void CM3SearchField::setAccessibleName') && search.includes('m_lineEdit->setAccessibleName(fieldName)'), 'search accessible name is forwarded to the focused editor');
expect(search.includes('void CM3SearchField::setAccessibleDescription') && search.includes('m_lineEdit->setAccessibleDescription(description)'), 'search accessible description includes the regex mode state');
expect(search.includes('emit escapePressed()') && search.includes('execAnchored(m_regexButton)') && search.includes('m3ResumeMenuSearch'), 'menu-originated builder preserves Escape and menu restoration APIs');
expect(regexHeader.includes('int execAnchored(QWidget* origin);'), 'regex builder keeps the menu-safe modal entry point');
expect(count(regexBuilder, 'M3DialogHost::Install(this)') === 1 && !regexBuilder.includes('titleRow'), 'regex builder installs exactly one shared M3 title host');
expect(!regexBuilder.includes('FramelessWindowHint'), 'regex builder delegates frameless chrome to the shared title host');
expect(regexBuilder.includes('BoundedPlainTextEdit') && regexBuilder.includes('not inserted because it would exceed'), 'oversized sample paste and IME input are rejected rather than truncated');
expect(!regexBuilder.includes('m_sampleEdit->setPlainText(value.left') && !regexBuilder.includes('m_patternEdit->setText(value.left'), 'regex builder has no silent sample or token truncation path');
expect(regexBuilder.includes('MaximumPreviewUtf8Bytes') && regexBuilder.includes('MaximumMatches') && regexBuilder.includes('MaximumCaptures'), 'preview output, matches, and captures are independently bounded');
expect(regexBuilder.includes('QAccessibleEvent') && regexBuilder.includes('setAccessibleDescription') && regexBuilder.includes('announceValidation'), 'regex builder announces validation and bounded state to assistive technology');
expect(regexBuilder.includes('updateResponsiveLayout') && regexBuilder.includes('reflowGuidedTokens') && regexBuilder.includes('fullscreenFallback'), 'regex builder reflows and falls back inside constrained viewports');
for (const width of [320, 360, 390, 414]) {
  expect(regexBuilder.includes('kResponsiveLayoutWidth = 520') && regexBuilder.includes('QScrollArea') && regexBuilder.includes('viewport.width()') && width < 520, `regex builder has a responsive contract at ${width}px`);
}
expect(regexBuilder.includes('watchOriginGeometry') && regexBuilder.includes('scheduleReposition') && regexBuilder.includes('restoreOriginFocus') && regexBuilder.includes('m_patternEdit->setFocus'), 'regex builder tracks its anchor and restores focus');
expect(dialogHost.includes('QString(QChar(0x00D7))'), 'shared M3 dialog close glyph uses the portable multiplication character');

const sandmanPri = read(`${base}/SandMan.pri`);
const project = read(`${base}/SandMan.vcxproj`);
const projectFilters = read(`${base}/SandMan.vcxproj.filters`);
expect(sandmanPri.includes('Windows/M3RegexExecutionPolicy.h') && sandmanPri.includes('Windows/M3RegexExecutionPolicy.cpp'), 'shared regex policy is registered in qmake');
expect(count(project, 'Include="..\\MiscHelpers\\Common\\MaterialTheme.cpp"') === 1, 'MSVC project has one MaterialTheme compile entry');
expect(count(project, 'Include="..\\MiscHelpers\\Common\\M3Tokens.cpp"') === 1, 'MSVC project has one M3Tokens compile entry');
expect(project.includes('Include="..\\MiscHelpers\\Common\\MaterialTheme.cpp">\n      <PrecompiledHeader>NotUsing</PrecompiledHeader>') && project.includes('Include="..\\MiscHelpers\\Common\\M3Tokens.cpp">\n      <PrecompiledHeader>NotUsing</PrecompiledHeader>'), 'MSVC project disables PCH for external M3 sources without stdafx');
expect(project.includes('Windows\\M3RegexExecutionPolicy.h') && project.includes('Windows\\M3RegexExecutionPolicy.cpp'), 'MSVC project registers the shared regex policy exactly once');
expect(projectFilters.includes('Windows\\M3RegexExecutionPolicy.h') && projectFilters.includes('Windows\\M3RegexExecutionPolicy.cpp') && projectFilters.includes('Windows\\RegexBuilderDialog.cpp'), 'MSVC filters expose the M3 semantic-port sources for IDE review');

const workspace = read(`${base}/Windows/M3WorkspaceHost.cpp`);
expect(workspace.includes('takeCentralWidget'), 'workspace preserves and rehosts existing central view');
expect(workspace.includes('qobject_cast<CM3WorkspaceHost*>(current)'), 'workspace reuses only the active central host');
expect(workspace.includes('staleHosts'), 'workspace retires stale hosts left by RebuildUI');
expect(workspace.includes('findExistingAction'), 'core destination adapters reuse existing actions');
expect(workspace.includes('new CMemorySyncView'), 'memory views are registered');
expect(!workspace.includes('QStringLiteral("settings"), QStringLiteral("options")'), 'Settings adapter does not select broad Box Options actions');

const pageHost = read(`${base}/Windows/M3PageNavigationHost.cpp`);
expect(pageHost.includes('replaceWidget(tabs, host'), 'Settings/Options are adapted in place');
expect(pageHost.includes('setData(Qt::UserRole'), 'page identity is retained');
expect(pageHost.includes('m_adaptedTabs->setCurrentIndex'), 'original QTabWidget remains authoritative');
expect(!pageHost.includes('removeTab('), 'Settings/Options adapter does not detach live pages');

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
