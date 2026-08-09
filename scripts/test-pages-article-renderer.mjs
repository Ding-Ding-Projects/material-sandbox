import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import test from 'node:test';
import { fileURLToPath } from 'node:url';
import vm from 'node:vm';

const BUILD_SCRIPT = fileURLToPath(new URL('./build-pages-articles.mjs', import.meta.url));
const REPOSITORY_ROOT = path.resolve(path.dirname(BUILD_SCRIPT), '..');
const REVISION = '0123456789abcdef0123456789abcdef01234567';
const FEATURE_ARTICLES = [
  ['material-design', 'material-design.md'],
  ['appearance-editor', 'features/appearance-editor.md'],
  ['settings-provenance', 'features/settings-provenance.md'],
  ['contributor-build', 'contributor-build.md'],
  ['contributor-build-audit', 'features/contributor-build-audit.md'],
  ['settings-history', 'features/settings-history.md'],
  ['notifications', 'features/notifications.md'],
  ['color-translator', 'features/color-translator.md'],
  ['school-mode', 'features/school-mode.md'],
  ['dim-sum-surprise', 'features/dim-sum-surprise.md'],
  ['scheduled-settings', 'features/scheduled-settings.md'],
  ['tab-discovery', 'features/tab-discovery.md'],
  ['command-palette', 'features/command-palette.md'],
  ['external-editor', 'features/external-editor.md'],
  ['destructive-confirmation', 'features/destructive-confirmation.md'],
  ['native-ci-evidence', 'features/native-ci-evidence.md'],
  ['build-entrypoints', 'features/build-entrypoints.md'],
  ['m3-shell-boundary', 'features/ui/m3-shell-boundary.md'],
  ['editor-settings', 'features/editor-settings.md'],
  ['changelog-viewer', 'features/changelog-viewer.md'],
  ['pages-language-tone', 'features/pages-language-tone.md'],
  ['pages-a11y-boundary', 'features/pages-a11y-boundary.md']
];
const FEATURE_COUNT = FEATURE_ARTICLES.length;

function articleSlug(index) {
  return FEATURE_ARTICLES[index - 1][0];
}

function stageSiteShell(root) {
  const source = fs.readFileSync(path.join(REPOSITORY_ROOT, 'docs', 'index.html'), 'utf8');
  const stamped = source.replaceAll('PAGES_SOURCE_REVISION', REVISION);
  assert.doesNotMatch(stamped, /PAGES_SOURCE_REVISION/);
  fs.writeFileSync(path.join(root, '.pages-site', 'index.html'), stamped, 'utf8');
}

function writeFixture(root, options = {}) {
  const docs = path.join(root, 'docs');
  const manifestDirectory = path.join(docs, 'articles');
  fs.mkdirSync(manifestDirectory, { recursive: true });
  fs.mkdirSync(path.join(root, '.pages-site'));

  const articles = [];
  for (let index = 1; index <= FEATURE_COUNT; index += 1) {
    const slug = articleSlug(index);
    const related = articleSlug(index === FEATURE_COUNT ? 1 : index + 1);
    const relativeSource = FEATURE_ARTICLES[index - 1][1];
    const sourcePath = path.join(docs, ...relativeSource.split('/'));
    fs.mkdirSync(path.dirname(sourcePath), { recursive: true });
    const manifestPath = path.relative(manifestDirectory, sourcePath).split(path.sep).join('/');
    articles.push({ slug, path: manifestPath, related: [related] });
    const extra = index === 1
      ? `
Raw HTML stays text: <script>alert("no")</script>

- First list item
- Second list item

| Setting | Value |
|---|---|
| Theme | Material 3 |

\`\`\`js
const escaped = "<tag>";
\`\`\`

[Read feature two](features/appearance-editor.md)

![Remote image must not load](https://example.test/tracker.png)
`
      : '';
    fs.writeFileSync(sourcePath, `# Feature ${index}\n\nFeature body ${index}.\n${extra}`, 'utf8');
  }

  if (options.duplicateSlug) articles[1].slug = articles[0].slug;
  if (options.missingFile) {
    fs.rmSync(path.join(docs, ...FEATURE_ARTICLES.at(-1)[1].split('/')));
  }
  if (options.traversal) articles[0].path = '../../../outside.md';
  if (options.swappedMapping) {
    const replacement = path.join(docs, 'features', 'material-design.md');
    fs.mkdirSync(path.dirname(replacement), { recursive: true });
    fs.writeFileSync(replacement, '# Replacement\n', 'utf8');
    articles[0].path = '../features/material-design.md';
  }
  if (options.extraFeature) {
    fs.writeFileSync(path.join(docs, 'features', 'unmanifested.md'), '# Unmanifested\n', 'utf8');
  }
  fs.writeFileSync(path.join(root, 'outside.md'), '# Outside\n', 'utf8');
  fs.writeFileSync(path.join(manifestDirectory, 'index.json'), JSON.stringify({
    version: 1,
    articles,
    changelog: { path: '../changelog.md', commit: REVISION }
  }, null, 2), 'utf8');
  fs.writeFileSync(path.join(docs, 'changelog.md'), '# Changelog\n\n## Release\n\nExact notes.\n', 'utf8');
  fs.writeFileSync(path.join(docs, 'screenshots.md'), '# Image index\n\nThe bounded image catalog.\n', 'utf8');
  stageSiteShell(root);
  if (options.missingLandingLink) {
    const stagedIndex = path.join(root, '.pages-site', 'index.html');
    const original = fs.readFileSync(stagedIndex, 'utf8');
    const changed = original.replace('href="articles/appearance-editor.html"', 'href="#features"');
    assert.notEqual(changed, original);
    fs.writeFileSync(stagedIndex, changed, 'utf8');
  }
  return { docs, target: path.join(root, '.pages-site') };
}

