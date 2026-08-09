import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { TextDecoder } from 'node:util';
import { pathToFileURL } from 'node:url';

export const canonicalArticles = Object.freeze([
  { slug: 'material-design', title: 'Material Design 3 desktop shell', path: '../material-design.md' },
  { slug: 'appearance-editor', title: 'Material appearance editor', path: '../features/appearance-editor.md' },
  { slug: 'settings-provenance', title: 'Settings explanations and provenance', path: '../features/settings-provenance.md' },
  { slug: 'contributor-build', title: 'Contributor capability mode', path: '../contributor-build.md' },
  { slug: 'contributor-build-audit', title: 'Contributor capability and quiet certificate boundary', path: '../features/contributor-build-audit.md' },
  { slug: 'settings-history', title: 'Local settings history', path: '../features/settings-history.md' },
  { slug: 'notifications', title: 'Notification center', path: '../features/notifications.md' },
  { slug: 'color-translator', title: 'Material color translator', path: '../features/color-translator.md' },
  { slug: 'school-mode', title: 'School mode', path: '../features/school-mode.md' },
  { slug: 'dim-sum-surprise', title: 'Dim-sum startup surprise', path: '../features/dim-sum-surprise.md' },
  { slug: 'scheduled-settings', title: 'Scheduled settings', path: '../features/scheduled-settings.md' },
  { slug: 'tab-discovery', title: 'Tab discovery', path: '../features/tab-discovery.md' },
  { slug: 'command-palette', title: 'Command palette', path: '../features/command-palette.md' },
  { slug: 'external-editor', title: 'External editor integration', path: '../features/external-editor.md' },
  { slug: 'destructive-confirmation', title: 'Destructive-action confirmation', path: '../features/destructive-confirmation.md' },
  { slug: 'native-ci-evidence', title: 'Native CI evidence and stale-run control', path: '../features/native-ci-evidence.md' },
  { slug: 'build-entrypoints', title: 'Windows build entrypoints', path: '../features/build-entrypoints.md' },
  { slug: 'm3-shell-boundary', title: 'Material 3 shell boundary', path: '../features/ui/m3-shell-boundary.md' },
  { slug: 'editor-settings', title: 'Editor settings', path: '../features/editor-settings.md' },
  { slug: 'changelog-viewer', title: 'In-app changelog viewer', path: '../features/changelog-viewer.md' },
  { slug: 'pages-language-tone', title: 'GitHub Pages language, tone, and appearance controls', path: '../features/pages-language-tone.md' },
  { slug: 'pages-a11y-boundary', title: 'GitHub Pages navigation and overlay boundary', path: '../features/pages-a11y-boundary.md' },
]);

export const canonicalSupplemental = Object.freeze([
  { slug: 'screenshots', title: 'Material desktop screenshot gallery', path: '../screenshots.md' },
]);

const utf8Decoder = new TextDecoder('utf-8', { fatal: true });
const slugPattern = /^[a-z0-9]+(?:-[a-z0-9]+)*$/;

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function countOccurrences(text, needle) {
  return text.split(needle).length - 1;
}

function isWithin(base, candidate) {
  const relative = path.relative(base, candidate);
  return relative === '' || (relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative));
}

function assertNoLinkComponents(base, candidate, label) {
  const absoluteBase = path.resolve(base);
  const absoluteCandidate = path.resolve(candidate);
  assert(isWithin(absoluteBase, absoluteCandidate), `${label} escapes ${absoluteBase}`);
  const relativeParts = path.relative(absoluteBase, absoluteCandidate).split(path.sep).filter(Boolean);
  let current = absoluteBase;
  assert(!fs.lstatSync(current).isSymbolicLink(), `${label} base must not be a symbolic link or junction: ${current}`);
  for (const part of relativeParts) {
    current = path.join(current, part);
    const stat = fs.lstatSync(current);
    assert(!stat.isSymbolicLink(), `${label} must not traverse a symbolic link or junction: ${current}`);
  }
}

