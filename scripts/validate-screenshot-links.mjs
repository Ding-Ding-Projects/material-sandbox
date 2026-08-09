import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const files = ['README.md', 'docs/screenshots.md'];
const imagePattern = /!\[[^\]]*\]\(([^)]+)\)/g;
let checked = 0;
for (const file of files) {
  const text = fs.readFileSync(path.join(root, file), 'utf8');
  for (const match of text.matchAll(imagePattern)) {
    const target = match[1].split(/[?#]/, 1)[0];
    if (/^(?:https?:|data:)/i.test(target)) continue;
    const resolved = path.resolve(path.dirname(path.join(root, file)), target);
    if (!fs.existsSync(resolved)) throw new Error(`${file}: missing screenshot ${target}`);
    checked++;
  }
}
console.log(`screenshot-link-contract images=${checked}`);
