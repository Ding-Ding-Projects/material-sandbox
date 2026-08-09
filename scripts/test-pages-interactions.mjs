import assert from 'node:assert/strict';
import fs from 'node:fs';
import test from 'node:test';
import vm from 'node:vm';
const index = fs.readFileSync(new URL('../docs/index.html', import.meta.url), 'utf8');
const app = fs.readFileSync(new URL('../docs/assets/app.js', import.meta.url), 'utf8');
const worker = fs.readFileSync(new URL('../docs/regex-worker.js', import.meta.url), 'utf8');

test('Pages shell loads only local workspace assets and carries stamped revision slots', () => {
  assert.match(index, /id="app"/);
  assert.match(index, /assets\/app\.css/);
  assert.match(index, /assets\/markdown-renderer\.js/);
  assert.match(index, /assets\/regex-safety\.js/);
  assert.match(index, /assets\/app\.js/);
  assert.equal((index.match(/PAGES_SOURCE_REVISION/g) || []).length, 3);
  assert.doesNotMatch(index, /<(?:script|link|img)\b[^>]+(?:src|href)=["']https?:/i);
});

test('application keeps in-site routes, bounded imports, Focus containment, and durable focus recovery wired', () => {
  for (const token of [
    'function articleFetchPath', 'function showArticle', 'function sanitizeState', 'FOCUSED_HIDDEN_ARTICLES',
    'function requestDestructive', 'function queuePostRenderFocus', 'function restorePostRenderFocus',
    'new Worker(\'regex-worker.js\')', 'function queueRegexPreview', 'function regexPreviewStatus',
  ]) assert.ok(app.includes(token), `missing ${token}`);
  assert.doesNotMatch(app, /\.matchAll\(/);
  assert.doesNotMatch(app, /\bconfirm\s*\(|\bprompt\s*\(/);
});

test('regex worker evaluates bounded benign values and capture groups off the page thread', () => {
  let listener; const messages = [];
  const context = vm.createContext({ self: { addEventListener(type, callback) { if (type === 'message') listener = callback; }, postMessage(value) { messages.push(value); } }, RegExp, String, Number, Math });
  new vm.Script(worker, { filename: 'regex-worker.js' }).runInContext(context);
  assert.equal(typeof listener, 'function');
  listener({ data: { id: 'proof', pattern: '(har) (gow)', flags: 'iu', values: ['Har Gow', 'Siomai'], sample: 'Har Gow; siomai', maxMatches: 12 } });
  assert.deepEqual(JSON.parse(JSON.stringify(messages[0])), { id: 'proof', ok: true, results: [true, false], matches: [{ value: 'Har Gow', captures: ['Har', 'Gow'] }] });
});

test('regex worker rejects oversized patterns without attempting evaluation', () => {
  let listener; const messages = [];
  const context = vm.createContext({ self: { addEventListener(type, callback) { if (type === 'message') listener = callback; }, postMessage(value) { messages.push(value); } }, RegExp, String, Number, Math });
  new vm.Script(worker, { filename: 'regex-worker.js' }).runInContext(context);
  listener({ data: { id: 'oversize', pattern: 'a'.repeat(257), flags: 'u' } });
  assert.equal(messages[0].id, 'oversize'); assert.equal(messages[0].ok, false); assert.match(messages[0].error, /at most 256/);
});
