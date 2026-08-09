import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';

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

const root = path.resolve(directory);
if (!fs.existsSync(root) || !fs.statSync(root).isDirectory()) throw new Error(`missing artifact directory: ${root}`);

const required = [
  'SandMan.exe', 'SbieSvc.exe', 'SbieDrv.sys', 'SbieDll.dll', 'SbieCtrl.exe',
  'Start.exe', 'kmdutil.exe', 'SbieIni.exe', 'SbieMsg.dll', 'SboxHostDll.dll',
  'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Network.dll', 'Qt6Widgets.dll', 'Qt6Qml.dll',
  '7z.dll', 'translations.7z', 'troubleshooting.7z',
  'Manifest0.txt', 'Manifest1.txt', 'Manifest2.txt', 'SbieSettings.ini',
  'platforms/qwindows.dll', 'platforms/qminimal.dll', 'tls/qschannelbackend.dll',
];
if (architecture === 'x64') required.push('libssl-3-x64.dll', 'libcrypto-3-x64.dll');
if (architecture === 'ARM64') required.push('libssl-1_1-ARM64.dll', 'libcrypto-1_1-ARM64.dll');
if (architecture === 'x64') required.push('32/SbieSvc.exe', '32/SbieDll.dll', 'SbieShellExt.dll', 'SbieShellPkg.msix');
if (architecture === 'ARM64') required.push('32/SbieSvc.exe', '32/SbieDll.dll', '64/SbieDll.dll', 'SbieShellExt.dll', 'SbieShellPkg.msix');

const missing = required.filter((relative) => {
  const file = path.join(root, relative);
  return !fs.existsSync(file) || !fs.statSync(file).isFile() || fs.statSync(file).size === 0;
});
if (missing.length) {
  console.error(`ci-artifact-contract failed architecture=${architecture}`);
  for (const file of missing) console.error(`- missing or empty: ${file}`);
  process.exit(1);
}

const files = [];
for (const relative of required) {
  const file = path.join(root, relative);
  const hash = crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
  files.push({ path: relative.replaceAll(path.sep, '/'), bytes: fs.statSync(file).size, sha256: hash });
}
const manifest = {
  schema: 1,
  architecture,
  generatedAt: new Date().toISOString(),
  evidence: 'CI artifact completeness only; no native launch, service/driver load, or headless capture is implied.',
  files,
};
fs.writeFileSync(path.join(root, 'ci-artifact-manifest.json'), `${JSON.stringify(manifest, null, 2)}\n`, 'utf8');
console.log(`ci-artifact-contract architecture=${architecture} files=${files.length}`);
