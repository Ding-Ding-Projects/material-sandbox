import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const checks = [];

function read(rel) {
  const file = path.join(root, rel);
  if (!fs.existsSync(file)) throw new Error(`missing ${rel}`);
  return fs.readFileSync(file, 'utf8');
}

function requireText(rel, needle, label) {
  const text = read(rel);
  if (!text.includes(needle)) throw new Error(`${rel}: missing ${label}`);
  checks.push(label);
}

function requireWindow(rel, start, end, needles, label) {
  const text = read(rel);
  const begin = text.indexOf(start);
  const finish = text.indexOf(end, begin + start.length);
  if (begin < 0 || finish < 0) throw new Error(`${rel}: cannot locate ${label}`);
  const window = text.slice(begin, finish);
  for (const needle of needles) {
    if (!window.includes(needle)) throw new Error(`${rel}: ${label} missing ${needle}`);
  }
  checks.push(label);
}

// The contributor profile must neutralize each unsolicited reminder boundary,
// while the ordinary certificate workflow remains available in non-contributor
// builds. These are explicit source contracts rather than a broad word search:
// license notices and user-initiated certificate entry are intentionally kept.
requireWindow(
  'SandboxiePlus/SandMan/SandMan.cpp',
  'void CSandMan::CheckSupport()',
  'void CSandMan::OnCertData',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'return;', '#endif'],
  'support reminder early return',
);
requireWindow(
  'SandboxiePlus/SandMan/SandMan.cpp',
  'void CSandMan::OnLogSbieMessage',
  'void CSandMan::ShowMessageBox',
  ['#ifndef SANDBOXIE_CONTRIBUTOR_BUILD', '6004', '#endif'],
  'certificate error popup guard',
);
requireWindow(
  'SandboxiePlus/SandMan/SandMan.cpp',
  'bool CSandMan::CheckCertificate',
  'void InitCertSlot();',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'return true;', '#else', '#endif'],
  'certificate feature gate neutralization',
);
requireWindow(
  'SandboxiePlus/SandMan/SandMan.cpp',
  'SB_STATUS CSandMan::ReloadCert',
  'void CSandMan::OnQueuedRequest',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'g_CertInfo.active = 1;', 'g_CertInfo.expired = 0;', '#ifndef SANDBOXIE_CONTRIBUTOR_BUILD'],
  'certificate capability and expiry neutralization',
);
requireWindow(
  'SandboxiePlus/SandMan/Windows/SupportDialog.cpp',
  'bool CSupportDialog::CheckSupport',
  'extern int g_CertAmount',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'return false;', '#endif'],
  'support dialog reminder guard',
);
requireWindow(
  'SandboxiePlus/SandMan/Windows/SupportDialog.cpp',
  'bool CSupportDialog::ShowDialog',
  'CSupportDialog::CSupportDialog',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'return false;', '#endif'],
  'support dialog display guard',
);
requireWindow(
  'SandboxiePlus/SandMan/Windows/SettingsWindow.cpp',
  'ui.setupUi(this);',
  'this->setWindowTitle',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'ui.tabs->removeTab', '#endif'],
  'support settings tab removal',
);
requireWindow(
  'SandboxiePlus/SandMan/Windows/SettingsWindow.cpp',
  'void CSettingsWindow::UpdateCert()',
  'void CSettingsWindow::OnGetCert()',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'ui.lblEvalCert->setVisible(false);', 'ui.txtCertificate->setVisible(false);', 'return;', '#endif'],
  'certificate prompt UI neutralization',
);
requireWindow(
  'SandboxiePlus/SandMan/Wizards/SetupWizard.cpp',
  'CSetupWizard::CSetupWizard',
  'void CSetupWizard::showHelp',
  ['#ifndef SANDBOXIE_CONTRIBUTOR_BUILD', 'setPage(Page_Certificate', '#endif'],
  'setup certificate page exclusion',
);
requireWindow(
  'SandboxiePlus/SandMan/Wizards/SetupWizard.cpp',
  'int CIntroPage::nextId() const',
  'bool CIntroPage::isComplete()',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'return CSetupWizard::Page_UI;', '#else', '#endif'],
  'setup certificate route exclusion',
);

// Keep the legal boundary explicit: contributor capability changes must never
// delete GPL/LGPL/Qt/upstream notices or their source files.
const readme = read('README.md');
requireText('README.md', 'license notices remain intact', 'license notice preservation documentation');
requireText('docs/contributor-build.md', 'Copyright and third-party license notices are not removed.', 'contributor license boundary documentation');
if (!readme.toLowerCase().includes('contributor')) throw new Error('README.md: contributor profile is undocumented');

console.log(`contributor-build-contract checks=${checks.length}`);
