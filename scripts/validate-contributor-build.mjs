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
requireWindow(
  'SandboxiePlus/SandMan/Windows/OptionsGeneral.cpp',
  'void COptionsWindow::CreateGeneral()',
  '\tm_HoldBoxType = false;',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'ui.lblSupportCert->setVisible(false);', '#else', '#endif'],
  'options supporter copy and capability badge exclusion',
);
requireWindow(
  'SandboxiePlus/SandMan/Wizards/BoxAssistant.cpp',
  'void CBeginPage::initializePage()',
  'void CBeginPage::OnCategory()',
  ['#ifndef SANDBOXIE_CONTRIBUTOR_BUILD', 'supporter certificate', '#endif'],
  'troubleshooting supporter footer exclusion',
);
requireWindow(
  'SandboxiePlus/SandMan/Wizards/SetupWizard.cpp',
  'void CSBUpdate::initializePage()',
  'void CSBUpdate::UpdateOptions()',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'm_pBottomLabel->setVisible(false);', '#else', '#endif'],
  'setup update supporter footer exclusion',
);
requireWindow(
  'SandboxiePlus/SandMan/SandMan.cpp',
  'void CSandMan::UpdateForceUSB()',
  'void CSandMan::OnMaintenance',
  ['#ifndef SANDBOXIE_CONTRIBUTOR_BUILD', 'if (!g_CertInfo.active)', '#endif'],
  'USB automation contributor capability guard',
);
requireWindow(
  'SandboxiePlus/SandMan/SandMan.cpp',
  'if (theAPI->GetSecureParam("UsageFlags"',
  'g_FeatureFlags = theAPI->GetFeatureFlags();',
  ['#ifndef SANDBOXIE_CONTRIBUTOR_BUILD', 'Non-Commercial use ONLY', '#endif'],
  'non-commercial title copy contributor guard',
);

// Native capability state is initialized before optional certificate I/O. The
// explicit field assignments keep the contributor contract reviewable and stop
// a missing Certificate.dat from disabling driver/service features.
requireWindow(
  'Sandboxie/core/drv/verify.c',
  '_FX NTSTATUS KphValidateCertificate()',
  'CleanupExit:',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'KphSetContributorCapabilities();', 'return STATUS_SUCCESS;'],
  'native early contributor capability initialization',
);
requireText('Sandboxie/core/drv/verify.c', 'Verify_CertInfo.type = eCertContributor;', 'contributor certificate type');
requireText('Sandboxie/core/drv/verify.c', 'Verify_CertInfo.level = eCertMaxLevel;', 'contributor maximum capability level');
for (const [rel, needles, label] of [
  ['Sandboxie/core/drv/api.c', ['Verify_CertInfo.opt_sec', 'Verify_CertInfo.opt_enc', 'Verify_CertInfo.opt_net'], 'driver feature flag consumers'],
  ['Sandboxie/core/drv/process.c', ['Verify_CertInfo.active && Verify_CertInfo.opt_sec', 'Verify_CertInfo.active && Verify_CertInfo.opt_enc', 'Verify_CertInfo.active'], 'driver security encryption breakout gates'],
  ['Sandboxie/core/svc/MountManager.cpp', ['CertInfo.active && (UseFileImage ? CertInfo.opt_enc : CertInfo.opt_sec)'], 'service image protection gate'],
  ['Sandboxie/core/svc/UserServer.cpp', ['CertInfo.active && CertInfo.opt_enc'], 'service encryption gate'],
  ['Sandboxie/core/dll/net.c', ['CertInfo.active && CertInfo.opt_net'], 'network feature gate'],
  ['Sandboxie/core/dll/dns_filter.c', ['CertInfo.active && CertInfo.opt_net'], 'DNS feature gate'],
  ['SandboxiePlus/SandMan/SandMan.cpp', ['ForceUsbDrives', 'g_CertInfo.active'], 'desktop USB gate'],
  ['SandboxiePlus/SandMan/Windows/SettingsWindow.cpp', ['chkSandboxUsb', 'g_CertInfo.active', 'CheckForUpdates'], 'settings USB and update gates'],
]) {
  for (const needle of needles) requireText(rel, needle, `${label}: ${needle}`);
}
{
  const sandman = read('SandboxiePlus/SandMan/SandMan.cpp');
  const debug = sandman.indexOf('Debug/IgnoreCertificate');
  const guard = sandman.lastIndexOf('#ifndef SANDBOXIE_CONTRIBUTOR_BUILD', debug);
  const close = sandman.indexOf('#endif', debug);
  if (debug < 0 || guard < 0 || close < 0 || close < debug)
    throw new Error('SandboxiePlus/SandMan/SandMan.cpp: debug certificate simulation exclusion is missing');
  if (!sandman.includes('Debug/CertFakeAboutToExpire', debug) || !sandman.includes('Debug/CertFakeOutdated', debug))
    throw new Error('SandboxiePlus/SandMan/SandMan.cpp: debug certificate simulation contract is incomplete');
  checks.push('debug certificate simulation exclusion');
}
requireWindow(
  'Sandboxie/apps/start/aboutdlg.cpp',
  'bool DoAboutDialog(bool bReminder)',
  'if (CertInfo.active)',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'if (g_bReminder)', 'return true;', '#endif'],
  'start-helper reminder suppression',
);

