import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';
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
  { slug: 'material-sandbox-ui-rewrite', title: 'Material Sandbox UI rewrite overlay', path: '../features/ui/material-sandbox-ui-rewrite.md' },
  { slug: 'editor-settings', title: 'Editor settings', path: '../features/editor-settings.md' },
  { slug: 'changelog-viewer', title: 'In-app changelog viewer', path: '../features/changelog-viewer.md' },
  { slug: 'pages-language-tone', title: 'GitHub Pages language, tone, and appearance controls', path: '../features/pages-language-tone.md' },
  { slug: 'pages-a11y-boundary', title: 'GitHub Pages navigation and overlay boundary', path: '../features/pages-a11y-boundary.md' },
]);

export const canonicalSupplemental = Object.freeze([
  { slug: 'screenshots', title: 'Material desktop screenshot gallery', path: '../screenshots.md' },
]);

export const canonicalScreenshotAssets = Object.freeze([
  { reference: '../SandboxiePlus/SandMan/Resources/SandMan.png', resource: 'SandMan.png', sha256: '6e80b03cc60544a24d789b7d4222ae4b2e030d074fb3c5abf056adcdd3680b47', width: 266, height: 266, bytes: 5203 },
  { reference: '../SandboxiePlus/SandMan/Resources/sandboxie-logo.png', resource: 'sandboxie-logo.png', sha256: 'a8f8db547da8575c4467aeff338da3880468d1e72b5eecb167a3ac31318d3c7b', width: 402, height: 91, bytes: 59666 },
  { reference: '../SandboxiePlus/SandMan/Resources/sandboxie-back.png', resource: 'sandboxie-back.png', sha256: '5371d50c1e52056b60f6fd4da3c9ff955b911421fdcdc66f1881bdc2f05c2df3', width: 4, height: 91, bytes: 531 },
  { reference: '../SandboxiePlus/SandMan/Resources/sandbox-empty.png', resource: 'sandbox-empty.png', sha256: '884d1950f9220f002e2e22c0935b1c3362934303ef90f6618d0da322aa70a7df', width: 32, height: 32, bytes: 1367 },
  { reference: '../SandboxiePlus/SandMan/Resources/sandbox-full.png', resource: 'sandbox-full.png', sha256: 'e57e07ebeddaeb157b6beb6080131e23737049fc81dfba32c6c2857bcbc51cf3', width: 32, height: 32, bytes: 1376 },
  { reference: '../SandboxiePlus/SandMan/Resources/Simple.png', resource: 'Simple.png', sha256: 'dc6dd93864bb6a0258304537d97389c6497e78cc8c2b836792a41323774aa86d', width: 445, height: 340, bytes: 6704 },
  { reference: '../SandboxiePlus/SandMan/Resources/SimpleD.png', resource: 'SimpleD.png', sha256: '9b0bc9daa57ffe901854ecebdd3df7410f817ad7245cd55f85b10ce788a51044', width: 445, height: 340, bytes: 6879 },
  { reference: '../SandboxiePlus/SandMan/Resources/Classic.png', resource: 'Classic.png', sha256: '8ed57dcacfb3932abae427ae07aa38cf984949ee896a20dd710e9e785d059f63', width: 445, height: 341, bytes: 23765 },
  { reference: '../SandboxiePlus/SandMan/Resources/ClassicD.png', resource: 'ClassicD.png', sha256: '31d33047c92551a4d09fdfd84b541d260bbc64a7fbc3c57fbe2ee2e634e2e00f', width: 445, height: 341, bytes: 24210 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Config.png', resource: 'Actions/Config.png', sha256: '8aaf30d5ac11cb0e1834fbe4c6ca0c9997db617d004e2036299c6db8392c9808', width: 50, height: 50, bytes: 2055 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Interface.png', resource: 'Actions/Interface.png', sha256: 'c0ff3eb0d87b58d8d649be160c5f3bd61ddcb2b1fd4dcf97ff44470ebc9b743a', width: 50, height: 50, bytes: 2189 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Design.png', resource: 'Actions/Design.png', sha256: 'faaed29e4940e0b104a21b70a57da4a7bf5c2165701235b0712b36a85b0e658a', width: 50, height: 50, bytes: 1624 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Security.png', resource: 'Actions/Security.png', sha256: '5f233832ab879cfd69089568d95799be5217acacbc4abfd9b39b77f8e7601433', width: 50, height: 50, bytes: 3062 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Network.png', resource: 'Actions/Network.png', sha256: 'ccb2ada428c0185f20750345392281fbbed6fb92e48c35a2dfa127f3f380f2f4', width: 50, height: 50, bytes: 1694 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/recover.png', resource: 'Actions/recover.png', sha256: 'f1078dddce98b2b7c7db1353a4fa7e5c1d07db21a06fb3562d1091fd8de6a3d7', width: 50, height: 50, bytes: 1964 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/box.png', resource: 'Actions/box.png', sha256: 'f1d6aa6dcb19babdecba853e20333b5ba39594f7ff7cbd47c82f32770c26a7d4', width: 50, height: 50, bytes: 732 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Folder.png', resource: 'Actions/Folder.png', sha256: 'f1f7924b105cec5b8948a837ea9065e062d6727ec1e70f056e3ed11676529bde', width: 50, height: 50, bytes: 1216 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Editor.png', resource: 'Actions/Editor.png', sha256: '50392dbdf01accba3fe23f28b66974afd49166e120f4a721a63a3d8c7267dc6d', width: 50, height: 50, bytes: 1559 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Accessibility.png', resource: 'Actions/Accessibility.png', sha256: '41fc72c12b3396e747142dfe5dbdc22743bd8aff8559919b9d97317580d598aa', width: 50, height: 50, bytes: 1903 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Notification.png', resource: 'Actions/Notification.png', sha256: 'f5f6b67a5367e8444d01e524f2305b51bb4854ef79f445112901b64216fb2d9c', width: 50, height: 50, bytes: 1574 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Help.png', resource: 'Actions/Help.png', sha256: '2642a05b30554798bd9d393b1faa03037832fdd6b00e31ad62e7473ff995e815', width: 50, height: 50, bytes: 1675 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Connect.png', resource: 'Actions/Connect.png', sha256: '72bb30c4724f56d4b3186a99d7bac8168e5acad27ed86c209adca64e4fd1b2d8', width: 50, height: 50, bytes: 2226 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/Disconnect.png', resource: 'Actions/Disconnect.png', sha256: 'cbfb58835406b014e800f3f6006b3d3ccf539242911b2c30a590627dac886313', width: 50, height: 50, bytes: 2961 },
  { reference: '../SandboxiePlus/SandMan/Resources/Actions/clean.png', resource: 'Actions/clean.png', sha256: '56c26d7f30933b6c8b332a2bde17b3169928c9ef076f20b1d9a8a1ddfd888262', width: 50, height: 50, bytes: 1332 },
]);

const utf8Decoder = new TextDecoder('utf-8', { fatal: true });
const slugPattern = /^[a-z0-9]+(?:-[a-z0-9]+)*$/;
const maximumChangelogBytes = 512 * 1024;
const maximumChangelogEntries = 512;

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
  assert(stat.nlink === 1, `${label} must have exactly one hard link; found ${stat.nlink}: ${absoluteCandidate}`);
  const realBase = fs.realpathSync.native(absoluteBase);
  const realCandidate = fs.realpathSync.native(absoluteCandidate);
  assert(isWithin(realBase, realCandidate), `${label} resolves outside ${realBase}: ${realCandidate}`);
  return {
    absolute: absoluteCandidate,
    real: realCandidate,
    identity: `${stat.dev}:${stat.ino}`,
    linkCount: stat.nlink,
    bytes: fs.readFileSync(absoluteCandidate),
  };
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

