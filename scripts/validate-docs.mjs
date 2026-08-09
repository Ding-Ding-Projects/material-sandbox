import fs from 'node:fs';
import path from 'node:path';

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
if (!new RegExp(`\\b${manifest.changelog.commit}\\b`).test(changelog)) throw new Error('changelog commit is missing from changelog.md');
const qrc = fs.readFileSync(path.join(root, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc'), 'utf8');
for (const resource of ['Docs/material-design.md', 'Docs/contributor-build.md', 'Docs/changelog.md', 'Docs/scheduled-settings.md']) if (!qrc.includes(resource)) throw new Error(`missing Qt resource: ${resource}`);
console.log(`docs-valid articles=${manifest.articles.length} changelog=${manifest.changelog.commit}`);
