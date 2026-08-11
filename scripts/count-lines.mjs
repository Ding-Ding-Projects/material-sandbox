#!/usr/bin/env node

import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { spawn, spawnSync } from 'node:child_process';
import { isDeepStrictEqual } from 'node:util';

const BINARY_EXTENSIONS = new Set([
  '.bmp', '.cur', '.exe', '.gif', '.ico', '.jpg', '.obj', '.png', '.zip',
]);
const EXCLUDED_PREFIXES = [
  'sandboxie/common/detours/',
  'sandboxie/common/json/',
  'sandboxieplus/mischelpers/archive/7z/',
  'sandboxieplus/qtsingleapp/',
  'sandboxieplus/uglobalhotkey/',
  'sandboxietools/common/json/',
  'sandboxietools/imbox/dc/',
  'sandboxietools/imdisk/',
  'design/',
];
const GENERATED_PATHS = new Set([
  'sandboxie/sboxhostdll/resource.h',
  'sandboxie/apps/control/resource.h',
  'sandboxie/apps/start/resource.h',
  'sandboxieplus/qsbieapi/resource.h',
  'sandboxieplus/sandman/resource.h',
  'sandboxietools/imbox/resource.h',
  'sandboxietools/minidump/resource.h',
  'sandboxietools/updutil/resource.h',
  'sandboxie/core/dll/pstore.h',
  'sandboxie/install/templates.ini',
  'sandboxieplus/mischelpers/mischelpers.pri',
  'sandboxieplus/qsbieapi/qsbieapi.pri',
  'sandboxieplus/sandman/sandman.pri',
  'sandboxieplus/sandboxieplus.pro',
]);
const WEBSITE_STYLE_PATHS = new Set([
  'docs/articles/index.json',
  'docs/assets/app.css',
  'docs/assets/app.js',
  'docs/assets/article-routes.json',
  'docs/assets/markdown-renderer.js',
  'docs/assets/regex-safety.js',
  'docs/index.html',
  'docs/regex-worker.js',
]);
const LICENSE_STYLE_PATHS = new Set([
  'installer/license.txt',
  'sandboxie/install/license.txt',
  'sandboxieplus/mischelpers/license',
  'sandboxieplus/qsbieapi/license',
  'sandboxieplus/sandman/license',
]);
const SOURCE_ROOTS = [
  'sandboxie/', 'sandboxieplus/', 'sandboxietools/', 'installer/', 'scripts/', '.github/',
];
const SOURCE_EXTENSIONS = new Set([
  '.asm', '.bat', '.c', '.cmd', '.config', '.cpp', '.ddf', '.def', '.filters',
  '.h', '.html', '.idl', '.inf', '.ini', '.iss', '.js', '.json', '.manifest',
  '.mjs', '.nsh', '.nsi', '.pro', '.props', '.ps1', '.sln', '.txt', '.vcxproj', '.yml',
]);
const LEGACY_WINDOWS_1252_PATHS = new Set([
  'installer/isl/swedish.isl',
  'sandboxie/msgs/report/report-albanian.txt',
  'sandboxie/msgs/report/report-czech.txt',
  'sandboxie/msgs/report/report-danish.txt',
  'sandboxie/msgs/report/report-dutch.txt',
  'sandboxie/msgs/report/report-estonian.txt',
  'sandboxie/msgs/report/report-finnish.txt',
  'sandboxie/msgs/report/report-hungarian.txt',
  'sandboxie/msgs/report/report-portuguese.txt',
  'sandboxie/msgs/report/report-portuguesebr.txt',
  'sandboxie/msgs/report/report-slovak.txt',
  'sandboxie/msgs/report/report-spanish.txt',
  'sandboxie/msgs/report/report-swedish.txt',
  'sandboxietools/imbox/dc/crypto_fast/amd64/xts_serpent_avx_amd64.asm',
  'sandboxietools/imbox/dc/crypto_fast/amd64/xts_serpent_sse2_amd64.asm',
  'sandboxietools/imbox/dc/crypto_fast/i386/xts_serpent_avx_i386.asm',
  'sandboxietools/imbox/dc/crypto_fast/i386/xts_serpent_sse2_i386.asm',
]);
const CATEGORY_ORDER = ['source', 'tests', 'styles', 'generated', 'excluded'];
const CATEGORY_LABELS = {
  source: 'Source',
  tests: 'Tests',
  styles: 'Styles / markup / docs',
  generated: 'Generated',
  excluded: 'Excluded / third-party / binary',
};
const POLICY = Object.freeze({
  version: 3,
  categories: CATEGORY_ORDER,
  projectCategories: ['source', 'tests', 'styles'],
  binaryExtensions: [...BINARY_EXTENSIONS],
  excludedPrefixes: EXCLUDED_PREFIXES,
  generatedPaths: [...GENERATED_PATHS],
  websiteStylePaths: [...WEBSITE_STYLE_PATHS],
  licenseStylePaths: [...LICENSE_STYLE_PATHS],
  sourceRoots: SOURCE_ROOTS,
  sourceExtensions: [...SOURCE_EXTENSIONS],
  legacyWindows1252Paths: [...LEGACY_WINDOWS_1252_PATHS],
  orderedRules: [
    'audited vendor and design prefixes, localization families, and known binary extensions are excluded',
    'the exact generated path allowlist is generated; new generated markers fail review',
    'the exact scripts test/validate/smoke families and TestCI.cmd are tests',
    'the exact site allowlist, declarative resource extensions, non-.github Markdown, and license paths are styles',
    'audited repository roots and extensions plus root config/build files are source',
  ],
  attribution: 'immutable surviving lines from git blame; exact agent identity or canonical final Co-Authored-By trailer',
  lineSemantics: 'Git LF records; a terminal LF does not add an empty line and a lone CR remains content',
  fallback: 'unmatched tracked content is review-required and fails without entering a public total',
});
const POLICY_HASH = crypto.createHash('sha256').update(JSON.stringify(POLICY)).digest('hex');
const GIT_ENV = {
  ...process.env,
  GIT_NO_REPLACE_OBJECTS: '1',
  LANG: 'C',
  LC_ALL: 'C',
  TZ: 'UTC',
};