export function canonicalHeadingSlug(text) {
  const slug = text.normalize('NFKD').toLocaleLowerCase('en-US')
    .replace(/[^\p{L}\p{N}]+/gu, '-')
    .replace(/^-+|-+$/g, '');
  return slug || 'section';
}

function renderedHeadingText(markdownHeading) {
  return markdownHeading
    .replace(/!\[([^\]]*)\]\([^)]*\)/g, '$1')
    .replace(/\[([^\]]+)\]\([^)]*\)/g, '$1')
    .replace(/\[([^\]]+)\]\[[^\]]*\]/g, '$1')
    .replace(/\[([^\]]+)\]/g, '$1')
    .replace(/`+([^`]*)`+/g, '$1')
    .replace(/[*_~]/g, '')
    .trim();
}

export function buildHeadingSlugs(markdown) {
  const duplicateCounts = new Map();
  const headings = new Set();
  const masked = maskMarkdownCode(markdown, { maskInlineCode: false });
  for (const match of masked.matchAll(/^#{1,6}\s+(.+?)\s*#*\s*$/gm)) {
    const baseSlug = canonicalHeadingSlug(renderedHeadingText(unescapeMarkdown(match[1])));
    const duplicateIndex = duplicateCounts.get(baseSlug) ?? 0;
    duplicateCounts.set(baseSlug, duplicateIndex + 1);
    headings.add(duplicateIndex === 0 ? baseSlug : `${baseSlug}-${duplicateIndex}`);
  }
  return headings;
}

function assertSupportedHeadingSyntax(markdown, label) {
  const masked = maskMarkdownCode(markdown);
  assert(!/^[ \t]{0,3}>[ \t]*#{1,6}[ \t]+/m.test(masked),
    `${label} uses a blockquote heading that the canonical anchor contract deliberately rejects`);
  assert(!/^(?![ \t]*$).+\r?\n[ \t]{0,3}(?:=+|-+)[ \t]*$/m.test(masked),
    `${label} uses a Setext heading that the canonical anchor contract deliberately rejects`);
}

function extractCanonicalAtxTitle(markdown, label) {
  const masked = maskMarkdownCode(markdown, { maskInlineCode: false });
  const headings = [...masked.matchAll(/^#\s+(.+?)\s*#*\s*$/gm)];
  assert(headings.length === 1, `${label} must contain exactly one rendered ATX H1 heading; found ${headings.length}`);
  return renderedHeadingText(unescapeMarkdown(headings[0][1]));
}

function assertTrailingSuggestedArticles(markdown, article, currentSource, knownSources,
    headingSlugsBySource, featureSlugBySource) {
  const slug = article.slug;
  const trimmed = markdown.trimEnd();
  const masked = maskMarkdownCode(trimmed);
  const markers = [...masked.matchAll(/^(?:## Suggested articles[ \t]*|Suggested articles:.*)$/gmi)];
  const marker = markers.at(-1);
  const tail = marker ? trimmed.slice(marker.index) : '';
  assert(tail.length > 0, `feature article ${slug} must end with a Suggested articles section`);
  const maskedTail = marker ? masked.slice(marker.index) : '';
  const afterMarker = maskedTail.slice(maskedTail.indexOf('\n') + 1);
  assert(!/^#{1,6}[ \t]+/m.test(afterMarker),
    `feature article ${slug} must keep Suggested articles as its final section`);
  const links = parseMarkdownLinks(tail);
  assert(links.length > 0, `feature article ${slug} Suggested articles section must contain a rendered link`);
  const bundledNonSelfFeatures = links.filter(href => {
    const resolved = resolveInternalDocumentationLink(currentSource, href, knownSources, headingSlugsBySource);
    if (resolved.external) return false;
    const suggestedSlug = featureSlugBySource.get(resolved.source);
    return Boolean(suggestedSlug && suggestedSlug !== slug);
  });
  assert(bundledNonSelfFeatures.length > 0,
    `feature article ${slug} Suggested articles must link to at least one bundled non-self feature article`);
}

export function resolveInternalDocumentationLink(currentSource, href, knownSources, headingSlugsBySource = new Map()) {
  assert(typeof href === 'string' && href.length > 0, 'documentation link destination is empty');
  assert(countOccurrences(href, '#') <= 1, `documentation link contains more than one fragment delimiter: ${href}`);
  const validateFragment = (source, fragment) => {
    if (!fragment) return;
    const headings = headingSlugsBySource.get(source);
    assert(headings?.has(fragment), `documentation fragment does not match a canonical heading: ${source}#${fragment}`);
  };
  if (href.startsWith('#')) {
    const fragment = href.slice(1);
    validateFragment(currentSource, fragment);
    return { source: currentSource, fragment };
  }
  const scheme = href.match(/^([A-Za-z][A-Za-z0-9+.-]*):/);
  if (scheme) {
    assert(scheme[1].toLowerCase() === 'https', `unsupported external documentation scheme: ${scheme[1]}`);
    assert(/^https:\/\//i.test(href), `external documentation HTTPS link must include // and a host: ${href}`);
    let externalUrl;
    try {
      externalUrl = new URL(href);
    } catch {
      throw new Error(`external documentation HTTPS link is invalid: ${href}`);
    }
    assert(externalUrl.protocol === 'https:' && externalUrl.hostname.length > 0,
      `external documentation HTTPS link must have a valid host: ${href}`);
    assert(externalUrl.username.length === 0 && externalUrl.password.length === 0,
      `external documentation HTTPS link must not contain embedded credentials: ${href}`);
    return { external: href };
  }
  const [withoutFragment, fragment = ''] = href.split('#', 2);
  const withoutQuery = withoutFragment.split('?', 1)[0];
  assert(!withoutQuery.includes('\\') && !path.posix.isAbsolute(withoutQuery), `unsafe local documentation link: ${href}`);
  const target = path.posix.normalize(path.posix.join(path.posix.dirname(currentSource), withoutQuery));
  assert(target !== '..' && !target.startsWith('../'), `local documentation link escapes docs: ${href}`);
  assert(knownSources.has(target), `local documentation destination is not bundled: ${currentSource} -> ${href} (${target})`);
  assert(currentSource !== 'changelog.md' && target !== 'changelog.md',
    `local documentation links between feature articles and changelog are not supported: ${currentSource} -> ${target}`);
  validateFragment(target, fragment);
  return { source: target, fragment };
}

function parseMarkdownLinks(markdown) {
  return parseMarkdownDestinations(markdown).links.map(link => link.destination);
}

