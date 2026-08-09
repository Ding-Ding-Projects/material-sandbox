import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const checks = [
  ['SandboxiePlus/SandMan/Windows/ColorTranslatorDialog.h', 'updateFromHex'],
  ['SandboxiePlus/SandMan/Windows/ColorTranslatorDialog.cpp', 'contrastRatio'],
  ['SandboxiePlus/SandMan/Windows/ColorTranslatorDialog.cpp', 'QColor::fromHslF'],
  ['SandboxiePlus/SandMan/Windows/ColorTranslatorDialog.cpp', 'QString normalized = value.trimmed()'],
  ['SandboxiePlus/SandMan/Windows/SettingsWindow.cpp', 'CColorTranslatorDialog'],
  ['SandboxiePlus/SandMan/SandMan.pri', 'ColorTranslatorDialog.cpp'],
  ['SandboxiePlus/SandMan/Resources/SandMan.qrc', 'Docs/color-translator.md'],
  ['docs/features/color-translator.md', 'HEX/HEX8'],
];
for (const [file, needle] of checks) {
  if (!read(file).includes(needle)) throw new Error(`${file} is missing ${needle}`);
}
console.log(`color-translator-contract checks=${checks.length}`);