function fail(message, code = 1) {
  console.error('line-counter failed: ' + message);
  process.exit(code);
}

function parseArguments(argv) {
  const options = {
    root: process.cwd(),
    revision: 'HEAD',
    jsonPath: '',
    markdownPath: '',
    allowDirty: false,
    verifyPath: '',
    jobs: Math.max(1, Math.min(16, Number(process.env.LINE_COUNTER_JOBS || 8))),
  };
  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index];
    const value = () => {
      index += 1;
      if (index >= argv.length) fail(argument + ' requires a value', 2);
      return argv[index];
    };
    if (argument === '--root') options.root = value();
    else if (argument === '--revision') options.revision = value();
    else if (argument === '--json') options.jsonPath = value();
    else if (argument === '--markdown') options.markdownPath = value();
    else if (argument === '--verify') options.verifyPath = value();
    else if (argument === '--jobs') options.jobs = Math.max(1, Math.min(32, Number(value())));
    else if (argument === '--allow-dirty') options.allowDirty = true;
    else if (argument === '--help' || argument === '-h') {
      console.log('usage: node scripts/count-lines.mjs [--root DIR] [--revision HEAD|FULL_SHA] [--json FILE] [--markdown FILE] [--jobs N] [--allow-dirty]');
      console.log('       node scripts/count-lines.mjs --verify FILE');
      process.exit(0);
    } else fail('unknown argument: ' + argument, 2);
  }
  if (!Number.isFinite(options.jobs)) fail('--jobs must be a number', 2);
  if (options.revision !== 'HEAD' && !/^[0-9a-f]{40,64}$/.test(options.revision)) {
    fail('--revision must be HEAD or a full commit object ID', 2);
  }
  options.root = path.resolve(options.root);
  return options;
}

function runGit(root, args, { allowFailure = false, encoding = 'utf8', input, maxBuffer = 512 * 1024 * 1024 } = {}) {
  const result = spawnSync('git', args, {
    cwd: root,
    encoding,
    env: GIT_ENV,
    input,
    maxBuffer,
    windowsHide: true,
  });
  if (result.error) throw result.error;
  if (!allowFailure && result.status !== 0) {
    const errorText = Buffer.isBuffer(result.stderr) ? result.stderr.toString('utf8') : (result.stderr || '');
    throw new Error('git ' + args.join(' ') + ' exited ' + result.status + ': ' + errorText.trim());
  }
  return result;
}

function requireRepository(root) {
  if (runGit(root, ['rev-parse', '--is-inside-work-tree']).stdout.trim() !== 'true') {
    throw new Error('root is not a Git working tree');
  }
  const shallow = runGit(root, ['rev-parse', '--is-shallow-repository']).stdout.trim();
  if (shallow !== 'false') {
    throw new Error('surviving-line attribution requires a full Git history; fetch with depth 0 before counting');
  }
}

