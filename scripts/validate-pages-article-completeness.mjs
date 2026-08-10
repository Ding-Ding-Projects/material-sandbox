import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const docsRoot = path.join(root, 'docs');
const manifestPath = path.join(docsRoot, 'articles', 'index.json');
const routeRegistryPath = path.join(docsRoot, 'assets', 'article-routes.json');
const expectedArticleCount = 23;
const failures = [];

function fail(message) {
  failures.push(message);
}

function readJson(filePath, label) {
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf8'));
  } catch (error) {
    fail(`${label} is not valid JSON: ${error.message}`);
    return {};
  }
}

function isInside(parentPath, candidatePath) {
  const relativePath = path.relative(parentPath, candidatePath);
  return relativePath !== '' && relativePath !== '..' && !relativePath.startsWith(`..${path.sep}`) && !path.isAbsolute(relativePath);
}

function canonicalArticleRoute(slug) {
  return `#/articles/${slug}`;
}

function assertInSiteArticleRoute(route, slug, label) {
  const expectedRoute = canonicalArticleRoute(slug);
  if (route !== expectedRoute) {
    fail(`${label} for ${slug} must be the stable local route ${expectedRoute}; received ${String(route)}`);
  }
}

function markdownLinkDestinations(markdown) {
  const withoutFencedCode = markdown.replace(/^ {0,3}(?:```|~~~)[\s\S]*?^ {0,3}(?:```|~~~)\s*$/gm, '');
  const destinations = [];
  const inlineLink = /!?\[[^\]\r\n]*\]\(\s*(?:<([^>\r\n]+)>|([^\s)]+))[^)]*\)/g;
  for (const match of withoutFencedCode.matchAll(inlineLink)) {
    destinations.push(match[1] ?? match[2]);
  }
  return destinations;
}

