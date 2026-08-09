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
  [settings.includes('ui.chkUseW11Style->hide()') && settings.includes('SetValue("Options/UseW11Style", false)'), 'legacy Windows style setting is hidden and migrated off'],
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
];
for (const [pass, message] of checks) if (!pass) throw new Error(`M3 shell validation failed: ${message}`);
console.log(`m3-shell-contract checks=${checks.length}`);
