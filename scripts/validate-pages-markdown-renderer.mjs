import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const rendererPath = path.join(root, 'docs', 'assets', 'markdown-renderer.js');
const articleManifestPath = path.join(root, 'docs', 'articles', 'index.json');
const rendererSource = fs.readFileSync(rendererPath, 'utf8');
const sandbox = { URL };
sandbox.globalThis = sandbox;
vm.runInNewContext(rendererSource, sandbox, { filename: rendererPath });
const renderer = sandbox.MaterialSandboxMarkdown;
assert.equal(typeof renderer?.render, 'function', 'renderer must expose a browser-global render function');
assert.equal(typeof renderer?.normalizeArticlePath, 'function', 'renderer must expose route normalization');

const manifest = JSON.parse(fs.readFileSync(articleManifestPath, 'utf8'));
const indexDirectory = path.dirname(articleManifestPath);
const docsRoot = fs.realpathSync(path.join(root, 'docs'));

function resolveManifestArticlePath(article) {
  const resolved = path.resolve(indexDirectory, article.path);
  const realPath = fs.realpathSync(resolved);
  const relativeToDocs = path.relative(docsRoot, realPath);
  if (!relativeToDocs || relativeToDocs === '..' || relativeToDocs.startsWith(`..${path.sep}`) || path.isAbsolute(relativeToDocs)) {
    throw new Error(`article path escapes docs/: ${article.path}`);
  }
  return path.relative(root, realPath).replace(/\\/g, '/');
}

const articleRoutes = {};
const qrcRoutes = {};
for (const article of manifest.articles) {
  const articlePath = resolveManifestArticlePath(article);
  const route = `#/articles/${article.slug}`;
  articleRoutes[articlePath] = route;
  qrcRoutes[`Docs/${path.posix.basename(articlePath)}`] = route;
}

const options = {
  sourcePath: 'docs/features/example.md',
  basePath: '/material-sandbox/',
  articleRoutes,
  qrcRoutes,
};
const sample = [
  '# Safe article',
  '',
  'Paragraph with **strong** text, *emphasis*, and `inline code`.',
  '',
  '[Known article](../material-design.md#behavior) [external](https://example.com/docs?q=1) [unsafe](javascript:alert(1)) [unknown](appearance.md)',
  '',
  '![Local asset](../assets/logo.png) ![Remote asset](https://example.com/logo.png)',
  '',
  '> Quoted `context`.',
  '',
  '- Parent',
  '  - Child',
  '- Second',
  '',
  '1. First',
  '2. Second',
  '',
  'A fresh sequence follows.',
  '',
  '3. Third',
  '4. Fourth',
  '',
  '| Name | Status |',
  '| --- | --- |',
  '| Local | Ready |',
  '',
  '```powershell',
  '<script>alert(1)</script>',
  '```',
  '',
  '<details open>',
  '<summary>More <config></summary>',
  'Details remain safe.',
  '</details>',
  '',
  '::: details Directive detail',
  'The directive is local too.',
  ':::',
  '',
  '<details>',
  '<summary>Outer</summary>',
  '<details>',
  '<summary>Inner</summary>',
  'Nested body.',
  '</details>',
  '</details>',
  '',
  '<script>outside-code-is-escaped</script>',
].join('\n');

