import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const stampScript = path.join(repositoryRoot, 'scripts', 'stamp-pages-revision.mjs');
const revision = '0123456789abcdef0123456789abcdef01234567';

function withFixture(run) {
  const fixture = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-pages-stamp-'));
  try {
    fs.mkdirSync(path.join(fixture, 'docs'));
    fs.writeFileSync(
      path.join(fixture, 'docs', 'index.html'),
      '<p>PAGES_SOURCE_REVISION</p><p>PAGES_SOURCE_REVISION</p><p>PAGES_SOURCE_REVISION</p>',
      'utf8',
    );
    return run(fixture);
  } finally {
    fs.rmSync(fixture, { recursive: true, force: true });
  }
}

function invoke(fixture) {
  return spawnSync(
    process.execPath,
    [stampScript, 'docs', '.pages-site', revision],
    { cwd: fixture, encoding: 'utf8' },
  );
}

function makeDirectoryLink(target, linkPath) {
  fs.symlinkSync(target, linkPath, process.platform === 'win32' ? 'junction' : 'dir');
}

test('stamps the exact revision into a separate ordinary staging directory', () => {
  withFixture((fixture) => {
    const result = invoke(fixture);
    assert.equal(result.status, 0, result.stderr);
    const staged = fs.readFileSync(path.join(fixture, '.pages-site', 'index.html'), 'utf8');
    assert.equal(staged.includes('PAGES_SOURCE_REVISION'), false);
    assert.equal(staged.split(revision).length - 1, 3);
  });
});

test('rejects a linked source child before replacing existing staging', () => {
  withFixture((fixture) => {
    const outside = path.join(fixture, 'outside-source');
    fs.mkdirSync(outside);
    fs.writeFileSync(path.join(outside, 'private.txt'), 'must not be copied', 'utf8');
    makeDirectoryLink(outside, path.join(fixture, 'docs', 'linked-child'));
    const staging = path.join(fixture, '.pages-site');
    fs.mkdirSync(staging);
    const sentinel = path.join(staging, 'sentinel.txt');
    fs.writeFileSync(sentinel, 'preserve me', 'utf8');

    const result = invoke(fixture);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /symbolic-link or reparse entry in Pages source/);
    assert.equal(fs.readFileSync(sentinel, 'utf8'), 'preserve me');
  });
});

test('rejects a linked staging endpoint without touching its destination', () => {
  withFixture((fixture) => {
    const outside = path.join(fixture, 'outside-target');
    fs.mkdirSync(outside);
    const sentinel = path.join(outside, 'sentinel.txt');
    fs.writeFileSync(sentinel, 'preserve me too', 'utf8');
    makeDirectoryLink(outside, path.join(fixture, '.pages-site'));

    const result = invoke(fixture);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /symbolic-link or reparse staging target/);
    assert.equal(fs.readFileSync(sentinel, 'utf8'), 'preserve me too');
  });
});
