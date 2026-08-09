import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const source = fs.readFileSync(path.join(root, 'docs/assets/regex-safety.js'), 'utf8');
const context = { window: {} };
vm.runInNewContext(source, context, { filename: 'regex-safety.js' });
const safety = context.window.MaterialSandboxRegexSafety;
assert.ok(safety, 'regex safety surface is registered');
assert.equal(safety.MAX_PATTERN, 256, 'pattern bound is stable');
assert.equal(safety.MAX_SAMPLE, 512, 'sample bound is stable');

const unsafe = ['(a?)+$', '(a{1,2})+$', '(a+)+$', '(a|aa)+$', '(.*)+$', '\\1', '(?=a)a', 'a*a*a*$', 'a?a?a?a?a?a?a?a?a?$'];
const safe = ['^Sandboxie$', '^Sand(box|Man)$', '[A-Z]+', '^foo\\s+bar$', '^\\w{1,64}$'];
unsafe.forEach((pattern) => assert.equal(safety.isRiskyPattern(pattern), true, `unsafe pattern must be rejected: ${pattern}`));
safe.forEach((pattern) => assert.equal(safety.isRiskyPattern(pattern), false, `ordinary pattern must remain usable: ${pattern}`));

console.log(`pages-regex-safety unsafe=${unsafe.length} safe=${safe.length} maxPattern=${safety.MAX_PATTERN} maxSample=${safety.MAX_SAMPLE}`);
