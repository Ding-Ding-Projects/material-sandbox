import fs from 'node:fs';
import path from 'node:path';

const FEATURE_ARTICLE_COUNT = 22;
const MAX_MANIFEST_BYTES = 128 * 1024;
const MAX_MARKDOWN_BYTES = 1024 * 1024;
const RESERVED_SLUGS = new Set(['changelog', 'screenshots']);

class BuildFailure extends Error {}

function fail(message) { throw new BuildFailure(message); }
function normalize(candidate) { return path.resolve(candidate).replace(/^\\\\\?\\/, ''); }
function inside(root, candidate, allowRoot = false) {
  const relative = path.relative(root, candidate);
  return allowRoot ? !relative || (relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative)) : Boolean(relative) && relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative);
}
function assertRegular(root, candidate, label, maximum = MAX_MARKDOWN_BYTES) {
  if (!inside(root, candidate)) fail(`${label} escapes docs`);
  const stat = fs.lstatSync(candidate);
  if (stat.isSymbolicLink() || !stat.isFile()) fail(`${label} must be an ordinary file`);
  if (stat.size > maximum) fail(`${label} exceeds ${maximum} bytes`);
  return stat;
}
function assertNoLinks(directory, label) {
  const rootStat = fs.lstatSync(directory);
  if (rootStat.isSymbolicLink() || !rootStat.isDirectory()) fail(`${label} must be an ordinary directory`);
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const candidate = path.join(directory, entry.name);
    const stat = fs.lstatSync(candidate);
    if (entry.isSymbolicLink() || stat.isSymbolicLink()) fail(`${label} contains a symbolic-link or reparse entry: ${entry.name}`);
    if (entry.isDirectory()) assertNoLinks(candidate, `${label}/${entry.name}`);
  }
}
function escapeHtml(value) {
  return String(value).replace(/[&<>'"]/g, (character) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', "'": '&#39;', '"': '&quot;' }[character]));
}
function titleFromMarkdown(markdown, fallback) {
  const line = /^#\s+(.+)$/m.exec(markdown)?.[1] || fallback;
  return line.replace(/[`*_~]/g, '').replace(/\[([^\]]+)\]\([^)]*\)/g, '$1').replace(/\s+/g, ' ').trim().slice(0, 160) || fallback;
}
function parseManifest(docsRoot) {
  const manifestPath = path.join(docsRoot, 'articles', 'index.json');
  assertRegular(docsRoot, manifestPath, 'article manifest', MAX_MANIFEST_BYTES);
  let manifest;
  try { manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8')); } catch (error) { fail(`article manifest is invalid JSON: ${error.message}`); }
  if (!Array.isArray(manifest.articles) || manifest.articles.length !== FEATURE_ARTICLE_COUNT) fail(`article manifest must contain exactly ${FEATURE_ARTICLE_COUNT} feature articles`);
  const manifestDirectory = path.dirname(manifestPath); const seen = new Set();
  const articles = manifest.articles.map((entry) => {
    if (!entry || typeof entry !== 'object' || typeof entry.slug !== 'string' || !/^[a-z\d]+(?:-[a-z\d]+)*$/.test(entry.slug) || RESERVED_SLUGS.has(entry.slug)) fail('article manifest contains an invalid or reserved slug');
    if (seen.has(entry.slug)) fail(`article manifest contains duplicate slug ${entry.slug}`); seen.add(entry.slug);
    if (typeof entry.path !== 'string' || !entry.path || entry.path.includes('\\') || path.isAbsolute(entry.path)) fail(`${entry.slug} has an invalid Markdown path`);
    const sourcePath = normalize(path.join(manifestDirectory, ...entry.path.split('/')));
    assertRegular(docsRoot, sourcePath, `${entry.slug} Markdown source`);
    return { slug: entry.slug, sourcePath, title: titleFromMarkdown(fs.readFileSync(sourcePath, 'utf8'), entry.slug) };
  });
  if (!manifest.changelog || typeof manifest.changelog.path !== 'string') fail('article manifest has no changelog path');
  const changelogPath = normalize(path.join(manifestDirectory, ...manifest.changelog.path.split('/')));
  assertRegular(docsRoot, changelogPath, 'changelog Markdown source');
  const screenshotsPath = path.join(docsRoot, 'screenshots.md');
  assertRegular(docsRoot, screenshotsPath, 'screenshots Markdown source');
  return [
    ...articles,
    { slug: 'changelog', sourcePath: changelogPath, title: titleFromMarkdown(fs.readFileSync(changelogPath, 'utf8'), 'Changelog') },
    { slug: 'screenshots', sourcePath: screenshotsPath, title: titleFromMarkdown(fs.readFileSync(screenshotsPath, 'utf8'), 'Screenshots') },
  ];
}
function redirectPage(record, revision) {
  const route = `../#/articles/${encodeURIComponent(record.slug)}`;
  const title = escapeHtml(record.title);
  return `<!doctype html>\n<html lang="en">\n<head>\n  <meta charset="utf-8">\n  <meta name="viewport" content="width=device-width, initial-scale=1">\n  <meta name="source-revision" content="${revision}">\n  <meta http-equiv="refresh" content="0; url=${route}">\n  <title>${title} · Sandboxie documentation</title>\n  <link rel="stylesheet" href="../assets/app.css">\n</head>\n<body>\n  <main class="app-shell"><section class="empty-state" aria-labelledby="article-title"><div><h1 id="article-title">${title}</h1><p>Opening this local article in the Material Design 3 documentation workspace.</p><p><a class="tonal" href="${route}">Open article</a></p></div></section></main>\n  <script>location.replace(${JSON.stringify(route)});</script>\n</body>\n</html>\n`;
}
function clearGeneratedArticles(directory) {
  if (!fs.existsSync(directory)) { fs.mkdirSync(directory, { recursive: true }); return; }
  const stat = fs.lstatSync(directory); if (stat.isSymbolicLink() || !stat.isDirectory()) fail('generated articles target must be an ordinary directory');
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    if (!entry.name.endsWith('.html')) continue;
    const candidate = path.join(directory, entry.name); const entryStat = fs.lstatSync(candidate);
    if (entry.isSymbolicLink() || entryStat.isSymbolicLink() || !entry.isFile()) fail(`generated article ${entry.name} is not an ordinary file`);
    fs.unlinkSync(candidate);
  }
}
function main() {
  const [sourceArgument, targetArgument, revisionArgument, ...rest] = process.argv.slice(2);
  if (rest.length || !sourceArgument || !targetArgument || !/^[a-f\d]{40}$/i.test(revisionArgument || '')) fail('usage: node scripts/build-pages-articles.mjs docs .pages-site <40-char-revision>');
  const checkout = normalize(process.cwd()); const docsRoot = normalize(sourceArgument); const targetRoot = normalize(targetArgument);
  if (docsRoot !== path.join(checkout, 'docs') || targetRoot !== path.join(checkout, '.pages-site')) fail('source and target must be the checkout docs and .pages-site directories');
  assertNoLinks(docsRoot, 'Pages source'); assertNoLinks(targetRoot, 'Pages staging');
  const stagedIndex = path.join(targetRoot, 'index.html'); assertRegular(targetRoot, stagedIndex, 'stamped site shell', 2 * 1024 * 1024);
  const stamped = fs.readFileSync(stagedIndex, 'utf8'); const revision = revisionArgument.toLowerCase();
  if (stamped.includes('PAGES_SOURCE_REVISION') || !stamped.includes(revision)) fail('stamped site shell must carry the exact requested revision');
  const records = parseManifest(docsRoot); const output = path.join(targetRoot, 'articles'); clearGeneratedArticles(output);
  for (const record of records) fs.writeFileSync(path.join(output, `${record.slug}.html`), redirectPage(record, revision), { encoding: 'utf8', flag: 'wx' });
  const inventory = fs.readdirSync(output).filter((name) => name.endsWith('.html')).sort();
  const expected = records.map((record) => `${record.slug}.html`).sort();
  if (JSON.stringify(inventory) !== JSON.stringify(expected)) fail('generated article inventory does not match manifest plus supplemental routes');
  process.stdout.write(`pages-articles-built features=${FEATURE_ARTICLE_COUNT} supplemental=2 revision=${revision} target=${output}\n`);
}

try { main(); } catch (error) { process.stderr.write(`pages-article-build-failed: ${error instanceof Error ? error.message : String(error)}\n`); process.exitCode = 1; }
