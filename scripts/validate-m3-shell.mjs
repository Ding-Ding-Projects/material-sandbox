import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const pri = read('SandboxiePlus/SandMan/SandMan.pri');
const cpp = read('SandboxiePlus/SandMan/SandMan.cpp');
const host = read('SandboxiePlus/SandMan/Windows/M3ShellHost.cpp');
const theme = read('SandboxiePlus/MiscHelpers/Common/MaterialTheme.cpp');
const sandman = read('SandboxiePlus/SandMan/SandMan.cpp');
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
];
for (const [pass, message] of checks) if (!pass) throw new Error(`M3 shell validation failed: ${message}`);
console.log(`m3-shell-contract checks=${checks.length}`);
