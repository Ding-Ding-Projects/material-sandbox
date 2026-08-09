import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (relative) => fs.readFileSync(path.join(root, relative), 'utf8');
const index = read('docs/index.html');
const css = read('docs/assets/app.css');
const app = read('docs/assets/app.js');
const renderer = read('docs/assets/markdown-renderer.js');
const regexSafety = read('docs/assets/regex-safety.js');
const regexWorker = read('docs/regex-worker.js');
const manifest = JSON.parse(read('docs/articles/index.json'));
const routeRegistry = JSON.parse(read('docs/assets/article-routes.json'));

const checks = [
  ['HTML shell has a local application root', /id="app"/.test(index)],
  ['HTML shell uses the local Material stylesheet', /href="assets\/app\.css"/.test(index)],
  ['HTML shell loads the local Markdown renderer first', /src="assets\/markdown-renderer\.js"/.test(index)],
  ['HTML shell loads the local regex safety guard before application code', /src="assets\/regex-safety\.js"/.test(index)],
  ['HTML shell loads the local application module', /src="assets\/app\.js"/.test(index)],
  ['M3 primary color role exists', css.includes('--md-sys-color-primary')],
  ['M3 shape roles exist', css.includes('--md-sys-shape-xl') && css.includes('--md-sys-shape-m')],
  ['M3 elevation roles exist', css.includes('--md-sys-elevation-1') && css.includes('--md-sys-elevation-3')],
  ['M3 motion role exists', css.includes('--md-sys-motion')],
  ['dark Material color roles exist', css.includes(':root[data-theme="dark"]')],
  ['reduced motion has a CSS fallback', css.includes('prefers-reduced-motion') && css.includes('data-motion="reduced"')],
  ['touch targets are sized', css.includes('min-height: 44px')],
  ['compact actions retain 44px targets', /\.text-button \{ min-height: 44px/.test(css) && /\.tab-more \{ min-width: 44px; min-height: 44px/.test(css) && /\.toast button \{ min-height: 44px; min-width: 44px/.test(css)],
  ['compact tabs retain their overflow action', !/\.tab-more\s*\{[^}]*display:\s*none/.test(css)],
  ['visible keyboard focus exists', css.includes(':focus-visible')],
  ['responsive narrow layout exists', css.includes('@media (max-width: 620px)')],
  ['persisted local preferences exist', app.includes('localStorage') && app.includes('material-sandbox-pages-v3')],
  ['English, Cantonese, and bilingual modes exist', app.includes("language: 'en'") && app.includes("value=\"zh\"") && app.includes("value=\"bi\"")],
  ['independent funny levels exist', app.includes('funnyEnglish') && app.includes('funnyCantonese')],
  ['dialog emoji preference exists', app.includes('toggle-emoji') && app.includes('state.emoji')],
  ['School-mode behavior exists', app.includes('state.school.enabled') && app.includes('sha256')],
  ['School mode omits language, schedule, and dim-sum discovery plus prior notifications', app.includes('const FOCUSED_HIDDEN_ARTICLES') && app.includes("'pages-language-tone'") && app.includes("function visibleArticles() { return state.school.enabled ? articles.filter((article) => !FOCUSED_HIDDEN_ARTICLES.has(article.slug))") && app.includes('function visibleNotifications') && app.includes('runtimeToasts = runtimeToasts.filter((item) => !isDimSumNotice(item))') && app.includes("['language', 'funny', 'schedules']") && app.includes('if (state.school.enabled) return; notify')],
  ['renamed Focus mode replaces shipped-name history presentation', app.includes("record('settings changed', focusedModeLabel()") && !app.includes("record('settings changed', 'school mode'") && app.includes("String(entry.field).toLowerCase() === 'school mode'")],
  ['scheduled settings boundary exists', app.includes('state.schedules') && app.includes('Add local schedule')],
  ['local schedules resolve into the live presentation', app.includes('function scheduleMatches') && app.includes('function effectivePresentation') && app.includes('setInterval(() =>')],
  ['left-default four-edge tabs exist', app.includes("tabDock: 'left'") && app.includes('value="right"') && app.includes('value="top"') && app.includes('value="bottom"')],
  ['settings sections are nested workspace tabs', app.includes('const SETTINGS_SECTIONS') && app.includes('function renderSettingsTabs') && app.includes('data-settings-tab')],
  ['tabs have dynamic axis-aware accessibility', app.includes('aria-orientation') && app.includes("vertical ? 'ArrowUp' : 'ArrowLeft'")],
  ['narrow tab axis follows the rendered rail', app.includes('function tabRailIsVertical') && app.includes("matchMedia('(max-width: 900px)')")],
  ['four tab discovery searches exist', ['tab-strip', 'tab-group-', 'tab-groups', 'master-tabs'].every((token) => app.includes(token))],
  ['tab pins, groups, reordering, and bulk review exist', app.includes('togglePin') && app.includes('chooseGroup') && app.includes('moveTab') && app.includes('reviewBulkClose')],
  ['every search uses the bounded regex builder', app.includes('function renderSearch') && app.includes('MAX_PATTERN') && app.includes('isRiskyPattern')],
  ['command palette has the required shortcut', app.includes("event.key.toLowerCase() === 'f'") && app.includes('palette-dialog')],
  ['command palette exposes concrete settings destinations', app.includes("['appName', 'Display name'") && app.includes("['schedules', 'Scheduled presentation'") && app.includes('action: `setting:${id}`') && app.includes('applyPaletteAction')],
  ['appearance editor persists per target', app.includes('appearance-dialog') && app.includes('state.appearance[target]') && app.includes('saveAppearance')],
  ['color translator reports multiple formats', ['RGB', 'HSL', 'HSV', 'HWB', 'CMYK', 'OKLab'].every((token) => app.includes(token))],
  ['local history is append-only and exportable', app.includes('settingsHistory.unshift') && app.includes('export-history')],
  ['notification center persists, selects, and exports', app.includes('state.notifications') && app.includes('notificationSelection') && app.includes('notification-export')],
  ['non-blocking toast stack is announced', app.includes('toast-stack') && app.includes('aria-live="polite"')],
  ['whole-app rerenders are not a live region', !/id="app"[^>]*aria-live/i.test(index)],
  ['in-site Markdown rendering is wired', app.includes('MaterialSandboxMarkdown?.render') && renderer.includes('data-markdown-renderer="local"')],
  ['article hash routes are wired', app.includes('#/articles/') && app.includes('routeFromHash') && app.includes('articleRoutes')],
  ['article fetches stay within the deployed Pages base path', app.includes('function articleFetchPath') && app.includes('fetch(articleFetchPath(meta.path)')],
  ['preference imports are bounded and cannot export the school credential', app.includes('sanitizeState(imported.preferences, false)') && app.includes('function exportablePreferences') && app.includes("exported.school = { ...exported.school, enabled: false, hash: '' }")],
  ['appearance imports are constrained to safe values', app.includes('function safeAppearance') && app.includes('function validColor')],
  ['regex evaluation is bounded, worker-isolated for previews, and blocks nested quantifiers', app.includes('window.MaterialSandboxRegexSafety') && app.includes('function isRiskyPattern') && app.includes("new Worker('regex-worker.js')") && app.includes('REGEX_WORKER_TIMEOUT') && regexSafety.includes('MAX_VARIABLE_QUANTIFIERS = 2') && regexSafety.includes('quantified group') && regexWorker.includes('REGEX_SAMPLE_LIMIT = 512')],
  ['destructive actions use in-page two-key slider confirmation', app.includes('function renderDestructiveDialog') && app.includes('function requestDestructive') && app.includes('destructiveConfirmation.range >= 100') && app.includes('function renderResetDialog')],
  ['preset deletion, appearance reset, and preference replacement use the destructive gate', app.includes("requestDestructive('delete-preset'") && app.includes("requestDestructive('reset-appearance-target'") && app.includes("requestDestructive('import-preferences'") && app.includes("type === 'delete-preset'") && app.includes("type === 'reset-appearance-target'") && app.includes("type === 'import-preferences'")],
  ['destructive completion restores durable focus with visible fallbacks', app.includes('function queuePostRenderFocus') && app.includes('function restorePostRenderFocus') && app.includes("queueFocus('[data-action=\"save-preset\"]')") && app.includes("queueFocus('[data-action=\"open-import-preferences\"]')")],
  ['preference import retains the invoking visible workspace tab', app.includes('activeTab: activeTab()') && app.includes('state.activeTab = TAB_BY_ID[payload.activeTab] ? payload.activeTab : \'overview\'')],
  ['tab, appearance, and group menus have their own regex search', app.includes("renderSearch('tab-menu'") && app.includes("renderSearch('appearance-menu'") && app.includes("renderSearch('group-picker'")],
  ['shipped tab groups localize visible and accessible names', app.includes("Explore: '探索', Workspace: '工作台'") && app.includes('function groupNamePair') && app.includes('aria-label="${escapeHtml(textPair(`${pair.en} tabs`, `${pair.zh} 分頁`))}"')],
  ['display-name editing is a persisted setting', app.includes('data-setting="appName"') && app.includes('state.appName')],
  ['informational overlays use non-modal dialog presentation', app.includes('function dialogNeedsModal') && !app.includes("'palette-dialog', 'bulk-close-dialog', 'reset-dialog', 'destructive-dialog'")],
  ['legacy browser confirmation and prompt calls are absent', !/\bconfirm\s*\(/.test(app) && !/\bprompt\s*\(/.test(app)],
  ['public dim-sum asset is the only network image exception', app.includes('dim-sum-photos/releases/download/catalog-v1-part-003/')],
  ['installer remains honestly absent without a verified manifest', app.includes('No installer is advertised')],
  ['feature articles are catalogued', Array.isArray(manifest.articles) && manifest.articles.length === 22],
  ['article routes cover the whole catalog', Array.isArray(routeRegistry.articles) && routeRegistry.articles.length === manifest.articles.length],
];

const failures = checks.filter(([, pass]) => !pass).map(([name]) => name);
const remoteMarkup = /<(?:script|link|img)\b[^>]+\b(?:src|href)=['"]https?:/i.test(index);
if (remoteMarkup) failures.push('HTML shell includes a remote script, stylesheet, or image');
const remoteImports = /@import\s+(?:url\()?['"]?https?:/i.test(css);
if (remoteImports) failures.push('stylesheet imports a remote resource');
const networkUrls = [...app.matchAll(/https?:\/\/[^'"`\s)]+/g)].map((match) => match[0]);
if (networkUrls.some((url) => !url.startsWith('https://github.com/Ding-Ding-Projects/dim-sum-photos/releases/download/catalog-v1-part-003/'))) failures.push('application has a network URL outside the documented public dim-sum exception');

if (failures.length) {
  console.error('Pages Material validation failed');
  failures.forEach((failure) => console.error(`- ${failure}`));
  process.exit(1);
}
if (process.argv.includes('--self-test')) {
  const mutations = [
    ['local app mount', !/id="app"/.test(index.replace('id="app"', 'id="not-app"'))],
    ['local stylesheet', !/assets\/app\.css/.test(index.replace('assets/app.css', 'https://example.test/app.css'))],
    ['regex worker isolation', !app.replace("new Worker('regex-worker.js')", 'new RegExp()').includes("new Worker('regex-worker.js')")],
  ];
  const missed = mutations.filter(([, detected]) => !detected).map(([name]) => name);
  if (missed.length) { console.error(`pages-material-self-test failed: ${missed.join(', ')}`); process.exit(1); }
  console.log(`pages-material-self-test mutations=${mutations.length}`);
}
console.log(`pages-material-contract checks=${checks.length} articles=${manifest.articles.length}`);
