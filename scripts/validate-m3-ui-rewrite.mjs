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
  `${common}/TabStateManager.h`, `${common}/TabStateManager.cpp`,
  `${base}/Windows/M3ShellHost.h`, `${base}/Windows/M3ShellHost.cpp`,
  `${base}/Windows/M3RegexExecutionPolicy.h`, `${base}/Windows/M3RegexExecutionPolicy.cpp`,
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
expect(search.includes('void CM3SearchField::paintEvent') && search.includes('M3Tokens::colors(dark).error') && search.includes('m_lineEdit->hasFocus()'), 'search capsule paints Material error and focus outlines');
expect(search.includes('m_clearButton->installEventFilter(this)') && search.includes('m_regexButton->installEventFilter(this)') && search.includes('setProperty("m3Focus", focused)'), 'search action focus updates the capsule state');
expect(search.includes('setProperty("m3Invalid", invalid)') && search.includes('m_regexButton->setProperty("m3Invalid", invalid)'), 'invalid regex state reaches the pill and its builder action');
expect(search.includes('openAnchored(m_regexButton, m_lineEdit)') && search.includes('execAnchored(m_regexButton, m_lineEdit)') && search.includes('popupAncestor->windowType() != Qt::Popup'), 'builder cancellation returns focus to the originating search editor without changing its anchor');
expect(search.includes('restoreSearchStateAfterCancellation') && search.includes('QPointer<CM3SearchField> self(this)') && search.includes('QPointer<QWidget> popupGuard(popupAncestor)'), 'cancellation restores the regex action state and preserves popup lifetime across nested execution');
expect(search.includes('if (popupGuard)\n        popupGuard->setProperty("m3ChildDialogActive", false);\n    if (!self || !popupGuard)'), 'nested popup cleanup clears the child-dialog state before a retired search field can return');
expect(search.includes('void CM3SearchField::clearSearch()') && search.includes('m_lineEdit->clear();') && search.includes('focusEditor();'), 'clearing a query returns keyboard focus to the editor');
expect(search.includes('emit escapePressed()') && search.includes('execAnchored(m_regexButton, m_lineEdit)') && search.includes('m3ResumeMenuSearch'), 'menu-originated builder preserves Escape and menu restoration APIs');
expect(regexHeader.includes('int execAnchored(QWidget* origin, QWidget* focusReturnTarget = nullptr);') && regexHeader.includes('QPointer<QWidget> m_focusReturnTarget;'), 'regex builder keeps the menu-safe modal entry point with an explicit focus-return target');
expect(regexBuilder.includes('m_focusReturnTarget = focusReturnTarget ? focusReturnTarget : origin;') && regexBuilder.includes('if (!m_restoreOriginFocus || !m_focusReturnTarget)'), 'regex builder restores the requested focus target after cancellation');
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
const materialTheme = read(`${common}/MaterialTheme.cpp`);
expect(materialTheme.includes('QWidget[m3SearchSurface="true"] QLineEdit#m3SearchInput') && materialTheme.includes('QToolButton#m3RegexBuilderButton[m3Invalid="true"]'), 'Material theme renders the search capsule interior and invalid builder action');

const workspace = read(`${base}/Windows/M3WorkspaceHost.cpp`);
expect(workspace.includes('takeCentralWidget'), 'workspace preserves and rehosts existing central view');
expect(workspace.includes('qobject_cast<CM3WorkspaceHost*>(current)'), 'workspace reuses only the active central host');
expect(workspace.includes('staleHosts'), 'workspace retires stale hosts left by RebuildUI');
expect(workspace.includes('findExistingAction'), 'core destination adapters reuse existing actions');
expect(workspace.includes('new CMemorySyncView'), 'memory views are registered');
expect(!workspace.includes('QStringLiteral("settings"), QStringLiteral("options")'), 'Settings adapter does not select broad Box Options actions');

