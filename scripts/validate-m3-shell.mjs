import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const sandmanPri = read('SandboxiePlus/SandMan/SandMan.pri');
const cpp = read('SandboxiePlus/SandMan/SandMan.cpp');
const host = read('SandboxiePlus/SandMan/Windows/M3ShellHost.cpp');
const theme = read('SandboxiePlus/MiscHelpers/Common/MaterialTheme.cpp');
const sandman = read('SandboxiePlus/SandMan/SandMan.cpp');
const main = read('SandboxiePlus/SandMan/main.cpp');
const settings = read('SandboxiePlus/SandMan/Windows/SettingsWindow.cpp');
const pri = read('SandboxiePlus/SandMan/SandMan.pri');
const dialogHost = read('SandboxiePlus/SandMan/Windows/M3DialogHost.cpp');
const appearance = read('SandboxiePlus/SandMan/Windows/AppearanceEditorDialog.cpp');
const color = read('SandboxiePlus/SandMan/Windows/ColorTranslatorDialog.cpp');
const docs = read('SandboxiePlus/SandMan/Windows/DocumentationBrowser.cpp');
const destructive = read('SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.cpp');
const mainSource = read('SandboxiePlus/SandMan/main.cpp');
const extractHeader = read('SandboxiePlus/SandMan/Windows/ExtractDialog.h');
const extractSource = read('SandboxiePlus/SandMan/Windows/ExtractDialog.cpp');
const renameHeader = read('SandboxiePlus/SandMan/Windows/RenameSandboxDialog.h');
const renameSource = read('SandboxiePlus/SandMan/Windows/RenameSandboxDialog.cpp');
const compressHeader = read('SandboxiePlus/SandMan/Windows/CompressDialog.h');
const compressSource = read('SandboxiePlus/SandMan/Windows/CompressDialog.cpp');
const editorHeader = read('SandboxiePlus/SandMan/Windows/EditorSettingsWindow.h');
const editorSource = read('SandboxiePlus/SandMan/Windows/EditorSettingsWindow.cpp');
const testProxyHeader = read('SandboxiePlus/SandMan/Windows/TestProxyDialog.h');
const testProxySource = read('SandboxiePlus/SandMan/Windows/TestProxyDialog.cpp');
const selectBoxHeader = read('SandboxiePlus/SandMan/Windows/SelectBoxWindow.h');
const selectBoxSource = read('SandboxiePlus/SandMan/Windows/SelectBoxWindow.cpp');
const boxImageHeader = read('SandboxiePlus/SandMan/Windows/BoxImageWindow.h');
const boxImageSource = read('SandboxiePlus/SandMan/Windows/BoxImageWindow.cpp');
const recoveryHeader = read('SandboxiePlus/SandMan/Windows/RecoveryWindow.h');
const recoverySource = read('SandboxiePlus/SandMan/Windows/RecoveryWindow.cpp');
const snapshotsHeader = read('SandboxiePlus/SandMan/Windows/SnapshotsWindow.h');
const snapshotsSource = read('SandboxiePlus/SandMan/Windows/SnapshotsWindow.cpp');
const popupHeader = read('SandboxiePlus/SandMan/Windows/PopUpWindow.h');
const popupSource = read('SandboxiePlus/SandMan/Windows/PopUpWindow.cpp');
const optionsSource = read('SandboxiePlus/SandMan/Windows/OptionsWindow.cpp');
const settingsSource = read('SandboxiePlus/SandMan/Windows/SettingsWindow.cpp');
const migratedViews = [
  'SandboxiePlus/SandMan/Views/FileView.cpp',
  'SandboxiePlus/SandMan/Views/NtObjectView.cpp',
  'SandboxiePlus/SandMan/Views/SbieView.cpp',
  'SandboxiePlus/SandMan/Views/TraceView.cpp',
  'SandboxiePlus/SandMan/Windows/SnapshotsWindow.cpp',
  'SandboxiePlus/SandMan/Windows/RecoveryWindow.cpp',
  'SandboxiePlus/MiscHelpers/Common/PanelView.h',
  'SandboxiePlus/MiscHelpers/Common/SplitTreeView.cpp',
  'SandboxiePlus/MiscHelpers/Common/SettingsWidgets.cpp',
].map(read);
const checks = [
  [pri.includes('Windows/M3ShellHost.h'), 'header is listed in SandMan.pri'],
  [pri.includes('Windows/M3ShellHost.cpp'), 'source is listed in SandMan.pri'],
  [cpp.includes('M3ShellHost::Install(this, m_pMenuBar)'), 'CSandMan installs the M3 shell'],
  [host.includes('m3ShellInstalled'), 'installation is idempotent'],
  [host.includes('FramelessWindowHint'), 'native chrome is replaced'],
  [!theme.includes('QStyleFactory::create'), 'theme does not reset to a legacy style'],
  [!sandman.includes('QApplication::setStyle(new KeepSubMenusVisibleStyle'), 'main window does not re-wrap legacy chrome'],
  [migratedViews.every((source) => !source.includes('QStyleFactory::create')), 'data views inherit Material 3 instead of forcing Windows style'],
  [!main.includes('app.setStyle("windowsvista")'), 'startup does not select legacy Windows chrome'],
  [!settings.includes('chkUseW11Style') && settings.includes('SetValue("Options/UseW11Style", false)'), 'legacy Windows style control is removed and migrated off'],
  [!fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/CustomStyles.h')) && !read('SandboxiePlus/SandMan/SandMan.pri').includes('CustomStyles.h'), 'obsolete proxy chrome source is removed from the build'],
  [sandmanPri.includes('Windows/M3DialogHost.h') && sandmanPri.includes('Windows/M3DialogHost.cpp'), 'dialog host is registered in qmake'],
  [dialogHost.includes('FramelessWindowHint') && dialogHost.includes('m3DialogInstalled'), 'dialog host is frameless and idempotent'],
  [appearance.includes('M3DialogHost::Install(this)') && color.includes('M3DialogHost::Install(this)'), 'appearance and color dialogs use M3 chrome'],
  [docs.includes('M3DialogHost::Install(this)') && destructive.includes('M3DialogHost::Install(this)'), 'docs and destructive dialogs use M3 chrome'],
  [dialogHost.includes('InstallForApplication') && dialogHost.includes('QEvent::Show') && mainSource.includes('InstallForApplication(&app)'), 'all application dialogs are routed through the M3 host'],
  [extractHeader.includes('m_name') && extractSource.includes('QFormLayout') && !extractSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/ExtractDialog.ui')), 'extract dialog is rebuilt without a legacy .ui form'],
  [renameHeader.includes('m_boxName') && renameSource.includes('QFormLayout') && !renameSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/RenameSandboxDialog.ui')), 'rename dialog is rebuilt without a legacy .ui form'],
  [compressHeader.includes('m_format') && compressSource.includes('QFormLayout') && !compressSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/CompressDialog.ui')), 'compress dialog is rebuilt without a legacy .ui form'],
  [editorHeader.includes('QTableWidget* settingsTable') && editorSource.includes('QVBoxLayout') && !editorSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/EditorSettingsWindow.ui')), 'editor settings dialog is rebuilt without a legacy .ui form'],
  [testProxyHeader.includes('struct Controls') && testProxySource.includes('QStackedWidget') && testProxySource.includes('M3DialogHost::Install(this)') && !testProxySource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/TestProxyDialog.ui')), 'test proxy dialog is rebuilt without a legacy .ui form'],
  [selectBoxHeader.includes('struct Controls') && selectBoxSource.includes('QDialogButtonBox') && selectBoxSource.includes('M3DialogHost::Install(this)') && !selectBoxSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/SelectBoxWindow.ui')), 'select sandbox dialog is rebuilt without a legacy .ui form'],
  [boxImageHeader.includes('struct Controls') && boxImageSource.includes('QGridLayout') && boxImageSource.includes('M3DialogHost::Install(this)') && !boxImageSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/BoxImageWindow.ui')), 'box image dialog is rebuilt without a legacy .ui form'],
  [recoveryHeader.includes('struct Controls') && recoverySource.includes('QGridLayout') && recoverySource.includes('M3DialogHost::Install(this)') && !recoverySource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/RecoveryWindow.ui')), 'recovery dialog is rebuilt without a legacy .ui form'],
  [snapshotsHeader.includes('struct Controls') && snapshotsSource.includes('QGridLayout') && snapshotsSource.includes('M3DialogHost::Install(this)') && !snapshotsSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/SnapshotsWindow.ui')), 'snapshots dialog is rebuilt without a legacy .ui form'],
  [popupHeader.includes('QTableWidget* table') && popupSource.includes('QTableWidget') && popupSource.includes('M3ShellHost::Install(this, nullptr)') && !popupSource.includes('ui.setupUi') && !fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/PopUpWindow.ui')), 'notification popup is rebuilt without a legacy .ui form'],
  [optionsSource.includes('M3DialogHost::Install(this)') && optionsSource.includes('ui.setupUi') && fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/OptionsWindow.ui')), 'options dialog uses native M3 chrome while its large content remains an explicit migration boundary'],
  [settingsSource.includes('M3DialogHost::Install(this)') && settingsSource.includes('ui.setupUi') && !settingsSource.includes('chkFusionTheme') && fs.existsSync(path.join(root, 'SandboxiePlus/SandMan/Forms/SettingsWindow.ui')), 'settings dialog uses native M3 chrome while its large content remains an explicit migration boundary'],
  [settingsSource.includes('nativeLocation') && settingsSource.includes('ui.tabsGUI->insertTab') && settingsSource.includes('ui.cmbFallbackActiveMonitor = new QComboBox(nativeLocation)'), 'settings window options tab is rebuilt with native controls'],
  [settingsSource.includes('m3PresentationSurface') && settingsSource.includes('m3AppearanceIdentitySurface') && settingsSource.includes('m3ScheduledAppearanceSurface') && settingsSource.includes('m3ExternalEditorSurface') && settingsSource.includes('m3NativeSurface'), 'settings appearance and presentation feature groups are native Material 3 surfaces'],
  [settingsSource.includes('nativeGui') && settingsSource.includes('ui.tabsGUI->insertTab(guiIndex, nativeGui') && settingsSource.includes('ui.cmbFontScale = new QComboBox(nativeGui)') && settingsSource.includes('ui.txtEditor = new QLineEdit(nativeGui)'), 'settings GUI appearance/editor child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeNotifications') && settingsSource.includes('ui.tabsGeneral->insertTab(notificationIndex, nativeNotifications') && settingsSource.includes('ui.treeMessages = new QTreeWidget(nativeNotifications)') && settingsSource.includes('ui.chkSilentMode = new QCheckBox'), 'settings notifications child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeGeneral') && settingsSource.includes('ui.tabsGeneral->insertTab(generalIndex, nativeGeneral') && settingsSource.includes('ui.uiLang = new QComboBox(nativeGeneral)') && settingsSource.includes('addShortcut(ui.chkPanic, ui.keyPanic'), 'settings general child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeTray') && settingsSource.includes('ui.tabsShell->insertTab(trayIndex, nativeTray') && settingsSource.includes('ui.cmbSysTray = new QComboBox(nativeTray)') && settingsSource.includes('ui.spnTrayAliasChars = new QSpinBox(nativeTray)'), 'settings system tray child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeRun') && settingsSource.includes('ui.tabsShell->insertTab(runIndex, nativeRun') && settingsSource.includes('ui.treeRun = new QTreeWidget(nativeRun)') && settingsSource.includes('ui.btnAddCmd = new QToolButton(nativeRun)'), 'settings run menu child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeShell') && settingsSource.includes('ui.tabsShell->insertTab(shellIndex, nativeShell') && settingsSource.includes('ui.cmbIntegrateMenu = new QComboBox(nativeShell)') && settingsSource.includes('parentWidget()->layout()->replaceWidget'), 'settings Windows Shell child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeAddonConfig') && settingsSource.includes('ui.tabsAddons->insertTab(addonConfigIndex, nativeAddonConfig') && settingsSource.includes('ui.txtRamLimit = new QLineEdit(nativeAddonConfig)') && settingsSource.includes('ui.cmbRamLetter = new QComboBox(nativeAddonConfig)'), 'settings add-on configuration child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeAddonList') && settingsSource.includes('ui.tabsAddons->insertTab(addonListIndex, nativeAddonList') && settingsSource.includes('ui.treeAddons = new QTreeWidget(nativeAddonList)') && settingsSource.includes('ui.btnInstallAddon = new QPushButton'), 'settings optional add-on list child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeAlert') && settingsSource.includes('ui.tabsControl->insertTab(alertIndex, nativeAlert') && settingsSource.includes('ui.treeWarnProgs = new QTreeWidget(nativeAlert)') && settingsSource.includes('ui.btnAddWarnProg = new QPushButton'), 'settings program alerts child tab is rebuilt with native controls'],
  [settingsSource.includes('nativeForce') && settingsSource.includes('ui.tabsControl->insertTab(forceIndex, nativeForce') && settingsSource.includes('ui.cmbMoTWSandbox = new QComboBox(nativeForce)') && settingsSource.includes('ui.chkForceBoxDocs = new QCheckBox'), 'settings force process child tab is rebuilt with native controls'],
];
for (const [pass, message] of checks) if (!pass) throw new Error(`M3 shell validation failed: ${message}`);
console.log(`m3-shell-contract checks=${checks.length}`);