function maskMarkdownCode(markdown, { maskInlineCode = true } = {}) {
  const mask = value => value.replace(/[^\r\n]/g, ' ');
  const lines = markdown.match(/[^\r\n]*(?:\r\n|\r|\n|$)/g) ?? [];
  let fenceCharacter = '';
  let fenceLength = 0;
  let fenceBaseIndent = 0;
  const listContainers = [];
  const indentation = value => {
    let columns = 0;
    let characters = 0;
    for (const character of value) {
      if (character === ' ') columns += 1;
      else if (character === '\t') columns += 4 - (columns % 4);
      else break;
      characters += 1;
    }
    return { columns, characters };
  };
  let masked = '';
  for (const line of lines) {
    const body = line.replace(/(?:\r\n|\r|\n)$/, '');
    const leading = indentation(body);
    if (fenceCharacter) {
      const relativeIndent = leading.columns - fenceBaseIndent;
      const closing = relativeIndent >= 0 && relativeIndent <= 3
        && new RegExp(`^${fenceCharacter}{${fenceLength},}[ \\t]*$`).test(body.slice(leading.characters));
      masked += mask(line);
      if (closing) {
        fenceCharacter = '';
        fenceLength = 0;
        fenceBaseIndent = 0;
      }
      continue;
    }

    if (body.trim().length === 0) {
      masked += line;
      continue;
    }

    const marker = /^(?:[-+*]|\d+[.)])([ \t]+)/.exec(body.slice(leading.characters));
    let listMarker = false;
    if (marker) {
      const parent = [...listContainers].reverse().find(container => leading.columns >= container.contentIndent
        && leading.columns - container.contentIndent <= 3);
      const validMarker = parent ? true : leading.columns <= 3;
      if (validMarker) {
        while (listContainers.length > 0 && listContainers.at(-1).markerIndent >= leading.columns)
          listContainers.pop();
        const markerWidth = marker[0].length - marker[1].length;
        const padding = indentation(marker[1]).columns;
        listContainers.push({
          markerIndent: leading.columns,
          contentIndent: leading.columns + markerWidth + Math.min(Math.max(padding, 1), 4),
        });
        listMarker = true;
      }
    }

    while (!listMarker && listContainers.length > 0 && leading.columns < listContainers.at(-1).contentIndent)
      listContainers.pop();
    const activeContainer = listContainers.at(-1);
    const baseIndent = !listMarker && activeContainer && leading.columns >= activeContainer.contentIndent
      ? activeContainer.contentIndent : 0;
    const relativeIndent = leading.columns - baseIndent;
    const content = body.slice(leading.characters);
    const openingCandidate = relativeIndent >= 0 && relativeIndent <= 3 ? /^(`{3,}|~{3,})(.*)$/.exec(content) : null;
    const opening = openingCandidate
      && !(openingCandidate[1][0] === '`' && openingCandidate[2].includes('`')) ? openingCandidate : null;
    if (!listMarker && opening) {
      fenceCharacter = opening[1][0];
      fenceLength = opening[1].length;
      fenceBaseIndent = baseIndent;
      masked += mask(line);
      continue;
    }
    if (!listMarker && relativeIndent >= 4) {
      masked += mask(line);
      continue;
    }
    masked += line;
  }

  if (!maskInlineCode) return masked;

  let cursor = 0;
  while (cursor < masked.length) {
    if (masked[cursor] !== '`') {
      cursor += 1;
      continue;
    }
    let runEnd = cursor + 1;
    while (masked[runEnd] === '`') runEnd += 1;
    const delimiter = masked.slice(cursor, runEnd);
    let closing = -1;
    let candidate = runEnd;
    while (candidate < masked.length) {
      candidate = masked.indexOf('`', candidate);
      if (candidate < 0) break;
      let candidateEnd = candidate + 1;
      while (masked[candidateEnd] === '`') candidateEnd += 1;
      if (candidateEnd - candidate === delimiter.length) {
        closing = candidate;
        break;
      }
      candidate = candidateEnd;
    }
    if (closing < 0) {
      cursor = runEnd;
      continue;
    }
    const end = closing + delimiter.length;
    masked = masked.slice(0, cursor) + mask(masked.slice(cursor, end)) + masked.slice(end);
    cursor = end;
  }
  return masked;
}

function normalizedReferenceLabel(label) {
  return unescapeMarkdown(label).trim().replace(/\s+/g, ' ').toLocaleLowerCase('en-US');
}

function unescapeMarkdown(value) {
  const escapable = new Set("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~".split(''));
  return value
    .replace(/\\(.)/gs, (match, character) => escapable.has(character) ? character : match)
    .replace(/&(?:#(\d+)|#x([0-9a-f]+)|(amp|lt|gt|quot|apos));/gi, (match, decimal, hexadecimal, named) => {
      if (decimal) return String.fromCodePoint(Number.parseInt(decimal, 10));
      if (hexadecimal) return String.fromCodePoint(Number.parseInt(hexadecimal, 16));
      return { amp: '&', lt: '<', gt: '>', quot: '"', apos: "'" }[named.toLowerCase()];
    });
}

function findClosingBracket(text, opening, limit = text.length) {
  let nested = 0;
  for (let index = opening + 1; index < limit; index += 1) {
    if (text[index] === '\\') {
      index += 1;
      continue;
    }
    if (text[index] === '[') nested += 1;
    else if (text[index] === ']') {
      if (nested === 0) return index;
      nested -= 1;
    }
  }
  return -1;
}

function skipMarkdownWhitespace(text, cursor, limit = text.length) {
  while (cursor < limit && /[ \t\r\n]/.test(text[cursor])) cursor += 1;
  return cursor;
}

function parseMarkdownDestination(text, cursor, limit, closingCharacter = '') {
  cursor = skipMarkdownWhitespace(text, cursor, limit);
  if (cursor >= limit) return null;
  if (text[cursor] === '<') {
    const start = cursor + 1;
    cursor = start;
    while (cursor < limit && text[cursor] !== '>' && text[cursor] !== '\r' && text[cursor] !== '\n') {
      if (text[cursor] === '\\') cursor += 1;
      cursor += 1;
    }
    if (cursor >= limit || text[cursor] !== '>') return null;
    const rawDestination = text.slice(start, cursor);
    const destination = unescapeMarkdown(rawDestination);
    cursor += 1;
    if (!closingCharacter) return { destination, rawDestination, end: cursor, angle: true };
    cursor = skipMarkdownWhitespace(text, cursor, limit);
    if (text[cursor] === closingCharacter) return { destination, rawDestination, end: cursor + 1, angle: true };
    const titleEnd = findMarkdownTitleEnd(text, cursor, limit);
    if (titleEnd < 0) return null;
    cursor = skipMarkdownWhitespace(text, titleEnd, limit);
    return text[cursor] === closingCharacter ? { destination, rawDestination, end: cursor + 1, angle: true } : null;
  }

  const start = cursor;
  let nestedParentheses = 0;
  while (cursor < limit) {
    const character = text[cursor];
    if (character === '\\') {
      cursor += 2;
      continue;
    }
    if (closingCharacter && character === '(') {
      nestedParentheses += 1;
      cursor += 1;
      continue;
    }
    if (closingCharacter && character === ')') {
      if (nestedParentheses > 0) {
        nestedParentheses -= 1;
        cursor += 1;
        continue;
      }
      if (closingCharacter === ')')
        return {
          destination: unescapeMarkdown(text.slice(start, cursor)),
          rawDestination: text.slice(start, cursor),
          end: cursor + 1,
          angle: false,
        };
    }
    if (/[ \t\r\n]/.test(character)) break;
    cursor += 1;
  }
  const rawDestination = text.slice(start, cursor);
  const destination = unescapeMarkdown(rawDestination);
  if (!closingCharacter) return destination ? { destination, rawDestination, end: cursor, angle: false } : null;
  cursor = skipMarkdownWhitespace(text, cursor, limit);
  if (text[cursor] === closingCharacter) return { destination, rawDestination, end: cursor + 1, angle: false };
  const titleEnd = findMarkdownTitleEnd(text, cursor, limit);
  if (titleEnd < 0) return null;
  cursor = skipMarkdownWhitespace(text, titleEnd, limit);
  return text[cursor] === closingCharacter ? { destination, rawDestination, end: cursor + 1, angle: false } : null;
}

function findMarkdownTitleEnd(text, cursor, limit) {
  const opener = text[cursor];
  const closer = opener === '(' ? ')' : opener;
  if (opener !== '"' && opener !== "'" && opener !== '(') return -1;
  for (let index = cursor + 1; index < limit; index += 1) {
    if (text[index] === '\\') {
      index += 1;
      continue;
    }
    if (text[index] === closer) return index + 1;
  }
  return -1;
}