const pageHost = read(`${base}/Windows/M3PageNavigationHost.cpp`);
const pageHostHeader = read(`${base}/Windows/M3PageNavigationHost.h`);
const tabStateManager = read(`${common}/TabStateManager.cpp`);
const tabStateManagerHeader = read(`${common}/TabStateManager.h`);
expect(pageHost.includes('replaceWidget(tabs, host'), 'Settings/Options are adapted in place');
expect(pageHost.includes('setData(Qt::UserRole'), 'page identity is retained');
expect(pageHost.includes('m_adaptedTabs->setCurrentIndex'), 'original QTabWidget remains authoritative');
expect(pageHostHeader.includes('void rebind(QTabWidget* tabs)'), 'page host can rebind a final tab container');
expect(pageHostHeader.includes('QStackedLayout* pages'), 'page host can bind a final tree-converted stack');
expect(pageHost.includes('m_adaptedStack->setCurrentIndex'), 'M3 navigation drives tree-converted pages');
expect(pageHost.includes('new CTabStateManager(tabs'), 'page host binds state management to the final tab source');
expect(pageHost.includes('new CTabStateManager(pages'), 'page host binds state management to the final tree stack');
expect(pageHost.includes('releaseStateManager();'), 'page host synchronously detaches stale state managers');
expect(tabStateManagerHeader.includes('QPointer<QTabWidget> m_tabs'), 'tab state manager guards retired tab sources');
expect(tabStateManagerHeader.includes('QPointer<QStackedLayout> m_pages'), 'tab state manager guards final tree stacks');
expect(tabStateManager.includes('assignStablePageKeys();') || tabStateManager.includes('if (!assignStablePageKeys())'),
  'tab state manager assigns nonempty stable page identities');
expect(tabStateManager.includes('delete shortcut.data();'), 'tab state manager removes host-parented shortcuts on recreation');
expect(!pageHost.includes('removeTab('), 'Settings/Options adapter does not detach live pages');

