import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const checks = [
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.h', 'QString id;'],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'QSaveFile'],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'QDataStream'],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'kMaxSnapshotBytes'],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'SnapshotToBase64'],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'SnapshotFromBase64'],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'kSnapshotSchemaVersion'],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'checkpoint('],
  ['SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp', 'settings restore checkpoint'],
  ['SandboxiePlus/MiscHelpers/Common/Settings.cpp', 'm_History->record'],
  ['SandboxiePlus/MiscHelpers/Common/Settings.cpp', 'SnapshotValues'],
  ['SandboxiePlus/MiscHelpers/Common/Settings.cpp', 'ApplySnapshot'],
  ['SandboxiePlus/SandMan/Windows/SettingsWindow.cpp', 'Create full settings checkpoint'],
  ['SandboxiePlus/SandMan/Windows/SettingsWindow.cpp', 'Open settings history'],
  ['SandboxiePlus/SandMan/Resources/SandMan.qrc', 'Docs/settings-history.md'],
  ['SandboxiePlus/MiscHelpers/MiscHelpers.pri', 'LocalSettingsHistory.cpp'],
];
for (const [file, needle] of checks) {
  if (!read(file).includes(needle))
    throw new Error(`${file} is missing ${needle}`);
}
console.log(`settings-history-contract checks=${checks.length}`);
