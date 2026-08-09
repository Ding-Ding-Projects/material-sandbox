import fs from 'node:fs';
import path from 'node:path';

const FEATURE_ARTICLE_COUNT = 22;
const MAX_MANIFEST_BYTES = 128 * 1024;
const MAX_MARKDOWN_BYTES = 1024 * 1024;
const MAX_SITE_TEMPLATE_BYTES = 2 * 1024 * 1024;
const MAX_EMBEDDED_ASSET_BYTES = 1024 * 1024;
const MAX_PAGE_ASSET_BYTES = 8 * 1024 * 1024;
const MAX_RELATED_ARTICLES = 12;
const RESERVED_SLUGS = new Set(['changelog', 'screenshots']);
const CANONICAL_FEATURE_ARTICLES = new Map([
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
]);

class BuildFailure extends Error {
  constructor(message, exitCode = 1) {
    super(message);
    this.exitCode = exitCode;
  }
}

function normalizedPath(value) {
  const resolved = path.resolve(value).replace(/^\\\\\?\\/, '');
  return process.platform === 'win32' ? resolved.toLowerCase() : resolved;
}

function pathsMatch(left, right) {
  return normalizedPath(left) === normalizedPath(right);
}

function isWithin(root, candidate, allowRoot = false) {
  const relative = path.relative(root, candidate);
  if (!relative) return allowRoot;
  return relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative);
}

function lstatRequired(candidate, label) {
  let stats;
  try {
    stats = fs.lstatSync(candidate);
  } catch (error) {
    throw new BuildFailure(`${label} is unavailable: ${error.message}`);
  }
  if (stats.isSymbolicLink()) {
    throw new BuildFailure(`${label} must not be a symbolic-link or reparse path`);
  }
  return stats;
}

function assertNoLinkedComponents(root, candidate, label) {
  if (!isWithin(root, candidate, true)) {
    throw new BuildFailure(`${label} escapes its allowed root`);
  }
  const relative = path.relative(root, candidate);
  let current = root;
  lstatRequired(current, `${label} root`);
  for (const segment of relative.split(path.sep).filter(Boolean)) {
    current = path.join(current, segment);
    if (!fs.existsSync(current)) break;
    lstatRequired(current, label);
  }
}

function assertDirectory(candidate, label) {
  const stats = lstatRequired(candidate, label);
  if (!stats.isDirectory()) throw new BuildFailure(`${label} must be a directory`);
  const real = fs.realpathSync.native(candidate);
  if (!pathsMatch(real, candidate)) {
    throw new BuildFailure(`${label} must not resolve through a symbolic-link or reparse path`);
  }
}

function assertRegularFile(root, candidate, label) {
  assertNoLinkedComponents(root, candidate, label);
  const stats = lstatRequired(candidate, label);
  if (!stats.isFile()) throw new BuildFailure(`${label} must be a regular file`);
  const real = fs.realpathSync.native(candidate);
  if (!pathsMatch(real, candidate)) {
    throw new BuildFailure(`${label} must not resolve through a symbolic-link or reparse path`);
  }
  return stats;
}

function readBoundedUtf8(root, candidate, label, maximumBytes) {
  const stats = assertRegularFile(root, candidate, label);
  if (stats.size > maximumBytes) {
    throw new BuildFailure(`${label} exceeds the ${maximumBytes}-byte limit`);
  }
  return fs.readFileSync(candidate, 'utf8');
}

function resolveManifestPath(manifestDirectory, docsRoot, rawPath, label) {
  if (
    typeof rawPath !== 'string'
    || rawPath.length === 0
    || rawPath.length > 240
    || rawPath.includes('\0')
    || rawPath.includes('\\')
    || path.isAbsolute(rawPath)
  ) {
    throw new BuildFailure(`${label} must be a bounded relative POSIX-style path`);
  }
  const resolved = path.resolve(manifestDirectory, ...rawPath.split('/'));
  if (!isWithin(docsRoot, resolved)) throw new BuildFailure(`${label} escapes docs`);
  if (path.extname(resolved).toLowerCase() !== '.md') {
    throw new BuildFailure(`${label} must identify a Markdown file`);
  }
  assertRegularFile(docsRoot, resolved, label);
  return resolved;
}

function discoverFeatureMarkdown(docsRoot) {
  const featureRoot = path.join(docsRoot, 'features');
  const discovered = [];
  const visit = (directory) => {
    assertNoLinkedComponents(docsRoot, directory, 'feature documentation directory');
    assertDirectory(directory, 'feature documentation directory');
    for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
      const candidate = path.join(directory, entry.name);
      const stats = lstatRequired(candidate, `feature documentation path ${entry.name}`);
      if (entry.isSymbolicLink() || stats.isSymbolicLink()) {
        throw new BuildFailure(`feature documentation path ${entry.name} must not be linked`);
      }
      if (entry.isDirectory()) {
        visit(candidate);
      } else if (entry.isFile() && path.extname(entry.name).toLowerCase() === '.md') {
        const relative = path.relative(docsRoot, candidate).split(path.sep).join('/');
        if (relative !== 'features/README.md') discovered.push(relative);
      }
      if (discovered.length > 256) throw new BuildFailure('feature Markdown corpus exceeds 256 files');
    }
  };
  visit(featureRoot);
  return discovered.sort();
}

function escapeHtml(value) {
  return String(value)
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#39;');
}