const html = renderer.render(sample, options);
let checks = 0;
assert.throws(() => resolveManifestArticlePath({ path: '../../README.md' }), /escapes docs\//, 'article paths outside docs/ must be rejected');
checks += 1;
function includes(fragment, description) {
  assert.ok(html.includes(fragment), description);
  checks += 1;
}
function excludes(pattern, description) {
  assert.doesNotMatch(html, pattern, description);
  checks += 1;
}
function decodeAttribute(value) {
  return value
    .replace(/&#x([0-9a-f]+);/gi, (_, hex) => String.fromCodePoint(Number.parseInt(hex, 16)))
    .replace(/&#(\d+);/g, (_, decimal) => String.fromCodePoint(Number.parseInt(decimal, 10)))
    .replace(/&colon;/gi, ':')
    .replace(/&amp;/gi, '&');
}
function assertSafeRenderedMarkup(markup, description) {
  assert.doesNotMatch(markup, /<(?:a|article|blockquote|code|details|h[1-6]|img|li|ol|p|pre|span|summary|table|tbody|td|th|thead|tr|ul)\b[^>]*\son[a-z]+\s*=/i, `${description} must not contain live event attributes`);
  let attributeChecks = 1;
  for (const match of markup.matchAll(/\s(href|src|srcset)\s*=\s*(["'])(.*?)\2/gi)) {
    const name = match[1].toLowerCase();
    const value = decodeAttribute(match[3]).replace(/[\u0000-\u0020\u007f-\u009f]/g, '');
    if (name === 'href') {
      assert.ok(value.startsWith('#') || (value.startsWith('/') && !value.startsWith('//')) || /^https:\/\//i.test(value), `${description} has an unsafe href: ${match[3]}`);
    } else if (name === 'src') {
      assert.ok(value.startsWith('/') && !value.startsWith('//'), `${description} has a non-local image source: ${match[3]}`);
    } else {
      assert.fail(`${description} must not emit srcset`);
    }
    attributeChecks += 1;
  }
  return attributeChecks;
}

includes('data-markdown-renderer="local"', 'the output must be an in-site article shell');
includes('<h1 id="safe-article">Safe article</h1>', 'ATX headings must render with stable anchors');
includes('<strong>strong</strong>', 'strong text must render');
includes('<em>emphasis</em>', 'emphasis must render');
includes('<code>inline code</code>', 'inline code must render');
includes('href="#/articles/material-design#behavior"', 'known local Markdown must become an explicit article route');
includes('href="https://example.com/docs?q=1" target="_blank" rel="noopener noreferrer"', 'HTTPS links must remain safe external links');
includes('markdown-link-blocked', 'unknown and unsafe document links must stay honest and non-navigable');
includes('src="/material-sandbox/docs/assets/logo.png"', 'local static assets must resolve from the source Markdown file');
includes('markdown-image-blocked', 'remote images must be blocked by the local asset policy');
includes('<blockquote><p>Quoted <code>context</code>.</p></blockquote>', 'blockquotes must render');
includes('<ul><li>Parent<ul><li>Child</li></ul></li><li>Second</li></ul>', 'nested unordered lists must render');
includes('<ol><li>First</li><li>Second</li></ol>', 'ordered lists must render');
includes('<ol start="3"><li>Third</li><li>Fourth</li></ol>', 'ordered lists must preserve their first ordinal');
includes('<table><thead>', 'GFM tables must render');
includes('<pre><code class="language-powershell">&lt;script&gt;alert(1)&lt;/script&gt;</code></pre>', 'fenced code must be escaped');
includes('<details open><summary>More &lt;config&gt;</summary>', 'the details whitelist must preserve safe disclosure markup');
includes('<details><summary>Directive detail</summary>', 'details directives must render');
includes('<details><summary>Outer</summary><details><summary>Inner</summary><p>Nested body.</p></details></details>', 'nested details must preserve their disclosure boundaries');
includes('&lt;script&gt;outside-code-is-escaped&lt;/script&gt;', 'raw HTML must render as text');
excludes(/<script\b/i, 'the renderer must never emit executable script markup');
excludes(/href="(?:javascript|data|http):/i, 'unsafe or non-HTTPS schemes must not become links');
excludes(/src="https:/i, 'remote image URLs must not be emitted by default');
checks += assertSafeRenderedMarkup(html, 'the sample article');
assert.throws(() => assertSafeRenderedMarkup('<a href="//tracker.example/pixel">external</a>', 'protocol-relative link fixture'), /unsafe href/, 'protocol-relative hrefs must fail the output audit');
assert.throws(() => assertSafeRenderedMarkup('<img src="//tracker.example/pixel.png">', 'protocol-relative image fixture'), /non-local image source/, 'protocol-relative image sources must fail the output audit');
checks += 2;
assert.throws(() => assertSafeRenderedMarkup('<a href="&#x2f;&#x2f;tracker.example/pixel">entity-encoded external</a>', 'entity protocol-relative fixture'), /unsafe href/, 'entity-encoded protocol-relative hrefs must fail the output audit');
checks += 1;

assert.equal(renderer.normalizeArticlePath('../material-design.md#behavior', options), '#/articles/material-design#behavior');
assert.equal(renderer.normalizeArticlePath('appearance.md', options), null, 'unmapped Markdown paths must not be guessed');
assert.equal(renderer.normalizeArticlePath('qrc:/Docs/m3-shell-boundary.md', options), '#/articles/m3-shell-boundary');
assert.equal(renderer.normalizeArticlePath('qrc:/Docs/not-in-the-route-map.md', options), null, 'unmapped qrc documents must not be guessed');
assert.equal(renderer.sanitizeUrl('../assets/logo.png', { ...options, kind: 'image' }), '/material-sandbox/docs/assets/logo.png');
assert.equal(renderer.sanitizeUrl('https://example.com/reference.md', options), 'https://example.com/reference.md');
assert.equal(renderer.sanitizeUrl('javascript:alert(1)', options), null);
assert.equal(renderer.sanitizeUrl('java%73cript:alert(1)', options), null);
assert.equal(renderer.sanitizeUrl('data:text/html,boom', options), null);
assert.equal(renderer.sanitizeUrl('http://example.com', options), null);
assert.equal(renderer.sanitizeUrl('//example.com', options), null);
assert.equal(renderer.sanitizeUrl('..%2f..%2fprivate', options), null);
assert.equal(renderer.sanitizeUrl('/%2Ftracker.example/pixel.png', { ...options, basePath: '/', kind: 'image' }), null);
assert.equal(renderer.sanitizeUrl('../assets/report.pdf?source=article.md', options), '/material-sandbox/docs/assets/report.pdf?source=article.md');
assert.equal(renderer.sanitizeUrl('../assets/report.pdf#readme.md', options), '/material-sandbox/docs/assets/report.pdf#readme.md');
const escapedMarkup = renderer.render('[encoded](java%73cript:alert(1)) <img src=x onerror=alert(1)> <details ontoggle=alert(1)>blocked</details> <details open ontoggle=alert(1)>also blocked</details>', options);
assert.doesNotMatch(escapedMarkup, /<(?:img|details)\b[^>]*\bon\w+=/i, 'raw event attributes must not become live markup');
assert.match(escapedMarkup, /&lt;img src=x onerror=alert\(1\)&gt;/, 'raw markup must remain visible text');
assert.match(escapedMarkup, /&lt;details ontoggle=alert\(1\)&gt;/, 'details attributes outside the allowlist must remain visible text');
assert.match(escapedMarkup, /&lt;details open ontoggle=alert\(1\)&gt;/, 'open details with extra attributes must remain visible text');
checks += 19;
checks += assertSafeRenderedMarkup(escapedMarkup, 'escaped raw markup');

const screenshots = fs.readFileSync(path.join(root, 'docs', 'screenshots.md'), 'utf8');
const screenshotsHtml = renderer.render(screenshots, { ...options, sourcePath: 'docs/screenshots.md' });
assert.match(screenshotsHtml, /<table><thead>/, 'the existing screenshot gallery must render as a table');
assert.match(screenshotsHtml, /src="\/material-sandbox\/SandboxiePlus\/SandMan\/Resources\/SandMan\.png"/, 'the existing gallery must normalize source-relative local assets');
checks += 2;
checks += assertSafeRenderedMarkup(screenshotsHtml, 'screenshot gallery');

const fencedRawDetails = renderer.render(['<details>', '<summary>Fence-safe</summary>', '```html', '<details>', '</details>', '```', 'Body', '</details>'].join('\n'), options);
const fencedDirectiveDetails = renderer.render(['::: details Directive fence-safe', '```text', ':::', '```', 'Body', ':::'].join('\n'), options);
assert.match(fencedRawDetails, /<details><summary>Fence-safe<\/summary><pre><code class="language-html">&lt;details&gt;\n&lt;\/details&gt;<\/code><\/pre><p>Body<\/p><\/details>/, 'raw disclosures must ignore literal details markers in fenced code');
assert.match(fencedDirectiveDetails, /<details><summary>Directive fence-safe<\/summary><pre><code class="language-text">:::<\/code><\/pre><p>Body<\/p><\/details>/, 'directive disclosures must ignore literal closing markers in fenced code');
checks += 2;
checks += assertSafeRenderedMarkup(fencedRawDetails, 'raw fenced disclosure');
checks += assertSafeRenderedMarkup(fencedDirectiveDetails, 'directive fenced disclosure');

for (const article of manifest.articles) {
  const sourcePath = resolveManifestArticlePath(article);
  const markdown = fs.readFileSync(path.join(root, sourcePath), 'utf8');
  const articleHtml = renderer.render(markdown, { ...options, sourcePath });
  assert.match(articleHtml, /<article class="markdown-content"/, `${article.slug} must render in the local shell`);
  assert.match(articleHtml, /<h1\b/, `${article.slug} must retain its title`);
  assert.doesNotMatch(articleHtml, /<script\b/i, `${article.slug} must not emit script markup`);
  assert.doesNotMatch(articleHtml, /(?:href|src)="qrc:/i, `${article.slug} must not emit qrc URLs into a browser`);
  assert.doesNotMatch(articleHtml, /src="https:/i, `${article.slug} must not emit remote images`);
  checks += 5;
  checks += assertSafeRenderedMarkup(articleHtml, article.slug);
}

console.log(`pages-markdown-renderer-valid checks=${checks} articles=${manifest.articles.length}`);
