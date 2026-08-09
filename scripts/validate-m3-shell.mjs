import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const pri = read('SandboxiePlus/SandMan/SandMan.pri');
const cpp = read('SandboxiePlus/SandMan/SandMan.cpp');
const host = read('SandboxiePlus/SandMan/Windows/M3ShellHost.cpp');
const checks = [
  [pri.includes('Windows/M3ShellHost.h'), 'header is listed in SandMan.pri'],
  [pri.includes('Windows/M3ShellHost.cpp'), 'source is listed in SandMan.pri'],
  [cpp.includes('M3ShellHost::Install(this, m_pMenuBar)'), 'CSandMan installs the M3 shell'],
  [host.includes('m3ShellInstalled'), 'installation is idempotent'],
  [host.includes('FramelessWindowHint'), 'native chrome is replaced'],
];
for (const [pass, message] of checks) if (!pass) throw new Error(`M3 shell validation failed: ${message}`);
console.log(`m3-shell-contract checks=${checks.length}`);