function plainHeading(value) {
  return value
    .replace(/!\[([^\]]*)\]\([^)]*\)/g, '$1')
    .replace(/\[([^\]]+)\]\([^)]*\)/g, '$1')
    .replace(/[`*_~]/g, '')
    .replace(/\s+/g, ' ')
    .trim();
}

function extractTitle(markdown, fallback) {
  const match = /^#\s+(.+)$/m.exec(markdown);
  return plainHeading(match?.[1] ?? fallback) || fallback;
}

function slugifyHeading(value) {
  const slug = plainHeading(value)
    .normalize('NFKD')
    .replace(/[\u0300-\u036f]/g, '')
    .toLowerCase()
    .replace(/[^a-z0-9\u3400-\u9fff]+/g, '-')
    .replace(/^-+|-+$/g, '')
    .slice(0, 72);
  return slug || 'section';
}

function parseDestination(rawDestination) {
  let value = rawDestination.trim();
  const titled = /^(<[^>]+>|\S+)(?:\s+["'][^"']*["'])$/.exec(value);
  if (titled) value = titled[1];
  if (value.startsWith('<') && value.endsWith('>')) value = value.slice(1, -1);
  return value.trim();
}

function splitReference(value) {
  const hashIndex = value.indexOf('#');
  const beforeHash = hashIndex >= 0 ? value.slice(0, hashIndex) : value;
  const hash = hashIndex >= 0 ? value.slice(hashIndex) : '';
  const queryIndex = beforeHash.indexOf('?');
  return {
    pathname: queryIndex >= 0 ? beforeHash.slice(0, queryIndex) : beforeHash,
    suffix: `${queryIndex >= 0 ? beforeHash.slice(queryIndex) : ''}${hash}`
  };
}

function safeRelativeUrl(value) {
  if (!value || /[\u0000-\u001f\u007f]/.test(value) || value.includes('\\')) return null;
  if (value.startsWith('//')) return null;
  if (/^[a-z][a-z0-9+.-]*:/i.test(value)) {
    return /^(https?:|mailto:)/i.test(value) ? value : null;
  }
  return value;
}

function pathKey(value) {
  return normalizedPath(value);
}

function makeInlineRenderer(context) {
  const formatText = (value) => {
    let rendered = escapeHtml(value);
    rendered = rendered.replace(/\*\*([^*\n]+)\*\*/g, '<strong>$1</strong>');
    rendered = rendered.replace(/__([^_\n]+)__/g, '<strong>$1</strong>');
    rendered = rendered.replace(/~~([^~\n]+)~~/g, '<del>$1</del>');
    rendered = rendered.replace(/(^|[^*])\*([^*\n]+)\*(?!\*)/g, '$1<em>$2</em>');
    return rendered;
  };

  const resolveLink = (destination, sourcePath) => {
    const safe = safeRelativeUrl(destination);
    if (!safe) return { href: null, unavailable: 'unsafe destination omitted' };
    if (/^(https?:|mailto:)/i.test(safe) || safe.startsWith('#')) {
      return { href: safe, external: /^(https?:|mailto:)/i.test(safe) };
    }
    const { pathname, suffix } = splitReference(safe);
    if (path.extname(pathname).toLowerCase() !== '.md') return { href: safe };
    let decodedPath;
    try {
      decodedPath = decodeURIComponent(pathname);
    } catch {
      return { href: null, unavailable: 'invalid destination omitted' };
    }
    const resolved = path.resolve(path.dirname(sourcePath), ...decodedPath.split('/'));
    const mappedSlug = context.articlePathToSlug.get(pathKey(resolved));
    if (mappedSlug) return { href: `articles/${mappedSlug}.html${suffix}` };
    return { href: null, unavailable: 'article is not part of this published build' };
  };

  const renderImage = (alt, destination, sourcePath) => {
    const safe = safeRelativeUrl(destination);
    if (!safe || /^(https?:|mailto:)/i.test(safe) || safe.startsWith('#')) {
      return `<span class="image-note" role="note">External or unsafe image omitted: ${formatText(alt || 'unnamed image')}</span>`;
    }
    const { pathname } = splitReference(safe);
    let decodedPath;
    try {
      decodedPath = decodeURIComponent(pathname);
    } catch {
      throw new BuildFailure(`image destination in ${sourcePath} is not valid UTF-8 URL text`);
    }
    const resolved = path.resolve(path.dirname(sourcePath), ...decodedPath.split('/'));
    if (!isWithin(context.checkoutRoot, resolved)) {
      throw new BuildFailure(`image destination escapes the checkout: ${destination}`);
    }
    const stats = assertRegularFile(context.checkoutRoot, resolved, `image ${destination}`);
    if (stats.size > MAX_EMBEDDED_ASSET_BYTES) {
      throw new BuildFailure(`image ${destination} exceeds the ${MAX_EMBEDDED_ASSET_BYTES}-byte limit`);
    }
    context.embeddedAssetBytes += stats.size;
    if (context.embeddedAssetBytes > MAX_PAGE_ASSET_BYTES) {
      throw new BuildFailure(`embedded images exceed the ${MAX_PAGE_ASSET_BYTES}-byte page limit`);
    }
    const extension = path.extname(resolved).toLowerCase();
    const mimeTypes = new Map([
      ['.png', 'image/png'],
      ['.jpg', 'image/jpeg'],
      ['.jpeg', 'image/jpeg'],
      ['.gif', 'image/gif'],
      ['.webp', 'image/webp']
    ]);
    const mimeType = mimeTypes.get(extension);
    if (!mimeType) throw new BuildFailure(`image ${destination} uses an unsupported format`);
    const encoded = fs.readFileSync(resolved).toString('base64');
    return `<img loading="lazy" decoding="async" src="data:${mimeType};base64,${encoded}" alt="${escapeHtml(plainHeading(alt) || 'Documentation image')}">`;
  };

  return (value, sourcePath) => {
    const tokenPattern = /(!?)\[([^\]\n]*)\]\(([^)\n]+)\)|`([^`\n]+)`/g;
    let cursor = 0;
    let rendered = '';
    let match;
    while ((match = tokenPattern.exec(value))) {
      rendered += formatText(value.slice(cursor, match.index));
      if (match[4] !== undefined) {
        rendered += `<code>${escapeHtml(match[4])}</code>`;
      } else if (match[1] === '!') {
        rendered += renderImage(match[2], parseDestination(match[3]), sourcePath);
      } else {
        const destination = parseDestination(match[3]);
        const resolved = resolveLink(destination, sourcePath);
        const label = formatText(match[2]);
        if (resolved.href) {
          const external = resolved.external ? ' rel="noopener noreferrer"' : '';
          rendered += `<a href="${escapeHtml(resolved.href)}"${external}>${label}</a>`;
        } else {
          rendered += `<span class="unavailable-link">${label} <span class="link-note">(${escapeHtml(resolved.unavailable)})</span></span>`;
        }
      }
      cursor = tokenPattern.lastIndex;
    }
    rendered += formatText(value.slice(cursor));
    return rendered;
  };
}

function splitTableRow(line) {
  let value = line.trim();
  if (value.startsWith('|')) value = value.slice(1);
  if (value.endsWith('|')) value = value.slice(0, -1);
  const cells = [];
  let current = '';
  let escaped = false;
  for (const character of value) {
    if (escaped) {
      current += character;
      escaped = false;
    } else if (character === '\\') {
      escaped = true;
    } else if (character === '|') {
      cells.push(current.trim());
      current = '';
    } else {
      current += character;
    }
  }
  cells.push(current.trim());
  return cells;
}

function isTableDivider(line) {
  const cells = splitTableRow(line);
  return cells.length > 0 && cells.every((cell) => /^:?-{3,}:?$/.test(cell));
}

function beginsBlock(lines, index) {
  const line = lines[index] ?? '';
  return /^\s*$/.test(line)
    || /^\s*```/.test(line)
    || /^#{1,6}\s+/.test(line)
    || /^\s*(?:[-+*]|\d+[.)])\s+/.test(line)
    || /^\s*>/.test(line)
    || /^\s*(?:-{3,}|\*{3,}|_{3,})\s*$/.test(line)
    || (line.includes('|') && isTableDivider(lines[index + 1] ?? ''));
}

function renderMarkdown(markdown, sourcePath, context) {
  const inline = makeInlineRenderer(context);
  const lines = markdown.replace(/\r\n?/g, '\n').split('\n');
  const output = [];
  const usedHeadingIds = new Map([['article-title', 1]]);
  let foundPrimaryHeading = false;
  let index = 0;

  const uniqueHeadingId = (heading) => {
    const base = slugifyHeading(heading);
    const count = usedHeadingIds.get(base) ?? 0;
    usedHeadingIds.set(base, count + 1);
    return count === 0 ? base : `${base}-${count + 1}`;
  };

  while (index < lines.length) {
    const line = lines[index];
    if (/^\s*$/.test(line)) {
      index += 1;
      continue;
    }

    const fence = /^\s*```\s*([\w+-]{0,32})\s*$/.exec(line);
    if (fence) {
      const code = [];
      index += 1;
      while (index < lines.length && !/^\s*```\s*$/.test(lines[index])) {
        code.push(lines[index]);
        index += 1;
      }
      if (index < lines.length) index += 1;
      const language = fence[1] ? ` class="language-${escapeHtml(fence[1])}"` : '';
      output.push(`<pre><code${language}>${escapeHtml(code.join('\n'))}</code></pre>`);
      continue;
    }

    const heading = /^(#{1,6})\s+(.+?)\s*#*\s*$/.exec(line);
    if (heading) {
      const level = heading[1].length;
      const id = !foundPrimaryHeading && level === 1 ? 'article-title' : uniqueHeadingId(heading[2]);
      if (level === 1) foundPrimaryHeading = true;
      output.push(`<h${level} id="${escapeHtml(id)}">${inline(heading[2], sourcePath)}</h${level}>`);
      index += 1;
      continue;
    }

    if (/^\s*(?:-{3,}|\*{3,}|_{3,})\s*$/.test(line)) {
      output.push('<hr>');
      index += 1;
      continue;
    }

    if (line.includes('|') && isTableDivider(lines[index + 1] ?? '')) {
      const headers = splitTableRow(line);
      index += 2;
      const rows = [];
      while (index < lines.length && lines[index].includes('|') && !/^\s*$/.test(lines[index])) {
        rows.push(splitTableRow(lines[index]));
        index += 1;
      }
      const head = headers.map((cell) => `<th scope="col">${inline(cell, sourcePath)}</th>`).join('');
      const body = rows.map((row) => {
        const normalized = headers.map((_, cellIndex) => row[cellIndex] ?? '');
        return `<tr>${normalized.map((cell) => `<td>${inline(cell, sourcePath)}</td>`).join('')}</tr>`;
      }).join('');
      output.push(`<div class="table-scroll" role="region" aria-label="Scrollable documentation table" tabindex="0"><table><thead><tr>${head}</tr></thead><tbody>${body}</tbody></table></div>`);
      continue;
    }

    const list = /^\s*((?:[-+*])|(\d+)[.)])\s+(.+)$/.exec(line);
    if (list) {
      const ordered = Boolean(list[2]);
      const items = [];
      const start = ordered ? Number.parseInt(list[2], 10) : 1;
      while (index < lines.length) {
        const item = /^\s*((?:[-+*])|(\d+)[.)])\s+(.+)$/.exec(lines[index]);
        if (!item || Boolean(item[2]) !== ordered) break;
        items.push(item[3]);
        index += 1;
      }
      const tag = ordered ? 'ol' : 'ul';
      const startAttribute = ordered && start !== 1 ? ` start="${start}"` : '';
      output.push(`<${tag}${startAttribute}>${items.map((item) => `<li>${inline(item, sourcePath)}</li>`).join('')}</${tag}>`);
      continue;
    }

    if (/^\s*>/.test(line)) {
      const quote = [];
      while (index < lines.length && /^\s*>/.test(lines[index])) {
        quote.push(lines[index].replace(/^\s*>\s?/, ''));
        index += 1;
      }
      output.push(`<blockquote><p>${inline(quote.join(' '), sourcePath)}</p></blockquote>`);
      continue;
    }

    const paragraph = [line.trim()];
    index += 1;
    while (index < lines.length && !beginsBlock(lines, index)) {
      paragraph.push(lines[index].trim());
      index += 1;
    }
    output.push(`<p>${inline(paragraph.join(' '), sourcePath)}</p>`);
  }

  return { html: output.join('\n'), foundPrimaryHeading };
}

const ARTICLE_ROUTE_CSS = `
.article-route-layout { display: grid; gap: 18px; }
.article-document, .suggested { padding: clamp(18px, 4vw, 34px); border: 1px solid var(--md-sys-color-outline-variant); border-radius: var(--md-sys-shape-extra-large); background: var(--md-sys-color-surface-container); box-shadow: var(--md-sys-elevation-1); }
.article-document h1 { margin-top: 0; font-size: clamp(2rem, 5vw, 3.3rem); line-height: 1.15; text-wrap: balance; }
.article-document h2 { margin-top: 2rem; font-size: clamp(1.45rem, 3vw, 2.1rem); }
.article-document h3 { margin-top: 1.6rem; font-size: 1.25rem; }
.article-document p, .article-document li { max-width: 78ch; }
.article-document pre { max-width: 100%; overflow: auto; padding: 16px; border-radius: var(--md-sys-shape-medium); background: #111318; color: #f1f1f8; }
.article-document pre code { padding: 0; background: transparent; color: inherit; }
.article-document blockquote { margin-inline: 0; padding: 8px 16px; border-inline-start: 5px solid var(--md-sys-color-primary); border-radius: 0 var(--md-sys-shape-small) var(--md-sys-shape-small) 0; background: var(--md-sys-color-primary-container); color: var(--md-sys-color-on-primary-container); }
.table-scroll { max-width: 100%; overflow: auto; margin-block: 16px; border: 1px solid var(--md-sys-color-outline-variant); border-radius: var(--md-sys-shape-medium); }
.table-scroll table { width: 100%; min-width: 32rem; border-collapse: collapse; background: var(--md-sys-color-surface); }
.table-scroll th, .table-scroll td { padding: 12px 16px; border-bottom: 1px solid var(--md-sys-color-outline-variant); text-align: start; vertical-align: top; }
.table-scroll th { background: var(--md-sys-color-primary-container); color: var(--md-sys-color-on-primary-container); }
.article-document img { display: block; max-width: 100%; height: auto; margin: 12px auto; border-radius: var(--md-sys-shape-medium); }
.revision, .article-language-note, .unavailable-link, .image-note { color: var(--md-sys-color-on-surface-variant); }
.link-note { font-size: .85em; }
.suggested { background: var(--md-sys-color-surface-container-high); }
.suggested ul { display: grid; grid-template-columns: repeat(auto-fit, minmax(min(100%, 14rem), 1fr)); gap: 10px; padding: 0; list-style: none; }
.suggested a { display: flex; align-items: center; min-height: 48px; padding: 10px 14px; border-radius: var(--md-sys-shape-small); background: var(--md-sys-color-surface); font-weight: 650; }
@media (max-width: 760px) { .article-document, .suggested { padding: 16px; border-radius: var(--md-sys-shape-large); } }
@media print { .article-search, .suggested { display: none; } .article-document { padding: 0; border: 0; box-shadow: none; background: transparent; } .article-document pre { white-space: pre-wrap; } }
`;

function replaceOnce(source, needle, replacement, label) {
  const first = source.indexOf(needle);
  if (first < 0 || source.indexOf(needle, first + needle.length) >= 0) {
    throw new BuildFailure(`stamped site shell has an unexpected ${label} contract`);
  }
  return `${source.slice(0, first)}${replacement}${source.slice(first + needle.length)}`;
}

function insertBeforeClosingTag(source, openingNeedle, closingTag, insertion, label) {
  const opening = source.indexOf(openingNeedle);
  if (opening < 0) throw new BuildFailure(`stamped site shell is missing ${label}`);
  const closing = source.indexOf(closingTag, opening);
  if (closing < 0) throw new BuildFailure(`stamped site shell has an incomplete ${label}`);
  return `${source.slice(0, closing)}${insertion}${source.slice(closing)}`;
}

function renderSuggested(related, articleBySlug) {
  const items = related.map((slug) => {
    const article = articleBySlug.get(slug);
    return `<li><a href="articles/${escapeHtml(slug)}.html">${escapeHtml(article.title)}</a></li>`;
  }).join('');
  return `<aside class="suggested" aria-labelledby="suggested-title"><h2 id="suggested-title" data-en="Suggested articles" data-zh="建議文章">Suggested articles</h2><ul>${items}</ul></aside>`;
}

function renderArticleSearch(title) {
  return `<div class="search-surface article-search" id="articleSearchSurface">
    <div class="search-row"><input id="articleSearch" type="search" maxlength="120" placeholder="Search this article" aria-label="Search this article"><button id="articleBuilderButton" class="tonal" type="button" aria-expanded="false" aria-controls="articleRegexBuilder" data-en="Regex builder" data-zh="Regex 建構器">Regex builder</button><span id="articleCount" class="helper" aria-live="polite"></span></div>
    <div id="articleRegexBuilder" class="builder" role="group" aria-label="Article search regular expression builder">
      <label class="mode-label"><input id="articleRegexMode" type="checkbox"> <span data-en="Use regular expression" data-zh="使用正則表達式">Use regular expression</span></label>
      <div class="builder-grid"><label><span data-en="Pattern" data-zh="模式">Pattern</span><input id="articlePattern" type="text" maxlength="120"></label><label><span data-en="Flags" data-zh="旗標">Flags</span><input id="articleFlags" type="text" maxlength="6" value="i" aria-describedby="articleRegexHelp"></label></div>
      <div class="builder-grid"><label><span data-en="Literal text" data-zh="純文字">Literal text</span><input id="articleLiteral" type="text" maxlength="80"></label><label><span data-en="Guided construct" data-zh="引導式結構">Guided construct</span><select id="articleConstruct"><option value="^" data-en="Start anchor ^" data-zh="開頭錨點 ^">Start anchor ^</option><option value="$" data-en="End anchor $" data-zh="結尾錨點 $">End anchor $</option><option value="[A-Za-z]" data-en="Character class" data-zh="字元類別">Character class</option><option value="(...)" data-en="Capture group" data-zh="擷取群組">Capture group</option><option value="(?:...)" data-en="Non-capture group" data-zh="非擷取群組">Non-capture group</option><option value="|" data-en="Alternation" data-zh="選擇分支">Alternation</option><option value="?" data-en="Optional ?" data-zh="可選 ?">Optional ?</option><option value="*" data-en="Zero or more *" data-zh="零次或以上 *">Zero or more *</option><option value="+" data-en="One or more +" data-zh="一次或以上 +">One or more +</option><option value="{2}" data-en="Exact count {2}" data-zh="準確次數 {2}">Exact count {2}</option></select></label></div>
      <div class="builder-actions"><button id="articleAddLiteral" class="outlined" type="button" data-en="Add literal" data-zh="加入純文字">Add literal</button><button id="articleAddConstruct" class="outlined" type="button" data-en="Add construct" data-zh="加入結構">Add construct</button><button id="articleCopyPattern" class="text-button" type="button" data-en="Copy pattern" data-zh="複製模式">Copy pattern</button></div>
      <label><span data-en="Sample text" data-zh="範例文字">Sample text</span><textarea id="articleSample" maxlength="1200">${escapeHtml(title)}</textarea></label>
      <span id="articleRegexHelp" class="helper" data-en="JavaScript RegExp in a time-limited worker; supported flags: gimsuy. Pattern length 120, sample length 1200. High-risk nested repetition is rejected." data-zh="JavaScript RegExp 會在有時間限制的 worker 內執行；支援旗標：gimsuy。模式上限 120，範例上限 1200；高風險巢狀重複會被拒絕。">JavaScript RegExp in a time-limited worker; supported flags: gimsuy. Pattern length 120, sample length 1200. High-risk nested repetition is rejected.</span><output id="articleRegexStatus" class="helper" aria-live="polite">Plain text is active.</output><output id="articleSampleResult" class="helper" aria-live="polite"></output>
    </div>
  </div>`;
}

const ARTICLE_FILTER_SCRIPT = `
  const articleBlocks = [...$('articleContent').children].filter((node) => !node.classList.contains('revision'));
  let articleFilterGeneration = 0;
  const getArticleMatcher = bindRegexBuilder('article', () => filterArticle());
  async function filterArticle() {
    const generation = ++articleFilterGeneration;
    const matcher = getArticleMatcher();
    if (!matcher.valid) {
      cancelBoundedRegex('filter:article');
      articleBlocks.forEach((block) => { block.hidden = true; });
      $('articleCount').textContent = localized('No results until the pattern is valid.', '模式有效後才會顯示結果。');
      return;
    }
    try {
      const matches = await evaluateMatcher(matcher, articleBlocks.map((block) => bilingualIndex(block)), 'filter:article');
      if (generation !== articleFilterGeneration) return;
      let visible = 0;
      articleBlocks.forEach((block, index) => { block.hidden = !matches[index]; if (matches[index]) visible += 1; });
      $('articleCount').textContent = visible + ' / ' + articleBlocks.length;
    } catch (error) {
      if (generation !== articleFilterGeneration) return;
      articleBlocks.forEach((block) => { block.hidden = true; });
      $('articleRegexStatus').textContent = error.message;
      $('articleCount').textContent = localized('Evaluation stopped safely; no results are shown.', '評估已安全停止；不會顯示結果。');
    }
  }

`;

function renderPage(article, revision, context, articleBySlug, siteTemplate) {
  context.embeddedAssetBytes = 0;
  const rendered = renderMarkdown(article.markdown, article.sourcePath, context);
  const titleHeading = rendered.foundPrimaryHeading ? '' : `<h1 id="article-title">${escapeHtml(article.title)}</h1>\n`;
  const sourceLabel = path.relative(context.checkoutRoot, article.sourcePath).split(path.sep).join('/');
  const articlePanel = `
      <section id="article" class="panel article-route" role="tabpanel" tabindex="0" aria-labelledby="tab-article">
        <div class="section-heading"><div><p class="eyebrow" data-en="Documentation article" data-zh="文件文章">Documentation article</p><h2 data-en="${escapeHtml(article.title)}" data-zh="${escapeHtml(article.title)}">${escapeHtml(article.title)}</h2><p class="article-language-note" data-en="The shared site controls remain available here. The source article is currently maintained in English." data-zh="共用網站控制仍可在此使用；來源文章目前以英文維護。">The shared site controls remain available here. The source article is currently maintained in English.</p></div></div>
        ${renderArticleSearch(article.title)}
        <div class="article-route-layout">
          <article id="articleContent" class="article-document" lang="en" aria-labelledby="article-title">${titleHeading}${rendered.html}<p class="revision">Source revision <code>${revision}</code></p></article>
          ${renderSuggested(article.related, articleBySlug)}
        </div>
      </section>
`;
  const articleTab = `\n      <button id="tab-article" class="site-tab" role="tab" aria-selected="true" aria-controls="article" data-tab="article" data-en="Article: ${escapeHtml(article.title)}" data-zh="文章：${escapeHtml(article.title)}">Article: ${escapeHtml(article.title)}</button>`;

  let page = siteTemplate;
  page = replaceOnce(page, '<head>', `<head>\n<meta name="source-revision" content="${revision}">\n<meta name="article-slug" content="${escapeHtml(article.slug)}">`, 'head');
  page = page.replace(/<title>[\s\S]*?<\/title>/, `<title>${escapeHtml(article.title)} · Sandboxie documentation</title>`);
  page = replaceOnce(page, '</style>', `${ARTICLE_ROUTE_CSS}\n</style>`, 'style boundary');
  page = replaceOnce(page, '<button id="tab-overview" class="site-tab" role="tab" aria-selected="true"', '<button id="tab-overview" class="site-tab" role="tab" aria-selected="false"', 'default overview tab');
  page = replaceOnce(page, '<section id="overview" class="panel" role="tabpanel" tabindex="0" aria-labelledby="tab-overview">', '<section id="overview" class="panel" role="tabpanel" tabindex="0" aria-labelledby="tab-overview" hidden>', 'default overview panel');
  page = insertBeforeClosingTag(page, '<nav id="siteTabs"', '</nav>', articleTab, 'site tab list');
  page = insertBeforeClosingTag(page, '<main id="active-panel"', '</main>', articlePanel, 'active panel host');
  page = replaceOnce(page, '  const commands = [];', `${ARTICLE_FILTER_SCRIPT}  const commands = [];`, 'command inventory hook');
  page = replaceOnce(page, "      [$('settingsRegexBuilder'), 'Settings search regular expression builder', '設定搜尋正則表達式建構器'],", "      [$('settingsRegexBuilder'), 'Settings search regular expression builder', '設定搜尋正則表達式建構器'],\n      [$('articleSearch'), 'Search this article', '搜尋此文章'],\n      [$('articleRegexBuilder'), 'Article search regular expression builder', '文章搜尋正則表達式建構器'],", 'accessible-name inventory');
  page = replaceOnce(page, "    $('settingsSearch').placeholder = localized('Search setting labels, descriptions, and values', '搜尋設定標籤、說明及目前值');", "    $('settingsSearch').placeholder = localized('Search setting labels, descriptions, and values', '搜尋設定標籤、說明及目前值');\n    $('articleSearch').placeholder = localized('Search this article', '搜尋此文章');", 'localized placeholder hook');
  page = replaceOnce(page, "    if (typeof filterSettings === 'function') filterSettings();", "    if (typeof filterSettings === 'function') filterSettings();\n    if (typeof filterArticle === 'function') filterArticle();", 'presentation filter hook');
  page = replaceOnce(page, "  openTab(hash && $(hash) ? hash : 'overview', false);", "  openTab(hash && $(hash) ? hash : 'article', false);", 'initial article route');
  page = replaceOnce(page, "new Worker('regex-worker.js')", "new Worker('../regex-worker.js')", 'article worker path');
  page = page.replace(/href="articles\/([a-z0-9-]+\.html(?:[?#][^"]*)?)"/gi, 'href="$1"');
  page = page.replace('PAGES_SOURCE_REVISION', revision);
  if (page.includes('PAGES_SOURCE_REVISION')) throw new BuildFailure('stamped site shell still contains a revision placeholder');
  if (!page.includes(revision)) throw new BuildFailure('article route does not carry the exact stamped revision');
  if (!page.includes(`<code>${revision}</code>`)) throw new BuildFailure('article route is missing its visible exact revision');
  if (!page.includes(escapeHtml(sourceLabel))) {
    page = replaceOnce(page, '</footer>', `<p class="helper">Rendered from <code>${escapeHtml(sourceLabel)}</code>.</p>\n  </footer>`, 'footer source record');
  }
  return page;
}

function validateManifest(manifest, manifestDirectory, docsRoot) {
  if (!manifest || manifest.version !== 1 || !Array.isArray(manifest.articles)) {
    throw new BuildFailure('docs/articles/index.json must use the version 1 article schema');
  }
  if (manifest.articles.length !== FEATURE_ARTICLE_COUNT) {
    throw new BuildFailure(`article manifest must contain exactly ${FEATURE_ARTICLE_COUNT} feature articles`);
  }
  const seenSlugs = new Set();
  const seenSources = new Set();
  const articles = manifest.articles.map((entry, index) => {
    const label = `article ${index + 1}`;
    if (!entry || typeof entry !== 'object' || Array.isArray(entry)) {
      throw new BuildFailure(`${label} must be an object`);
    }
    if (typeof entry.slug !== 'string' || !/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(entry.slug) || entry.slug.length > 64) {
      throw new BuildFailure(`${label} has an invalid bounded slug`);
    }
    if (seenSlugs.has(entry.slug) || RESERVED_SLUGS.has(entry.slug)) {
      throw new BuildFailure(`${label} duplicates or reserves slug ${entry.slug}`);
    }
    seenSlugs.add(entry.slug);
    const sourcePath = resolveManifestPath(manifestDirectory, docsRoot, entry.path, `${label} path`);
    const relativeSource = path.relative(docsRoot, sourcePath).split(path.sep).join('/');
    const canonicalSource = CANONICAL_FEATURE_ARTICLES.get(entry.slug);
    if (!canonicalSource || relativeSource !== canonicalSource) {
      throw new BuildFailure(`${label} does not match the canonical slug and source inventory`);
    }
    const sourceKey = pathKey(sourcePath);
    if (seenSources.has(sourceKey)) throw new BuildFailure(`${label} duplicates a source Markdown file`);
    seenSources.add(sourceKey);
    if (!Array.isArray(entry.related) || entry.related.length === 0 || entry.related.length > MAX_RELATED_ARTICLES) {
      throw new BuildFailure(`${label} must have 1-${MAX_RELATED_ARTICLES} suggested article slugs`);
    }
    if (new Set(entry.related).size !== entry.related.length || entry.related.includes(entry.slug)) {
      throw new BuildFailure(`${label} has duplicate or self-referential suggested articles`);
    }
    return { slug: entry.slug, sourcePath, related: [...entry.related] };
  });
  for (const article of articles) {
    for (const relatedSlug of article.related) {
      if (!seenSlugs.has(relatedSlug)) {
        throw new BuildFailure(`article ${article.slug} references unknown suggested article ${relatedSlug}`);
      }
    }
  }
  for (const canonicalSlug of CANONICAL_FEATURE_ARTICLES.keys()) {
    if (!seenSlugs.has(canonicalSlug)) {
      throw new BuildFailure(`article manifest is missing canonical article ${canonicalSlug}`);
    }
  }
  const canonicalFeatureCorpus = [...CANONICAL_FEATURE_ARTICLES.values()]
    .filter((relative) => relative.startsWith('features/'))
    .sort();
  const discoveredFeatureCorpus = discoverFeatureMarkdown(docsRoot);
  if (JSON.stringify(discoveredFeatureCorpus) !== JSON.stringify(canonicalFeatureCorpus)) {
    throw new BuildFailure('feature Markdown corpus does not match the canonical manifest inventory');
  }
  if (!manifest.changelog || typeof manifest.changelog !== 'object') {
    throw new BuildFailure('article manifest must identify the changelog Markdown file');
  }
  const changelogPath = resolveManifestPath(manifestDirectory, docsRoot, manifest.changelog.path, 'changelog path');
  return { articles, changelogPath };
}

function assertSafeOutputFile(articlesDirectory, outputPath) {
  if (!isWithin(articlesDirectory, outputPath)) throw new BuildFailure('generated article path escaped the article directory');
  if (fs.existsSync(outputPath)) {
    throw new BuildFailure(`generated output ${path.basename(outputPath)} was not safely cleared before writing`);
  }
}

function clearOwnedHtmlOutputs(articlesDirectory) {
  const htmlOutputs = fs.readdirSync(articlesDirectory, { withFileTypes: true })
    .filter((entry) => entry.name.toLowerCase().endsWith('.html'));
  for (const entry of htmlOutputs) {
    const outputPath = path.join(articlesDirectory, entry.name);
    const stats = lstatRequired(outputPath, `existing HTML output ${entry.name}`);
    if (!entry.isFile() || !stats.isFile()) {
      throw new BuildFailure(`existing HTML output ${entry.name} must be a regular file`);
    }
    if (stats.nlink !== 1) {
      throw new BuildFailure(`existing HTML output ${entry.name} must not be hard-linked`);
    }
    const real = fs.realpathSync.native(outputPath);
    if (!pathsMatch(real, outputPath)) {
      throw new BuildFailure(`existing HTML output ${entry.name} must not resolve through a symbolic-link or reparse path`);
    }
  }
  for (const entry of htmlOutputs) {
    fs.unlinkSync(path.join(articlesDirectory, entry.name));
  }
}

function main() {
  const [sourceArgument, targetArgument, revisionArgument, ...extraArguments] = process.argv.slice(2);
  if (
    extraArguments.length
    || !sourceArgument
    || !targetArgument
    || !/^[0-9a-f]{40}$/i.test(revisionArgument || '')
  ) {
    throw new BuildFailure('usage: node scripts/build-pages-articles.mjs docs .pages-site <40-char-revision>', 2);
  }

  const checkoutRoot = path.resolve(process.cwd());
  const docsRoot = path.resolve(sourceArgument);
  const targetRoot = path.resolve(targetArgument);
  const expectedDocsRoot = path.join(checkoutRoot, 'docs');
  const expectedTargetRoot = path.join(checkoutRoot, '.pages-site');
  if (!pathsMatch(docsRoot, expectedDocsRoot) || !pathsMatch(targetRoot, expectedTargetRoot)) {
    throw new BuildFailure('source and target must be the checkout docs and .pages-site directories', 2);
  }
  assertNoLinkedComponents(checkoutRoot, docsRoot, 'docs source');
  assertNoLinkedComponents(checkoutRoot, targetRoot, 'Pages staging target');
  assertDirectory(docsRoot, 'docs source');
  assertDirectory(targetRoot, 'Pages staging target');

  const manifestDirectory = path.join(docsRoot, 'articles');
  assertNoLinkedComponents(docsRoot, manifestDirectory, 'article manifest directory');
  assertDirectory(manifestDirectory, 'article manifest directory');
  const manifestPath = path.join(manifestDirectory, 'index.json');
  const manifestText = readBoundedUtf8(docsRoot, manifestPath, 'article manifest', MAX_MANIFEST_BYTES);
  let manifest;
  try {
    manifest = JSON.parse(manifestText);
  } catch (error) {
    throw new BuildFailure(`article manifest is not valid JSON: ${error.message}`);
  }
  const validated = validateManifest(manifest, manifestDirectory, docsRoot);
  const screenshotsPath = path.join(docsRoot, 'screenshots.md');
  assertRegularFile(docsRoot, screenshotsPath, 'screenshots path');

  const records = validated.articles.map((article) => ({ ...article, kind: 'feature' }));
  records.push({
    slug: 'changelog',
    sourcePath: validated.changelogPath,
    related: ['changelog-viewer', 'native-ci-evidence'],
    kind: 'supplemental'
  });
  records.push({
    slug: 'screenshots',
    sourcePath: screenshotsPath,
    related: ['material-design', 'm3-shell-boundary', 'native-ci-evidence'],
    kind: 'supplemental'
  });

  const articleBySlug = new Map();
  for (const record of records) {
    const markdown = readBoundedUtf8(
      record.sourcePath.startsWith(docsRoot) ? docsRoot : checkoutRoot,
      record.sourcePath,
      `Markdown source ${record.slug}`,
      MAX_MARKDOWN_BYTES
    );
    articleBySlug.set(record.slug, {
      ...record,
      markdown,
      title: extractTitle(markdown, record.slug.replaceAll('-', ' '))
    });
  }
  for (const record of records) {
    for (const relatedSlug of record.related) {
      if (!articleBySlug.has(relatedSlug)) {
        throw new BuildFailure(`article ${record.slug} references unavailable suggested article ${relatedSlug}`);
      }
    }
  }

  const articlePathToSlug = new Map(records.map((record) => [pathKey(record.sourcePath), record.slug]));
  const revision = revisionArgument.toLowerCase();
  const siteTemplatePath = path.join(targetRoot, 'index.html');
  const siteTemplate = readBoundedUtf8(targetRoot, siteTemplatePath, 'stamped site shell', MAX_SITE_TEMPLATE_BYTES);
  if (siteTemplate.includes('PAGES_SOURCE_REVISION') || !siteTemplate.includes(revision)) {
    throw new BuildFailure('stamped site shell must carry the exact requested revision with no placeholder');
  }
  if (/<base\b/i.test(siteTemplate)) throw new BuildFailure('stamped site shell must not already define a base URL');
  const requiredShellHooks = [
    'id="language"',
    'id="funnyEnglish"',
    'id="funnyCantonese"',
    'id="paletteButton"',
    'id="siteTabs"',
    'id="settings"',
    "bindRegexBuilder('feature'",
    'Ctrl+Shift+F'
  ];
  for (const hook of requiredShellHooks) {
    if (!siteTemplate.includes(hook)) throw new BuildFailure(`stamped site shell is missing required global behavior: ${hook}`);
  }

  const expectedHtml = records.map((record) => `${record.slug}.html`).sort();
  const expectedHtmlSet = new Set(expectedHtml);
  const landingArticleLinks = [...siteTemplate.matchAll(/href=["']articles\/([a-z0-9-]+\.html)(?:[?#][^"']*)?["']/gi)]
    .map((match) => match[1]);
  for (const linkedName of landingArticleLinks) {
    if (!expectedHtmlSet.has(linkedName)) {
      throw new BuildFailure(`landing page links to missing generated article ${linkedName}`);
    }
  }
  const uniqueLandingLinks = [...new Set(landingArticleLinks)].sort();
  if (JSON.stringify(uniqueLandingLinks) !== JSON.stringify(expectedHtml)) {
    throw new BuildFailure(`landing page must link every canonical ${FEATURE_ARTICLE_COUNT}+2 generated article`);
  }

  const articlesOutput = path.join(targetRoot, 'articles');
  if (fs.existsSync(articlesOutput)) {
    assertNoLinkedComponents(targetRoot, articlesOutput, 'generated articles directory');
    assertDirectory(articlesOutput, 'generated articles directory');
  } else {
    fs.mkdirSync(articlesOutput);
    assertDirectory(articlesOutput, 'generated articles directory');
  }
  clearOwnedHtmlOutputs(articlesOutput);

  const renderContext = { checkoutRoot, docsRoot, articlePathToSlug, embeddedAssetBytes: 0 };
  for (const record of records) {
    const article = articleBySlug.get(record.slug);
    const outputPath = path.join(articlesOutput, `${record.slug}.html`);
    assertSafeOutputFile(articlesOutput, outputPath);
    const html = renderPage(article, revision, renderContext, articleBySlug, siteTemplate);
    fs.writeFileSync(outputPath, html, { encoding: 'utf8', flag: 'wx' });
  }

  const actualHtml = fs.readdirSync(articlesOutput)
    .filter((name) => name.toLowerCase().endsWith('.html'))
    .sort();
  if (JSON.stringify(actualHtml) !== JSON.stringify(expectedHtml)) {
    throw new BuildFailure(`generated HTML inventory does not exactly match the canonical ${FEATURE_ARTICLE_COUNT}+2 article set`);
  }

  process.stdout.write(`pages-articles-built features=${FEATURE_ARTICLE_COUNT} supplemental=2 revision=${revision} target=${articlesOutput}\n`);
}

try {
  main();
} catch (error) {
  const failure = error instanceof BuildFailure ? error : new BuildFailure(error?.message || String(error));
  process.stderr.write(`pages-article-build-failed: ${failure.message}\n`);
  process.exitCode = failure.exitCode;
}