function markdownContainerContentOffset(line) {
  let cursor = 0;
  const consumeUpToThreeSpaces = () => {
    let count = 0;
    while (count < 3 && line[cursor] === ' ') {
      cursor += 1;
      count += 1;
    }
  };
  consumeUpToThreeSpaces();
  while (cursor < line.length) {
    if (line[cursor] === '>') {
      cursor += 1;
      if (line[cursor] === ' ') cursor += 1;
      consumeUpToThreeSpaces();
      continue;
    }
    const marker = /^(?:[-+*]|\d+[.)])([ \t]+)/.exec(line.slice(cursor));
    if (marker) {
      cursor += marker[0].length;
      consumeUpToThreeSpaces();
      continue;
    }
    break;
  }
  return cursor;
}

function parseMarkdownDefinitions(masked) {
  const definitions = new Map();
  const definitionRanges = [];
  let lineStart = 0;
  while (lineStart < masked.length) {
    let lineEnd = masked.indexOf('\n', lineStart);
    if (lineEnd < 0) lineEnd = masked.length;
    const line = masked.slice(lineStart, lineEnd).replace(/\r$/, '');
    const contentOffset = markdownContainerContentOffset(line);
    const opening = lineStart + contentOffset;
    if (masked[opening] === '[') {
      const closing = findClosingBracket(masked, opening, lineStart + line.length);
      if (closing > opening && masked[closing + 1] === ':') {
        let parsed = parseMarkdownDestination(masked, closing + 2, lineStart + line.length);
        let definitionEnd = lineStart + line.length;
        let consumedLineEnd = lineEnd;
        if (!parsed && lineEnd < masked.length) {
          const continuationStart = lineEnd + 1;
          let continuationEnd = masked.indexOf('\n', continuationStart);
          if (continuationEnd < 0) continuationEnd = masked.length;
          const continuationLine = masked.slice(continuationStart, continuationEnd).replace(/\r$/, '');
          const continuationOffset = markdownContainerContentOffset(continuationLine);
          if (/\S/.test(continuationLine.slice(continuationOffset))) {
            parsed = parseMarkdownDestination(masked,
              continuationStart + continuationOffset,
              continuationStart + continuationLine.length);
            if (parsed) {
              definitionEnd = continuationStart + continuationLine.length;
              consumedLineEnd = continuationEnd;
            }
          }
        }
        if (parsed) {
          const label = normalizedReferenceLabel(masked.slice(opening + 1, closing));
          assert(!definitions.has(label), `Markdown reference definition is duplicated: ${label}`);
          definitions.set(label, parsed);
          definitionRanges.push({ start: opening, end: definitionEnd });
          lineEnd = consumedLineEnd;
        }
      }
    }
    lineStart = lineEnd + 1;
  }
  return { definitions, definitionRanges };
}

function parseMarkdownDestinations(markdown) {
  const masked = maskMarkdownCode(markdown);
  const { definitions, definitionRanges } = parseMarkdownDefinitions(masked);
  const links = [];
  const images = [];
  const occupiedLinks = [];
  const overlaps = (ranges, start, end) => ranges.some(range => start < range.end && end > range.start);
  const isEscaped = index => {
    let backslashes = 0;
    for (let cursor = index - 1; cursor >= 0 && masked[cursor] === '\\'; cursor -= 1)
      backslashes += 1;
    return backslashes % 2 === 1;
  };
  const record = (collection, destination, rawDestination, start, end, syntax) => {
    collection.push({ destination, rawDestination, index: start, end, syntax });
  };
  for (const image of [true, false]) {
    for (let cursor = 0; cursor < masked.length; cursor += 1) {
      if (!image && occupiedLinks.some(range => cursor >= range.start && cursor < range.end))
        continue;
      const opening = image ? cursor + 1 : cursor;
      if (image) {
        if (masked[cursor] !== '!' || masked[opening] !== '[' || isEscaped(cursor)) continue;
      } else if (masked[opening] !== '[' || (opening > 0 && masked[opening - 1] === '!')
          || isEscaped(opening)) continue;
      if (definitionRanges.some(range => opening >= range.start && opening < range.end)) continue;
      const closing = findClosingBracket(masked, opening);
      if (closing < 0) continue;
      let end = closing + 1;
      let destination = '';
      let rawDestination = '';
      let syntax = '';
      if (masked[end] === '(') {
        const parsed = parseMarkdownDestination(masked, end + 1, masked.length, ')');
        if (!parsed) continue;
        destination = parsed.destination;
        rawDestination = parsed.rawDestination;
        end = parsed.end;
        const labelUsesEscapes = masked.slice(opening + 1, closing).includes('\\');
        syntax = parsed.angle ? 'angle-inline' : labelUsesEscapes ? 'escaped-inline' : 'inline';
      } else if (masked[end] === '[') {
        const labelEnd = findClosingBracket(masked, end);
        if (labelEnd < 0) continue;
        const explicitLabel = masked.slice(end + 1, labelEnd);
        const label = normalizedReferenceLabel(explicitLabel || masked.slice(opening + 1, closing));
        if (!definitions.has(label)) continue;
        const definition = definitions.get(label);
        destination = definition.destination;
        rawDestination = definition.rawDestination;
        end = labelEnd + 1;
        syntax = explicitLabel ? 'full-reference' : 'collapsed-reference';
      } else {
        const label = normalizedReferenceLabel(masked.slice(opening + 1, closing));
        if (!definitions.has(label)) continue;
        const definition = definitions.get(label);
        destination = definition.destination;
        rawDestination = definition.rawDestination;
        syntax = 'shortcut-reference';
      }
      record(image ? images : links, destination, rawDestination, cursor, end, syntax);
      occupiedLinks.push({ start: cursor, end });
    }
  }

  const angleAutolink = /<(?:([A-Za-z][A-Za-z0-9+.-]*:[^<>\s]+)|([^<>\s@]+@[^<>\s@]+))>/g;
  for (const match of masked.matchAll(angleAutolink)) {
    const start = match.index;
    const end = start + match[0].length;
    if (overlaps(occupiedLinks, start, end)) continue;
    const destination = match[1] ?? `mailto:${match[2]}`;
    record(links, destination, destination, start, end, 'angle-autolink');
    occupiedLinks.push({ start, end });
  }
  const extendedAutolink = /(?:https?:\/\/|ftp:\/\/|www\.)[^\s<>]+/gi;
  for (const match of masked.matchAll(extendedAutolink)) {
    const start = match.index;
    const end = start + match[0].length;
    if (overlaps(occupiedLinks, start, end)) continue;
    const destination = match[0].startsWith('www.') ? `http://${match[0]}` : match[0];
    const trimmedDestination = destination.replace(/[.,;:!?]+$/, '');
    record(links, trimmedDestination, trimmedDestination, start, end, 'extended-autolink');
    occupiedLinks.push({ start, end });
  }
  const bareEmailAutolink = /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b/g;
  for (const match of masked.matchAll(bareEmailAutolink)) {
    const start = match.index;
    const end = start + match[0].length;
    if (overlaps(occupiedLinks, start, end)) continue;
    const destination = `mailto:${match[0]}`;
    record(links, destination, destination, start, end, 'bare-email-autolink');
    occupiedLinks.push({ start, end });
  }
  return {
    links: links.sort((left, right) => left.index - right.index),
    images: images.sort((left, right) => left.index - right.index),
  };
}

function parseMarkdownImages(markdown) {
  return parseMarkdownDestinations(markdown).images;
}