const pageHostTests = read(`${base}/Tests/M3PageNavigationHostTests.cpp`);
const pageHostTestProject = read(`${base}/Tests/M3PageNavigationHostTests.pro`);
const pageHostVisualStudioProject = read(`${base}/Tests/M3PageNavigationHostTests.vcxproj`);
const pageHostVisualStudioFilters = read(`${base}/Tests/M3PageNavigationHostTests.vcxproj.filters`);
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
expect(pageHostTests.includes('host->isVisible()'), 'page-host tests require effective host visibility');
expect(pageHostTests.includes('navigation->viewport()->isVisible()'), 'page-host tests require rendered navigation visibility');
expect(pageHostTests.includes('PortableSettingsFixture'), 'page-host tests use real CSettings persistence');
expect(pageHostTests.includes('findChildren<CTabStateManager*>'), 'page-host tests drive the production state-manager seam');
expect(pageHostTests.includes('Ctrl+Shift+O'), 'page-host tests verify the final search shortcuts');
expect(pageHostTests.includes('tabStateManagerKey'), 'page-host tests require unique stable page identities');
expect(pageHostTests.includes('#include <QtWidgets>'), 'page-host tests include the widget declarations required by SettingsWidgets.h');
expect(pageHostTests.includes('convertWithProductionConfigDialog'), 'page-host tree tests call the production CConfigDialog conversion');
expect(!pageHostTests.includes('convertToTreeLikeConfigDialog'), 'page-host tree tests do not retain the removed conversion fixture');
expect(pageHostTestProject.includes('QT += core gui network widgets testlib'), 'page-host QtTest target links Qt Test and Widgets');
expect(pageHostTestProject.includes('-lMiscHelpers'), 'page-host QtTest target links the production state manager');
expect(pageHostTestProject.includes('../Windows/M3RegexExecutionPolicy.cpp') && pageHostTestProject.includes('../Windows/M3DialogHost.cpp'), 'page-host QtTest links the search policy and shared dialog host');
expect(pageHostTestProject.includes('../Windows/M3Menu.cpp') && pageHostTestProject.includes('../Windows/M3Menu.h'), 'page-host QtTest links the native menu restoration path');
expect(pageHostTestProject.includes('../../MiscHelpers/Common/MaterialTheme.cpp') && pageHostTestProject.includes('../../MiscHelpers/Common/M3Tokens.cpp'), 'page-host QtTest links the Material paint sources');
expect(pageHostVisualStudioProject.includes('<QtModules>core;gui;network;widgets;testlib</QtModules>'), 'page-host QtTest target is wired for Visual Studio');
expect(pageHostVisualStudioProject.includes('<AdditionalDependencies>MiscHelpers.lib;'), 'Visual Studio page-host tests link the production state manager');
expect(pageHostVisualStudioProject.includes('..\\Windows\\M3RegexExecutionPolicy.cpp') && pageHostVisualStudioProject.includes('..\\Windows\\M3DialogHost.cpp'), 'Visual Studio page-host QtTest compiles the search policy and shared dialog host');
expect(pageHostVisualStudioProject.includes('..\\Windows\\M3Menu.cpp') && pageHostVisualStudioProject.includes('<QtMoc Include="..\\Windows\\M3Menu.h" />'), 'Visual Studio page-host QtTest compiles and MOCs the native menu restoration path');
expect(pageHostVisualStudioProject.includes('Include="..\\..\\MiscHelpers\\Common\\MaterialTheme.cpp">\n      <PrecompiledHeader>NotUsing</PrecompiledHeader>') && pageHostVisualStudioProject.includes('Include="..\\..\\MiscHelpers\\Common\\M3Tokens.cpp">\n      <PrecompiledHeader>NotUsing</PrecompiledHeader>'), 'Visual Studio page-host QtTest compiles the Material paint sources without PCH');
expect(pageHostVisualStudioFilters.includes('..\\Windows\\M3Menu.cpp') && pageHostVisualStudioFilters.includes('..\\Windows\\M3Menu.h'), 'Visual Studio page-host test filters expose the native menu restoration path');
expect(pageHostVisualStudioFilters.includes('..\\..\\MiscHelpers\\Common\\MaterialTheme.cpp') && pageHostVisualStudioFilters.includes('..\\..\\MiscHelpers\\Common\\M3Tokens.cpp'), 'Visual Studio page-host test filters expose the Material paint sources');
expect(pageHostTests.includes('searchCapsuleStatesAreVisibleAndAccessible') && pageHostTests.includes('searchBuilderCancellationReturnsFocusToEditor') && pageHostTests.includes('nativeMenuBuilderCancellationPreservesFilterAndFocus'), 'page-host QtTest covers capsule states and cancel/Escape focus return');
expect(pageHostTests.includes('m3ResumeMenuSearch') && pageHostTests.includes('m3ChildDialogActive') && pageHostTests.includes('QVERIFY2(dismissed && !deadline'), 'page-host QtTest proves native menu restoration with a bounded nested-dialog watchdog');
expect(visualStudioSolution.includes('SandMan\\Tests\\M3PageNavigationHostTests.vcxproj'), 'page-host QtTest target is present in the Visual Studio solution');
expect(qmakeBuild.includes('SandMan\\Tests\\M3PageNavigationHostTests.pro'), 'x64 qmake build compiles the page-host QtTest target');
expect(qmakeBuild.includes('M3PageNavigationHostTests.exe -o M3PageNavigationHostTests.txt,txt'), 'x64 qmake build runs the page-host lifecycle suite');
expect(qmakeBuild.includes('if /I "%~3"=="build_only" set "SBIE_QMAKE_BUILD_ONLY=1"'), 'qmake build-only mode is explicit for workflow builds');
expect(qmakeBuild.includes('IF "%SBIE_QMAKE_BUILD_ONLY%"=="1" GOTO :after_page_host_tests'), 'workflow builds skip the local lifecycle executable');

