/* Dependency-free, safe Markdown rendering for the local Pages article shell. */
(function attachMaterialSandboxMarkdown(global) {
  'use strict';

  const CONTROL_CHARACTERS = /[\u0000-\u001f\u007f-\u009f]/;
  const UNSAFE_SCHEME = /^[a-z][a-z0-9+.-]*:/i;
  const SAFE_CLASS = /^[a-z0-9_-]+$/i;

  function escapeHtml(value) {
    return String(value ?? '')
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  function escapeAttribute(value) {
    return escapeHtml(value).replace(/`/g, '&#96;');
  }

  function normalizeSegments(rawPath) {
    const segments = [];
    for (const rawSegment of String(rawPath ?? '').replace(/\\/g, '/').split('/')) {
      if (!rawSegment || rawSegment === '.') continue;
      let segment = rawSegment;
      try {
        segment = decodeURIComponent(rawSegment);
      } catch {
        return null;
      }
      if (segment.includes('/') || segment.includes('\\')) return null;
      if (segment === '.') continue;
      if (segment === '..') {
        segments.pop();
        continue;
      }
      if (CONTROL_CHARACTERS.test(segment)) return null;
      segments.push(encodeURI(segment));
    }
    return segments.join('/');
  }

  function sourceDirectory(sourcePath) {
    const source = normalizeSegments(sourcePath);
    if (source === null || !source) return '';
    return String(sourcePath).replace(/\\/g, '/').endsWith('/')
      ? source
      : source.slice(0, Math.max(0, source.lastIndexOf('/')));
  }

  function normalizeBasePath(basePath) {
    const normalized = normalizeSegments(String(basePath ?? '/').split(/[?#]/, 1)[0]);
    if (normalized === null || !normalized) return '/';
    return `/${normalized.replace(/^\/+|\/+$/g, '')}/`;
  }

  function splitReference(rawReference) {
    const value = String(rawReference ?? '').trim();
    const hashIndex = value.indexOf('#');
    const queryIndex = value.indexOf('?');
    const splitAt = [hashIndex, queryIndex].filter(index => index >= 0).sort((a, b) => a - b)[0];
    if (splitAt === undefined) return { pathname: value, suffix: '' };
    return { pathname: value.slice(0, splitAt), suffix: value.slice(splitAt) };
  }

  function inspectionValue(value) {
    let decoded = value;
    try {
      decoded = decodeURIComponent(value);
    } catch {
      // Invalid percent encoding is rejected by the local resolver.
    }
    return decoded.replace(/[\u0000-\u0020\u007f-\u009f]/g, '');
  }

  function resolveLocalReference(rawReference, options = {}) {
    const value = String(rawReference ?? '').trim();
    if (!value || CONTROL_CHARACTERS.test(value) || value.startsWith('//') || value.startsWith('\\\\')) return null;
    const parts = splitReference(value);
    if (!parts.pathname) return { path: '', href: parts.suffix || '#', suffix: parts.suffix };
    if (UNSAFE_SCHEME.test(inspectionValue(parts.pathname))) return null;

    const source = sourceDirectory(options.sourcePath || '');
    const relativePath = parts.pathname.replace(/\\/g, '/');
    const candidate = relativePath.startsWith('/')
      ? relativePath.slice(1)
      : [source, relativePath].filter(Boolean).join('/');
    const path = normalizeSegments(candidate);
    if (path === null) return null;
    return {
      path,
      href: `${normalizeBasePath(options.basePath)}${path}${parts.suffix}`,
      suffix: parts.suffix,
    };
  }

  function routeEntries(articleRoutes) {
    if (!articleRoutes) return [];
    if (Array.isArray(articleRoutes)) return articleRoutes;
    if (Array.isArray(articleRoutes.articles)) return articleRoutes.articles;
    if (typeof articleRoutes === 'object') return Object.entries(articleRoutes).map(([path, route]) => ({ path, route }));
    return [];
  }

  function routeForPath(path, articleRoutes) {
    const normalizedPath = normalizeSegments(path);
    if (normalizedPath === null) return null;
    for (const candidate of routeEntries(articleRoutes)) {
      const candidatePath = normalizeSegments(candidate.path || candidate.sourcePath || '');
      const candidateRoute = String(candidate.route || candidate.href || '').trim();
      if (candidatePath === normalizedPath && candidateRoute.startsWith('#/articles/')) return candidateRoute;
    }
    return null;
  }

  function normalizeArticlePath(rawReference, options = {}) {
    const raw = String(rawReference ?? '').trim();
    if (!raw) return null;
    if (/^qrc:\/Docs\//i.test(raw)) {
      const qrcParts = splitReference(raw.replace(/^qrc:\/Docs\//i, 'Docs/'));
      const qrcRoute = routeForPath(qrcParts.pathname, options.qrcRoutes);
      return qrcRoute ? `${qrcRoute}${qrcParts.suffix}` : null;
    }
    const local = resolveLocalReference(raw, options);
    if (!local || !/\.md$/i.test(splitReference(raw).pathname)) return null;
    const route = routeForPath(local.path, options.articleRoutes);
    return route ? `${route}${local.suffix}` : null;
  }

  function sanitizeUrl(rawReference, options = {}) {
    const value = String(rawReference ?? '').trim();
    if (!value || CONTROL_CHARACTERS.test(value) || value.startsWith('//') || value.startsWith('\\\\')) return null;
    const articleRoute = options.kind === 'image' ? null : normalizeArticlePath(value, options);
    if (articleRoute) return articleRoute;
    const scheme = inspectionValue(value).match(UNSAFE_SCHEME);
    if (options.kind !== 'image' && !scheme && /\.md$/i.test(splitReference(value).pathname)) return null;
    if (scheme) {
      if (scheme[0].toLowerCase() !== 'https:' || (options.kind === 'image' && options.allowHttpsImages !== true)) return null;
      try {
        const parsed = new global.URL(value);
        return parsed.protocol === 'https:' && !parsed.username && !parsed.password ? parsed.href : null;
      } catch {
        return null;
      }
    }
    return resolveLocalReference(value, options)?.href || null;
  }

  function plainText(markdown) {
    return String(markdown ?? '')
      .replace(/!\[([^\]]*)\]\([^)]*\)/g, '$1')
      .replace(/\[([^\]]+)\]\([^)]*\)/g, '$1')
      .replace(/[`*_~]/g, '')
      .replace(/<[^>]*>/g, '')
      .trim();
  }

  function renderEmphasis(text) {
    return text
      .replace(/(\*\*|__)([^\n]+?)\1/g, '<strong>$2</strong>')
      .replace(/~~([^\n]+?)~~/g, '<del>$1</del>')
      .replace(/(^|[^*])\*([^*\n]+)\*(?!\*)/g, '$1<em>$2</em>')
      .replace(/(^|[^_])_([^_\n]+)_(?!_)/g, '$1<em>$2</em>');
  }

  function findClosingBracket(value, startIndex) {
    for (let index = startIndex; index < value.length; index += 1) {
      if (value[index] === '\\') {
        index += 1;
      } else if (value[index] === ']') {
        return index;
      }
    }
    return -1;
  }

  function findClosingParenthesis(value, startIndex) {
    let depth = 0;
    for (let index = startIndex; index < value.length; index += 1) {
      if (value[index] === '\\') {
        index += 1;
      } else if (value[index] === '(') {
        depth += 1;
      } else if (value[index] === ')') {
        depth -= 1;
        if (depth === 0) return index;
      }
    }
    return -1;
  }

  function destinationFromMarkdown(value) {
    const trimmed = String(value ?? '').trim();
    if (trimmed.startsWith('<') && trimmed.endsWith('>')) return trimmed.slice(1, -1).trim();
    const whitespace = trimmed.search(/\s/);
    return whitespace < 0 ? trimmed : trimmed.slice(0, whitespace);
  }

  function renderInline(markdown, options = {}, depth = 0) {
    if (depth > 4) return escapeHtml(markdown);
    const value = String(markdown ?? '');
    let output = '';
    let plainBuffer = '';
    const flushPlain = () => {
      if (plainBuffer) output += renderEmphasis(escapeHtml(plainBuffer));
      plainBuffer = '';
    };

    for (let index = 0; index < value.length; index += 1) {
      const character = value[index];
      if (character === '\\' && index + 1 < value.length && /[\\`*_{}\[\]()#+\-.!|]/.test(value[index + 1])) {
        flushPlain();
        output += escapeHtml(value[index + 1]);
        index += 1;
        continue;
      }
      if (character === '`') {
        const end = value.indexOf('`', index + 1);
        if (end >= 0) {
          flushPlain();
          output += `<code>${escapeHtml(value.slice(index + 1, end))}</code>`;
          index = end;
          continue;
        }
      }

      const image = character === '!' && value[index + 1] === '[';
      if (!options.disableLinks && (character === '[' || image)) {
        const labelStart = index + (image ? 2 : 1);
        const labelEnd = findClosingBracket(value, labelStart);
        if (labelEnd >= 0 && value[labelEnd + 1] === '(') {
          const destinationEnd = findClosingParenthesis(value, labelEnd + 1);
          if (destinationEnd >= 0) {
            flushPlain();
            const label = value.slice(labelStart, labelEnd);
            const destination = destinationFromMarkdown(value.slice(labelEnd + 2, destinationEnd));
            const href = sanitizeUrl(destination, { ...options, kind: image ? 'image' : 'link' });
            if (image) {
              const alt = escapeAttribute(plainText(label));
              output += href
                ? `<img class="markdown-image" src="${escapeAttribute(href)}" alt="${alt}" loading="lazy" decoding="async">`
                : `<span class="markdown-image-blocked" role="note">${escapeHtml(plainText(label) || 'Image unavailable in local documentation')}</span>`;
            } else {
              const labelHtml = renderInline(label, { ...options, disableLinks: true }, depth + 1);
              const external = href && /^https:/i.test(href);
              output += href
                ? `<a href="${escapeAttribute(href)}"${external ? ' target="_blank" rel="noopener noreferrer"' : ''}>${labelHtml}</a>`
                : `<span class="markdown-link-blocked" role="note">${labelHtml}</span>`;
            }
            index = destinationEnd;
            continue;
          }
        }
      }
      plainBuffer += character;
    }
    flushPlain();
    return output;
  }

  function headingId(markdown, state) {
    const base = plainText(markdown)
      .toLowerCase()
      .normalize('NFKD')
      .replace(/[\u0300-\u036f]/g, '')
      .replace(/[^\p{L}\p{N}]+/gu, '-')
      .replace(/^-+|-+$/g, '') || 'section';
    const count = state.headingIds.get(base) || 0;
    state.headingIds.set(base, count + 1);
    return count ? `${base}-${count + 1}` : base;
  }

  function splitTableRow(line) {
    const text = String(line ?? '').trim().replace(/^\|/, '').replace(/\|$/, '');
    const cells = [];
    let cell = '';
    let escaped = false;
    for (const character of text) {
      if (escaped) {
        cell += character;
        escaped = false;
      } else if (character === '\\') {
        escaped = true;
      } else if (character === '|') {
        cells.push(cell.trim());
        cell = '';
      } else {
        cell += character;
      }
    }
    cells.push(cell.trim());
    return cells;
  }

  function isTableDivider(line) {
    const cells = splitTableRow(line);
    return cells.length > 0 && cells.every(cell => /^:?-{3,}:?$/.test(cell));
  }

  function listMatch(line) {
    const match = String(line ?? '').match(/^(\s*)([-+*]|\d+[.)])\s+(.+)$/);
    return match ? {
      indent: match[1].replace(/\t/g, '    ').length,
      ordered: /^\d/.test(match[2]),
      start: /^\d/.test(match[2]) ? Number.parseInt(match[2], 10) : 1,
      content: match[3],
    } : null;
  }

  function renderList(lines, start, options, state) {
    const first = listMatch(lines[start]);
    if (!first) return null;
    const tag = first.ordered ? 'ol' : 'ul';
    const items = [];
    let index = start;
    while (index < lines.length) {
      const current = listMatch(lines[index]);
      if (!current || current.indent !== first.indent || current.ordered !== first.ordered) break;
      index += 1;
      const itemLines = [current.content];
      let nested = '';
      while (index < lines.length) {
        const next = listMatch(lines[index]);
        if (next && next.indent > first.indent) {
          const child = renderList(lines, index, options, state);
          nested += child.html;
          index = child.next;
          continue;
        }
        if (next && next.indent === first.indent) break;
        if (!lines[index].trim()) {
          if (listMatch(lines[index + 1])?.indent === first.indent) {
            index += 1;
            break;
          }
          itemLines.push('');
          index += 1;
          continue;
        }
        const indentation = lines[index].match(/^(\s*)/)[1].replace(/\t/g, '    ').length;
        if (indentation <= first.indent) break;
        itemLines.push(lines[index].trim());
        index += 1;
      }
      items.push(`<li>${renderInline(itemLines.join(' ').trim(), options)}${nested}</li>`);
    }
    const startAttribute = first.ordered && first.start !== 1 ? ` start="${first.start}"` : '';
    return { html: `<${tag}${startAttribute}>${items.join('')}</${tag}>`, next: index };
  }

  function readDetails(lines, start, options, state) {
    const line = String(lines[start] ?? '').trim();
    const directive = line.match(/^:::\s*details(?:\s+(.+))?$/i);
    const raw = line.match(/^<details(\s+open)?\s*>(?:\s*<summary>(.*?)<\/summary>)?\s*$/i);
    if (!directive && !raw) return null;
    const closing = directive ? /^:::\s*$/ : /^<\/details>\s*$/i;
    const nestedOpening = directive
      ? /^:::\s*details(?:\s+.+)?$/i
      : /^<details(\s+open)?\s*>(?:\s*<summary>(.*?)<\/summary>)?\s*$/i;
    const open = Boolean(raw?.[1]);
    let summary = directive?.[1] || raw?.[2] || 'Details';
    let index = start + 1;
    const summaryLine = String(lines[index] ?? '').trim().match(/^<summary>(.*?)<\/summary>\s*$/i);
    if (!directive && summaryLine) {
      summary = summaryLine[1] || summary;
      index += 1;
    }
    const content = [];
    let depth = 1;
    let fence = null;
    while (index < lines.length) {
      const candidate = String(lines[index] ?? '').trim();
      const fenceMatch = candidate.match(/^(`{3,}|~{3,})/);
      if (fenceMatch) {
        const marker = fenceMatch[1];
        if (!fence) {
          fence = marker[0];
        } else if (marker[0] === fence && marker.length >= 3) {
          fence = null;
        }
        content.push(lines[index]);
        index += 1;
        continue;
      }
      if (!fence) {
        if (nestedOpening.test(candidate)) depth += 1;
        if (closing.test(candidate)) {
          depth -= 1;
          if (depth === 0) break;
        }
      }
      content.push(lines[index]);
      index += 1;
    }
    if (index >= lines.length) return null;
    return {
      html: `<details${open ? ' open' : ''}><summary>${renderInline(summary, options)}</summary>${renderBlocks(content, options, state)}</details>`,
      next: index + 1,
    };
  }

  function startsBlock(lines, index) {
    const line = String(lines[index] ?? '');
    const detailsOpen = /^\s*:::\s*details\b/i.test(line) || /^\s*<details(?:\s+open)?\s*>/i.test(line);
    return !line.trim()
      || /^\s*```/.test(line)
      || /^\s{0,3}#{1,6}\s+/.test(line)
      || /^\s{0,3}>/.test(line)
      || /^\s{0,3}(?:[-+*]|\d+[.)])\s+/.test(line)
      || /^\s{0,3}(?:---+|\*\*\*+|___+)\s*$/.test(line)
      || detailsOpen
      || (index + 1 < lines.length && line.includes('|') && isTableDivider(lines[index + 1]));
  }

  function renderBlocks(lines, options, state) {
    const output = [];
    let index = 0;
    while (index < lines.length) {
      const line = String(lines[index] ?? '');
      if (!line.trim()) {
        index += 1;
        continue;
      }
      const fence = line.match(/^\s*```\s*([^\s`]*)\s*$/);
      if (fence) {
        const language = SAFE_CLASS.test(fence[1]) ? fence[1].toLowerCase() : '';
        const code = [];
        index += 1;
        while (index < lines.length && !/^\s*```\s*$/.test(String(lines[index] ?? ''))) {
          code.push(lines[index]);
          index += 1;
        }
        if (index < lines.length) index += 1;
        output.push(`<pre><code${language ? ` class="language-${language}"` : ''}>${escapeHtml(code.join('\n'))}</code></pre>`);
        continue;
      }
      const details = readDetails(lines, index, options, state);
      if (details) {
        output.push(details.html);
        index = details.next;
        continue;
      }
      const heading = line.match(/^\s{0,3}(#{1,6})\s+(.+?)\s*#*\s*$/);
      if (heading) {
        const level = heading[1].length;
        output.push(`<h${level} id="${headingId(heading[2], state)}">${renderInline(heading[2], options)}</h${level}>`);
        index += 1;
        continue;
      }
      if (/^\s{0,3}(?:---+|\*\*\*+|___+)\s*$/.test(line)) {
        output.push('<hr>');
        index += 1;
        continue;
      }
      if (index + 1 < lines.length && line.includes('|') && isTableDivider(lines[index + 1])) {
        const headers = splitTableRow(line);
        index += 2;
        const rows = [];
        while (index < lines.length && String(lines[index] ?? '').includes('|') && String(lines[index] ?? '').trim()) {
          rows.push(splitTableRow(lines[index]));
          index += 1;
        }
        const headerHtml = headers.map(cell => `<th scope="col">${renderInline(cell, options)}</th>`).join('');
        const rowsHtml = rows.map(cells => `<tr>${headers.map((_, column) => `<td>${renderInline(cells[column] || '', options)}</td>`).join('')}</tr>`).join('');
        output.push(`<table><thead><tr>${headerHtml}</tr></thead><tbody>${rowsHtml}</tbody></table>`);
        continue;
      }
      if (/^\s{0,3}>/.test(line)) {
        const quoted = [];
        while (index < lines.length) {
          const quotedLine = String(lines[index] ?? '');
          if (/^\s{0,3}>/.test(quotedLine)) {
            quoted.push(quotedLine.replace(/^\s{0,3}>\s?/, ''));
            index += 1;
          } else if (!quotedLine.trim()) {
            quoted.push('');
            index += 1;
          } else {
            break;
          }
        }
        output.push(`<blockquote>${renderBlocks(quoted, options, state)}</blockquote>`);
        continue;
      }
      const list = renderList(lines, index, options, state);
      if (list) {
        output.push(list.html);
        index = list.next;
        continue;
      }
      const paragraph = [line.trim()];
      index += 1;
      while (index < lines.length && !startsBlock(lines, index)) {
        paragraph.push(String(lines[index] ?? '').trim());
        index += 1;
      }
      output.push(`<p>${renderInline(paragraph.join(' '), options)}</p>`);
    }
    return output.join('');
  }

  function render(markdown, options = {}) {
    const state = { headingIds: new Map() };
    const body = renderBlocks(String(markdown ?? '').replace(/\r\n?/g, '\n').split('\n'), options, state);
    return `<article class="markdown-content" data-markdown-renderer="local" tabindex="-1">${body || '<p class="markdown-empty">No documentation content is available.</p>'}</article>`;
  }

  const api = Object.freeze({ escapeHtml, normalizeArticlePath, render, renderInline, resolveLocalReference, sanitizeUrl });
  global.MaterialSandboxMarkdown = api;
  if (typeof module === 'object' && module.exports) module.exports = api;
}(typeof globalThis !== 'undefined' ? globalThis : window));
