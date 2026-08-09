import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const checks = [
  ['SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.h', 'static bool Confirm'],
  ['SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.h', 'static bool ConfirmAction'],
  ['SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.cpp', 'Emergency exit'],
  ['SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.h', 'keyPressEvent'],
  ['SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.cpp', 'QSlider'],
  ['SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.cpp', 'QGraphicsOpacityEffect'],
  ['SandboxiePlus/SandMan/Windows/DestructiveConfirmationDialog.cpp', 'UIConfig/ReducedMotion'],
  ['SandboxiePlus/SandMan/Views/SbieView.cpp', 'CDestructiveConfirmationDialog::Confirm'],
  ['SandboxiePlus/SandMan/SandMan.cpp', 'ConfirmAction(this, tr("Cleanup all logs and process entries")'],
  ['SandboxiePlus/SandMan/SandMan.pri', 'DestructiveConfirmationDialog.cpp'],
  ['docs/features/destructive-confirmation.md', 'Suggested articles'],
  ['README.md', 'two-key, full-range destructive-action confirmation'],
];
for (const [file, needle] of checks) {
  if (!read(file).includes(needle))
    throw new Error(`${file} is missing ${needle}`);
}
console.log(`destructive-confirmation-contract checks=${checks.length}`);