function assertContainedRegularFile(base, candidate, label) {
  const absoluteBase = path.resolve(base);
  const absoluteCandidate = path.resolve(candidate);
  assert(fs.existsSync(absoluteCandidate), `${label} is missing: ${absoluteCandidate}`);
  assertNoLinkComponents(absoluteBase, absoluteCandidate, label);
  const stat = fs.lstatSync(absoluteCandidate);
  assert(stat.isFile(), `${label} must be an ordinary file: ${absoluteCandidate}`);
  const realBase = fs.realpathSync.native(absoluteBase);
  const realCandidate = fs.realpathSync.native(absoluteCandidate);
  assert(isWithin(realBase, realCandidate), `${label} resolves outside ${realBase}: ${realCandidate}`);
  return { absolute: absoluteCandidate, real: realCandidate, bytes: fs.readFileSync(absoluteCandidate) };
}

function decodeUtf8(bytes, label) {
  try {
    return utf8Decoder.decode(bytes);
  } catch {
    throw new Error(`${label} is not valid UTF-8`);
  }
}

function normalizeManifestSource(record, label) {
  assert(typeof record.path === 'string' && record.path.length > 0, `${label} path must be non-empty`);
  assert(!record.path.includes('\\') && !record.path.includes('\0'), `${label} path must use safe forward slashes`);
  assert(!path.posix.isAbsolute(record.path) && !/^[A-Za-z][A-Za-z0-9+.-]*:/.test(record.path), `${label} path must be relative`);
  const normalized = path.posix.normalize(path.posix.join('articles', record.path));
  assert(normalized !== '..' && !normalized.startsWith('../') && normalized.endsWith('.md'), `${label} path escapes docs or is not Markdown: ${record.path}`);
  return normalized;
}

function assertExactRecords(actual, expected, label) {
  assert(Array.isArray(actual), `${label} must be an array`);
  assert(actual.length === expected.length, `${label} must contain exactly ${expected.length} ordered records; found ${actual.length}`);
  const slugs = new Set();
  const sources = new Set();
  for (let index = 0; index < expected.length; index += 1) {
    const record = actual[index];
    const canonical = expected[index];
    assert(record && typeof record === 'object' && !Array.isArray(record), `${label}[${index}] must be an object`);
    assert(slugPattern.test(record.slug), `${label}[${index}] has an unsafe or non-lowercase slug: ${record.slug}`);
    assert(record.slug === canonical.slug, `${label}[${index}] slug must be ${canonical.slug}; found ${record.slug}`);
    assert(record.title === canonical.title, `${label}[${index}] title must be ${canonical.title}; found ${record.title}`);
    assert(record.path === canonical.path, `${label}[${index}] path must be ${canonical.path}; found ${record.path}`);
    const source = normalizeManifestSource(record, `${label}[${index}]`);
    assert(!slugs.has(record.slug), `${label} duplicates slug ${record.slug}`);
    assert(!sources.has(source), `${label} duplicates source ${source}`);
    slugs.add(record.slug);
    sources.add(source);
  }
}

function walkMarkdownFiles(directory, rootDirectory) {
  const result = [];
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const absolute = path.join(directory, entry.name);
    assert(!entry.isSymbolicLink(), `documentation discovery must not traverse a link: ${absolute}`);
    if (entry.isDirectory()) result.push(...walkMarkdownFiles(absolute, rootDirectory));
    else if (entry.isFile() && entry.name.toLowerCase().endsWith('.md')) result.push(path.relative(rootDirectory, absolute).replaceAll('\\', '/'));
  }
  return result;
}

function assertSameSet(actual, expected, label) {
  const actualSet = new Set(actual);
  const expectedSet = new Set(expected);
  assert(actualSet.size === actual.length, `${label} contains duplicates`);
  const missing = [...expectedSet].filter(value => !actualSet.has(value));
  const extra = [...actualSet].filter(value => !expectedSet.has(value));
  assert(missing.length === 0 && extra.length === 0, `${label} mismatch; missing=[${missing.join(', ')}] extra=[${extra.join(', ')}]`);
}

