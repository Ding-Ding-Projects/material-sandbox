import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const checks = [
  ['SandboxiePlus/MiscHelpers/Common/UserPresentationSettings.h', 'schoolModeEnabled'],
  ['SandboxiePlus/MiscHelpers/Common/UserPresentationSettings.cpp', 'Options/SchoolModeEnabled'],
  ['SandboxiePlus/MiscHelpers/Common/UserPresentationSettings.cpp', 'return LanguageMode::English'],
  ['SandboxiePlus/SandMan/Windows/SettingsWindow.cpp', 'Reset mode name'],
  ['SandboxiePlus/SandMan/Resources/SandMan.qrc', 'Docs/school-mode.md'],
  ['docs/features/school-mode.md', 'operating-system credential-vault'],
];
for (const [file, needle] of checks) {
  if (!read(file).includes(needle)) throw new Error(`${file} is missing ${needle}`);
}
console.log(`school-mode-contract checks=${checks.length}`);
