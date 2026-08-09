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
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.cpp', 'Regular-expression flags'],
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.cpp', 'Sample text'],
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.cpp', 'Valid Qt QRegularExpression'],
  ['SandboxiePlus/SandMan/Windows/NotificationCenter.cpp', 'CaseInsensitiveOption'],
  ['SandboxiePlus/SandMan/SandMan.cpp', 'm_pNotificationCenter->post'],
  ['SandboxiePlus/SandMan/OnlineUpdater.cpp', 'OnLogMessage(tr("No new updates found'],
  ['SandboxiePlus/SandMan/OnlineUpdater.cpp', 'OnLogMessage(tr("Failed to check for updates, error: %1").arg(Error), true)'],
  ['SandboxiePlus/SandMan/BoxTransfer.cpp', 'OnLogMessage(CBoxTransferDialog::tr("Nothing selected for export."), true)'],
  ['SandboxiePlus/SandMan/BoxTransfer.cpp', 'OnLogMessage(CBoxTransferDialog::tr("No boxes selected for separate file export."), true)'],
  ['SandboxiePlus/SandMan/Wizards/BoxAssistant.cpp', 'OnLogMessage(tr("Your issue report has been successfully submitted, thank you."), true)'],
  ['SandboxiePlus/SandMan/Windows/OptionsGeneral.cpp', 'OnLogMessage(tr("Image Password Changed"), true)'],
  ['SandboxiePlus/SandMan/SandMan.pri', 'NotificationCenter.cpp'],
  ['README.md', 'docs/screenshots.md'],
  ['docs/screenshots.md', 'Material desktop screenshot gallery'],
];
for (const [file, needle] of checks) {
  if (!read(file).includes(needle))
    throw new Error(`${file} is missing ${needle}`);
}
const updater = read('SandboxiePlus/SandMan/OnlineUpdater.cpp');
if (updater.includes('QMessageBox::information(theGUI') && updater.includes('No new updates found'))
  throw new Error('manual update success must use the non-blocking notification center');
if (updater.includes('QMessageBox::critical(theGUI, "Sandboxie-Plus", tr("Failed to check for updates'))
  throw new Error('manual update-check failure must use the non-blocking notification center');
const transfer = read('SandboxiePlus/SandMan/BoxTransfer.cpp');
if (transfer.includes('QMessageBox::information(parent, "Sandboxie-Plus", CBoxTransferDialog::tr("Nothing selected for export."))'))
  throw new Error('empty export selection must use the non-blocking notification center');
if (transfer.includes('QMessageBox::information(parent, "Sandboxie-Plus", CBoxTransferDialog::tr("No boxes selected for separate file export."))'))
  throw new Error('empty separate-file export selection must use the non-blocking notification center');
const assistant = read('SandboxiePlus/SandMan/Wizards/BoxAssistant.cpp');
if (assistant.includes('QMessageBox::information(this, "Sandboxie-Plus", tr("Your issue report has been successfully submitted, thank you."))'))
  throw new Error('issue report success must use the non-blocking notification center');
const optionsGeneral = read('SandboxiePlus/SandMan/Windows/OptionsGeneral.cpp');
if (optionsGeneral.includes('QMessageBox::information(this, "Sandboxie-Plus", tr("Image Password Changed"))'))
  throw new Error('image-password success must use the non-blocking notification center');
console.log(`notification-and-gallery-contract checks=${checks.length}`);