function resolveCommit(root, revision) {
  return runGit(root, ['rev-parse', '--verify', revision + '^{commit}']).stdout.trim();
}

function repositoryIsDirty(root) {
  const worktree = runGit(root, ['diff', '--quiet'], { allowFailure: true });
  const index = runGit(root, ['diff', '--cached', '--quiet'], { allowFailure: true });
  if (![0, 1].includes(worktree.status) || ![0, 1].includes(index.status)) {
    throw new Error('could not determine repository cleanliness');
  }
  return worktree.status === 1 || index.status === 1;
}

function treeEntries(root, commit) {
  const result = runGit(root, ['ls-tree', '-rz', '-l', '--full-tree', commit], { encoding: 'buffer' });
  const records = result.stdout.toString('utf8').split('\0').filter(Boolean);
  return records.map((record) => {
    const tab = record.indexOf('\t');
    if (tab < 0) throw new Error('malformed ls-tree record');
    const header = record.slice(0, tab);
    const relativePath = record.slice(tab + 1);
    const match = /^([0-9]+) ([a-z]+) ([0-9a-f]{40,64})\s+(\d+|-)$/.exec(header);
    if (!match) throw new Error('malformed ls-tree header for ' + relativePath);
    const [, mode, type, oid, sizeText] = match;
    if (type !== 'blob' || !['100644', '100755'].includes(mode)) {
      throw new Error('unsupported tracked entry ' + relativePath + ' mode=' + mode + ' type=' + type);
    }
    return {
      mode,
      oid,
      bytes: Number(sizeText),
      relativePath: relativePath.replaceAll('\\', '/'),
    };
  }).sort((left, right) => Buffer.from(left.relativePath).compare(Buffer.from(right.relativePath)));
}

function readBlobChunks(root, entries) {
  const output = new Map();
  const unique = [...new Map(entries.map((entry) => [entry.oid, entry])).values()];
  for (let offset = 0; offset < unique.length; offset += 128) {
    const chunk = unique.slice(offset, offset + 128);
    const input = Buffer.from(chunk.map((entry) => entry.oid).join('\n') + '\n', 'ascii');
    const result = runGit(root, ['cat-file', '--batch'], { encoding: 'buffer', input });
    let cursor = 0;
    for (const expected of chunk) {
      const newline = result.stdout.indexOf(0x0a, cursor);
      if (newline < 0) throw new Error('truncated cat-file header for ' + expected.oid);
      const header = result.stdout.subarray(cursor, newline).toString('ascii');
      const match = /^([0-9a-f]{40,64}) blob (\d+)$/.exec(header);
      if (!match || match[1] !== expected.oid) throw new Error('unexpected cat-file response for ' + expected.oid);
      const size = Number(match[2]);
      const start = newline + 1;
      const end = start + size;
      if (end >= result.stdout.length || result.stdout[end] !== 0x0a) throw new Error('truncated cat-file body for ' + expected.oid);
      output.set(expected.oid, result.stdout.subarray(start, end));
      cursor = end + 1;
    }
    if (cursor !== result.stdout.length) throw new Error('unexpected trailing cat-file output');
  }
  return output;
}

function decodeText(buffer, relativePath) {
  if (buffer.length === 0) return { text: '', encoding: 'utf-8' };
  if (buffer[0] === 0xff && buffer[1] === 0xfe) {
    return { text: new TextDecoder('utf-16le', { fatal: true }).decode(buffer).replace(/^\uFEFF/, ''), encoding: 'utf-16le' };
  }
  if (buffer[0] === 0xfe && buffer[1] === 0xff) {
    return { text: new TextDecoder('utf-16be', { fatal: true }).decode(buffer).replace(/^\uFEFF/, ''), encoding: 'utf-16be' };
  }
  if (buffer.includes(0)) return null;
  try {
    return { text: new TextDecoder('utf-8', { fatal: true }).decode(buffer).replace(/^\uFEFF/, ''), encoding: 'utf-8' };
  } catch {
    if (LEGACY_WINDOWS_1252_PATHS.has(relativePath.toLowerCase())) {
      return { text: new TextDecoder('windows-1252', { fatal: true }).decode(buffer), encoding: 'windows-1252' };
    }
    return null;
  }
}