function validateLocalMarkdownLink(destination, sourcePath, sourceSlug) {
  if (!destination || destination.startsWith('#')) return;
  if (destination.startsWith('//')) {
    fail(`${sourceSlug} contains a protocol-relative nonlocal docs link: ${destination}`);
    return;
  }
  if (/^[a-z][a-z\d+.-]*:/i.test(destination)) {
    const scheme = destination.slice(0, destination.indexOf(':')).toLowerCase();
    if (!['http', 'https', 'mailto'].includes(scheme)) {
      fail(`${sourceSlug} contains an unsupported nonlocal docs link: ${destination}`);
      return;
    }
    try {
      if (scheme !== 'mailto') new URL(destination);
    } catch {
      fail(`${sourceSlug} contains an invalid external docs link: ${destination}`);
    }
    return;
  }
  if (destination.includes('\\')) {
    fail(`${sourceSlug} contains a non-web docs link with a backslash: ${destination}`);
    return;
  }
  const suffixStart = destination.search(/[?#]/);
  const relativeTarget = suffixStart === -1 ? destination : destination.slice(0, suffixStart);
  if (!relativeTarget) return;
  if (relativeTarget.startsWith('/') || path.isAbsolute(relativeTarget)) {
    fail(`${sourceSlug} contains a nonlocal docs path: ${destination}`);
    return;
  }
  const targetPath = path.resolve(path.dirname(sourcePath), relativeTarget);
  if (!isInside(docsRoot, targetPath)) {
    fail(`${sourceSlug} contains a docs link outside docs/: ${destination}`);
    return;
  }
  if (!fs.existsSync(targetPath)) {
    fail(`${sourceSlug} contains a missing local docs link: ${destination}`);
  }
}

const manifest = readJson(manifestPath, 'docs/articles/index.json');
const registry = readJson(routeRegistryPath, 'docs/assets/article-routes.json');
const manifestArticles = Array.isArray(manifest.articles) ? manifest.articles : [];
const registryArticles = Array.isArray(registry.articles) ? registry.articles : [];

if (manifestArticles.length !== expectedArticleCount) {
  fail(`docs/articles/index.json must contain exactly ${expectedArticleCount} articles; received ${manifestArticles.length}`);
}
if (registryArticles.length !== expectedArticleCount) {
  fail(`docs/assets/article-routes.json must contain exactly ${expectedArticleCount} route records; received ${registryArticles.length}`);
}
if (registry.version !== 1) fail(`docs/assets/article-routes.json version must be 1; received ${String(registry.version)}`);
if (registry.routePrefix !== '#/articles/') fail(`docs/assets/article-routes.json routePrefix must be #/articles/; received ${String(registry.routePrefix)}`);

const manifestBySlug = new Map();
for (const article of manifestArticles) {
  const slug = article?.slug;
  if (typeof slug !== 'string' || !/^[a-z\d]+(?:-[a-z\d]+)*$/.test(slug)) {
    fail(`invalid article slug in docs/articles/index.json: ${String(slug)}`);
    continue;
  }
  if (manifestBySlug.has(slug)) {
    fail(`duplicate article slug in docs/articles/index.json: ${slug}`);
    continue;
  }
  manifestBySlug.set(slug, article);
  if (!Array.isArray(article.related) || article.related.length === 0) {
    fail(`${slug} must declare at least one related article in docs/articles/index.json`);
  } else if (new Set(article.related).size !== article.related.length) {
    fail(`${slug} has duplicate related article metadata in docs/articles/index.json`);
  }
  if (typeof article.path !== 'string' || article.path.length === 0) {
    fail(`${slug} has no documentation source path`);
    continue;
  }
  const sourcePath = path.resolve(path.dirname(manifestPath), article.path);
  if (!isInside(docsRoot, sourcePath)) {
    fail(`${slug} documentation source escapes docs/: ${article.path}`);
    continue;
  }
  if (!fs.existsSync(sourcePath)) {
    fail(`${slug} documentation source is missing: ${article.path}`);
    continue;
  }
  const markdown = fs.readFileSync(sourcePath, 'utf8');
  if (!markdown.startsWith('# ')) fail(`${slug} documentation source must start with a level-one title`);
  for (const destination of markdownLinkDestinations(markdown)) {
    validateLocalMarkdownLink(destination, sourcePath, slug);
  }
}

for (const [slug, article] of manifestBySlug) {
  for (const relatedSlug of article.related ?? []) {
    if (relatedSlug === slug) fail(`${slug} cannot relate to itself`);
    if (!manifestBySlug.has(relatedSlug)) {
      fail(`${slug} references an unknown related article: ${String(relatedSlug)}`);
    }
  }
}

const registryBySlug = new Map();
const featureIds = new Set();
const paletteIds = new Set();
let relatedRouteCount = 0;
for (const record of registryArticles) {
  const slug = record?.slug;
  if (typeof slug !== 'string' || !manifestBySlug.has(slug)) {
    fail(`route registry contains an unknown article slug: ${String(slug)}`);
    continue;
  }
  if (registryBySlug.has(slug)) {
    fail(`route registry contains a duplicate article slug: ${slug}`);
    continue;
  }
  registryBySlug.set(slug, record);
  assertInSiteArticleRoute(record.route, slug, 'article route');

  const manifestRelated = manifestBySlug.get(slug).related ?? [];
  if (!Array.isArray(record.relatedArticles)) {
    fail(`${slug} route registry record must include relatedArticles metadata`);
  } else {
    if (record.relatedArticles.length !== manifestRelated.length) {
      fail(`${slug} relatedArticles metadata must mirror docs/articles/index.json`);
    }
    const seenRelatedSlugs = new Set();
    for (let index = 0; index < record.relatedArticles.length; index += 1) {
      const related = record.relatedArticles[index];
      const relatedSlug = related?.slug;
      if (!manifestBySlug.has(relatedSlug)) {
        fail(`${slug} route registry has an unknown related article: ${String(relatedSlug)}`);
        continue;
      }
      if (seenRelatedSlugs.has(relatedSlug)) fail(`${slug} route registry duplicates related article ${relatedSlug}`);
      seenRelatedSlugs.add(relatedSlug);
      if (relatedSlug !== manifestRelated[index]) {
        fail(`${slug} relatedArticles metadata must preserve docs/articles/index.json order`);
      }
      assertInSiteArticleRoute(related.route, relatedSlug, `related article route from ${slug}`);
      relatedRouteCount += 1;
    }
  }

  const feature = record.featureDestination;
  if (!feature || typeof feature !== 'object') {
    fail(`${slug} route registry record must include featureDestination metadata`);
  } else {
    const expectedId = `feature:${slug}`;
    if (feature.id !== expectedId) fail(`${slug} featureDestination id must be ${expectedId}`);
    if (featureIds.has(feature.id)) fail(`featureDestination id is duplicated: ${String(feature.id)}`);
    featureIds.add(feature.id);
    assertInSiteArticleRoute(feature.route, slug, 'featureDestination route');
  }

  const palette = record.paletteDestination;
  if (!palette || typeof palette !== 'object') {
    fail(`${slug} route registry record must include paletteDestination metadata`);
  } else {
    const expectedId = `palette:article:${slug}`;
    if (palette.id !== expectedId) fail(`${slug} paletteDestination id must be ${expectedId}`);
    if (palette.kind !== 'article') fail(`${slug} paletteDestination kind must be article`);
    if (paletteIds.has(palette.id)) fail(`paletteDestination id is duplicated: ${String(palette.id)}`);
    paletteIds.add(palette.id);
    assertInSiteArticleRoute(palette.route, slug, 'paletteDestination route');
  }
}

for (const slug of manifestBySlug.keys()) {
  if (!registryBySlug.has(slug)) fail(`missing local route metadata for docs/articles/index.json record: ${slug}`);
}

if (failures.length > 0) {
  console.error('pages-article-completeness failed');
  for (const failure of failures) console.error(`- ${failure}`);
  process.exit(1);
}

console.log(`pages-article-completeness articles=${manifestArticles.length} localRoutes=${registryArticles.length} relatedRoutes=${relatedRouteCount} featureDestinations=${featureIds.size} paletteDestinations=${paletteIds.size}`);
