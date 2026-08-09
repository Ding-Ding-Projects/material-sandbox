import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

const root = process.cwd();
const read = (relative) => fs.readFileSync(path.join(root, relative), 'utf8');
const source = read('SandboxiePlus/SandMan/Windows/DimSumSurprise.cpp');
const header = read('SandboxiePlus/SandMan/Windows/DimSumSurprise.h');
const sandman = read('SandboxiePlus/SandMan/SandMan.cpp');
const pri = read('SandboxiePlus/SandMan/SandMan.pri');
const vcxproj = read('SandboxiePlus/SandMan/SandMan.vcxproj');
const docs = read('docs/features/dim-sum-surprise.md');

const checks = [
  ['startup hook', /DimSumSurprise::schedule\(this\)/.test(sandman) && /DimSumSurpriseStartupReady/.test(sandman) && /void schedule\(CSandMan\* parent\)/.test(header)],
  ['single 10 percent draw', /bounded\(10\) != 0/.test(source) && /static bool scheduled/.test(source)],
  ['delayed non-blocking scheduling', /QTimer::singleShot\(3500/.test(source) && /WA_ShowWithoutActivating/.test(source) && /WindowDoesNotAcceptFocus/.test(source)],
  ['automatic dismissal', /QTimer::singleShot\(9000/.test(source)],
  ['startup suppression', /-autorun/.test(source) && /g_PendingMessage/.test(source) && /activeModalWidget/.test(source) && /schoolModeEnabled/.test(source)],
  ['bilingual names and alt text', /QStringLiteral\("name"\)/.test(source) && /zhHant/.test(source) && /altEnglish/.test(source) && /altCantonese/.test(source) && /yue/.test(source)],
  ['public catalog source', /raw\.githubusercontent\.com\/Ding-Ding-Projects\/dim-sum-photos\/main\/catalog\/index\.json/.test(source) && /catalog-v1\(\?:-part/.test(source) && /localPath/.test(source)],
  ['source cache validation', /catalogRevision/.test(source) && /imageRecord\.value/.test(source) && /decodedImage/.test(source) && /8 \* 1024 \* 1024/.test(source)],
  ['build registration', /DimSumSurprise\.cpp/.test(vcxproj) && /DimSumSurprise\.cpp/.test(pri)],
  ['documentation contract', /10%/.test(docs) && /opt.?out/i.test(docs) && /offline/i.test(docs) && /catalog/i.test(docs)],
  ['no consumer image asset', (() => {
    const tracked = execFileSync('git', ['ls-files', 'SandboxiePlus/SandMan'], { cwd: root, encoding: 'utf8' });
    return !tracked.split(/\r?\n/).some((file) => /DimSum|dim.?sum/i.test(file) && /\.(png|jpe?g|webp|gif)$/i.test(file));
  })()],
];

const failed = checks.filter(([, ok]) => !ok).map(([name]) => name);
if (failed.length) {
  console.error(`dim-sum-surprise-contract failed: ${failed.join(', ')}`);
  process.exit(1);
}
console.log(`dim-sum-surprise-contract checks=${checks.length}`);