function splitGitLines(text) {
  if (text === '') return [];
  const lines = text.split('\n');
  if (text.endsWith('\n')) lines.pop();
  return lines;
}

function lineMetrics(decoded, buffer) {
  const logicalLines = splitGitLines(decoded.text);
  const blankFlags = logicalLines.map((line) => line.trim().length === 0);
  const nonblank = blankFlags.filter((blank) => !blank).length;
  if (!decoded.encoding.startsWith('utf-16')) {
    return { total: logicalLines.length, nonblank, blank: logicalLines.length - nonblank, blankFlags };
  }
  let lfBytes = 0;
  for (const value of buffer) if (value === 0x0a) lfBytes += 1;
  const total = buffer.length === 0 ? 0 : lfBytes + (buffer.at(-1) === 0x0a ? 0 : 1);
  if (nonblank > total) throw new Error('decoded UTF-16 nonblank lines exceed Git byte-line records');
  while (blankFlags.length < total) blankFlags.push(true);
  if (blankFlags.length !== total) throw new Error('decoded UTF-16 lines exceed Git byte-line records');
  return { total, nonblank, blank: total - nonblank, blankFlags };
}

function classification(relativePath, text) {
  const lower = relativePath.toLowerCase();
  const extension = path.posix.extname(lower);
  const basename = path.posix.basename(lower);

  const excludedPrefix = EXCLUDED_PREFIXES.some((prefix) => lower.startsWith(prefix));
  const localization = /^sandboxieplus\/sandman\/sandman_[^/]+[.]ts$/.test(lower)
    || /^sandboxieplus\/sandman\/troubleshooting\/(?:_lang|lang_[^/]+)[.]json$/.test(lower)
    || /^sandboxie\/msgs\/(?:sbie-|text-)[^/]+[.]txt$/.test(lower)
    || /^sandboxie\/msgs\/report\/report-[^/]+[.]txt$/.test(lower)
    || lower === 'installer/languages.iss'
    || /^installer\/isl\/[^/]+[.](?:isl|islu)$/.test(lower);
  const knownBinary = BINARY_EXTENSIONS.has(extension)
    || lower === 'design/.thumbnail'
    || lower === 'sandboxie/install/nsis_updates.zip';
  if (excludedPrefix || localization || knownBinary) {
    return 'excluded';
  }
  if (text === null) return null;
  if (GENERATED_PATHS.has(lower)) return 'generated';
  if (/(?:^|\/)(?:generated|autogen)(?:\/|$)/.test(lower)
      || /^(?:moc_|qrc_|ui_)/.test(basename)
      || basename === 'resource.h'
      || /(?:_generated|[.]generated)[.]/.test(basename)
      || extension === '.pri') {
    return null;
  }
  if (lower === 'testci.cmd'
      || /^scripts\/test-[^/]+[.]mjs$/.test(lower)
      || /^scripts\/validate-[^/]+[.](?:mjs|ps1)$/.test(lower)
      || /^scripts\/smoke-[^/]+[.]ps1$/.test(lower)) {
    return 'tests';
  }
  if (WEBSITE_STYLE_PATHS.has(lower)
      || ['.rc', '.rc2', '.qrc', '.ui', '.xml'].includes(extension)
      || (extension === '.md' && !lower.startsWith('.github/'))
      || LICENSE_STYLE_PATHS.has(lower)) {
    return 'styles';
  }
  if (['.editorconfig', '.gitattributes', '.gitignore', 'mergedbg.cmd', 'build.bat', 'build-installer.bat'].includes(lower)) {
    return 'source';
  }
  if (lower.startsWith('.github/') && ['.md', '.yml'].includes(extension)) return 'source';
  if (SOURCE_ROOTS.some((prefix) => lower.startsWith(prefix)) && SOURCE_EXTENSIONS.has(extension)) return 'source';
  return null;
}

function blankBucket(key) {
  return {
    key,
    label: CATEGORY_LABELS[key],
    files: 0,
    bytes: 0,
    total: 0,
    nonblank: 0,
    blank: 0,
    attribution: {
      agent: 0,
      agentNonblank: 0,
      agentBlank: 0,
      people: 0,
      peopleNonblank: 0,
      peopleBlank: 0,
      blamed: 0,
      unattributed: 0,
    },
  };
}

