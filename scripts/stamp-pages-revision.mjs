import fs from 'node:fs';
import path from 'node:path';

function assertSafeSourceDirectory(source, repositoryRoot) {
  const expectedSource = path.join(repositoryRoot, 'docs');
  if (source !== expectedSource) {
    throw new Error('source must be the checkout-local docs directory');
  }
  const sourceStats = fs.lstatSync(source);
  if (sourceStats.isSymbolicLink() || !sourceStats.isDirectory()) {
    throw new Error('refusing a symbolic-link, reparse, or non-directory docs source');
  }
}

function assertSafeStagingTarget(target, repositoryRoot) {
  const expectedTarget = path.join(repositoryRoot, '.pages-site');
  if (
    target !== expectedTarget
    || path.dirname(target) !== repositoryRoot
    || path.basename(target) !== '.pages-site'
  ) {
    throw new Error('target must be the checkout-local .pages-site staging directory');
  }

  if (fs.existsSync(target)) {
    const targetStats = fs.lstatSync(target);
    if (targetStats.isSymbolicLink()) {
      throw new Error('refusing to replace a symbolic-link or reparse staging target');
    }
    if (!targetStats.isDirectory()) {
      throw new Error('refusing to replace a non-directory staging target');
    }
  }
}

function assertNoLinkedTree(directory, label) {
  const visit = (current) => {
    for (const entry of fs.readdirSync(current, { withFileTypes: true })) {
      const absolute = path.join(current, entry.name);
      const stats = fs.lstatSync(absolute);
      if (entry.isSymbolicLink() || stats.isSymbolicLink()) {
        throw new Error(`refusing a symbolic-link or reparse entry in ${label}: ${absolute}`);
      }
      if (stats.isDirectory()) visit(absolute);
    }
  };
  visit(directory);
}

function findRevisionMarkers(directory) {
  const textExtensions = new Set(['.css', '.html', '.js', '.json', '.md', '.txt', '.xml', '.yaml', '.yml']);
  const findings = [];
  const visit = (current) => {
    for (const entry of fs.readdirSync(current, { withFileTypes: true })) {
      const absolute = path.join(current, entry.name);
      if (entry.isSymbolicLink()) throw new Error(`refusing a symbolic-link or reparse entry in staged Pages output: ${absolute}`);
      if (entry.isDirectory()) visit(absolute);
      else if (textExtensions.has(path.extname(entry.name).toLowerCase())) {
        if (fs.readFileSync(absolute, 'utf8').includes('PAGES_SOURCE_REVISION')) findings.push(absolute);
      }
    }
  };
  visit(directory);
  return findings;
}

const [sourceDirectory, targetDirectory, revision] = process.argv.slice(2);
if (!sourceDirectory || !targetDirectory || !/^[0-9a-f]{40}$/i.test(revision || '')) {
  console.error('usage: node scripts/stamp-pages-revision.mjs <source-docs> <target-dir> <40-char-revision>');
  process.exit(2);
}

const source = path.resolve(sourceDirectory);
const target = path.resolve(targetDirectory);
const checkout = path.resolve(process.cwd());
const expectedSource = path.join(checkout, 'docs');
const expectedTarget = path.join(checkout, '.pages-site');
if (source !== expectedSource || target !== expectedTarget) {
  console.error('source and target must be the checkout docs and .pages-site directories');
  process.exit(2);
}
try {
  assertSafeSourceDirectory(source, checkout);
  assertSafeStagingTarget(target, checkout);
  assertNoLinkedTree(source, 'Pages source');
} catch (error) {
  console.error(error.message);
  process.exit(2);
}

fs.rmSync(target, { recursive: true, force: true });
fs.cpSync(source, target, { recursive: true, dereference: false });
assertNoLinkedTree(target, 'Pages staging');

const indexPath = path.join(target, 'index.html');
const original = fs.readFileSync(indexPath, 'utf8');
const occurrences = original.split('PAGES_SOURCE_REVISION').length - 1;
if (occurrences < 3) {
  console.error(`expected at least 3 revision markers, found ${occurrences}`);
  process.exit(1);
}
const stamped = original.replaceAll('PAGES_SOURCE_REVISION', revision.toLowerCase());
if (stamped.includes('PAGES_SOURCE_REVISION')) {
  console.error('revision marker remains after stamping');
  process.exit(1);
}
fs.writeFileSync(indexPath, stamped, 'utf8');
const remainingMarkers = findRevisionMarkers(target);
if (remainingMarkers.length) {
  console.error(`revision marker remains in staged Pages output: ${remainingMarkers.join(', ')}`);
  process.exit(1);
}
console.log(`pages-revision-stamped revision=${revision.toLowerCase()} markers=${occurrences}`);
