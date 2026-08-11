import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';

const [, , directory, architecture] = process.argv;
if (directory === '--help' || directory === '-h') {
  console.log('usage: node scripts/validate-ci-artifacts.mjs <artifact-directory> <x64|ARM64>');
  process.exit(0);
}
if (!directory || !architecture) {
  console.error('usage: node scripts/validate-ci-artifacts.mjs <artifact-directory> <x64|ARM64>');
  process.exit(2);
}
if (!['x64', 'ARM64'].includes(architecture)) throw new Error(`unsupported architecture: ${architecture}`);

const commonRequired = [
  'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Network.dll', 'Qt6Widgets.dll', 'Qt6Qml.dll', 'Qt6Concurrent.dll',
  'platforms/qdirect2d.dll', 'platforms/qminimal.dll', 'platforms/qoffscreen.dll', 'platforms/qwindows.dll',
  'styles/qmodernwindowsstyle.dll', 'tls/qcertonlybackend.dll', 'tls/qopensslbackend.dll', 'tls/qschannelbackend.dll',
  '7z.dll', 'MiscHelpers.dll', 'MiscHelpers.pdb', 'QSbieAPI.dll', 'QSbieAPI.pdb', 'QtSingleApp.dll',
  'QtSingleApp.pdb', 'UGlobalHotkey.dll', 'UGlobalHotkey.pdb', 'SandMan.exe', 'SandMan.pdb',
  'translations.7z', 'troubleshooting.7z', 'SbieSvc.exe', 'SbieSvc.pdb', 'SbieDll.dll', 'SbieDll.pdb',
  'SbieDrv.sys', 'SbieDrv.pdb', 'SbieCtrl.exe', 'SbieCtrl.pdb', 'Start.exe', 'Start.pdb', 'kmdutil.exe',
  'kmdutil.pdb', 'SbieIni.exe', 'SbieIni.pdb', 'SbieMsg.dll', 'SboxHostDll.dll', 'SboxHostDll.pdb',
  'SandboxieBITS.exe', 'SandboxieBITS.pdb', 'SandboxieCrypto.exe', 'SandboxieCrypto.pdb',
  'SandboxieDcomLaunch.exe', 'SandboxieDcomLaunch.pdb', 'SandboxieRpcSs.exe', 'SandboxieRpcSs.pdb',
  'SandboxieWUAU.exe', 'SandboxieWUAU.pdb', '32/SbieSvc.exe', '32/SbieSvc.pdb', '32/SbieDll.dll',
  '32/SbieDll.pdb', 'SbieShellExt.dll', 'SbieShellPkg.msix', 'Templates.ini', 'Manifest0.txt',
  'Manifest1.txt', 'Manifest2.txt', 'SbieSettings.ini', 'ImBox.exe', 'ImBox.pdb', 'UpdUtil.exe',
  'UpdUtil.pdb', 'MiniDump.exe', 'MiniDump.pdb',
];

function requiredFilesFor(target) {
  const required = [...commonRequired];
  if (target === 'x64') required.push('libssl-3-x64.dll', 'libcrypto-3-x64.dll');
  else required.push('libssl-1_1-ARM64.dll', 'libcrypto-1_1-ARM64.dll', '64/SbieDll.dll', '64/SbieDll.pdb');
  return required;
}