function canonicalTrailers(body) {
  const lines = body.replace(/\r\n?/g, '\n').split('\n');
  while (lines.length && lines.at(-1).trim() === '') lines.pop();
  const trailers = [];
  let index = lines.length - 1;
  while (index >= 0) {
    const match = /^([A-Za-z0-9-]+):[ \t]+(.+)$/.exec(lines[index]);
    if (!match) break;
    trailers.unshift({ key: match[1].toLowerCase(), value: match[2].trim() });
    index -= 1;
  }
  if (!trailers.length) return [];
  if (index >= 0 && lines[index].trim() !== '') return [];
  return trailers;
}

function identityIsAgent(name, email) {
  const normalizedName = name.trim();
  const normalizedEmail = email.trim().toLowerCase();
  if (normalizedName === 'Claude Fable 5' && normalizedEmail === 'noreply@anthropic.com') return true;
  if (normalizedName === 'Smoke User' && normalizedEmail === 'smoke@example.invalid') return true;
  return /\[bot\]$/i.test(normalizedName)
    && /^(?:[0-9]+\+)?[a-z0-9_.-]+\[bot\]@users[.]noreply[.]github[.]com$/i.test(normalizedEmail);
}

function trailerIdentity(value) {
  const match = /^(.*?)\s*<([^<>]+)>$/.exec(value);
  return match ? { name: match[1].trim(), email: match[2].trim() } : null;
}

function loadCommitActors(root, commit) {
  const result = runGit(root, ['log', '--no-show-signature', '--format=%H%x1f%an%x1f%ae%x1f%B%x1e', commit]);
  const actors = new Map();
  for (const record of result.stdout.split('\x1e')) {
    const cleaned = record.replace(/^\r?\n/, '').replace(/\r?\n$/, '');
    if (!cleaned.trim()) continue;
    const [rawHash, author = '', email = '', ...bodyParts] = cleaned.split('\x1f');
    const hash = rawHash.trim();
    const body = bodyParts.join('\x1f');
    const trailerAgent = canonicalTrailers(body)
      .filter((trailer) => trailer.key === 'co-authored-by')
      .map((trailer) => trailerIdentity(trailer.value))
      .some((identity) => identity && identityIsAgent(identity.name, identity.email));
    actors.set(hash, identityIsAgent(author, email) || trailerAgent ? 'agent' : 'people');
  }
  return actors;
}

function blameFile(root, commit, relativePath, actors, blankFlags) {
  return new Promise((resolve, reject) => {
    const child = spawn('git', ['-c', 'blame.ignoreRevsFile=', 'blame', '--incremental', '--root', commit, '--', relativePath], {
      cwd: root,
      env: GIT_ENV,
      windowsHide: true,
      stdio: ['ignore', 'pipe', 'pipe'],
    });
    let stdout = '';
    let stderr = '';
    child.stdout.setEncoding('utf8');
    child.stderr.setEncoding('utf8');
    child.stdout.on('data', (chunk) => { stdout += chunk; });
    child.stderr.on('data', (chunk) => { stderr += chunk; });
    child.on('error', reject);
    child.on('close', (code) => {
      if (code !== 0) return reject(new Error('git blame failed for ' + relativePath + ': ' + stderr.trim()));
      const attribution = {
        agent: 0,
        agentNonblank: 0,
        agentBlank: 0,
        people: 0,
        peopleNonblank: 0,
        peopleBlank: 0,
        blamed: 0,
        unattributed: 0,
      };
      for (const line of stdout.split(/\r?\n/)) {
        const match = /^([0-9a-f]{40,64}) \d+ (\d+) (\d+)$/.exec(line);
        if (!match) continue;
        const hash = match[1];
        const finalStart = Number(match[2]) - 1;
        const count = Number(match[3]);
        if (finalStart < 0 || finalStart + count > blankFlags.length) {
          return reject(new Error('blame range exceeds counted lines for ' + relativePath));
        }
        if (/^0+$/.test(hash)) {
          attribution.unattributed += count;
        } else if (!actors.has(hash)) {
          return reject(new Error('blame referenced unreachable commit ' + hash + ' for ' + relativePath));
        } else {
          const actor = actors.get(hash);
          let blank = 0;
          for (let index = finalStart; index < finalStart + count; index += 1) if (blankFlags[index]) blank += 1;
          attribution[actor] += count;
          attribution[actor + 'Blank'] += blank;
          attribution[actor + 'Nonblank'] += count - blank;
          attribution.blamed += count;
        }
      }
      resolve(attribution);
    });
  });
}

