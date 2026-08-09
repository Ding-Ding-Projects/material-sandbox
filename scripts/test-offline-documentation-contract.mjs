import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { after, test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { resolveInternalDocumentationLink } from './validate-docs.mjs';

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const validatorPath = path.join(repositoryRoot, 'scripts', 'validate-docs.mjs');
const temporaryRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-offline-docs-'));

after(() => fs.rmSync(temporaryRoot, { recursive: true, force: true }));

function copyFile(fixture, relativePath) {
  const source = path.join(repositoryRoot, relativePath);
  const target = path.join(fixture, relativePath);
  fs.mkdirSync(path.dirname(target), { recursive: true });
  fs.copyFileSync(source, target);
}

function makeFixture(name) {
  const fixture = path.join(temporaryRoot, name);
  fs.mkdirSync(fixture, { recursive: true });
  fs.cpSync(path.join(repositoryRoot, 'docs'), path.join(fixture, 'docs'), { recursive: true });
  for (const relativePath of [
    'SandboxiePlus/SandMan/Resources/SandMan.qrc',
    'SandboxiePlus/SandMan/Windows/DocumentationBrowser.cpp',
    'SandboxiePlus/SandMan/SandMan.cpp',
    'SandboxiePlus/SandMan/SandMan.pri',
    'SandboxiePlus/SandMan/SandMan.vcxproj',
  ]) copyFile(fixture, relativePath);
  return fixture;
}

function runValidator(fixture) {
  return spawnSync(process.execPath, [validatorPath, '--root', fixture, '--skip-git-history'], {
    cwd: fixture,
    encoding: 'utf8',
    windowsHide: true,
  });
}

function replaceOnce(file, before, afterText) {
  const original = fs.readFileSync(file, 'utf8');
  assert.equal(original.includes(before), true, `fixture mutation anchor is missing: ${before}`);
  fs.writeFileSync(file, original.replace(before, afterText), 'utf8');
}

test('complete repository contract validates 22 feature articles and one supplemental document', () => {
  const result = runValidator(makeFixture('complete'));
  assert.equal(result.status, 0, result.stderr || result.stdout);
  assert.match(result.stdout, /articles=22 supplemental=1 internal-links=60/);
});

test('removing a Qt article resource fails the exact inventory guard', () => {
  const fixture = makeFixture('missing-qrc-article');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  replaceOnce(qrc,
    '        <file alias="Docs/articles/pages-a11y-boundary.md">../../../docs/features/pages-a11y-boundary.md</file>\n',
    '');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /Qt documentation resource set must contain exactly 25 entries; found 24/);
});

test('removing the browser manifest hook fails the runtime inventory guard', () => {
  const fixture = makeFixture('missing-browser-hook');
  const browser = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Windows', 'DocumentationBrowser.cpp');
  replaceOnce(browser,
    'QFile manifestFile(QStringLiteral(":/Docs/articles/index.json"))',
    'QFile manifestFile(QStringLiteral(":/Docs/articles/missing.json"))');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /DocumentationBrowser contract missing: QFile manifestFile/);
});

test('resource copies cannot replace the canonical documentation source', () => {
  const fixture = makeFixture('copied-resource-drift');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  replaceOnce(qrc,
    '../../../docs/material-design.md</file>',
    'Docs/material-design.md</file>');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /Qt documentation resource source for Docs\/articles\/material-design\.md must be/);
});

test('an unresolved or escaping Markdown destination fails the link guard', () => {
  const fixture = makeFixture('bad-markdown-link');
  const article = path.join(fixture, 'docs', 'features', 'native-ci-evidence.md');
  replaceOnce(article, '../screenshots.md', '../../outside.md');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /local documentation link escapes docs/);
});

test('ordered manifest identity cannot be replaced while retaining the count', () => {
  const fixture = makeFixture('manifest-identity-drift');
  const manifest = path.join(fixture, 'docs', 'articles', 'index.json');
  replaceOnce(manifest, '"slug":"material-design"', '"slug":"replacement-design"');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /manifest articles\[0\] slug must be material-design/);
});

test('link resolution retains fragments and rejects unsafe destinations', () => {
  const known = new Set(['material-design.md', 'features/pages-a11y-boundary.md']);
  assert.deepEqual(
    resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '#verification', known),
    { source: 'features/pages-a11y-boundary.md', fragment: 'verification' });
  assert.deepEqual(
    resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '../material-design.md#failure-modes', known),
    { source: 'material-design.md', fragment: 'failure-modes' });
  assert.deepEqual(
    resolveInternalDocumentationLink('material-design.md', 'https://example.test/docs', known),
    { external: 'https://example.test/docs' });
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'http://example.test', known), /unsupported external documentation scheme/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'javascript:alert(1)', known), /unsupported external documentation scheme/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'file:///tmp/readme.md', known), /unsupported external documentation scheme/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', '../outside.md', known), /local documentation link escapes docs/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'missing.md', known), /local documentation destination is not bundled/);
});