function sha256(file) {
  return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

function hasCrt(root, architecture) {
  return fs.existsSync(path.join(root, architecture, 'Microsoft.VC143.CRT'));
}

function findRedistRoot() {
  const requested = process.env.VCToolsRedistDir ? path.resolve(process.env.VCToolsRedistDir) : null;
  const candidates = [];
  if (requested) {
    candidates.push(requested);
    const parent = path.dirname(requested);
    if (parent !== requested) candidates.push(parent);
  }
  const addVersionCandidates = (base) => {
    if (!fs.existsSync(base) || !fs.statSync(base).isDirectory()) return;
    for (const entry of fs.readdirSync(base, { withFileTypes: true })) {
      if (entry.isDirectory()) candidates.push(path.join(base, entry.name));
    }
  };
  for (const candidate of [...candidates]) addVersionCandidates(candidate);
  for (const candidate of candidates) {
    if (hasCrt(candidate, 'x64') || hasCrt(candidate, 'arm64')) return candidate;
  }
  const programFilesX86 = process.env['ProgramFiles(x86)'];
  if (!programFilesX86) throw new Error('VCToolsRedistDir is unset and ProgramFiles(x86) cannot be resolved');
  const vswhere = path.join(programFilesX86, 'Microsoft Visual Studio', 'Installer', 'vswhere.exe');
  if (!fs.existsSync(vswhere)) throw new Error(`missing Visual Studio discovery tool: ${vswhere}`);
  const discovered = spawnSync(vswhere, [
    '-latest', '-products', '*', '-version', '[17.0,18.0)',
    '-requires', 'Microsoft.VisualStudio.Component.VC.Redist.14.Latest', '-property', 'installationPath',
  ], { encoding: 'utf8', windowsHide: true });
  if (discovered.status !== 0) throw new Error(`vswhere failed with exit ${discovered.status}: ${discovered.stderr}`);
  const installation = discovered.stdout.split(/\r?\n/).map((line) => line.trim()).find(Boolean);
  if (!installation) throw new Error('no compatible Visual Studio 2022 VC143 redistributable was discovered');
  const base = path.join(installation, 'VC', 'Redist', 'MSVC');
  const versions = fs.readdirSync(base, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .sort((a, b) => b.localeCompare(a, undefined, { numeric: true }));
  if (!versions.length) throw new Error(`no VC143 redistributable versions were found below ${base}`);
  const discoveredVersions = versions.map((version) => path.join(base, version));
  const usable = discoveredVersions.find((candidate) => hasCrt(candidate, 'x64') || hasCrt(candidate, 'arm64'));
  if (!usable) throw new Error(`no VC143 runtime architecture directories were found below ${base}`);
  return usable;
}

function resolveSevenZip() {
  if (process.env.SBIE_7ZIP_EXE && fs.existsSync(process.env.SBIE_7ZIP_EXE)) return process.env.SBIE_7ZIP_EXE;
  const result = spawnSync('where.exe', ['7z.exe'], { encoding: 'utf8', windowsHide: true });
  const executable = result.stdout?.split(/\r?\n/).map((line) => line.trim()).find(Boolean);
  if (result.status !== 0 || !executable) throw new Error('7z.exe is required to validate archive contents');
  return executable;
}

function archiveListing(sevenZip, archive) {
  const result = spawnSync(sevenZip, ['l', '-ba', archive], { encoding: 'utf8', windowsHide: true });
  if (result.status !== 0) throw new Error(`could not list archive ${archive}: ${result.stderr}`);
  return result.stdout;
}

function peMachine(file) {
  const data = fs.readFileSync(file);
  if (data.length < 64 || data[0] !== 0x4d || data[1] !== 0x5a) throw new Error(`not a Windows PE image: ${file}`);
  const header = data.readUInt32LE(0x3c);
  if (header + 6 > data.length || data.toString('ascii', header, header + 4) !== 'PE\0\0') throw new Error(`invalid PE header: ${file}`);
  return data.readUInt16LE(header + 4);
}

function assertMachine(root, relative, expected, label) {
  const actual = peMachine(path.join(root, relative));
  if (actual !== expected) {
    throw new Error(`PE architecture mismatch for ${relative}: expected ${label} (0x${expected.toString(16)}), received 0x${actual.toString(16)}`);
  }
}

const root = path.resolve(directory);
if (!fs.existsSync(root) || !fs.statSync(root).isDirectory()) throw new Error(`missing artifact directory: ${root}`);
const required = requiredFilesFor(architecture);
if (required.length !== (architecture === 'x64' ? 73 : 75)) throw new Error('internal artifact inventory count changed without review');

const missing = required.filter((relative) => {
  const file = path.join(root, relative);
  return !fs.existsSync(file) || !fs.statSync(file).isFile() || fs.statSync(file).size === 0;
});
if (missing.length) {
  console.error(`ci-artifact-contract failed architecture=${architecture}`);
  for (const file of missing) console.error(`- missing or empty: ${file}`);
  process.exit(1);
}

const redistArchitecture = architecture === 'ARM64' ? 'arm64' : 'x64';
const crtRoot = path.join(findRedistRoot(), redistArchitecture, 'Microsoft.VC143.CRT');
if (!fs.existsSync(crtRoot)) throw new Error(`missing VC143 runtime source directory: ${crtRoot}`);
const crtFiles = fs.readdirSync(crtRoot, { withFileTypes: true }).filter((entry) => entry.isFile()).map((entry) => entry.name);
if (!crtFiles.length) throw new Error(`no VC143 runtime files found at ${crtRoot}`);
for (const name of crtFiles) {
  const source = path.join(crtRoot, name);
  const staged = path.join(root, name);
  if (!fs.existsSync(staged) || sha256(source) !== sha256(staged)) throw new Error(`staged VC143 runtime does not match ${source}`);
}

const sevenZip = resolveSevenZip();
const translations = archiveListing(sevenZip, path.join(root, 'translations.7z'));
for (const pattern of [/\bsandman_[^\r\n\\/ ]+\.qm\s*$/im, /\bqt_[^\r\n\\/ ]+\.qm\s*$/im, /\bqtbase_[^\r\n\\/ ]+\.qm\s*$/im, /\bqtmultimedia_[^\r\n\\/ ]+\.qm\s*$/im]) {
  if (!pattern.test(translations)) throw new Error(`translations.7z is missing required member family ${pattern}`);
}
const troubleshooting = archiveListing(sevenZip, path.join(root, 'troubleshooting.7z'));
for (const pattern of [/\blayout\.json\s*$/im, /\bAppCompatibility\.js\s*$/im, /(?:^|[\\/])UI[\\/]shell\.js\s*$/im, /(?:^|[\\/])Sandboxing[\\/]SBIEMSG[\\/]SBIEMSG\.js\s*$/im]) {
  if (!pattern.test(troubleshooting)) throw new Error(`troubleshooting.7z is missing required member ${pattern}`);
}

const targetMachine = architecture === 'x64' ? 0x8664 : 0xaa64;
for (const relative of ['SandMan.exe', 'SbieSvc.exe', 'SbieDll.dll', 'SbieDrv.sys', 'tls/qopensslbackend.dll']) {
  assertMachine(root, relative, targetMachine, architecture);
}
for (const relative of architecture === 'x64' ? ['libssl-3-x64.dll', 'libcrypto-3-x64.dll'] : ['libssl-1_1-ARM64.dll', 'libcrypto-1_1-ARM64.dll']) {
  assertMachine(root, relative, targetMachine, architecture);
}
assertMachine(root, '32/SbieDll.dll', 0x014c, 'Win32');
if (architecture === 'ARM64') assertMachine(root, '64/SbieDll.dll', 0xa641, 'ARM64EC');

const files = [];
for (const relative of [...required, ...crtFiles]) {
  const file = path.join(root, relative);
  files.push({ path: relative.replaceAll(path.sep, '/'), bytes: fs.statSync(file).size, sha256: sha256(file) });
}
const manifest = {
  schema: 2,
  architecture,
  generatedAt: new Date().toISOString(),
  explicitFileCount: required.length,
  vcRuntimeFileCount: crtFiles.length,
  evidence: 'CI artifact completeness, archive membership, PE architecture, and VC143 source-hash equivalence; no native launch, service/driver load, or headless capture is implied.',
  files,
};
fs.writeFileSync(path.join(root, 'ci-artifact-manifest.json'), `${JSON.stringify(manifest, null, 2)}\n`, 'utf8');
console.log(`ci-artifact-contract architecture=${architecture} explicitFiles=${required.length} vcRuntimeFiles=${crtFiles.length}`);