async function runPool(items, concurrency, worker) {
  let next = 0;
  const workers = Array.from({ length: Math.min(concurrency, items.length) }, async () => {
    while (next < items.length) {
      const index = next;
      next += 1;
      await worker(items[index], index);
    }
  });
  await Promise.all(workers);
}

function sumBuckets(buckets) {
  return buckets.reduce((sum, bucket) => ({
    files: sum.files + bucket.files,
    bytes: sum.bytes + bucket.bytes,
    total: sum.total + bucket.total,
    nonblank: sum.nonblank + bucket.nonblank,
    blank: sum.blank + bucket.blank,
    attribution: {
      agent: sum.attribution.agent + bucket.attribution.agent,
      agentNonblank: sum.attribution.agentNonblank + bucket.attribution.agentNonblank,
      agentBlank: sum.attribution.agentBlank + bucket.attribution.agentBlank,
      people: sum.attribution.people + bucket.attribution.people,
      peopleNonblank: sum.attribution.peopleNonblank + bucket.attribution.peopleNonblank,
      peopleBlank: sum.attribution.peopleBlank + bucket.attribution.peopleBlank,
      blamed: sum.attribution.blamed + bucket.attribution.blamed,
      unattributed: sum.attribution.unattributed + bucket.attribution.unattributed,
    },
  }), {
    files: 0,
    bytes: 0,
    total: 0,
    nonblank: 0,
    blank: 0,
    attribution: {
      agent: 0,
      agentNonblank: 0,
      agentBlank: 0,
      people: 0,
      peopleNonblank: 0,
      peopleBlank: 0,
      blamed: 0,
      unattributed: 0,
    },
  });
}

function validateTotals(label, actual, expected, errors) {
  for (const field of ['files', 'bytes', 'total', 'nonblank', 'blank']) {
    if (actual?.[field] !== expected[field]) errors.push(label + '.' + field + ' arithmetic mismatch');
  }
  for (const field of ['agent', 'agentNonblank', 'agentBlank', 'people', 'peopleNonblank', 'peopleBlank', 'blamed', 'unattributed']) {
    if (actual?.attribution?.[field] !== expected.attribution[field]) {
      errors.push(label + '.attribution.' + field + ' arithmetic mismatch');
    }
  }
}

function validateReport(report) {
  const errors = [];
  if (report.schema !== 3) errors.push('schema must be 3');
  if (!/^[0-9a-f]{40,64}$/.test(report.revision || '')) errors.push('revision must be a full commit object ID');
  if (!/^[0-9a-f]{40,64}$/.test(report.tree || '')) errors.push('tree must be a full tree object ID');
  if (report.shallow !== false) errors.push('shallow must be false');
  if (report.policyVersion !== POLICY.version || report.policyHash !== POLICY_HASH) errors.push('classification policy identity mismatch');
  if (!Array.isArray(report.categories) || report.categories.map(({ key }) => key).join(',') !== CATEGORY_ORDER.join(',')) {
    errors.push('category inventory must be source, tests, styles, generated, excluded');
  }
  for (const bucket of report.categories || []) {
    if (!Number.isInteger(bucket.files) || !Number.isInteger(bucket.bytes) || bucket.files < 0 || bucket.bytes < 0) {
      errors.push(bucket.key + ': invalid files or bytes');
    }
    if (bucket.total !== bucket.nonblank + bucket.blank) errors.push(bucket.key + ': total does not equal nonblank plus blank');
    const attributed = bucket.attribution.agent + bucket.attribution.people + bucket.attribution.unattributed;
    if (attributed !== bucket.total) errors.push(bucket.key + ': attribution ' + attributed + ' does not equal total ' + bucket.total);
    if (bucket.attribution.agent !== bucket.attribution.agentNonblank + bucket.attribution.agentBlank) {
      errors.push(bucket.key + ': agent lines do not equal agent nonblank plus blank');
    }
    if (bucket.attribution.people !== bucket.attribution.peopleNonblank + bucket.attribution.peopleBlank) {
      errors.push(bucket.key + ': people lines do not equal people nonblank plus blank');
    }
    if (bucket.nonblank !== bucket.attribution.agentNonblank + bucket.attribution.peopleNonblank) {
      errors.push(bucket.key + ': nonblank lines do not equal agent plus people nonblank lines');
    }
    if (bucket.blank !== bucket.attribution.agentBlank + bucket.attribution.peopleBlank) {
      errors.push(bucket.key + ': blank lines do not equal agent plus people blank lines');
    }
    if (bucket.attribution.blamed !== bucket.attribution.agent + bucket.attribution.people) {
      errors.push(bucket.key + ': blamed lines do not equal agent plus people lines');
    }
    if (bucket.attribution.unattributed !== 0) errors.push(bucket.key + ': unattributed lines must be zero');
  }
  const byKey = new Map((report.categories || []).map((bucket) => [bucket.key, bucket]));
  if (CATEGORY_ORDER.every((key) => byKey.has(key))) {
    validateTotals('project', report.project, sumBuckets(POLICY.projectCategories.map((key) => byKey.get(key))), errors);
    validateTotals('grand', report.grand, sumBuckets(CATEGORY_ORDER.map((key) => byKey.get(key))), errors);
  }
  return errors;
}

