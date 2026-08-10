import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const buildScript = path.join(repositoryRoot, 'scripts', 'build-pages-articles.mjs');
const revision = '0123456789abcdef0123456789abcdef01234567';

function withFixture(run) {
  const fixture = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-pages-articles-'));
  try {
    fs.cpSync(path.join(repositoryRoot, 'docs'), path.join(fixture, 'docs'), { recursive: true });
    fs.mkdirSync(path.join(fixture, '.pages-site'));
    const source = fs.readFileSync(path.join(fixture, 'docs', 'index.html'), 'utf8');
    const staged = source.replaceAll('PAGES_SOURCE_REVISION', revision);
    assert.doesNotMatch(staged, /PAGES_SOURCE_REVISION/);
    fs.writeFileSync(path.join(fixture, '.pages-site', 'index.html'), staged, 'utf8');
    return run(fixture);
  } finally {
    fs.rmSync(fixture, { recursive: true, force: true });
  }
}
function build(fixture) {
  return spawnSync(process.execPath, [buildScript, 'docs', '.pages-site', revision], { cwd: fixture, encoding: 'utf8', windowsHide: true });
}

test('build creates a local direct-entry route for every catalogued and supplemental article', () => {
  withFixture((fixture) => {
    const output = path.join(fixture, '.pages-site', 'articles');
    fs.mkdirSync(output); fs.writeFileSync(path.join(output, 'retired.html'), 'retired', 'utf8'); fs.writeFileSync(path.join(output, 'keep.txt'), 'keep', 'utf8');
    const result = build(fixture);
    assert.equal(result.status, 0, result.stderr);
    assert.match(result.stdout, /features=23 supplemental=2/);
    const manifest = JSON.parse(fs.readFileSync(path.join(fixture, 'docs', 'articles', 'index.json'), 'utf8'));
    const expected = [...manifest.articles.map((article) => article.slug), 'changelog', 'screenshots'].sort();
    const actual = fs.readdirSync(output).filter((name) => name.endsWith('.html')).map((name) => name.slice(0, -5)).sort();
    assert.deepEqual(actual, expected);
    assert.equal(fs.existsSync(path.join(output, 'retired.html')), false);
    assert.equal(fs.readFileSync(path.join(output, 'keep.txt'), 'utf8'), 'keep');
    for (const slug of expected) {
      const page = fs.readFileSync(path.join(output, `${slug}.html`), 'utf8');
      assert.match(page, new RegExp(`<meta name="source-revision" content="${revision}">`));
      assert.match(page, new RegExp(`href="\.\./#/articles/${slug}"`));
      assert.match(page, /location\.replace\("\.\.\/#\/articles\//);
      assert.match(page, /href="\.\.\/assets\/app\.css"/);
      assert.doesNotMatch(page, /PAGES_SOURCE_REVISION|<(?:script|link)[^>]+https?:/i);
    }
  });
});

test('build rejects a manifest that no longer represents the canonical 23-article catalog', () => {
  withFixture((fixture) => {
    const manifestPath = path.join(fixture, 'docs', 'articles', 'index.json');
    const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
    manifest.articles.pop(); fs.writeFileSync(manifestPath, JSON.stringify(manifest), 'utf8');
    const result = build(fixture);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /exactly 23 feature articles/);
  });
});

test('build rejects a path that escapes the copied documentation source', () => {
  withFixture((fixture) => {
    const manifestPath = path.join(fixture, 'docs', 'articles', 'index.json');
    const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
    manifest.articles[0].path = '../../../outside.md'; fs.writeFileSync(manifestPath, JSON.stringify(manifest), 'utf8');
    const result = build(fixture);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /escapes docs/);
  });
});