// Native capability state is initialized before optional certificate I/O. The
// explicit field assignments keep the contributor contract reviewable and stop
// a missing Certificate.dat from disabling driver/service features.
requireWindow(
  'Sandboxie/core/drv/verify.c',
  '_FX NTSTATUS KphValidateCertificate()',
  'CleanupExit:',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'KphSetContributorCapabilities();', 'return STATUS_SUCCESS;'],
  'native early contributor capability initialization',
);
requireText('Sandboxie/core/drv/verify.c', 'Verify_CertInfo.type = eCertContributor;', 'contributor certificate type');
requireText('Sandboxie/core/drv/verify.c', 'Verify_CertInfo.level = eCertMaxLevel;', 'contributor maximum capability level');
for (const [rel, needles, label] of [
  ['Sandboxie/core/drv/api.c', ['Verify_CertInfo.opt_sec', 'Verify_CertInfo.opt_enc', 'Verify_CertInfo.opt_net'], 'driver feature flag consumers'],
  ['Sandboxie/core/drv/process.c', ['Verify_CertInfo.active && Verify_CertInfo.opt_sec', 'Verify_CertInfo.active && Verify_CertInfo.opt_enc', 'Verify_CertInfo.active'], 'driver security encryption breakout gates'],
  ['Sandboxie/core/svc/MountManager.cpp', ['CertInfo.active && (UseFileImage ? CertInfo.opt_enc : CertInfo.opt_sec)'], 'service image protection gate'],
  ['Sandboxie/core/svc/UserServer.cpp', ['CertInfo.active && CertInfo.opt_enc'], 'service encryption gate'],
  ['Sandboxie/core/dll/net.c', ['CertInfo.active && CertInfo.opt_net'], 'network feature gate'],
  ['Sandboxie/core/dll/dns_filter.c', ['CertInfo.active && CertInfo.opt_net'], 'DNS feature gate'],
  ['SandboxiePlus/SandMan/SandMan.cpp', ['ForceUsbDrives', 'g_CertInfo.active'], 'desktop USB gate'],
  ['SandboxiePlus/SandMan/Windows/SettingsWindow.cpp', ['chkSandboxUsb', 'g_CertInfo.active', 'CheckForUpdates'], 'settings USB and update gates'],
]) {
  for (const needle of needles) requireText(rel, needle, `${label}: ${needle}`);
}
{
  const sandman = read('SandboxiePlus/SandMan/SandMan.cpp');
  const debug = sandman.indexOf('Debug/IgnoreCertificate');
  const guard = sandman.lastIndexOf('#ifndef SANDBOXIE_CONTRIBUTOR_BUILD', debug);
  const close = sandman.indexOf('#endif', debug);
  if (debug < 0 || guard < 0 || close < 0 || close < debug)
    throw new Error('SandboxiePlus/SandMan/SandMan.cpp: debug certificate simulation exclusion is missing');
  if (!sandman.includes('Debug/CertFakeAboutToExpire', debug) || !sandman.includes('Debug/CertFakeOutdated', debug))
    throw new Error('SandboxiePlus/SandMan/SandMan.cpp: debug certificate simulation contract is incomplete');
  checks.push('debug certificate simulation exclusion');
}
requireWindow(
  'Sandboxie/apps/start/aboutdlg.cpp',
  'bool DoAboutDialog(bool bReminder)',
  'if (CertInfo.active)',
  ['#ifdef SANDBOXIE_CONTRIBUTOR_BUILD', 'if (g_bReminder)', 'return true;', '#endif'],
  'start-helper reminder suppression',
);

// Keep the legal boundary explicit: contributor capability changes must never
// delete GPL/LGPL/Qt/upstream notices or their source files.
const readme = read('README.md');
requireText('README.md', 'license notices remain intact', 'license notice preservation documentation');
requireText('docs/contributor-build.md', 'Copyright and third-party license notices are not removed.', 'contributor license boundary documentation');
if (!readme.toLowerCase().includes('contributor')) throw new Error('README.md: contributor profile is undocumented');

console.log(`contributor-build-contract checks=${checks.length}`);
