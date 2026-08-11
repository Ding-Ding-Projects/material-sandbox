import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { validateUnsignedPackaging } from './validate-unsigned-packaging.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
process.chdir(root);

function run(file, args, { env = process.env, expected = 0 } = {}) {
  const result = spawnSync(file, args, { cwd: root, env, encoding: 'utf8', windowsHide: true });
  if (result.error) throw result.error;
  if (result.status !== expected) {
    throw new Error(`${file} ${args.join(' ')}: expected exit ${expected}, received ${result.status}\n${result.stdout}${result.stderr}`);
  }
  return `${result.stdout ?? ''}${result.stderr ?? ''}`;
}

function assertIncludes(text, marker, description) {
  if (!text.includes(marker)) throw new Error(`${description}: missing ${marker}`);
}

const wrappers = ['build.bat', 'build-installer.bat'];
for (const file of wrappers) {
  if (!fs.existsSync(file)) throw new Error(`missing ${file}`);
  const text = fs.readFileSync(file, 'utf8');
  assertIncludes(text, 'set "SCRIPT_ROOT=%~dp0"', `${file}: snapshots its directory before argument shifts`);
  assertIncludes(text, '%SCRIPT_ROOT%scripts\\windows-build-bootstrap.ps1', `${file}: uses the preserved directory after argument parsing`);
  for (const marker of ['/s', '--silent', 'SILENT', 'errorlevel', '--plan', 'windows-build-bootstrap.ps1']) {
    if (!text.toLowerCase().includes(marker.toLowerCase())) throw new Error(`${file}: missing ${marker}`);
  }
}

const helper = fs.readFileSync('scripts/windows-build-bootstrap.ps1', 'utf8');
for (const marker of [
  "[ValidateSet('Build', 'Installer')]",
  "[ValidateSet('x64', 'ARM64')]",
  'VERSION_MJR',
  'VERSION_MIN',
  'VERSION_REV',
  'VERSION_UPD',
  'Select-ExactlyOne',
  'Get-AuthenticodeSignature',
  '$output = & $FilePath @Arguments 2>&1',
  'Invoke-External leaked child output into its return value.',
  "-ne 'NotSigned'",
  'SignerCertificate',
  'TimeStamperCertificate',
  "'^[0-9a-f]{64}$'",
  'SBIE_INSTALL_STAGE',
  "'Sandboxie-Plus.iss'",
]) assertIncludes(helper, marker, 'bootstrap helper');

const workflow = fs.readFileSync('.github/workflows/main.yml', 'utf8');
if ((workflow.match(/name: Expose Qt host tools/g) || []).length !== 2) {
  throw new Error('main workflow must expose Qt host tools in both x64 and ARM64 jobs');
}
const qmake = fs.readFileSync('SandboxiePlus/qmake_plus.cmd', 'utf8');
assertIncludes(qmake, 'QT_HOST_PATH', 'qmake host-root fallback');
assertIncludes(qmake, '%QT_HOST_PATH%\\..\\..', 'qmake host-root derivation');
for (const marker of [
  'GITHUB_PATH',
  'GITHUB_ENV',
  'Trim()',
  'Qt version is empty after trimming',
  'qmake.exe',
  'qmake.bat',
  'qmake6.exe',
  'lrelease.exe',
]) assertIncludes(workflow, marker, 'main workflow Qt host-tool preflight');

const statusBefore = run('git.exe', ['status', '--porcelain=v1', '--untracked-files=all']);
const probeRoot = path.join(os.tmpdir(), `sandboxie-bootstrap-plan-probe-${process.pid}-${Date.now()}`);
const planEnv = { ...process.env, SBIE_TOOLCHAIN_ROOT: probeRoot, SBIE_BOOTSTRAP_PLAN: '' };

const selfTest = run('powershell.exe', [
  '-NoLogo', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
  'scripts\\windows-build-bootstrap.ps1', '-SelfTest',
], { env: planEnv });
assertIncludes(selfTest, 'windows-build-bootstrap-self-test checks=18', 'PowerShell 5.1 self-test');

const x64Plan = run('cmd.exe', ['/d', '/s', '/c', 'call build.bat --plan'], {
  env: { ...planEnv, SBIE_ARCH: 'x64' },
});
for (const marker of [
  'PLAN mode=Build',
  'PLAN architecture=x64',
  'PLAN source-version=1.18.2',
  'PIN Ewdk version=10.0.26100.6584',
  'PIN Qt version=6.8.3',
  'Platform=Win32',
  'SandboxiePlus\\qmake_plus.cmd x64 build_qt6 build_only',
  'SandboxiePlus\\SbieShell\\SbieShell.sln',
  'SandboxieTools\\SandboxieTools.sln',
  'STAGE Installer\\copy_build.cmd x64 build_qt6',
]) assertIncludes(x64Plan, marker, 'x64 plan');

const arm64Plan = run('cmd.exe', ['/d', '/s', '/c', 'call build.bat --plan'], {
  env: { ...planEnv, SBIE_ARCH: 'ARM64' },
});
for (const marker of [
  'PLAN architecture=ARM64',
  'Platform=ARM64EC',
  'SandboxiePlus\\qmake_plus.cmd ARM64 build_qt6 build_only',
  'STAGE Installer\\copy_build.cmd ARM64 build_qt6',
]) assertIncludes(arm64Plan, marker, 'ARM64 plan');

const installerPlan = run('cmd.exe', ['/d', '/s', '/c', 'call build-installer.bat --plan'], {
  env: { ...planEnv, SBIE_ARCH: 'ARM64' },
});
for (const marker of [
  'PLAN mode=Installer',
  'PLAN architecture=x64',
  'PLAN source-version=1.18.2',
  'PLAN stage=Installer\\Release\\SbiePlus_x64-<run-id>',
  'PLAN output=Installer\\Output\\<run-id>',
  'VERIFY exact-output-count=1',
  'VERIFY authenticode=NotSigned',
  'VERIFY provenance=commit,tree-state,version,architecture,sha256',
]) assertIncludes(installerPlan, marker, 'installer plan');

run('cmd.exe', ['/d', '/s', '/c', 'call build.bat --unknown'], { env: planEnv, expected: 64 });
run('cmd.exe', ['/d', '/s', '/c', 'call build-installer.bat --unknown'], { env: planEnv, expected: 64 });

if (fs.existsSync(probeRoot)) throw new Error(`plan-only mode mutated its isolated tool root: ${probeRoot}`);
const statusAfter = run('git.exe', ['status', '--porcelain=v1', '--untracked-files=all']);
if (statusAfter !== statusBefore) throw new Error('entrypoint self-tests mutated the source tree');

const unsignedChecks = validateUnsignedPackaging();
console.log(`build-entrypoints-contract checks=${53 + unsignedChecks} helperSelfTests=18`);
