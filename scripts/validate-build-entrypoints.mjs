import fs from 'node:fs';
import { validateUnsignedPackaging } from './validate-unsigned-packaging.mjs';

const files = ['build.bat', 'build-installer.bat'];
for (const file of files) {
  if (!fs.existsSync(file)) throw new Error(`missing ${file}`);
  const text = fs.readFileSync(file, 'utf8');
  for (const marker of ['/s', '--silent', 'SILENT', 'errorlevel']) {
    if (!text.toLowerCase().includes(marker.toLowerCase())) throw new Error(`${file}: missing ${marker}`);
  }
}
const installer = fs.readFileSync('build-installer.bat', 'utf8');
if (!installer.toLowerCase().includes('unsigned')) throw new Error('build-installer.bat: missing unsigned status');
if (!installer.includes('Get-AuthenticodeSignature')) throw new Error('installer must verify the generated executable is unsigned');
const unsignedChecks = validateUnsignedPackaging();
console.log(`build-entrypoints-contract checks=${12 + unsignedChecks}`);