const pageHostLaunch = 'release\\M3PageNavigationHostTests.exe -o M3PageNavigationHostTests.txt,txt';
const runtimePath = 'set "PATH=%qt_path%\\bin;%~dp0bin\\%build_arch%\\Release;%PATH%"';
const platformPath = 'set "QT_QPA_PLATFORM_PLUGIN_PATH=%qt_path%\\plugins\\platforms"';
const hasRunScopedQtRuntime = source => {
  const launchIndex = source.indexOf(pageHostLaunch);
  const pathIndex = source.indexOf(runtimePath);
  const pluginIndex = source.indexOf(platformPath);
  const coreProbeIndex = source.indexOf('where "%qt_core_dll%" >nul 2>&1');
  return launchIndex >= 0
    && pathIndex >= 0 && pathIndex < launchIndex
    && pluginIndex >= 0 && pluginIndex < launchIndex
    && coreProbeIndex >= 0 && coreProbeIndex < launchIndex;
};
expect(hasRunScopedQtRuntime(qmakeBuild), 'page-host QtTest resolves Qt and plugins from the verified run-scoped path');
expect(!hasRunScopedQtRuntime(qmakeBuild.replace(runtimePath, 'rem runtime path removed')),
  'page-host runtime wiring Chut fails when the Qt DLL path is removed');

const hasFreshPageHostBuild = source => {
  const cleanIndex = source.indexOf('rmdir /S /Q "%~dp0Build_M3PageNavigationHostTests_%build_arch%"');
  const qmakeIndex = source.indexOf('%qt_path%\\bin\\qmake.exe %~dp0\\SandMan\\Tests\\M3PageNavigationHostTests.pro');
  const jomIndex = source.indexOf('"%jom%" -f Makefile.Release -j 8', qmakeIndex);
  const qmakeExitIndex = source.indexOf('IF %ERRORLEVEL% NEQ 0 goto :error', qmakeIndex);
  return cleanIndex >= 0 && cleanIndex < qmakeIndex
    && qmakeIndex >= 0 && qmakeExitIndex > qmakeIndex
    && qmakeExitIndex < jomIndex;
};
expect(hasFreshPageHostBuild(qmakeBuild), 'page-host QtTest uses a fresh build directory and checks qmake before jom');
expect(qmakeBuild.includes(':error'), 'page-host QtTest has a concrete failure label');

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
expect(settingsWindow.includes('m_pPageNavigationHost->rebind(ui.tabs, theConf, QStringLiteral("SettingsWindow/Tabs"))'), 'Settings binds state to the final normal tab container');
expect(optionsWindow.includes('m_pPageNavigationHost->rebind(ui.tabs, theConf, QStringLiteral("OptionsWindow/Tabs"))'), 'Box Options binds state to the final normal tab topology');
expect(!settingsWindow.includes('new CTabStateManager'), 'Settings does not restore tab state before final topology');
expect(!optionsWindow.includes('new CTabStateManager'), 'Box Options does not restore tab state before final topology');

const hasOrderedTreeBinding = (source, functionName, stateKey) => {
  const start = source.indexOf(functionName);
  const body = start >= 0 ? source.slice(start, source.indexOf('\n}', start) + 2) : '';
  const releaseIndex = body.indexOf('releaseStateManager()');
  const conversionIndex = body.indexOf('ConvertToTree(ui.tabs)');
  const managedRebindIndex = body.indexOf('m_pPageNavigationHost->rebind(pAltView');
  const stateKeyIndex = body.indexOf(stateKey);
  return releaseIndex >= 0 && releaseIndex < conversionIndex
    && conversionIndex < managedRebindIndex
    && managedRebindIndex < stateKeyIndex;
};
expect(hasOrderedTreeBinding(settingsWindow, 'void CSettingsWindow::OnSetTree()', 'SettingsWindow/Tabs/Tree'), 'Settings detaches before conversion and binds state to the final tree stack');
expect(hasOrderedTreeBinding(optionsWindow, 'void COptionsWindow::OnSetTree()', 'OptionsWindow/Tabs/Tree'), 'Box Options detaches before conversion and binds state to the final tree stack');
expect(settingsWindow.includes('ui.tabEdit = nativeEdit;'), 'Settings retargets OnTab to the surviving native editor page');

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