function markdown(report) {
  const rows = [
    ...report.categories,
    { ...report.project, label: 'Project total', key: 'project' },
    { ...report.grand, label: 'Grand total', key: 'grand' },
  ];
  const lines = [
    'Revision: ' + report.revision,
    'Tree: ' + report.tree,
    '',
    '| Classification | Files | Bytes | Total | Nonblank | Blank | Agent | Agent nonblank | Agent blank | People | People nonblank | People blank | Blamed | Unattributed |',
    '| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |',
  ];
  for (const row of rows) {
    const label = ['project', 'grand'].includes(row.key) ? '**' + row.label + '**' : row.label;
    lines.push('| ' + label + ' | ' + row.files + ' | ' + row.bytes + ' | ' + row.total + ' | ' + row.nonblank + ' | ' + row.blank + ' | ' + row.attribution.agent + ' | ' + row.attribution.agentNonblank + ' | ' + row.attribution.agentBlank + ' | ' + row.attribution.people + ' | ' + row.attribution.peopleNonblank + ' | ' + row.attribution.peopleBlank + ' | ' + row.attribution.blamed + ' | ' + row.attribution.unattributed + ' |');
  }
  lines.push(
    '',
    report.attributionRule,
    '',
    report.classificationPolicy,
    '',
    'Excluded-path policy: ' + report.exclusionPolicy.join('; ') + '.',
    '',
    'Policy SHA-256: ' + report.policyHash
  );
  return lines.join('\n') + '\n';
}

function writeAtomic(root, requestedPath, content) {
  if (!requestedPath) return;
  const outputPath = path.resolve(root, requestedPath);
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  const temporary = outputPath + '.tmp-' + process.pid + '-' + crypto.randomBytes(6).toString('hex');
  try {
    fs.writeFileSync(temporary, content, 'utf8');
    fs.renameSync(temporary, outputPath);
  } finally {
    if (fs.existsSync(temporary)) fs.rmSync(temporary, { force: true });
  }
}