function runBuild(root, source = 'docs', target = '.pages-site') {
  return spawnSync(process.execPath, [BUILD_SCRIPT, source, target, REVISION], {
    cwd: root,
    encoding: 'utf8',
    windowsHide: true
  });
}

function withFixture(callback, options) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-pages-articles-'));
  try {
    writeFixture(root, options);
    return callback(root);
  } finally {
    fs.rmSync(root, { recursive: true, force: true });
  }
}

test('build renders 22 manifest articles and two supplemental pages in the interactive M3 shell', () => {
  withFixture((root) => {
    const target = path.join(root, '.pages-site');
    fs.writeFileSync(path.join(target, 'sentinel.txt'), 'preserve me', 'utf8');
    fs.mkdirSync(path.join(target, 'articles'));
    fs.writeFileSync(path.join(target, 'articles', 'unrelated.txt'), 'preserve me too', 'utf8');
    fs.writeFileSync(path.join(target, 'articles', 'retired-feature.html'), 'retired', 'utf8');

    const result = runBuild(root);
    assert.equal(result.status, 0, result.stderr);
    assert.match(result.stdout, /features=22 supplemental=2/);

    const outputDirectory = path.join(target, 'articles');
    const htmlFiles = fs.readdirSync(outputDirectory).filter((name) => name.endsWith('.html'));
    assert.equal(htmlFiles.length, FEATURE_COUNT + 2);
    for (let index = 1; index <= FEATURE_COUNT; index += 1) {
      assert.ok(htmlFiles.includes(`${articleSlug(index)}.html`));
    }
    assert.ok(htmlFiles.includes('changelog.html'));
    assert.ok(htmlFiles.includes('screenshots.html'));
    assert.equal(fs.readFileSync(path.join(target, 'sentinel.txt'), 'utf8'), 'preserve me');
    assert.equal(fs.readFileSync(path.join(outputDirectory, 'unrelated.txt'), 'utf8'), 'preserve me too');
    assert.equal(fs.existsSync(path.join(outputDirectory, 'retired-feature.html')), false);

    const article = fs.readFileSync(path.join(outputDirectory, 'material-design.html'), 'utf8');
    assert.match(article, /&lt;script&gt;alert\(&quot;no&quot;\)&lt;\/script&gt;/);
    assert.doesNotMatch(article, /<script[^>]+src=/i);
    assert.match(article, /<ul><li>First list item<\/li><li>Second list item<\/li><\/ul>/);
    assert.match(article, /<table>/);
    assert.match(article, /<pre><code class="language-js">const escaped = &quot;&lt;tag&gt;&quot;;<\/code><\/pre>/);
    assert.doesNotMatch(article, /<base\b/i);
    assert.match(article, /href="appearance-editor\.html"/);
    assert.match(article, /<aside class="suggested"/);
    assert.match(article, /href="appearance-editor\.html">Feature 2<\/a>/);
    assert.match(article, new RegExp(`<meta name="source-revision" content="${REVISION}">`));
    assert.match(article, new RegExp(`Source revision <code>${REVISION}</code>`));
    assert.match(article, /--md-sys-color-primary:/);
    assert.match(article, /@media \(prefers-reduced-motion: reduce\)/);
    assert.match(article, /@media print/);
    assert.match(article, /@media \(max-width: 760px\)/);
    assert.match(article, /:focus-visible/);
    assert.match(article, /min-height: 48px/);
    assert.match(article, /<header class="top-app-bar">/);
    assert.match(article, /id="language"/);
    assert.match(article, /id="funnyEnglish"/);
    assert.match(article, /id="funnyCantonese"/);
    assert.match(article, /id="paletteButton"/);
    assert.match(article, /Ctrl\+Shift\+F/);
    assert.match(article, /id="siteTabs"/);
    assert.match(article, /id="tab-article"[^>]+data-en="Article:/);
    assert.match(article, /id="articleSearch"/);
    assert.match(article, /id="articleBuilderButton"[^>]+aria-controls="articleRegexBuilder"/);
    assert.match(article, /bindRegexBuilder\('article'/);
    assert.match(article, /new Worker\('\.\.\/regex-worker\.js'\)/);
    assert.match(article, /openTab\(hash && \$\(hash\) \? hash : 'article', false\)/);
    assert.match(article, /<main id="active-panel"/);
    assert.match(article, /<article id="articleContent" class="article-document" lang="en" aria-labelledby="article-title">/);
    assert.match(article, /<footer class="footer">/);
    assert.match(article, /External or unsafe image omitted/);
    assert.doesNotMatch(article, /<link\b|<iframe\b|<object\b|<embed\b|@import\b|\bfetch\s*\(/i);
    assert.doesNotMatch(article, /(?:src|href)="https:\/\/example\.test\/tracker\.png"/i);
    const inlineScripts = [...article.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/gi)];
    assert.equal(inlineScripts.length, 1);
    assert.doesNotThrow(() => new vm.Script(inlineScripts[0][1], { filename: 'generated-article-inline.js' }));
    const routeUrl = new URL('https://example.test/material-sandbox/articles/material-design.html');
    const localArticleHrefs = [...article.matchAll(/href="([a-z0-9-]+\.html(?:[?#][^"]*)?)"/gi)]
      .map((match) => match[1]);
    assert.ok(localArticleHrefs.length > 0);
    for (const href of localArticleHrefs) {
      const resolved = new URL(href, routeUrl);
      assert.match(resolved.pathname, /^\/material-sandbox\/articles\/[a-z0-9-]+\.html$/);
      assert.ok(fs.existsSync(path.join(outputDirectory, path.basename(resolved.pathname))), `${href} resolves to a missing output`);
    }
    const fragmentHrefs = [...article.matchAll(/href="(#[a-z0-9-]+)"/gi)].map((match) => match[1]);
    assert.ok(fragmentHrefs.includes('#active-panel'));
    assert.ok(fragmentHrefs.includes('#overview'));
    for (const href of fragmentHrefs) {
      const resolved = new URL(href, routeUrl);
      assert.equal(resolved.pathname, routeUrl.pathname, `${href} must retain the article pathname`);
    }
    assert.match(article, /history\.replaceState\(null, '', '#' \+ panelId\)/);
    assert.equal(new URL('#article', routeUrl).pathname, routeUrl.pathname);

    const changelog = fs.readFileSync(path.join(outputDirectory, 'changelog.html'), 'utf8');
    const screenshots = fs.readFileSync(path.join(outputDirectory, 'screenshots.html'), 'utf8');
    assert.match(changelog, /<h1 id="article-title">Changelog<\/h1>/);
    assert.match(changelog, /href="changelog-viewer\.html"/);
    assert.match(screenshots, /<h1 id="article-title">Image index<\/h1>/);
    assert.match(screenshots, /href="material-design\.html"/);
  });
});

test('current repository manifest renders every declared page with local screenshots', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-pages-current-'));
  try {
    fs.cpSync(path.join(REPOSITORY_ROOT, 'docs'), path.join(root, 'docs'), { recursive: true });
    fs.mkdirSync(path.join(root, 'SandboxiePlus', 'SandMan'), { recursive: true });
    fs.cpSync(
      path.join(REPOSITORY_ROOT, 'SandboxiePlus', 'SandMan', 'Resources'),
      path.join(root, 'SandboxiePlus', 'SandMan', 'Resources'),
      { recursive: true }
    );
    fs.mkdirSync(path.join(root, '.pages-site'));
    stageSiteShell(root);

    const result = runBuild(root);
    assert.equal(result.status, 0, result.stderr);
    const manifest = JSON.parse(fs.readFileSync(path.join(root, 'docs', 'articles', 'index.json'), 'utf8'));
    assert.equal(manifest.articles.length, FEATURE_COUNT);
    const outputDirectory = path.join(root, '.pages-site', 'articles');
    for (const entry of manifest.articles) {
      assert.ok(fs.existsSync(path.join(outputDirectory, `${entry.slug}.html`)), `missing ${entry.slug}.html`);
    }
    assert.ok(fs.existsSync(path.join(outputDirectory, 'changelog.html')));
    const screenshots = fs.readFileSync(path.join(outputDirectory, 'screenshots.html'), 'utf8');
    assert.match(screenshots, /src="data:image\/png;base64,/);
    assert.doesNotMatch(screenshots, /<img[^>]+src="https?:/i);
    const stagedIndex = fs.readFileSync(path.join(root, '.pages-site', 'index.html'), 'utf8');
    const linkedArticleNames = [...stagedIndex.matchAll(/href=["']articles\/([a-z0-9-]+\.html)(?:[?#][^"']*)?["']/gi)]
      .map((match) => match[1]);
    assert.ok(linkedArticleNames.length > 0, 'staged index must link generated articles');
    for (const linkedName of linkedArticleNames) {
      assert.ok(fs.existsSync(path.join(outputDirectory, linkedName)), `staged index links missing ${linkedName}`);
    }
    const expectedArticleNames = fs.readdirSync(outputDirectory).filter((name) => name.endsWith('.html')).sort();
    assert.deepEqual([...new Set(linkedArticleNames)].sort(), expectedArticleNames);
    for (const filename of fs.readdirSync(outputDirectory).filter((name) => name.endsWith('.html'))) {
      const html = fs.readFileSync(path.join(outputDirectory, filename), 'utf8');
      assert.doesNotMatch(html, /href="[^"]*\.md(?:[?#][^"]*)?"/i, `${filename} retained a raw Markdown destination`);
    }
  } finally {
    fs.rmSync(root, { recursive: true, force: true });
  }
});

test('build rejects a non-checkout source or target', () => {
  withFixture((root) => {
    fs.mkdirSync(path.join(root, 'other'));
    const wrongSource = runBuild(root, 'other', '.pages-site');
    assert.equal(wrongSource.status, 2);
    assert.match(wrongSource.stderr, /source and target must be the checkout docs and \.pages-site directories/);

    const wrongTarget = runBuild(root, 'docs', 'other');
    assert.equal(wrongTarget.status, 2);
    assert.match(wrongTarget.stderr, /source and target must be the checkout docs and \.pages-site directories/);
  });
});

test('build rejects a landing page that omits a canonical generated article', () => {
  withFixture((root) => {
    const result = runBuild(root);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /landing page must link every canonical 22\+2 generated article/);
  }, { missingLandingLink: true });
});

test('build rejects duplicate slugs, missing files, and traversal paths', () => {
  withFixture((root) => {
    const duplicate = runBuild(root);
    assert.notEqual(duplicate.status, 0);
    assert.match(duplicate.stderr, /duplicates or reserves slug/);
  }, { duplicateSlug: true });

  withFixture((root) => {
    const missing = runBuild(root);
    assert.notEqual(missing.status, 0);
    assert.match(missing.stderr, /is unavailable/);
  }, { missingFile: true });

  withFixture((root) => {
    const traversal = runBuild(root);
    assert.notEqual(traversal.status, 0);
    assert.match(traversal.stderr, /escapes docs/);
  }, { traversal: true });

  withFixture((root) => {
    const swapped = runBuild(root);
    assert.notEqual(swapped.status, 0);
    assert.match(swapped.stderr, /canonical slug and source inventory/);
  }, { swappedMapping: true });

  withFixture((root) => {
    const extra = runBuild(root);
    assert.notEqual(extra.status, 0);
    assert.match(extra.stderr, /feature Markdown corpus does not match/);
  }, { extraFeature: true });
});

test('build rejects hard-linked HTML without overwriting its other name', () => {
  withFixture((root) => {
    const outputDirectory = path.join(root, '.pages-site', 'articles');
    fs.mkdirSync(outputDirectory);
    const victim = path.join(root, 'victim.txt');
    const original = 'must remain unchanged';
    fs.writeFileSync(victim, original, 'utf8');
    fs.linkSync(victim, path.join(outputDirectory, 'material-design.html'));

    const result = runBuild(root);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /must not be hard-linked/);
    assert.equal(fs.readFileSync(victim, 'utf8'), original);
  });
});

test('build rejects symbolic-link or reparse targets and article directories', (t) => {
  withFixture((root) => {
    const target = path.join(root, '.pages-site');
    const replacement = path.join(root, 'linked-target');
    fs.mkdirSync(replacement);
    fs.rmSync(target, { recursive: true });
    try {
      fs.symlinkSync(replacement, target, process.platform === 'win32' ? 'junction' : 'dir');
    } catch (error) {
      if (['EPERM', 'EACCES', 'ENOTSUP'].includes(error.code)) {
        t.skip(`host cannot create a directory link for the safety proof: ${error.code}`);
        return;
      }
      throw error;
    }
    const result = runBuild(root);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /symbolic-link or reparse/);
  });

  if (t.skipped) return;
  withFixture((root) => {
    const linkedArticles = path.join(root, 'linked-articles');
    fs.mkdirSync(linkedArticles);
    fs.symlinkSync(
      linkedArticles,
      path.join(root, '.pages-site', 'articles'),
      process.platform === 'win32' ? 'junction' : 'dir'
    );
    const result = runBuild(root);
    assert.notEqual(result.status, 0);
    assert.match(result.stderr, /symbolic-link or reparse/);
  });
});