function assertNoRenderedMarkdownImages(markdown, label) {
  const images = parseMarkdownImages(markdown);
  assert(images.length === 0,
    `${label} must not render Markdown images; found ${images[0]?.syntax ?? 'unknown'} image destination ${images[0]?.destination ?? '<unknown>'}`);
}

function assertSafeSupplementalImageDestination(destination, label) {
  assert(typeof destination === 'string' && destination.length > 0, `${label} image destination is empty`);
  assert(!destination.includes('\\') && !destination.includes('\0'), `${label} image destination contains unsafe path characters: ${destination}`);
  assert(!destination.startsWith('//') && !path.posix.isAbsolute(destination), `${label} image destination must be a relative bundled path: ${destination}`);
  const scheme = destination.match(/^([A-Za-z][A-Za-z0-9+.-]*):/);
  assert(!scheme, `${label} image destination uses a forbidden scheme: ${scheme?.[1] ?? ''}`);
}

function assertValidBundledPng(bytes, label) {
  const signature = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
  assert(bytes.length >= 33 && bytes.length <= 32 * 1024 * 1024,
    `${label} PNG size must be between 33 bytes and 32 MiB; found ${bytes.length}`);
  assert(bytes.subarray(0, signature.length).equals(signature), `${label} has an invalid PNG signature`);
  assert(bytes.readUInt32BE(8) === 13 && bytes.subarray(12, 16).toString('ascii') === 'IHDR',
    `${label} must start with a 13-byte IHDR chunk`);
  const width = bytes.readUInt32BE(16);
  const height = bytes.readUInt32BE(20);
  assert(width > 0 && height > 0 && width <= 16384 && height <= 16384 && width * height <= 100_000_000,
    `${label} PNG dimensions are invalid or exceed the 100-megapixel bound: ${width}x${height}`);
  return { width, height };
}