async function buildReport(options, requestedRevision) {
  requireRepository(options.root);
  const revision = resolveCommit(options.root, requestedRevision);
  const head = resolveCommit(options.root, 'HEAD');
  if (revision !== head) throw new Error('target revision must equal the checked-out HEAD: target=' + revision + ' HEAD=' + head);
  const dirty = repositoryIsDirty(options.root);
  if (dirty && !options.allowDirty) {
    throw new Error('tracked worktree or index changes are present; commit them or use --allow-dirty for local diagnosis');
  }
  const tree = runGit(options.root, ['rev-parse', '--verify', revision + '^{tree}']).stdout.trim();
  const entries = treeEntries(options.root, revision);
  const likelyText = entries.filter((entry) => !BINARY_EXTENSIONS.has(path.posix.extname(entry.relativePath.toLowerCase())));
  const blobs = readBlobChunks(options.root, likelyText);
  const buckets = new Map(CATEGORY_ORDER.map((key) => [key, blankBucket(key)]));
  const records = [];
  const encodingWarnings = [];
  const reviewRequired = [];

  for (const entry of entries) {
    const extension = path.posix.extname(entry.relativePath.toLowerCase());
    const buffer = BINARY_EXTENSIONS.has(extension) ? null : blobs.get(entry.oid);
    const decoded = buffer ? decodeText(buffer, entry.relativePath) : null;
    const key = classification(entry.relativePath, decoded?.text ?? null);
    if (!key) {
      reviewRequired.push({
        path: entry.relativePath,
        mode: entry.mode,
        oid: entry.oid,
        bytes: entry.bytes,
        reason: decoded ? 'path is outside the audited classifier allowlist' : 'content is neither an allowlisted binary nor supported text encoding',
      });
      continue;
    }
    const metrics = decoded ? lineMetrics(decoded, buffer) : { total: 0, nonblank: 0, blank: 0, blankFlags: [] };
    const bucket = buckets.get(key);
    bucket.files += 1;
    bucket.bytes += entry.bytes;
    bucket.total += metrics.total;
    bucket.nonblank += metrics.nonblank;
    bucket.blank += metrics.blank;
    if (decoded && decoded.encoding !== 'utf-8') {
      encodingWarnings.push({ path: entry.relativePath, encoding: decoded.encoding, lineSemantics: 'Git LF byte records' });
    }
    records.push({
      relativePath: entry.relativePath,
      key,
      total: metrics.total,
      text: decoded !== null,
      blankFlags: metrics.blankFlags,
    });
  }

  if (reviewRequired.length) {
    const details = reviewRequired.slice(0, 50).map((entry) => entry.path + ' (' + entry.reason + ')').join('; ');
    throw new Error('review-required tracked paths=' + reviewRequired.length + ': ' + details);
  }

  const actors = loadCommitActors(options.root, revision);
  const blameable = records.filter((record) => record.text && record.total > 0);
  await runPool(blameable, options.jobs, async (record) => {
    const attribution = await blameFile(options.root, revision, record.relativePath, actors, record.blankFlags);
    const attributed = attribution.agent + attribution.people + attribution.unattributed;
    if (attributed !== record.total) {
      throw new Error(record.relativePath + ': blame lines ' + attributed + ' do not equal counted lines ' + record.total);
    }
    const bucket = buckets.get(record.key);
    for (const field of Object.keys(attribution)) bucket.attribution[field] += attribution[field];
  });

  const categories = CATEGORY_ORDER.map((key) => buckets.get(key));
  const report = {
    schema: 3,
    revision,
    tree,
    gitVersion: runGit(options.root, ['--version']).stdout.trim(),
    shallow: false,
    clean: !dirty,
    policyVersion: POLICY.version,
    policyHash: POLICY_HASH,
    encodingWarnings,
    categories,
    project: { key: 'project', label: 'Project total', ...sumBuckets(categories.filter(({ key }) => POLICY.projectCategories.includes(key))) },
    grand: { key: 'grand', label: 'Grand total', ...sumBuckets(categories) },
    attributionRule: 'Surviving lines are attributed from the immutable target commit with git blame. A commit is agent-written only when its exact author identity or a canonical final Co-Authored-By trailer names an approved agent or verified GitHub bot identity; all other committed lines are people-written.',
    classificationPolicy: 'Every tracked regular blob is classified exactly once by the audited vendor/historical/localization, binary, generated, test, styles/docs, and source/config allowlists. Any unmatched or ambiguously encoded path becomes review-required and fails before publication.',
    exclusionPolicy: [
      'the audited current binary extensions remain visible as excluded files and bytes with zero text lines',
      'the explicit Detours, JSON, 7z, QtSingleApp, UGlobalHotkey, DiskCryptor, ImDisk, and design prefixes are excluded from the project total',
      'the audited SandMan, troubleshooting, message, report, Inno Setup, and installer language families are excluded from the project total but retain text-line attribution',
      'the fourteen generated project/resource paths are separated from hand-written project lines and any new generated marker requires review',
    ],
  };
  const errors = validateReport(report);
  if (errors.length) throw new Error('arithmetic self-check failed: ' + errors.join('; '));
  return report;
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  if (options.verifyPath) {
    const evidencePath = path.resolve(options.root, options.verifyPath);
    const expected = JSON.parse(fs.readFileSync(evidencePath, 'utf8'));
    const evidenceErrors = validateReport(expected);
    if (evidenceErrors.length) throw new Error('evidence schema or arithmetic verification failed: ' + evidenceErrors.join('; '));
    const actual = await buildReport(options, expected.revision);
    if (!isDeepStrictEqual(actual, expected)) {
      throw new Error('evidence does not exactly match a fresh count of revision ' + expected.revision);
    }
    console.log('line-counter-evidence verified revision=' + expected.revision + ' grand=' + expected.grand.total);
    return;
  }

  const report = await buildReport(options, options.revision);
  const json = JSON.stringify(report, null, 2) + '\n';
  const markdownOutput = markdown(report);
  writeAtomic(options.root, options.jsonPath, json);
  writeAtomic(options.root, options.markdownPath, markdownOutput);
  process.stdout.write(markdownOutput);
}

main().catch((error) => fail(error.stack || error.message));
