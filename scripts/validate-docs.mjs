import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

const root = process.cwd();
const manifestPath = path.join(root, 'docs', 'articles', 'index.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
if (!Array.isArray(manifest.articles) || manifest.articles.length === 0) throw new Error('docs manifest has no articles');
const slugs = new Set();
for (const article of manifest.articles) {
  if (!article.slug || slugs.has(article.slug)) throw new Error(`duplicate or empty article slug: ${article.slug}`);
  slugs.add(article.slug);
  const articlePath = path.resolve(path.dirname(manifestPath), article.path);
  if (!fs.existsSync(articlePath)) throw new Error(`missing article: ${articlePath}`);
  const text = fs.readFileSync(articlePath, 'utf8');
  if (!text.startsWith('# ')) throw new Error(`article has no title: ${article.slug}`);
  for (const related of article.related ?? []) if (!slugs.has(related) && !manifest.articles.some(candidate => candidate.slug === related)) throw new Error(`unknown related article: ${related}`);
}
const changelogPath = path.resolve(path.dirname(manifestPath), manifest.changelog.path);
if (!fs.existsSync(changelogPath)) throw new Error('missing changelog');
const changelog = fs.readFileSync(changelogPath, 'utf8');
if (!/^[0-9a-f]{40}$/.test(manifest.changelog.commit)) throw new Error('changelog manifest commit must be a full SHA');
if (!new RegExp(`\\b${manifest.changelog.commit}\\b`).test(changelog)) throw new Error('changelog commit is missing from changelog.md');
const commitLinks = [...changelog.matchAll(/https:\/\/github\.com\/Ding-Ding-Projects\/material-sandbox\/commit\/([0-9a-f]{40})\b/g)].map(match => match[1]);
if (commitLinks.length === 0) throw new Error('changelog has no full commit links');
if (/Commit:\s*(?:pending|`[0-9a-f]{1,39}`|[^\n]*integration pending)/i.test(changelog)) throw new Error('changelog contains pending or short commit references');
for (const sha of new Set(commitLinks)) {
  try {
    execFileSync('git', ['cat-file', '-e', `${sha}^{commit}`], { stdio: 'ignore' });
  } catch {
    throw new Error(`changelog commit does not resolve locally: ${sha}`);
  }
}
const qrc = fs.readFileSync(path.join(root, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc'), 'utf8');
for (const resource of ['Docs/material-design.md', 'Docs/contributor-build.md', 'Docs/changelog.md', 'Docs/scheduled-settings.md']) if (!qrc.includes(resource)) throw new Error(`missing Qt resource: ${resource}`);
const browserCpp = fs.readFileSync(path.join(root, 'SandboxiePlus', 'SandMan', 'Windows', 'DocumentationBrowser.cpp'), 'utf8');
for (const contract of ['m_changelogSearch', 'openChangelogRegexBuilder', 'm_changelogFrom', 'm_changelogTo', 'copyFilteredChangelog', 'exportFilteredChangelog', 'QDate::fromString']) {
  if (!browserCpp.includes(contract)) throw new Error(`changelog viewer contract missing: ${contract}`);
}
console.log(`docs-valid articles=${manifest.articles.length} changelog=${manifest.changelog.commit}`);
