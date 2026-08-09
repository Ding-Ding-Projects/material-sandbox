import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const checks = [
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.h', 'dismissSelected'],
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.h', 'QRegularExpression'],
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.cpp', 'setSelectionMode(QAbstractItemView::ExtendedSelection)'],
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.cpp', 'setMaxLength(512)'],
  ['SandboxiePlus/SandMan/SandMan.cpp', 'm_pNotificationCenter->post'],
  ['SandboxiePlus/SandMan/SandMan.pri', 'NotificationCenter.cpp'],
  ['README.md', 'docs/screenshots.md'],
  ['docs/screenshots.md', 'Material desktop screenshot gallery'],
];
for (const [file, needle] of checks) {
  if (!read(file).includes(needle))
    throw new Error(`${file} is missing ${needle}`);
}
console.log(`notification-and-gallery-contract checks=${checks.length}`);