function decodeXmlValue(value, label) {
  const decoded = value.replace(/&(amp|lt|gt|quot|apos);/g, entity => ({
    '&amp;': '&', '&lt;': '<', '&gt;': '>', '&quot;': '"', '&apos;': "'",
  })[entity]);
  assert(!/&(?:#\d+|#x[0-9a-f]+|[A-Za-z_:][\w:.-]*);/i.test(decoded), `${label} contains an unsupported XML entity`);
  return decoded;
}

function scanXmlMarkupEnd(xml, start, declaration = false) {
  let quote = '';
  let bracketDepth = 0;
  for (let index = start; index < xml.length; index += 1) {
    const character = xml[index];
    if (quote) {
      if (character === quote) quote = '';
      continue;
    }
    if (character === '"' || character === "'") {
      quote = character;
      continue;
    }
    if (declaration && character === '[') bracketDepth += 1;
    else if (declaration && character === ']' && bracketDepth > 0) bracketDepth -= 1;
    else if (character === '>' && bracketDepth === 0) return index;
  }
  throw new Error('Qt resource XML has an unterminated declaration or tag');
}

function parseXmlAttributes(text, label) {
  const attributes = new Map();
  let cursor = 0;
  while (cursor < text.length) {
    while (/\s/.test(text[cursor] ?? '')) cursor += 1;
    if (cursor >= text.length) break;
    const nameMatch = /^[A-Za-z_:][\w:.-]*/.exec(text.slice(cursor));
    assert(nameMatch, `${label} contains an invalid attribute`);
    const name = nameMatch[0];
    cursor += name.length;
    while (/\s/.test(text[cursor] ?? '')) cursor += 1;
    assert(text[cursor] === '=', `${label} attribute ${name} is missing =`);
    cursor += 1;
    while (/\s/.test(text[cursor] ?? '')) cursor += 1;
    const quote = text[cursor];
    assert(quote === '"' || quote === "'", `${label} attribute ${name} must be quoted`);
    const end = text.indexOf(quote, cursor + 1);
    assert(end >= 0, `${label} attribute ${name} is unterminated`);
    assert(!attributes.has(name), `${label} repeats attribute ${name}`);
    attributes.set(name, decodeXmlValue(text.slice(cursor + 1, end), `${label} attribute ${name}`));
    cursor = end + 1;
  }
  return attributes;
}

function parseQrcFiles(qrc) {
  const files = [];
  const stack = [];
  let rootCount = 0;
  let cursor = 0;
  const appendText = text => {
    if (stack.length > 0) stack.at(-1).text += text;
  };
  while (cursor < qrc.length) {
    const opening = qrc.indexOf('<', cursor);
    if (opening < 0) {
      appendText(qrc.slice(cursor));
      break;
    }
    appendText(qrc.slice(cursor, opening));
    if (qrc.startsWith('<!--', opening)) {
      const end = qrc.indexOf('-->', opening + 4);
      assert(end >= 0, 'Qt resource XML has an unterminated comment');
      cursor = end + 3;
      continue;
    }
    if (qrc.startsWith('<?', opening)) {
      const end = qrc.indexOf('?>', opening + 2);
      assert(end >= 0, 'Qt resource XML has an unterminated processing instruction');
      cursor = end + 2;
      continue;
    }
    if (qrc.startsWith('<![CDATA[', opening)) {
      const end = qrc.indexOf(']]>', opening + 9);
      assert(end >= 0, 'Qt resource XML has an unterminated CDATA section');
      cursor = end + 3;
      continue;
    }
    if (qrc.startsWith('<!', opening)) {
      cursor = scanXmlMarkupEnd(qrc, opening + 2, true) + 1;
      continue;
    }

    const end = scanXmlMarkupEnd(qrc, opening + 1);
    let markup = qrc.slice(opening + 1, end).trim();
    if (markup.startsWith('/')) {
      const closingMatch = /^\/\s*([A-Za-z_:][\w:.-]*)\s*$/.exec(markup);
      assert(closingMatch, 'Qt resource XML has an invalid closing tag');
      const node = stack.pop();
      assert(node?.name === closingMatch[1], `Qt resource XML closes ${closingMatch[1]} out of order`);
      if (node.name === 'file') {
        assert(!node.hasElementChild, 'Qt resource file entries must contain text only');
        const qresource = [...stack].reverse().find(parent => parent.name === 'qresource');
        assert(qresource, 'Qt resource file entry is not enclosed by qresource');
        files.push({
          alias: node.attributes.get('alias') ?? '',
          source: decodeXmlValue(node.text.trim(), 'Qt resource file source'),
          attributes: node.attributes,
          qresourceAttributes: qresource.attributes,
        });
      }
      cursor = end + 1;
      continue;
    }

    const selfClosing = /\/\s*$/.test(markup);
    if (selfClosing) markup = markup.replace(/\/\s*$/, '').trimEnd();
    const nameMatch = /^([A-Za-z_:][\w:.-]*)/.exec(markup);
    assert(nameMatch, 'Qt resource XML has an invalid opening tag');
    const name = nameMatch[1];
    const attributes = parseXmlAttributes(markup.slice(name.length), `Qt resource ${name} tag`);
    const parent = stack.at(-1)?.name ?? '';
    if (name === 'RCC') {
      assert(parent === '' && !selfClosing && attributes.size === 0,
        'Qt resource XML must contain one attribute-free RCC root element');
      rootCount += 1;
      assert(rootCount === 1, 'Qt resource XML must contain exactly one RCC root element');
    } else if (name === 'qresource') {
      assert(parent === 'RCC' && !selfClosing,
        'Qt resource XML allows qresource only as a direct RCC child');
    } else if (name === 'file') {
      assert(parent === 'qresource' && !selfClosing,
        'Qt resource XML allows file only as a direct qresource child');
    } else {
      assert(false, `Qt resource XML contains unsupported element ${name}`);
    }
    if (stack.length > 0) stack.at(-1).hasElementChild = true;
    if (!selfClosing) stack.push({ name, attributes, text: '', hasElementChild: false });
    cursor = end + 1;
  }
  assert(stack.length === 0, `Qt resource XML has an unclosed ${stack.at(-1)?.name ?? 'tag'}`);
  assert(rootCount === 1, `Qt resource XML must contain exactly one RCC root element; found ${rootCount}`);
  return files;
}

function stripQmakeComment(line) {
  let quote = '';
  let escaped = false;
  for (let index = 0; index < line.length; index += 1) {
    const character = line[index];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (character === '\\') {
      escaped = true;
      continue;
    }
    if (quote) {
      if (character === quote) quote = '';
      continue;
    }
    if (character === '"' || character === "'") {
      quote = character;
      continue;
    }
    if (character === '#') return line.slice(0, index);
  }
  return line;
}

function activeQmakeText(text) {
  const statements = [];
  let pending = '';
  for (const physicalLine of text.split(/\r?\n/)) {
    let line = stripQmakeComment(physicalLine).trimEnd();
    let trailingBackslashes = 0;
    for (let index = line.length - 1; index >= 0 && line[index] === '\\'; index -= 1)
      trailingBackslashes += 1;
    const continues = trailingBackslashes % 2 === 1;
    if (continues) line = line.slice(0, -1).trimEnd();
    pending += `${pending && line ? ' ' : ''}${line.trimStart()}`;
    if (!continues) {
      if (pending.trim()) statements.push(pending.trim());
      pending = '';
    }
  }
  assert(!pending, 'SandMan.pri ends with an unterminated qmake continuation');
  return statements.join('\n');
}

function parseActiveXmlElements(xml, label) {
  const elements = [];
  const stack = [];
  let cursor = 0;
  while (cursor < xml.length) {
    const opening = xml.indexOf('<', cursor);
    if (opening < 0) break;
    if (xml.startsWith('<!--', opening)) {
      const end = xml.indexOf('-->', opening + 4);
      assert(end >= 0, `${label} has an unterminated XML comment`);
      cursor = end + 3;
      continue;
    }
    if (xml.startsWith('<?', opening)) {
      const end = xml.indexOf('?>', opening + 2);
      assert(end >= 0, `${label} has an unterminated processing instruction`);
      cursor = end + 2;
      continue;
    }
    if (xml.startsWith('<![CDATA[', opening)) {
      const end = xml.indexOf(']]>', opening + 9);
      assert(end >= 0, `${label} has an unterminated CDATA section`);
      cursor = end + 3;
      continue;
    }
    if (xml.startsWith('<!', opening)) {
      cursor = scanXmlMarkupEnd(xml, opening + 2, true) + 1;
      continue;
    }

    const end = scanXmlMarkupEnd(xml, opening + 1);
    let markup = xml.slice(opening + 1, end).trim();
    if (markup.startsWith('/')) {
      const closingMatch = /^\/\s*([A-Za-z_:][\w:.-]*)\s*$/.exec(markup);
      assert(closingMatch, `${label} has an invalid closing element`);
      assert(stack.pop()?.name === closingMatch[1], `${label} closes ${closingMatch[1]} out of order`);
      cursor = end + 1;
      continue;
    }
    const selfClosing = /\/\s*$/.test(markup);
    if (selfClosing) markup = markup.replace(/\/\s*$/, '').trimEnd();
    const nameMatch = /^([A-Za-z_:][\w:.-]*)/.exec(markup);
    assert(nameMatch, `${label} has an invalid opening element`);
    const name = nameMatch[1];
    const attributes = parseXmlAttributes(markup.slice(name.length), `${label} ${name} element`);
    const element = { name, attributes, ancestors: [...stack] };
    elements.push(element);
    if (!selfClosing) stack.push(element);
    cursor = end + 1;
  }
  assert(stack.length === 0, `${label} has an unclosed ${stack.at(-1) ?? 'element'}`);
  return elements;
}

function assertQmakeProjectRegistration(text, token, label) {
  const [variable, value] = token;
  const assignments = activeQmakeText(text).split('\n').map(statement => {
    const match = /^([A-Za-z_][A-Za-z0-9_.]*)\s*(\+?=)\s*(.*)$/.exec(statement);
    return match ? { variable: match[1], operator: match[2], values: match[3].split(/\s+/).filter(Boolean) } : null;
  }).filter(Boolean);
  const count = assignments.filter(assignment => assignment.variable === variable)
    .reduce((total, assignment) => total + assignment.values.filter(entry => entry === value).length, 0);
  assert(count === 1, `${label} ${variable} must register ${value} exactly once; found ${count}`);
}

function assertXmlProjectRegistration(elements, elementName, include, label) {
  const count = elements.filter(element => element.name === elementName
    && element.attributes.size === 1 && element.attributes.get('Include') === include
    && element.ancestors.every(ancestor => !ancestor.attributes.has('Condition'))).length;
  assert(count === 1, `${label} must register <${elementName} Include="${include}" /> exactly once; found ${count}`);
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
  const headingSlugsBySource = new Map();
  for (const record of allRecords) {
    const source = normalizeManifestSource(record, `manifest record ${record.slug}`);
    const sourceFile = assertContainedRegularFile(docsRoot, path.join(docsRoot, ...source.split('/')), `documentation source ${record.slug}`);
    assert(sourceFile.bytes.length <= 512 * 1024, `documentation source exceeds the 512 KiB search bound: ${record.slug}`);
    assert(!realSources.has(sourceFile.real), `documentation records resolve to the same file: ${record.slug}`);
    realSources.add(sourceFile.real);
    knownSources.add(source);
    const markdown = decodeUtf8(sourceFile.bytes, `documentation source ${record.slug}`);
    canonicalFiles.set(record.slug, { ...sourceFile, source, record, markdown });
    assertSupportedHeadingSyntax(markdown, `documentation source ${record.slug}`);
    const title = extractCanonicalAtxTitle(markdown, `documentation source ${record.slug}`);
    assert(title === record.title, `documentation title drift for ${record.slug}; manifest=${record.title} markdown=${title}`);
    headingSlugsBySource.set(source, buildHeadingSlugs(markdown));
  }

  let suggestedArticleCount = 0;
  const featureSlugBySource = new Map(manifest.articles.map(article => [
    normalizeManifestSource(article, `manifest article ${article.slug}`), article.slug,
  ]));
  for (const article of manifest.articles) {
    assert(Array.isArray(article.related) && article.related.length > 0, `article ${article.slug} must name related articles`);
    assert(new Set(article.related).size === article.related.length, `article ${article.slug} repeats a related slug`);
    for (const related of article.related) {
      assert(related !== article.slug && canonicalSlugs.has(related), `article ${article.slug} has unknown or self-related slug ${related}`);
    }
    const file = canonicalFiles.get(article.slug);
    assertTrailingSuggestedArticles(file.markdown, article, file.source, knownSources,
      headingSlugsBySource, featureSlugBySource);
    suggestedArticleCount += 1;
  }
  assert(suggestedArticleCount === canonicalArticles.length,
    `Suggested articles coverage must include all ${canonicalArticles.length} feature articles`);

  assert(manifest.changelog?.path === '../changelog.md', 'manifest changelog path must be ../changelog.md');
  assert(/^[0-9a-f]{40}$/.test(manifest.changelog.commit), 'changelog manifest commit must be a full lowercase SHA');
  const changelogSource = normalizeManifestSource({ path: manifest.changelog.path }, 'manifest changelog');
  const changelogFile = assertContainedRegularFile(docsRoot, path.join(docsRoot, ...changelogSource.split('/')), 'documentation changelog');
  assert(changelogFile.bytes.length <= maximumChangelogBytes,
    `documentation changelog exceeds the ${maximumChangelogBytes}-byte offline limit: ${changelogFile.bytes.length}`);
  assert(!realSources.has(changelogFile.real), 'changelog must be distinct from every article source');
  knownSources.add(changelogSource);
  const changelog = decodeUtf8(changelogFile.bytes, 'documentation changelog');
  const changelogSections = changelog.split(/(?=^## )/m).filter(Boolean);
  const changelogEntries = changelogSections.filter(section => section.startsWith('## '));
  assert(changelogSections.length <= maximumChangelogEntries + 1 && changelogEntries.length <= maximumChangelogEntries,
    `documentation changelog exceeds the ${maximumChangelogEntries}-entry offline limit: ${changelogEntries.length}`);
  assertSupportedHeadingSyntax(changelog, 'documentation changelog');
  assertNoRenderedMarkdownImages(changelog, 'documentation changelog');
  headingSlugsBySource.set(changelogSource, buildHeadingSlugs(changelog));
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
  for (const article of allRecords) {
    const source = canonicalFiles.get(article.slug).source;
    const markdown = canonicalFiles.get(article.slug).markdown;
    if (!article.supplemental && canonicalSlugs.has(article.slug))
      assertNoRenderedMarkdownImages(markdown, `feature article ${article.slug}`);
    for (const href of parseMarkdownLinks(markdown)) {
      const resolution = resolveInternalDocumentationLink(source, href, knownSources, headingSlugsBySource);
      if (resolution.external) externalLinkCount += 1;
      else internalLinkCount += 1;
    }
  }
  for (const href of parseMarkdownLinks(changelog)) {
    const resolution = resolveInternalDocumentationLink(changelogSource, href, knownSources, headingSlugsBySource);
    if (resolution.external) externalLinkCount += 1;
    else internalLinkCount += 1;
  }
  const samePageFragment = resolveInternalDocumentationLink('features/pages-a11y-boundary.md',
    '#verification', knownSources, headingSlugsBySource);
  assert(samePageFragment.source === 'features/pages-a11y-boundary.md'
    && samePageFragment.fragment === 'verification', 'same-page fragment routing contract failed');
  const crossPageFragment = resolveInternalDocumentationLink('features/pages-a11y-boundary.md',
    '../material-design.md#failure-modes-and-security', knownSources, headingSlugsBySource);
  assert(crossPageFragment.source === 'material-design.md'
    && crossPageFragment.fragment === 'failure-modes-and-security', 'cross-article fragment routing contract failed');
  const duplicateHeadingProof = buildHeadingSlugs('## Repeated heading\n\n## Repeated heading\n\n## Repeated heading\n');
  assertSameSet([...duplicateHeadingProof], ['repeated-heading', 'repeated-heading-1', 'repeated-heading-2'], 'duplicate heading slug contract');

  const screenshotsMarkdown = canonicalFiles.get('screenshots').markdown;
  const screenshotImages = parseMarkdownImages(screenshotsMarkdown);
  const screenshotReferences = screenshotImages.map(image => image.destination);
  assert(screenshotReferences.length === canonicalScreenshotAssets.length,
    `screenshots supplement must reference exactly ${canonicalScreenshotAssets.length} images; found ${screenshotReferences.length}`);
  for (let index = 0; index < canonicalScreenshotAssets.length; index += 1) {
    assert(screenshotImages[index].syntax === 'inline',
      `screenshot image[${index}] must use exact inline, non-angle syntax; found ${screenshotImages[index].syntax}`);
    assertSafeSupplementalImageDestination(screenshotReferences[index], `screenshots image[${index}]`);
    assert(screenshotImages[index].rawDestination === canonicalScreenshotAssets[index].reference,
      `screenshot image[${index}] destination spelling must be exactly ${canonicalScreenshotAssets[index].reference}; found ${screenshotImages[index].rawDestination}`);
    assert(screenshotReferences[index] === canonicalScreenshotAssets[index].reference,
      `screenshot image[${index}] must be ${canonicalScreenshotAssets[index].reference}; found ${screenshotReferences[index]}`);
  }
  let screenshotAssets = canonicalScreenshotAssets.map(asset => {
    assert(!asset.reference.includes('\\') && !path.posix.isAbsolute(asset.reference),
      `screenshot reference must be a safe relative path: ${asset.reference}`);
    const file = assertContainedRegularFile(repositoryRoot, path.resolve(docsRoot, asset.reference),
      `screenshot asset ${asset.reference}`);
    return { ...file, reference: asset.reference, resource: asset.resource, expected: asset };
  });
  assert(new Set(screenshotAssets.map(asset => asset.identity)).size === screenshotAssets.length,
    'screenshot assets must not be hard-linked to one another');
  screenshotAssets = screenshotAssets.map(asset => {
    const dimensions = assertValidBundledPng(asset.bytes, `screenshot asset ${asset.reference}`);
    const sha256 = createHash('sha256').update(asset.bytes).digest('hex');
    assert(asset.bytes.length === asset.expected.bytes,
      `screenshot asset ${asset.reference} byte length must be ${asset.expected.bytes}; found ${asset.bytes.length}`);
    assert(sha256 === asset.expected.sha256,
      `screenshot asset ${asset.reference} SHA-256 must be ${asset.expected.sha256}; found ${sha256}`);
    assert(dimensions.width === asset.expected.width && dimensions.height === asset.expected.height,
      `screenshot asset ${asset.reference} dimensions must be ${asset.expected.width}x${asset.expected.height}; found ${dimensions.width}x${dimensions.height}`);
    return { ...asset, ...dimensions, actualSha256: sha256 };
  });
  assert(new Set(screenshotAssets.map(asset => asset.real)).size === screenshotAssets.length,
    'screenshot assets must resolve to 24 distinct ordinary files');
  assert(new Set(screenshotAssets.map(asset => asset.actualSha256)).size === screenshotAssets.length,
    'screenshot assets must contain 24 distinct canonical PNG payloads');

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
    ...screenshotAssets.map(asset => ({
      alias: `Docs/assets/${asset.resource}`,
      source: asset.resource,
      real: asset.real,
      bytes: asset.bytes,
    })),
    { alias: 'Docs/changelog.md', source: '../../../docs/changelog.md', real: changelogFile.real, bytes: changelogFile.bytes },
  ];
  const actualQrc = parseQrcFiles(qrc).filter(record => record.alias.startsWith('Docs/'));
  assert(actualQrc.length === expectedQrc.length, `Qt documentation resource set must contain exactly ${expectedQrc.length} entries; found ${actualQrc.length}`);
  for (let index = 0; index < expectedQrc.length; index += 1) {
    const actual = actualQrc[index];
    const expected = expectedQrc[index];
    assert(actual.attributes.size === 1 && actual.attributes.has('alias'),
      `Qt documentation resource ${expected.alias} must use only its exact alias attribute`);
    assert(actual.qresourceAttributes.size === 1 && actual.qresourceAttributes.get('prefix') === '/',
      `Qt documentation resource ${expected.alias} must use exactly qresource prefix="/" with no language override`);
    assert(actual.alias === expected.alias, `Qt documentation resource alias[${index}] must be ${expected.alias}; found ${actual.alias}`);
    assert(actual.source === expected.source, `Qt documentation resource source for ${expected.alias} must be ${expected.source}; found ${actual.source}`);
    const bundled = assertContainedRegularFile(repositoryRoot, path.resolve(qrcDirectory, actual.source), `Qt documentation resource ${actual.alias}`);
    assert(bundled.real === expected.real, `Qt documentation resource ${actual.alias} does not resolve to its canonical source`);
    assert(bundled.bytes.equals(expected.bytes), `Qt documentation resource ${actual.alias} differs byte-for-byte from its canonical source`);
  }
  assert(new Set(actualQrc.map(record => record.alias)).size === actualQrc.length, 'Qt documentation resource aliases must be unique');

  const pri = decodeUtf8(assertContainedRegularFile(repositoryRoot, path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'SandMan.pri'), 'SandMan.pri').bytes, 'SandMan.pri');
  const vcxproj = decodeUtf8(assertContainedRegularFile(repositoryRoot, path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'SandMan.vcxproj'), 'SandMan.vcxproj').bytes, 'SandMan.vcxproj');
  for (const token of [
    ['SOURCES', './Windows/DocumentationBrowser.cpp'],
    ['HEADERS', './Windows/DocumentationBrowser.h'],
    ['RESOURCES', 'Resources/SandMan.qrc'],
  ]) assertQmakeProjectRegistration(pri, token, 'SandMan.pri');
  const projectElements = parseActiveXmlElements(vcxproj, 'SandMan.vcxproj');
  for (const [elementName, include] of [
    ['ClCompile', 'Windows\\DocumentationBrowser.cpp'],
    ['QtMoc', 'Windows\\DocumentationBrowser.h'],
    ['QtRcc', 'Resources\\SandMan.qrc'],
  ]) assertXmlProjectRegistration(projectElements, elementName, include, 'SandMan.vcxproj');

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
    'installHeadingAnchorsForDocument(m_view->document())',
    'anchorFormat.setAnchorNames(QStringList{slug})',
    'duplicateCounts.value(baseSlug, 0)',
    'anchors.contains(fragment)',
    'view->scrollToAnchor(fragment)',
    'rewriteSupplementalImageSources(article.body)',
    'qrc:/Docs/assets/',
    'class OfflineDocumentationView final : public QTextBrowser',
    'type != QTextDocument::ImageResource',
    'normalizedPath.startsWith(QStringLiteral("/Docs/assets/"))',
    'return QVariant()',
    'QTextDocument::MarkdownFeatures{QTextDocument::MarkdownDialectGitHub, QTextDocument::MarkdownNoHTML}',
    'QT_VERSION_CHECK(5, 14, 0)',
    'QRegularExpression::MultilineOption), Qt::SkipEmptyParts',
    'kMaximumChangelogBytes = 512 * 1024',
    'kMaximumChangelogEntries = 512',
    'changelogFile.size() > kMaximumChangelogBytes',
    'sections.size() > kMaximumChangelogEntries + 1',
    'setChangelogStatus(m_changelogLoadError)',
    'setChangelogStatus(tr("Changelog date filter is invalid"))',
    'setChangelogStatus(tr("%1 of %2 changelog entries match")',
    'setChangelogStatus(text)',
    'extractCanonicalAtxTitle(article.body, &canonicalTitle)',
    'const QString masked = maskMarkdownCode(markdown, false)',
    'activeRegexFlags(m_searchExpression',
    'activeRegexFlags(m_changelogExpression',
    'clearArticleSearchState()',
    'Unsupported flags. Use i for case-insensitive matching',
    'setAccessibleDescription(tr("Documentation status"))',
    'setAccessibleDescription(tr("Changelog status"))',
    'setAccessibleDescription(tr("Changelog date validation"))',
    'QAccessible::NameChanged',
    'updateAccessibleText(m_status, text)',
    'updateAccessibleText(m_changelogStatus, text)',
    'markdownToHtml(renderedBody)',
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
    'void CDocumentationBrowser::openChangelog()',
  ];
  for (const contract of browserContracts) assert(browserCpp.includes(contract), `DocumentationBrowser contract missing: ${contract}`);
  assert(countOccurrences(browserCpp, '&QTextBrowser::anchorClicked') === 2, 'both article and changelog views must route anchorClicked through the allowlist');
  assert(countOccurrences(browserCpp, 'supportedRegexOptions(flags->text(), &options)') === 2,
    'both regex builders must validate the exact supported flag set');
  assert(countOccurrences(browserCpp, 'clearArticleSearchState();') === 2,
    'both hidden-article navigation paths must clear regex search state before revealing the target');
  for (const forbidden of [
    'setOpenLinks(true)',
    'setOpenExternalLinks(true)',
    'scheme == QStringLiteral("http")',
    'const QStringList paths',
    'setAccessibleName(tr("Documentation status"))',
    'setAccessibleName(tr("Changelog status"))',
    'setAccessibleName(text)',
    'm_status->setText(',
    'm_changelogStatus->setText(',
    'm_changelogDateError->setText(',
    'feedback->setText(',
    'QAccessible::ValueChanged',
  ]) assert(!browserCpp.includes(forbidden), `DocumentationBrowser contains forbidden contract: ${forbidden}`);

  const sandManCppPath = path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'SandMan.cpp');
  const sandManCpp = decodeUtf8(assertContainedRegularFile(repositoryRoot, sandManCppPath, 'SandMan.cpp').bytes, 'SandMan.cpp');
  for (const paletteContract of [
    'static QRegularExpression BoundedPaletteExpression',
    '(*LIMIT_MATCH=100000)(*LIMIT_DEPTH=1000)',
    'QFile documentationManifest(QStringLiteral(":/Docs/articles/index.json"))',
    'inventory.value(QStringLiteral("articles")).toArray()',
    'inventory.value(QStringLiteral("supplemental")).toArray()',
    'article.value(QStringLiteral("slug")).toString()',
    'article.value(QStringLiteral("title")).toString()',
    'browser->openArticle(slug)',
    'Documentation · Changelog (changelog)',
    'browser->openChangelog()',
  ]) assert(sandManCpp.includes(paletteContract), `command palette documentation contract missing: ${paletteContract}`);
  assert(countOccurrences(sandManCpp, 'appendDocumentationCommands(inventory.value(') === 2,
    'command palette must index the exact feature and supplemental manifest inventories');
  assert(countOccurrences(sandManCpp, 'BoundedPaletteExpression(') === 3,
    'command palette validation and matching must both use the bounded expression helper');
  assert(!/const QRegularExpression expression\s*\(\s*(?:pattern|candidate)\s*,/.test(sandManCpp),
    'command palette must not synchronously construct an unbounded user expression');

  const browserHeaderPath = path.join(repositoryRoot, 'SandboxiePlus', 'SandMan', 'Windows', 'DocumentationBrowser.h');
  const browserHeader = decodeUtf8(assertContainedRegularFile(repositoryRoot, browserHeaderPath, 'DocumentationBrowser.h').bytes, 'DocumentationBrowser.h');
  assert(browserHeader.includes('void openChangelog();'), 'DocumentationBrowser must expose an exact Changelog teleport');

  return {
    articleCount: manifest.articles.length,
    supplementalCount: manifest.supplemental.length,
    screenshotAssetCount: screenshotAssets.length,
    suggestedArticleCount,
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
    console.log(`docs-valid articles=${result.articleCount} supplemental=${result.supplementalCount} suggested=${result.suggestedArticleCount} images=${result.screenshotAssetCount} internal-links=${result.internalLinkCount} external-links=${result.externalLinkCount} changelog=${result.changelogCommit}`);
  } catch (error) {
    console.error(`docs-invalid: ${error instanceof Error ? error.message : String(error)}`);
    process.exitCode = 1;
  }
}