export function resolveInternalDocumentationLink(currentSource, href, knownSources) {
  assert(typeof href === 'string' && href.length > 0, 'documentation link destination is empty');
  if (href.startsWith('#')) return { source: currentSource, fragment: href.slice(1) };
  const scheme = href.match(/^([A-Za-z][A-Za-z0-9+.-]*):/);
  if (scheme) {
    assert(scheme[1].toLowerCase() === 'https', `unsupported external documentation scheme: ${scheme[1]}`);
    return { external: href };
  }
  const [withoutFragment, fragment = ''] = href.split('#', 2);
  const withoutQuery = withoutFragment.split('?', 1)[0];
  assert(!withoutQuery.includes('\\') && !path.posix.isAbsolute(withoutQuery), `unsafe local documentation link: ${href}`);
  const target = path.posix.normalize(path.posix.join(path.posix.dirname(currentSource), withoutQuery));
  assert(target !== '..' && !target.startsWith('../'), `local documentation link escapes docs: ${href}`);
  assert(knownSources.has(target), `local documentation destination is not bundled: ${currentSource} -> ${href} (${target})`);
  return { source: target, fragment };
}

function parseMarkdownLinks(markdown) {
  return [...markdown.matchAll(/(?<!!)\[[^\]]*\]\(([^)\s]+)(?:\s+"[^"]*")?\)/g)].map(match => match[1]);
}

function parseQrcFiles(qrc) {
  return [...qrc.matchAll(/<file(?:\s+alias="([^"]+)")?>([^<]+)<\/file>/g)]
    .map(match => ({ alias: match[1] ?? '', source: match[2] }));
}

function assertProjectRegistration(text, token, label) {
  const count = countOccurrences(text, token);
  assert(count === 1, `${label} must register ${token} exactly once; found ${count}`);
}

export function validateDocumentationRepository(root = process.cwd(), { validateGitHistory = true } = {}) {
  const repositoryRoot = path.resolve(root);
  const docsRoot = path.join(repositoryRoot, 'docs');
  const manifestPath = path.join(docsRoot, 'articles', 'index.json');
  const manifestFile = assertContainedRegularFile(repositoryRoot, manifestPath, 'documentation manifest');
  const manifest = JSON.parse(decodeUtf8(manifestFile.bytes, 'documentation manifest'));
  assert(manifest.version === 1, `documentation manifest version must be 1; found ${manifest.version}`);
  assertExactRecords(manifest.articles, canonicalArticles, 'manifest articles');
  assertExactRecords(manifest.supplemental, canonicalSupplemental, 'manifest supplemental documents');

  const canonicalSlugs = new Set(canonicalArticles.map(record => record.slug));
  const allRecords = [...manifest.articles, ...manifest.supplemental];
  const knownSources = new Set();
  const canonicalFiles = new Map();
  const realSources = new Set();
  for (const record of allRecords) {
    const source = normalizeManifestSource(record, `manifest record ${record.slug}`);
    const sourceFile = assertContainedRegularFile(docsRoot, path.join(docsRoot, ...source.split('/')), `documentation source ${record.slug}`);
    assert(sourceFile.bytes.length <= 512 * 1024, `documentation source exceeds the 512 KiB search bound: ${record.slug}`);
    assert(!realSources.has(sourceFile.real), `documentation records resolve to the same file: ${record.slug}`);
    realSources.add(sourceFile.real);
    knownSources.add(source);
    canonicalFiles.set(record.slug, { ...sourceFile, source, record });
    const markdown = decodeUtf8(sourceFile.bytes, `documentation source ${record.slug}`);
    const title = markdown.match(/^#\s+(.+)$/m)?.[1]?.trim();
    assert(title === record.title, `documentation title drift for ${record.slug}; manifest=${record.title} markdown=${title ?? '<missing>'}`);
  }

  for (const article of manifest.articles) {
    assert(Array.isArray(article.related) && article.related.length > 0, `article ${article.slug} must name related articles`);
    assert(new Set(article.related).size === article.related.length, `article ${article.slug} repeats a related slug`);
    for (const related of article.related) {
      assert(related !== article.slug && canonicalSlugs.has(related), `article ${article.slug} has unknown or self-related slug ${related}`);
    }
  }

  assert(manifest.changelog?.path === '../changelog.md', 'manifest changelog path must be ../changelog.md');
  assert(/^[0-9a-f]{40}$/.test(manifest.changelog.commit), 'changelog manifest commit must be a full lowercase SHA');
  const changelogSource = normalizeManifestSource({ path: manifest.changelog.path }, 'manifest changelog');
  const changelogFile = assertContainedRegularFile(docsRoot, path.join(docsRoot, ...changelogSource.split('/')), 'documentation changelog');
  assert(!realSources.has(changelogFile.real), 'changelog must be distinct from every article source');
  knownSources.add(changelogSource);
  const changelog = decodeUtf8(changelogFile.bytes, 'documentation changelog');
  assert(new RegExp(`\\b${manifest.changelog.commit}\\b`).test(changelog), 'changelog commit is missing from changelog.md');
  const commitLinks = [...changelog.matchAll(/https:\/\/github\.com\/Ding-Ding-Projects\/material-sandbox\/commit\/([0-9a-f]{40})\b/g)].map(match => match[1]);
  assert(commitLinks.length > 0, 'changelog has no full commit links');
  assert(!/Commit:\s*(?:pending|`[0-9a-f]{1,39}`|[^\n]*integration pending)/i.test(changelog), 'changelog contains pending or short commit references');
  if (validateGitHistory) {
    for (const sha of new Set(commitLinks)) {
      try {
        execFileSync('git', ['cat-file', '-e', `${sha}^{commit}`], { cwd: repositoryRoot, stdio: 'ignore' });
      } catch {
        throw new Error(`changelog commit does not resolve locally: ${sha}`);
      }
    }
  }

  const discoveredFeatures = walkMarkdownFiles(path.join(docsRoot, 'features'), docsRoot).filter(source => source !== 'features/README.md').sort();
  const canonicalFeatures = canonicalArticles.map(record => normalizeManifestSource(record, `canonical ${record.slug}`)).filter(source => source.startsWith('features/')).sort();
  assertSameSet(discoveredFeatures, canonicalFeatures, 'feature Markdown discovery');

  const featureIndexPath = path.join(docsRoot, 'features', 'README.md');
  const featureIndex = decodeUtf8(assertContainedRegularFile(docsRoot, featureIndexPath, 'feature documentation index').bytes, 'feature documentation index');
  const indexSources = parseMarkdownLinks(featureIndex)
    .filter(href => !/^[A-Za-z][A-Za-z0-9+.-]*:/.test(href))
    .map(href => path.posix.normalize(path.posix.join('features', href.split('#', 1)[0])));
  const canonicalIndexSources = canonicalArticles.map(record => normalizeManifestSource(record, `canonical index ${record.slug}`));
  assertSameSet(indexSources, canonicalIndexSources, 'feature documentation index links');

  let internalLinkCount = 0;
  let externalLinkCount = 0;
  for (const article of manifest.articles) {
    const source = canonicalFiles.get(article.slug).source;
    const markdown = decodeUtf8(canonicalFiles.get(article.slug).bytes, `documentation source ${article.slug}`);
    for (const href of parseMarkdownLinks(markdown)) {
      const resolution = resolveInternalDocumentationLink(source, href, knownSources);
      if (resolution.external) externalLinkCount += 1;
      else internalLinkCount += 1;
    }
  }
  const samePageFragment = resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '#verification', knownSources);
  assert(samePageFragment.source === 'features/pages-a11y-boundary.md' && samePageFragment.fragment === 'verification', 'same-page fragment routing contract failed');
  const crossPageFragment = resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '../material-design.md#failure-modes', knownSources);
  assert(crossPageFragment.source === 'material-design.md' && crossPageFragment.fragment === 'failure-modes', 'cross-article fragment routing contract failed');

  const qrcPath = path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  const qrcFile = assertContainedRegularFile(repositoryRoot, qrcPath, 'SandMan Qt resource file');
  const qrc = decodeUtf8(qrcFile.bytes, 'SandMan Qt resource file');
  const qrcDirectory = path.dirname(qrcPath);
  const expectedQrc = [
    { alias: 'Docs/articles/index.json', source: '../../../docs/articles/index.json', real: manifestFile.real, bytes: manifestFile.bytes },
    ...canonicalArticles.map(record => {
      const file = canonicalFiles.get(record.slug);
      return { alias: `Docs/articles/${record.slug}.md`, source: `../../../docs/${file.source}`, real: file.real, bytes: file.bytes };
    }),
    ...canonicalSupplemental.map(record => {
      const file = canonicalFiles.get(record.slug);
      return { alias: `Docs/supplemental/${record.slug}.md`, source: `../../../docs/${file.source}`, real: file.real, bytes: file.bytes };
    }),
    { alias: 'Docs/changelog.md', source: '../../../docs/changelog.md', real: changelogFile.real, bytes: changelogFile.bytes },
  ];
  const actualQrc = parseQrcFiles(qrc).filter(record => record.alias.startsWith('Docs/'));
  assert(actualQrc.length === expectedQrc.length, `Qt documentation resource set must contain exactly ${expectedQrc.length} entries; found ${actualQrc.length}`);
  for (let index = 0; index < expectedQrc.length; index += 1) {
    const actual = actualQrc[index];
    const expected = expectedQrc[index];
    assert(actual.alias === expected.alias, `Qt documentation resource alias[${index}] must be ${expected.alias}; found ${actual.alias}`);
    assert(actual.source === expected.source, `Qt documentation resource source for ${expected.alias} must be ${expected.source}; found ${actual.source}`);
    const bundled = assertContainedRegularFile(repositoryRoot, path.resolve(qrcDirectory, actual.source), `Qt documentation resource ${actual.alias}`);
    assert(bundled.real === expected.real, `Qt documentation resource ${actual.alias} does not resolve to its canonical source`);
    assert(bundled.bytes.equals(expected.bytes), `Qt documentation resource ${actual.alias} differs byte-for-byte from its canonical source`);
  }
  assert(new Set(actualQrc.map(record => record.alias)).size === actualQrc.length, 'Qt documentation resource aliases must be unique');

  const pri = decodeUtf8(assertContainedRegularFile(repositoryRoot, path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'SandMan.pri'), 'SandMan.pri').bytes, 'SandMan.pri');
  const vcxproj = decodeUtf8(assertContainedRegularFile(repositoryRoot, path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'SandMan.vcxproj'), 'SandMan.vcxproj').bytes, 'SandMan.vcxproj');
  for (const [token, label] of [
    ['./Windows/DocumentationBrowser.cpp', 'SandMan.pri'],
    ['./Windows/DocumentationBrowser.h', 'SandMan.pri'],
    ['RESOURCES += Resources/SandMan.qrc', 'SandMan.pri'],
  ]) assertProjectRegistration(pri, token, label);
  for (const token of [
    '<ClCompile Include="Windows\\DocumentationBrowser.cpp" />',
    '<QtMoc Include="Windows\\DocumentationBrowser.h" />',
    '<QtRcc Include="Resources\\SandMan.qrc" />',
  ]) assertProjectRegistration(vcxproj, token, 'SandMan.vcxproj');

  const browserPath = path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'Windows', 'DocumentationBrowser.cpp');
  const browserCpp = decodeUtf8(assertContainedRegularFile(repositoryRoot, browserPath, 'DocumentationBrowser.cpp').bytes, 'DocumentationBrowser.cpp');
  const browserContracts = [
    'QFile manifestFile(QStringLiteral(":/Docs/articles/index.json"))',
    'manifest.value(QStringLiteral("articles")).toArray()',
    'manifest.value(QStringLiteral("supplemental")).toArray()',
    'articlesBySlug.contains(slug)',
    'articlesBySourcePath.contains(sourcePath)',
    'm_articleBySourcePath.constFind(targetPath)',
    'm_articleList->setCurrentRow(target.value())',
    'scrollToAnchor(url.fragment())',
    'QTextDocument::MarkdownFeatures{QTextDocument::MarkdownDialectGitHub, QTextDocument::MarkdownNoHTML}',
    'QT_VERSION_CHECK(5, 14, 0)',
    'markdownToHtml(article.body)',
    'scheme == QStringLiteral("https")',
    'url.userInfo().isEmpty()',
    'QDesktopServices::openUrl(url)',
    'targetPath.startsWith(QStringLiteral("../"))',
    '(*LIMIT_MATCH=100000)(*LIMIT_DEPTH=1000)',
    '&QLineEdit::textEdited',
    'setAccessibleName(tr("Bundled documentation articles"))',
    'No documentation articles match the active search.',
    '%1 of %2 bundled documents match',
    'bool CDocumentationBrowser::openArticle(const QString& slug)',
  ];
  for (const contract of browserContracts) assert(browserCpp.includes(contract), `DocumentationBrowser contract missing: ${contract}`);
  assert(countOccurrences(browserCpp, '&QTextBrowser::anchorClicked') === 2, 'both article and changelog views must route anchorClicked through the allowlist');
  for (const forbidden of [
    'setOpenLinks(true)',
    'setOpenExternalLinks(true)',
    'scheme == QStringLiteral("http")',
    'const QStringList paths',
  ]) assert(!browserCpp.includes(forbidden), `DocumentationBrowser contains forbidden contract: ${forbidden}`);

  const sandManCppPath = path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'SandMan.cpp');
  const sandManCpp = decodeUtf8(assertContainedRegularFile(repositoryRoot, sandManCppPath, 'SandMan.cpp').bytes, 'SandMan.cpp');
  for (const paletteContract of [
    'QFile documentationManifest(QStringLiteral(":/Docs/articles/index.json"))',
    'manifest.object().value(QStringLiteral("articles")).toArray()',
    'article.value(QStringLiteral("slug")).toString()',
    'article.value(QStringLiteral("title")).toString()',
    'browser->openArticle(slug)',
  ]) assert(sandManCpp.includes(paletteContract), `command palette documentation contract missing: ${paletteContract}`);

  return {
    articleCount: manifest.articles.length,
    supplementalCount: manifest.supplemental.length,
    internalLinkCount,
    externalLinkCount,
    changelogCommit: manifest.changelog.commit,
  };
}

function parseArguments(argv) {
  let root = process.cwd();
  let validateGitHistory = true;
  for (let index = 0; index < argv.length; index += 1) {
    if (argv[index] === '--root') {
      assert(index + 1 < argv.length, '--root requires a path');
      root = path.resolve(argv[index + 1]);
      index += 1;
    } else if (argv[index] === '--skip-git-history') {
      validateGitHistory = false;
    } else {
      throw new Error(`unknown argument: ${argv[index]}`);
    }
  }
  return { root, validateGitHistory };
}

if (process.argv[1] && pathToFileURL(path.resolve(process.argv[1])).href === import.meta.url) {
  try {
    const options = parseArguments(process.argv.slice(2));
    const result = validateDocumentationRepository(options.root, { validateGitHistory: options.validateGitHistory });
    console.log(`docs-valid articles=${result.articleCount} supplemental=${result.supplementalCount} internal-links=${result.internalLinkCount} external-links=${result.externalLinkCount} changelog=${result.changelogCommit}`);
  } catch (error) {
    console.error(`docs-invalid: ${error instanceof Error ? error.message : String(error)}`);
    process.exitCode = 1;
  }
}
