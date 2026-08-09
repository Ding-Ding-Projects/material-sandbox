/* Material Sandbox Pages: dependency-free, local-first static documentation workspace. */
const app = document.getElementById('app');
const STORE = 'material-sandbox-pages-v3';
const RegexSafety = window.MaterialSandboxRegexSafety;
const MAX_PATTERN = RegexSafety?.MAX_PATTERN || 256;
const MAX_SAMPLE = RegexSafety?.MAX_SAMPLE || 512;
const REGEX_WORKER_TIMEOUT = 220;
const now = () => new Date().toISOString();
const escapeHtml = (value = '') => String(value).replace(/[&<>'"]/g, (char) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', "'": '&#39;', '"': '&quot;' }[char]));
const uid = () => `${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
const download = (name, value, type = 'application/json') => {
  const url = URL.createObjectURL(new Blob([value], { type: `${type};charset=utf-8` }));
  const link = document.createElement('a'); link.href = url; link.download = name; link.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
};

const DEFAULTS = {
  version: 3,
  appName: 'Sandboxie',
  activeTab: 'overview',
  language: 'en',
  theme: 'light',
  density: 'standard',
  motion: 'full',
  emoji: true,
  funnyEnglish: 2,
  funnyCantonese: 3,
  seed: '#6750a4',
  fontFamily: 'system',
  fontScale: 100,
  fontWeight: 400,
  settingsSection: 'theme',
  tabDock: 'left',
  tabOrder: ['overview', 'features', 'docs', 'settings', 'history', 'notifications', 'status'],
  pinnedTabs: ['overview'],
  closedTabs: [],
  tabGroups: {
    explore: { name: 'Explore', tabs: ['features', 'docs'], collapsed: false },
    manage: { name: 'Workspace', tabs: ['settings', 'history', 'notifications'], collapsed: false },
  },
  appearance: {},
  presets: {},
  searches: {},
  notificationSelection: [],
  notifications: [],
  settingsHistory: [],
  schedules: [],
  school: { enabled: false, label: 'School mode', hash: '' },
  hasVisited: false,
  paletteSize: 'card',
};

const TAB_DEFS = [
  ['overview', 'Overview', '總覽', 'A calm starting point'],
  ['features', 'Features', '功能', 'Every shipped capability'],
  ['docs', 'Documentation', '說明文件', 'Rendered local articles'],
  ['settings', 'Settings', '設定', 'Appearance and presentation'],
  ['history', 'History', '歷史', 'Local settings revisions'],
  ['notifications', 'Notifications', '通知中心', 'Review and export messages'],
  ['status', 'Status', '狀態', 'Evidence, release and source boundaries'],
];
const TAB_BY_ID = Object.fromEntries(TAB_DEFS.map(([id, en, zh, detail]) => [id, { id, en, zh, detail }]));
const SETTINGS_SECTIONS = [
  ['theme', 'Theme', '主題'], ['language', 'Language', '語言'], ['funny', 'Funny levels', '幽默程度'], ['emoji', 'Message decoration', '訊息裝飾'],
  ['seed', 'Seed color', '種子顏色'], ['typography', 'Typography', '文字樣式'], ['density', 'Density and motion', '密度同動畫'], ['tabs', 'Tabs and groups', '分頁同群組'],
  ['school', 'Focused mode', '專注模式'], ['schedules', 'Scheduled presentation', '排程顯示'], ['appearance', 'Appearance editor', '外觀編輯器'], ['export', 'Portability', '可攜性'],
];
const SETTINGS_SECTION_IDS = SETTINGS_SECTIONS.map(([id]) => id);
const FOCUSED_HIDDEN_ARTICLES = new Set(['dim-sum-surprise', 'pages-language-tone', 'scheduled-settings', 'settings-provenance', 'm3-shell-boundary']);
const STATIC_ARTICLES = [
  'material-design.md', 'features/appearance-editor.md', 'features/scheduled-settings.md', 'features/school-mode.md',
  'features/tab-discovery.md', 'features/settings-history.md', 'features/notifications.md', 'features/external-editor.md',
  'features/command-palette.md', 'features/dim-sum-surprise.md', 'features/color-translator.md', 'features/contributor-build-audit.md',
  'features/settings-provenance.md', 'features/build-entrypoints.md', 'features/destructive-confirmation.md', 'features/native-ci-evidence.md',
  'features/ui/m3-shell-boundary.md', 'features/editor-settings.md', 'features/changelog-viewer.md', 'features/pages-language-tone.md', 'features/pages-a11y-boundary.md',
].map((path) => ({ path, slug: path.replace(/^features\//, '').replace(/^ui\//, '').replace(/\.md$/, ''), related: [] }));

const COPY = {
  en: {
    palette: 'Command palette', search: 'Search', regex: 'Regex builder', plain: 'Plain text', settings: 'Settings',
    appearance: 'Edit appearance…', reset: 'Reset', close: 'Close', save: 'Save', export: 'Export', import: 'Import',
    all: 'All', noResults: 'No matching results.', source: 'Source', details: 'Details', overview: 'Overview',
    live: 'Local preference', copied: 'Copied to clipboard.', invalid: 'The regular expression is invalid.',
    featureSearch: 'Search features', docSearch: 'Search documentation', settingsSearch: 'Search settings',
    notificationSearch: 'Search notification history', tabSearch: 'Search tabs', groupSearch: 'Search this tab group',
    groupNameSearch: 'Search tab groups', historySearch: 'Search history',
  },
  zh: {
    palette: '指令面板', search: '搜尋', regex: '正則建構器', plain: '普通文字', settings: '設定',
    appearance: '編輯外觀…', reset: '重設', close: '關閉', save: '儲存', export: '匯出', import: '匯入',
    all: '全部', noResults: '未有相符結果。', source: '來源', details: '詳情', overview: '總覽',
    live: '本機偏好', copied: '已複製到剪貼簿。', invalid: '正則表達式無效。',
    featureSearch: '搜尋功能', docSearch: '搜尋說明文件', settingsSearch: '搜尋設定',
    notificationSearch: '搜尋通知記錄', tabSearch: '搜尋分頁', groupSearch: '搜尋此分頁群組',
    groupNameSearch: '搜尋分頁群組', historySearch: '搜尋歷史',
  },
};

let state = loadState();
let articles = STATIC_ARTICLES;
let articleRoutes = [];
let openArticle = null;
let menuState = null;
let runtimeToasts = [];
let returnFocus = null;
let returnFocusSelector = '';
let dialogAnchorSelector = '';
let activeDialogId = null;
let pendingBulkClose = null;
let groupDialogState = null;
let groupReturnDialog = null;
let resetConfirmation = { keyA: false, keyB: false, range: 0 };
let destructiveRequest = null;
let destructiveConfirmation = { keyA: false, keyB: false, range: 0 };
let destructiveReturnDialog = '';
let importOrigin = null;
let preserveDialogFocusId = '';
let postRenderFocus = null;
let lastPresentationSignature = '';
let regexWorker = null;
let regexWorkerSequence = 0;
const regexPreviewJobs = new Map();
const regexPreviewJobBySearch = new Map();
const regexPreviewResults = new Map();

function deepMerge(base, value) {
  if (Array.isArray(base)) return Array.isArray(value) ? value : base;
  if (base && typeof base === 'object') {
    const merged = { ...base };
    for (const [key, item] of Object.entries(value || {})) merged[key] = key in base ? deepMerge(base[key], item) : item;
    return merged;
  }
  return value ?? base;
}
function isRecord(value) { return Boolean(value) && typeof value === 'object' && !Array.isArray(value); }
function boundedText(value, fallback = '', limit = 120) { return typeof value === 'string' ? value.trim().slice(0, limit) : fallback; }
function boundedNumber(value, fallback, min, max) { const number = Number(value); return Number.isFinite(number) ? Math.min(max, Math.max(min, number)) : fallback; }
function enumValue(value, choices, fallback) { return choices.includes(value) ? value : fallback; }
function validColor(value, fallback) { return /^#[0-9a-f]{6}$/i.test(String(value || '')) ? String(value).toLowerCase() : fallback; }
function safeTabList(value, fallback = []) { return Array.isArray(value) ? [...new Set(value.filter((item) => TAB_BY_ID[item]))] : fallback; }
function safeAppearance(value) {
  if (!isRecord(value)) return null;
  const background = validColor(value.background, ''); const foreground = validColor(value.foreground, '');
  if (!background || !foreground) return null;
  return { background, foreground, radius: boundedNumber(value.radius, 18, 0, 40), spacing: boundedNumber(value.spacing, 20, 8, 64) };
}
function sanitizeState(candidate, allowSchoolCredential = true) {
  if (!isRecord(candidate)) throw new Error('Preferences must contain an object.');
  const clean = structuredClone(DEFAULTS);
  clean.appName = boundedText(candidate.appName, clean.appName, 60) || clean.appName;
  clean.language = enumValue(candidate.language, ['en', 'zh', 'bi'], clean.language);
  clean.theme = enumValue(candidate.theme, ['light', 'dark'], clean.theme);
  clean.density = enumValue(candidate.density, ['compact', 'standard', 'comfortable'], clean.density);
  clean.motion = enumValue(candidate.motion, ['full', 'reduced'], clean.motion);
  clean.emoji = typeof candidate.emoji === 'boolean' ? candidate.emoji : clean.emoji;
  clean.funnyEnglish = boundedNumber(candidate.funnyEnglish, clean.funnyEnglish, 1, 5);
  clean.funnyCantonese = boundedNumber(candidate.funnyCantonese, clean.funnyCantonese, 1, 5);
  clean.seed = validColor(candidate.seed, clean.seed);
  clean.fontFamily = enumValue(candidate.fontFamily, ['system', 'serif', 'mono'], clean.fontFamily);
  clean.fontScale = boundedNumber(candidate.fontScale, clean.fontScale, 85, 130);
  clean.fontWeight = enumValue(Number(candidate.fontWeight), [400, 500, 700], clean.fontWeight);
  clean.settingsSection = enumValue(candidate.settingsSection, SETTINGS_SECTION_IDS, clean.settingsSection);
  clean.tabDock = enumValue(candidate.tabDock, ['left', 'right', 'top', 'bottom'], clean.tabDock);
  clean.tabOrder = safeTabList(candidate.tabOrder, clean.tabOrder); if (!clean.tabOrder.length) clean.tabOrder = [...DEFAULTS.tabOrder];
  clean.pinnedTabs = safeTabList(candidate.pinnedTabs); clean.closedTabs = safeTabList(candidate.closedTabs);
  if (isRecord(candidate.tabGroups)) {
    clean.tabGroups = {};
    Object.entries(candidate.tabGroups).slice(0, 20).forEach(([id, group]) => { if (!/^[a-z0-9-]{1,80}$/i.test(id) || !isRecord(group)) return; clean.tabGroups[id] = { name: boundedText(group.name, 'Untitled group', 60) || 'Untitled group', tabs: safeTabList(group.tabs), collapsed: Boolean(group.collapsed) }; });
  }
  if (isRecord(candidate.appearance)) {
    clean.appearance = {};
    Object.entries(candidate.appearance).slice(0, 40).forEach(([target, value]) => { if (!/^(?:hero|app-bar|tab-rail|feature-card|preferences-card|search-card|setting-card|tab-(?:overview|features|docs|settings|history|notifications|status))$/.test(target)) return; const safe = safeAppearance(value); if (safe) clean.appearance[target] = safe; });
  }
  if (isRecord(candidate.presets)) {
    clean.presets = {};
    Object.entries(candidate.presets).slice(0, 12).forEach(([name, preset]) => { if (!isRecord(preset)) return; const safeName = boundedText(name, '', 60); if (!safeName) return; clean.presets[safeName] = { seed: validColor(preset.seed, clean.seed), theme: enumValue(preset.theme, ['light', 'dark'], clean.theme), density: enumValue(preset.density, ['compact', 'standard', 'comfortable'], clean.density), fontScale: boundedNumber(preset.fontScale, clean.fontScale, 85, 130), fontFamily: enumValue(preset.fontFamily, ['system', 'serif', 'mono'], clean.fontFamily), fontWeight: enumValue(Number(preset.fontWeight), [400, 500, 700], clean.fontWeight), appearance: isRecord(preset.appearance) ? Object.fromEntries(Object.entries(preset.appearance).map(([target, value]) => [target, safeAppearance(value)]).filter(([, value]) => value)) : {} }; });
  }
  if (isRecord(candidate.searches)) { clean.searches = {}; Object.entries(candidate.searches).slice(0, 48).forEach(([id, search]) => { if (!/^[a-z0-9-]{1,100}$/i.test(id) || !isRecord(search)) return; clean.searches[id] = { plain: boundedText(search.plain, '', MAX_PATTERN), regex: Boolean(search.regex), pattern: boundedText(search.pattern, '', MAX_PATTERN), flags: boundedText(search.flags, 'iu', 8).replace(/[^dgimsuvy]/g, ''), sample: boundedText(search.sample, '', MAX_SAMPLE) }; }); }
  if (Array.isArray(candidate.notifications)) { clean.notifications = candidate.notifications.slice(0, 300).filter(isRecord).map((item) => ({ id: boundedText(item.id, uid(), 90), at: /^\d{4}-\d{2}-\d{2}T/.test(String(item.at || '')) ? String(item.at) : now(), kind: enumValue(item.kind, ['info', 'success', 'warning', 'error'], 'info'), title: boundedText(item.title, 'Information', 100), message: boundedText(item.message, '', 600), persistent: Boolean(item.persistent), dismissed: Boolean(item.dismissed), image: String(item.image || '').startsWith('https://github.com/Ding-Ding-Projects/dim-sum-photos/releases/download/catalog-v1-part-003/') ? String(item.image) : '' })); }
  if (Array.isArray(candidate.settingsHistory)) { clean.settingsHistory = candidate.settingsHistory.slice(0, 300).filter(isRecord).map((item) => ({ id: boundedText(item.id, uid(), 90), at: /^\d{4}-\d{2}-\d{2}T/.test(String(item.at || '')) ? String(item.at) : now(), action: boundedText(item.action, 'settings changed', 80), field: boundedText(item.field, '', 120), value: boundedText(item.value, '', 240) })); }
  clean.notificationSelection = Array.isArray(candidate.notificationSelection) ? [...new Set(candidate.notificationSelection.map((item) => boundedText(item, '', 90)).filter((id) => clean.notifications.some((notice) => notice.id === id)))].slice(0, 300) : [];
  if (Array.isArray(candidate.schedules)) clean.schedules = candidate.schedules.slice(0, 24).filter(isRecord).map((item) => ({ id: /^[a-z0-9-]{1,80}$/i.test(String(item.id || '')) ? String(item.id) : uid(), label: boundedText(item.label, 'Local presentation rule', 80) || 'Local presentation rule', startDate: /^\d{4}-\d{2}-\d{2}$/.test(String(item.startDate || '')) ? String(item.startDate) : '', endDate: /^\d{4}-\d{2}-\d{2}$/.test(String(item.endDate || '')) ? String(item.endDate) : '', start: /^\d{2}:\d{2}$/.test(item.start) ? item.start : '09:00', end: /^\d{2}:\d{2}$/.test(item.end) ? item.end : '17:00', days: Array.isArray(item.days) ? item.days.filter((day) => ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'].includes(day)).slice(0, 7) : item.days === 'Every day' ? 'Every day' : 'Every day', settings: isRecord(item.settings) ? { theme: enumValue(item.settings.theme, ['light', 'dark'], clean.theme), density: enumValue(item.settings.density, ['compact', 'standard', 'comfortable'], clean.density), language: enumValue(item.settings.language, ['en', 'zh', 'bi'], clean.language) } : { theme: clean.theme, density: clean.density, language: clean.language } }));
  if (isRecord(candidate.school)) { clean.school.label = boundedText(candidate.school.label, clean.school.label, 60) || clean.school.label; clean.school.enabled = Boolean(candidate.school.enabled); clean.school.hash = allowSchoolCredential && /^[a-f0-9]{64}$/i.test(String(candidate.school.hash || '')) ? String(candidate.school.hash).toLowerCase() : ''; }
  clean.hasVisited = Boolean(candidate.hasVisited); clean.paletteSize = enumValue(candidate.paletteSize, ['card', 'full'], clean.paletteSize);
  return clean;
}
function loadState() {
  try { return sanitizeState(JSON.parse(localStorage.getItem(STORE) || '{}')); } catch { return structuredClone(DEFAULTS); }
}
function saveState() { localStorage.setItem(STORE, JSON.stringify(state)); }
function timeToMinutes(value) { const match = /^(\d{2}):(\d{2})$/.exec(String(value || '')); if (!match) return null; const hours = Number(match[1]); const minutes = Number(match[2]); return hours < 24 && minutes < 60 ? hours * 60 + minutes : null; }
function localDateKey(date) { return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${String(date.getDate()).padStart(2, '0')}`; }
function scheduleMatches(rule, date = new Date()) {
  const start = timeToMinutes(rule.start); const end = timeToMinutes(rule.end); if (start === null || end === null || start === end) return false;
  const minute = date.getHours() * 60 + date.getMinutes(); const scheduleDate = new Date(date);
  if (start > end && minute < end) scheduleDate.setDate(scheduleDate.getDate() - 1);
  const day = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'][scheduleDate.getDay()]; const dateKey = localDateKey(scheduleDate);
  if (rule.startDate && dateKey < rule.startDate) return false; if (rule.endDate && dateKey > rule.endDate) return false;
  const included = rule.days === 'Every day' || (Array.isArray(rule.days) && rule.days.includes(day));
  if (!included) return false;
  return start < end ? minute >= start && minute < end : minute >= start || minute < end;
}
function effectivePresentation(date = new Date()) {
  const effective = { theme: state.theme, density: state.density, motion: state.motion, language: state.language, fontScale: state.fontScale, fontFamily: state.fontFamily, fontWeight: state.fontWeight, seed: state.seed };
  state.schedules.filter((rule) => scheduleMatches(rule, date)).forEach((rule) => Object.assign(effective, rule.settings || {}));
  return effective;
}
function activeLanguage() { return state.school.enabled ? 'en' : effectivePresentation().language; }
function visibleArticles() { return state.school.enabled ? articles.filter((article) => !FOCUSED_HIDDEN_ARTICLES.has(article.slug)) : articles; }
function text(key) { const mode = activeLanguage(); return mode === 'zh' ? COPY.zh[key] || COPY.en[key] || key : COPY.en[key] || key; }
function bilingual(en, zh) { const mode = activeLanguage(); if (mode === 'zh') return escapeHtml(zh); if (mode === 'bi') return `<span>${escapeHtml(en)}</span><span class="brand-subtitle">${escapeHtml(zh)}</span>`; return escapeHtml(en); }
function textPair(en, zh) { const mode = activeLanguage(); return mode === 'zh' ? zh : mode === 'bi' ? `${en} · ${zh}` : en; }
function focusedModeLabel() { return boundedText(state.school.label, 'School mode', 60) || 'School mode'; }
function focusedModeDialogTitle() { const name = focusedModeLabel(); return state.school.enabled ? textPair(`Unlock ${name}`, `解鎖 ${name}`) : textPair(`Turn on ${name}`, `開啟 ${name}`); }
const STATIC_UI_ZH = {
  Workspace: '工作台', 'Pinned and ungrouped tabs': '已釘選及未分組分頁', 'Documentation workspace tabs': '說明文件工作台分頁', 'Manage tab groups': '管理分頁群組', 'No matching tabs in this group.': '此群組未有相符分頁。', 'Edit feature card appearance': '編輯功能卡外觀', 'Edit preferences card appearance': '編輯偏好卡外觀', 'Edit search card appearance': '編輯搜尋卡外觀', 'Choose light or dark Material color roles.': '選擇淺色或深色 Material 色彩角色。', 'Toggle reduced motion': '切換減少動畫', 'Toggle dialog and message emoji': '切換對話框及訊息 emoji', 'Choose seed color': '選擇種子顏色', 'Seed color as hex': '以 HEX 輸入種子顏色', 'Close appearance editor': '關閉外觀編輯器', 'Close command palette': '關閉指令面板', 'Close school mode': '關閉專注模式', 'Close tabs and groups': '關閉分頁及群組', 'Close tab review': '關閉分頁檢查', 'Close dim sum surprise': '關閉點心驚喜', 'Close group dialog': '關閉群組對話框', 'Close reset confirmation': '關閉重設確認', 'Close confirmation': '關閉確認', 'Dismiss message': '關閉訊息', Notifications: '通知', 'Select all visible notifications': '選取所有可見通知', 'Select notification': '選取通知', 'Current color ': '目前顏色：', 'Enter a valid #RRGGBB color.': '請輸入有效的 #RRGGBB 顏色。', 'Loading canonical Markdown…': '正在載入正式 Markdown…', 'Article unavailable': '文章未能使用', 'No article body was returned.': '未有返回文章內容。', 'Related articles are being mapped in the local catalog.': '相關文章正在本機目錄中對應。', 'Move up': '向上移動', 'Move down': '向下移動', 'System UI': '系統介面', Serif: '襯線體', Monospace: '等寬體', Regular: '標準', Medium: '中等', Bold: '粗體', 'Enter text or a valid pattern first': '請先輸入文字或有效模式',
};
function localizeUiString(value) {
  const source = String(value ?? ''); if (activeLanguage() === 'en') return source;
  let translated = STATIC_UI_ZH[source];
  if (!translated && source.startsWith('More actions for ')) translated = `更多動作：${source.slice('More actions for '.length)}`;
  if (!translated && source.startsWith('Search ')) translated = `搜尋 ${source.slice('Search '.length)}`;
  if (!translated && source.endsWith(' home') && source.startsWith(state.appName)) translated = `${state.appName} 首頁`;
  if (!translated && source.startsWith('Current color ')) translated = `目前顏色：${source.slice('Current color '.length)}`;
  if (!translated) return source;
  return activeLanguage() === 'bi' ? `${source} · ${translated}` : translated;
}
function localizeStaticCopy(scope) {
  if (!scope || activeLanguage() === 'en') return;
  const walker = document.createTreeWalker(scope, NodeFilter.SHOW_TEXT, { acceptNode: (node) => node.parentElement?.closest('script,style') ? NodeFilter.FILTER_REJECT : NodeFilter.FILTER_ACCEPT });
  const nodes = []; while (walker.nextNode()) nodes.push(walker.currentNode);
  nodes.forEach((node) => { const raw = node.data; const trimmed = raw.trim(); if (!trimmed) return; const translated = localizeUiString(trimmed); if (translated !== trimmed) node.data = raw.replace(trimmed, translated); });
  scope.querySelectorAll('[aria-label],[title],[placeholder]').forEach((element) => ['aria-label', 'title', 'placeholder'].forEach((attribute) => { if (!element.hasAttribute(attribute)) return; const value = element.getAttribute(attribute); const translated = localizeUiString(value); if (translated !== value) element.setAttribute(attribute, translated); }));
}
function funny(message, locale = 'en') {
  const level = locale === 'zh' ? Number(state.funnyCantonese) : Number(state.funnyEnglish);
  if (level < 3) return message;
  if (locale === 'zh') return `${message}${level > 4 ? '（介面今日有少少醒神。）' : '（講清楚之餘，鬆一鬆。）'}`;
  return `${message}${level > 4 ? ' The interface has put on its tiny party hat.' : ' Calmly recorded, with a little warmth.'}`;
}
function record(action, field, value) {
  state.settingsHistory.unshift({ id: uid(), at: now(), action, field, value: String(value).slice(0, 240) });
  state.settingsHistory = state.settingsHistory.slice(0, 300);
}
function messageTitle(kind) { return ({ info: textPair('Information', '資訊'), success: textPair('Saved', '已儲存'), warning: textPair('Attention', '請留意'), error: textPair('Problem', '出咗狀況') }[kind] || textPair('Information', '資訊')); }
function isDimSumNotice(item) { return String(item?.image || '').includes('dim-sum-photos/releases/download/') || /Matcha Har Gow|抹茶蝦餃/.test(`${item?.title || ''} ${item?.message || ''}`); }
function visibleNotifications() { return state.school.enabled ? state.notifications.filter((item) => !isDimSumNotice(item)) : state.notifications; }
function notify(message, kind = 'info', persistent = false, image = '') {
  const locale = activeLanguage() === 'zh' ? 'zh' : 'en'; const styledMessage = funny(message, locale);
  const item = { id: uid(), at: now(), kind, title: messageTitle(kind), message: styledMessage, persistent, dismissed: false, image };
  state.notifications.unshift(item); state.notifications = state.notifications.slice(0, 300); saveState();
  runtimeToasts = [item, ...runtimeToasts].slice(0, 4); renderToasts();
  if (!persistent && kind !== 'warning' && kind !== 'error') setTimeout(() => { runtimeToasts = runtimeToasts.filter((toast) => toast.id !== item.id); renderToasts(); }, 4400);
}
function byId(id) { return document.getElementById(id); }
function selectorValue(value) { return String(value).replace(/\\/g, '\\\\').replace(/"/g, '\\"'); }
function focusSelector(element) {
  if (!(element instanceof HTMLElement)) return '';
  if (element.id) return `[id="${selectorValue(element.id)}"]`;
  for (const attribute of ['data-delete-schedule', 'data-delete-preset', 'data-dismiss-notification', 'data-tab-menu', 'data-appearance-trigger', 'data-appearance', 'data-article', 'data-reopen-tab', 'data-palette-action', 'data-action']) {
    if (element.hasAttribute(attribute)) return `[${attribute}="${selectorValue(element.getAttribute(attribute))}"]`;
  }
  return '';
}
function setReturnFocus(element) { returnFocus = element instanceof HTMLElement ? element : null; returnFocusSelector = focusSelector(element); }
function restoreReturnFocus() { const target = returnFocusSelector ? document.querySelector(returnFocusSelector) : returnFocus; returnFocus = null; returnFocusSelector = ''; target?.focus?.({ preventScroll: true }); }
function queuePostRenderFocus(selector, fallback = '') { if (selector || fallback) postRenderFocus = { selector, fallback, attempts: 4 }; }
function restorePostRenderFocus() {
  const pending = postRenderFocus; if (!pending) return;
  const focus = () => {
    if (postRenderFocus !== pending) return;
    const target = (pending.selector && document.querySelector(pending.selector)) || (pending.fallback && document.querySelector(pending.fallback));
    if (target && !target.closest('[hidden]')) target.focus?.({ preventScroll: true });
    pending.attempts -= 1;
    if (pending.attempts > 0) requestAnimationFrame(focus); else if (postRenderFocus === pending) postRenderFocus = null;
  };
  requestAnimationFrame(() => requestAnimationFrame(() => requestAnimationFrame(focus)));
}
function setDialogAnchor(id, element) { dialogAnchorSelector = ['appearance-dialog', 'group-dialog'].includes(id) ? focusSelector(element) : ''; }
function positionAnchoredDialog(id) {
  if (!['appearance-dialog', 'group-dialog'].includes(id)) return;
  const dialog = byId(id); const origin = dialogAnchorSelector ? document.querySelector(dialogAnchorSelector) : null;
  if (!dialog?.open || !origin) return;
  let rect = origin.getBoundingClientRect(); if (rect.bottom < 12 || rect.top > innerHeight - 12) { origin.scrollIntoView({ block: 'nearest', inline: 'nearest', behavior: 'auto' }); rect = origin.getBoundingClientRect(); }
  const width = dialog.offsetWidth; const height = dialog.offsetHeight; const left = Math.max(12, Math.min(rect.left, innerWidth - width - 12)); const below = Math.max(12, rect.bottom + 8);
  const top = rect.bottom >= 12 && below + height <= innerHeight - 12 ? below : Math.max(12, rect.top - height - 8);
  dialog.dataset.anchored = 'true'; dialog.style.inset = 'auto'; dialog.style.left = `${left}px`; dialog.style.top = `${top}px`; dialog.style.transform = 'none';
}
function ensureSearch(id) { return (state.searches[id] ||= { plain: '', regex: false, pattern: '', flags: 'iu', sample: '' }); }
function isRiskyPattern(value) { return !RegexSafety || RegexSafety.isRiskyPattern(value); }
function getMatcher(id) {
  const search = ensureSearch(id);
  const query = search.regex ? search.pattern : search.plain;
  if (!query) return { test: () => true, status: search.regex ? textPair('Enter a pattern to search.', '請輸入搜尋模式。') : textPair('Plain text mode is active.', '普通文字模式已啟用。') };
  if (query.length > MAX_PATTERN) return { error: textPair(`Pattern is limited to ${MAX_PATTERN} characters.`, `模式最多 ${MAX_PATTERN} 個字元。`) };
  if (search.regex && isRiskyPattern(query)) return { error: textPair('This pattern is blocked because it could stall the page.', '此模式可能令頁面停頓，所以已被阻擋。') };
  try {
    const regex = search.regex ? new RegExp(query, search.flags.replace(/[^dgimsuvy]/g, '').replace(/[gy]/g, '')) : new RegExp(query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'iu');
    return { test: (value) => { regex.lastIndex = 0; return regex.test(value); }, status: search.regex ? textPair('Valid regular expression.', '正則表達式有效。') : textPair('Plain text mode is active.', '普通文字模式已啟用。') };
  } catch (error) { return { error: textPair(`Invalid pattern: ${error.message}`, `無效模式：${error.message}`) }; }
}
function resetRegexWorker() {
  regexWorker?.terminate?.(); regexWorker = null;
  for (const job of regexPreviewJobs.values()) clearTimeout(job.timeout);
  regexPreviewJobs.clear(); regexPreviewJobBySearch.clear();
}
function formatRegexPreview(result) {
  const matches = result.matches || [];
  return `${matches.length} ${textPair(matches.length === 1 ? 'sample match' : 'sample matches', matches.length === 1 ? '個範例符合' : '個範例符合')}${matches[0]?.captures?.filter(Boolean).length ? ` · ${textPair(`captures: ${matches[0].captures.filter(Boolean).join(', ')}`, `擷取：${matches[0].captures.filter(Boolean).join(', ')}`)}` : ''}`;
}
function queueRegexPreview(id, search) {
  if (typeof Worker !== 'function') return;
  const key = JSON.stringify([search.pattern, search.flags, search.sample]);
  if (regexPreviewResults.get(id)?.key === key || regexPreviewJobBySearch.get(id)?.key === key) return;
  try {
    if (!regexWorker) {
      regexWorker = new Worker('regex-worker.js');
      regexWorker.addEventListener('message', (event) => {
        const job = regexPreviewJobs.get(event.data?.id); if (!job) return;
        clearTimeout(job.timeout); regexPreviewJobs.delete(event.data.id); if (regexPreviewJobBySearch.get(job.searchId)?.id === job.id) regexPreviewJobBySearch.delete(job.searchId);
        if (job.key !== JSON.stringify([ensureSearch(job.searchId).pattern, ensureSearch(job.searchId).flags, ensureSearch(job.searchId).sample])) return;
        regexPreviewResults.set(job.searchId, { key: job.key, error: event.data.ok ? '' : event.data.error || 'Regex worker could not evaluate this pattern.', text: event.data.ok ? formatRegexPreview(event.data) : '' });
        const status = document.querySelector(`[data-regex-status="${job.searchId}"]`); if (status) { const result = regexPreviewResults.get(job.searchId); status.textContent = result.error || result.text; status.classList.toggle('error-text', Boolean(result.error)); }
      });
      regexWorker.addEventListener('error', () => resetRegexWorker());
    }
    const jobId = `regex-${++regexWorkerSequence}`; const previous = regexPreviewJobBySearch.get(id); if (previous) { clearTimeout(previous.timeout); regexPreviewJobs.delete(previous.id); regexPreviewJobBySearch.delete(id); }
    const timeout = setTimeout(() => {
      const job = regexPreviewJobs.get(jobId); if (!job) return;
      regexPreviewJobs.delete(jobId); if (regexPreviewJobBySearch.get(id)?.id === jobId) regexPreviewJobBySearch.delete(id); regexPreviewResults.set(id, { key, error: textPair('Regex preview timed out and was stopped safely.', '正則預覽逾時，已安全停止。'), text: '' }); resetRegexWorker();
      const status = document.querySelector(`[data-regex-status="${id}"]`); if (status) { status.textContent = regexPreviewResults.get(id).error; status.classList.add('error-text'); }
    }, REGEX_WORKER_TIMEOUT);
    const job = { id: jobId, searchId: id, key, timeout }; regexPreviewJobs.set(jobId, job); regexPreviewJobBySearch.set(id, job);
    regexWorker.postMessage({ id: jobId, pattern: search.pattern, flags: search.flags.replace(/[gy]/g, ''), values: [], sample: search.sample.slice(0, MAX_SAMPLE), maxMatches: 12 });
  } catch { resetRegexWorker(); }
}
function regexPreviewStatus(id, search, matcher) {
  if (!search.regex || !search.sample) return matcher.status;
  if (matcher.error) return matcher.error;
  const key = JSON.stringify([search.pattern, search.flags, search.sample]); const cached = regexPreviewResults.get(id);
  if (cached?.key === key) return cached.error || cached.text;
  queueRegexPreview(id, search);
  return textPair('Checking sample safely in a local worker…', '正在本機 worker 安全檢查範例…');
}
function renderSearch(id, label, placeholder, help = '') {
  const s = ensureSearch(id); const matcher = getMatcher(id); const sample = s.sample.slice(0, MAX_SAMPLE);
  const sampleStatus = regexPreviewStatus(id, s, matcher);
  const labels = { pattern: textPair('Pattern', '模式'), flags: textPair('Flags', '旗標'), sample: textPair('Sample text', '範例文字'), tokens: textPair('Regex construction tokens', '正則建構符號'), start: textPair('Start', '開頭'), end: textPair('End', '結尾'), class: textPair('Class', '字元類別'), group: textPair('Group', '群組'), either: textPair('Either', '或'), one: textPair('One or more', '一個或以上'), range: textPair('Range', '範圍'), copy: textPair('Copy pattern', '複製模式'), export: textPair('Export pattern', '匯出模式') };
  return `<div class="search-shell" data-search-shell="${id}">
    <label class="sr-only" for="search-${id}">${escapeHtml(label)}</label>
    <div class="search-row"><input id="search-${id}" type="search" data-search="${id}" value="${escapeHtml(s.plain)}" placeholder="${escapeHtml(localizeUiString(placeholder))}" aria-label="${escapeHtml(label)}"><button class="outlined" type="button" data-action="toggle-regex" data-search-id="${id}" aria-expanded="${s.regex}">${escapeHtml(text('regex'))}</button></div>
    ${help ? `<small>${escapeHtml(help)}</small>` : ''}
    <section id="regex-${id}" class="regex-builder" ${s.regex ? '' : 'hidden'} aria-label="${escapeHtml(text('regex'))}">
       <div class="section-heading"><div><strong>${escapeHtml(text('regex'))}</strong><p class="muted">${escapeHtml(textPair(`ECMAScript regex · bounded to ${MAX_PATTERN} pattern and ${MAX_SAMPLE} sample characters.`, `ECMAScript 正則 · 模式最多 ${MAX_PATTERN} 個字元，範例最多 ${MAX_SAMPLE} 個字元。`))}</p></div><button class="text-button" type="button" data-action="regex-plain" data-search-id="${id}">${escapeHtml(text('plain'))}</button></div>
       <div class="regex-grid"><div class="field"><label for="pattern-${id}">${escapeHtml(labels.pattern)}</label><input id="pattern-${id}" type="text" data-pattern="${id}" value="${escapeHtml(s.pattern)}" autocomplete="off"></div><div class="field"><label for="flags-${id}">${escapeHtml(labels.flags)}</label><input id="flags-${id}" type="text" data-flags="${id}" value="${escapeHtml(s.flags)}" maxlength="8"></div></div>
       <div class="token-row" aria-label="${escapeHtml(labels.tokens)}"><button type="button" data-token="${id}" data-value="^">${escapeHtml(labels.start)}</button><button type="button" data-token="${id}" data-value="$">${escapeHtml(labels.end)}</button><button type="button" data-token="${id}" data-value="[A-Za-z]">${escapeHtml(labels.class)}</button><button type="button" data-token="${id}" data-value="( )">${escapeHtml(labels.group)}</button><button type="button" data-token="${id}" data-value="|">${escapeHtml(labels.either)}</button><button type="button" data-token="${id}" data-value="+">${escapeHtml(labels.one)}</button><button type="button" data-token="${id}" data-value="{1,3}">${escapeHtml(labels.range)}</button></div>
       <div class="field"><label for="sample-${id}">${escapeHtml(labels.sample)}</label><textarea id="sample-${id}" data-sample="${id}" maxlength="${MAX_SAMPLE}" placeholder="${escapeHtml(textPair('Try the pattern against local sample text.', '用本機範例文字測試模式。'))}">${escapeHtml(sample)}</textarea></div>
      <p class="regex-result ${matcher.error ? 'error-text' : ''}" data-regex-status="${id}">${escapeHtml(matcher.error || sampleStatus || matcher.status)}</p>
       <div class="action-row"><button class="tonal" type="button" data-action="copy-regex" data-search-id="${id}">${escapeHtml(labels.copy)}</button><button class="outlined" type="button" data-action="export-regex" data-search-id="${id}">${escapeHtml(labels.export)}</button></div>
    </section>
  </div>`;
}

function activeTab() { return TAB_BY_ID[state.activeTab] ? state.activeTab : 'overview'; }
function tabText(tab) { return textPair(tab.en, tab.zh); }
function tabRailIsVertical() { return !window.matchMedia('(max-width: 900px)').matches && ['left', 'right'].includes(state.tabDock); }
function normalizeTabState() {
  const ids = TAB_DEFS.map(([id]) => id);
  state.tabOrder = [...new Set(state.tabOrder.filter((id) => ids.includes(id)).concat(ids.filter((id) => !state.tabOrder.includes(id))))];
  state.pinnedTabs = state.pinnedTabs.filter((id) => ids.includes(id));
  state.closedTabs = state.closedTabs.filter((id) => ids.includes(id));
  for (const group of Object.values(state.tabGroups)) group.tabs = group.tabs.filter((id) => ids.includes(id));
  if (!ids.includes(state.activeTab) || state.closedTabs.includes(state.activeTab)) state.activeTab = state.tabOrder.find((id) => !state.closedTabs.includes(id)) || 'overview';
}
function groupedIds() { return new Set(Object.values(state.tabGroups).flatMap((group) => group.tabs)); }
function groupNamePair(id, group) {
  const shipped = DEFAULTS.tabGroups[id]?.name;
  if (shipped && group.name === shipped) return { en: shipped, zh: ({ Explore: '探索', Workspace: '工作台' }[shipped] || shipped) };
  return { en: group.name, zh: group.name };
}
function groupLabel(id, group) { const pair = groupNamePair(id, group); return textPair(pair.en, pair.zh); }
function renderTabButton(id) {
  const tab = TAB_BY_ID[id]; const pin = state.pinnedTabs.includes(id);
  return `<div class="tab-row"><button id="tab-${id}" class="tab-button" role="tab" aria-selected="${activeTab() === id}" aria-controls="view-${id}" tabindex="${activeTab() === id ? 0 : -1}" data-tab="${id}" data-appearance="tab-${id}"><span class="tab-meta">${pin ? '<span class="pin" aria-hidden="true"></span>' : ''}<span class="tab-label">${escapeHtml(tabText(tab))}</span></span></button><button class="tab-more" aria-label="More actions for ${escapeHtml(tabText(tab))}" type="button" data-tab-menu="${id}">•••</button></div>`;
}
function renderTabRail() {
  normalizeTabState(); const matcher = getMatcher('tab-strip'); const grouped = groupedIds(); const ordered = state.tabOrder.filter((id) => !state.closedTabs.includes(id) && matcher.test(`${tabText(TAB_BY_ID[id])} ${TAB_BY_ID[id].detail}`));
  const pinned = ordered.filter((id) => state.pinnedTabs.includes(id));
  const regular = ordered.filter((id) => !state.pinnedTabs.includes(id) && !grouped.has(id));
  const groups = Object.entries(state.tabGroups).map(([id, group]) => {
    const name = groupLabel(id, group); const pair = groupNamePair(id, group); const groupMatcher = getMatcher(`tab-group-${id}`); const items = ordered.filter((tab) => group.tabs.includes(tab) && groupMatcher.test(`${tabText(TAB_BY_ID[tab])} ${TAB_BY_ID[tab].detail}`));
    return `<details class="tab-group" ${group.collapsed ? '' : 'open'} data-group="${id}"><summary data-group-toggle="${id}"><span>${escapeHtml(name)} <small>(${items.length})</small></span><span aria-hidden="true">⌄</span></summary>${renderSearch(`tab-group-${id}`, text('groupSearch'), textPair(`Search ${pair.en}`, `搜尋${pair.zh}`))}<div class="tab-group-body" role="tablist" aria-orientation="${tabRailIsVertical() ? 'vertical' : 'horizontal'}" aria-label="${escapeHtml(textPair(`${pair.en} tabs`, `${pair.zh} 分頁`))}">${items.length ? items.map(renderTabButton).join('') : `<small class="muted">${escapeHtml(textPair('No matching tabs in this group.', '呢個群組未有符合分頁。'))}</small>`}</div></details>`;
  }).join('');
  return `<aside class="tab-rail" data-appearance="tab-rail"><div class="tab-rail-head"><span class="tab-rail-title">${escapeHtml(textPair('Workspace', '工作台'))}</span><button class="icon-button" type="button" data-action="open-tab-groups" aria-label="${escapeHtml(textPair('Manage tab groups', '管理分頁群組'))}">⌘</button></div>${renderSearch('tab-strip', text('tabSearch'), textPair('Search this tab strip', '搜尋此分頁列'))}<div class="tab-list" aria-label="${escapeHtml(textPair('Documentation workspace tabs', '說明文件工作台分頁'))}"><div class="tab-group-body" role="tablist" aria-orientation="${tabRailIsVertical() ? 'vertical' : 'horizontal'}" aria-label="${escapeHtml(textPair('Pinned and ungrouped tabs', '釘選及未分組分頁'))}">${pinned.map(renderTabButton).join('')}${regular.map(renderTabButton).join('')}</div>${groups}</div></aside>`;
}

function renderOverview() {
  return `<section id="view-overview" class="view" role="tabpanel" aria-labelledby="tab-overview" tabindex="-1">
    <section class="hero" data-appearance="hero"><div><p class="eyebrow">${escapeHtml(textPair('A local-first documentation workspace', '本機優先說明文件工作台'))}</p><h1>${bilingual('A safer sandbox deserves a calmer surface.', '安全沙盒，都值得一個更平靜嘅介面。')}</h1><p class="hero-copy">${escapeHtml(funny(textPair('Explore the complete Sandboxie feature inventory, tune the presentation, and keep every verification boundary visible.', '探索完整 Sandboxie 功能、調校顯示方式，亦保留每個驗證邊界。'), activeLanguage() === 'zh' ? 'zh' : 'en'))}</p><div class="action-row"><button class="filled" type="button" data-tab="features">${escapeHtml(textPair('Explore features', '探索功能'))}</button><button class="outlined" type="button" data-tab="docs">${escapeHtml(textPair('Read in-site docs', '閱讀站內文件'))}</button></div></div><aside class="hero-status"><div><span class="badge"><span class="status-dot"></span>${escapeHtml(textPair('Static source · local preferences', '靜態來源 · 本機偏好'))}</span><strong>${escapeHtml(textPair('Evidence stays honest.', '證據一向老實。'))}</strong><p class="muted">${escapeHtml(textPair('No installer is advertised until an immutable, verified release manifest exists.', '未有不可變而已驗證的發行清單前，不會展示安裝程式。'))}</p></div><button class="text-button" type="button" data-tab="status">${escapeHtml(textPair('View evidence status', '查看證據狀態'))}</button></aside></section>
    <section class="section"><div class="section-heading"><div><h2>${bilingual('Everything is connected.', '每一樣都連得埋。')}</h2><p>${escapeHtml(textPair('The page is a functioning workspace, not a brochure: tabs, documentation, preferences, local history, notifications, and discovery all share state.', '呢個頁面係真工作台，唔係淨係宣傳頁：分頁、文件、偏好、本機歷史、通知同搜尋共用同一份狀態。'))}</p></div></div><div class="grid"><article class="card" data-appearance="feature-card"><span class="metric">${articles.length}</span><h3>${bilingual('Feature articles', '功能文章')}</h3><p>${escapeHtml(textPair('Each canonical Markdown article is available inside the same Material shell.', '每篇正式 Markdown 文件都會喺同一個 Material 外殼入面閱讀。'))}</p><div class="card-footer"><button class="text-button" type="button" data-tab="docs">${escapeHtml(textPair('Open documentation', '開啟文件'))}</button><button class="icon-button" data-appearance-trigger="feature-card" aria-label="Edit feature card appearance">⌁</button></div></article><article class="card" data-appearance="preferences-card"><span class="metric">${state.settingsHistory.length}</span><h3>${bilingual('Local revisions', '本機修訂')}</h3><p>${escapeHtml(textPair('Presentation changes are remembered locally and can be exported or reset.', '顯示設定會本機記住，又可以匯出或者重設。'))}</p><div class="card-footer"><button class="text-button" type="button" data-tab="history">${escapeHtml(textPair('Review history', '查看歷史'))}</button><button class="icon-button" data-appearance-trigger="preferences-card" aria-label="Edit preferences card appearance">⌁</button></div></article><article class="card" data-appearance="search-card"><span class="metric">${TAB_DEFS.length}</span><h3>${bilingual('Discoverable surfaces', '可搜尋介面')}</h3><p>${escapeHtml(textPair('Every search surface has its own bounded regex builder and plain-text default.', '每一個搜尋位都有自己受限制嘅正則建構器，同時預設用普通文字。'))}</p><div class="card-footer"><button class="text-button" type="button" data-action="open-palette">${escapeHtml(textPair('Open command palette', '開啟指令面板'))}</button><button class="icon-button" data-appearance-trigger="search-card" aria-label="Edit search card appearance">⌁</button></div></article></div></section>
    <section class="section split"><article class="notice"><strong>${escapeHtml(textPair('Privacy boundary', '私隱邊界'))}</strong><p>${escapeHtml(textPair('Preferences, history, schedules, and notification selections remain in this browser. This static site does not collect analytics, account tokens, or remote settings credentials.', '偏好、歷史、排程同通知選擇都只留喺呢個瀏覽器。靜態網站唔會收集分析資料、帳戶 token 或遠端設定憑證。'))}</p></article><article class="notice warning"><strong>${escapeHtml(textPair('Release boundary', '發行邊界'))}</strong><p>${escapeHtml(textPair('A static Pages deployment proves the site. It does not prove a native desktop build, installer, update feed, or runtime capture.', '靜態 Pages 部署只證明網站；唔代表原生桌面版、安裝程式、更新 feed 或執行時畫面已經驗證。'))}</p></article></section>
  </section>`;
}
function articleTitle(article) { return article.title || article.slug.replace(/[-_]/g, ' ').replace(/\b\w/g, (char) => char.toUpperCase()); }
function renderFeatures() {
  const catalog = visibleArticles(); const matcher = getMatcher('features'); const visible = catalog.filter((article) => matcher.test(`${articleTitle(article)} ${article.slug} ${article.path}`));
  return `<section id="view-features" class="view" role="tabpanel" aria-labelledby="tab-features" tabindex="-1"><div class="section-heading"><div><h2>${bilingual('Feature inventory', '功能總覽')}</h2><p>${escapeHtml(textPair('Every delivered feature has a dedicated canonical article with behavior, configuration, failure modes, security notes, verification, and suggested next reading.', '每個已交付功能都有一篇正式文章，包括行為、設定、失敗模式、安全註解、驗證方法同建議下一篇。'))}</p></div><span class="badge">${visible.length} / ${catalog.length}</span></div>${renderSearch('features', text('featureSearch'), 'Filter every feature')}<div class="grid">${visible.map((article) => `<article class="card" data-appearance="feature-card"><span class="badge">${escapeHtml(article.slug)}</span><h3>${escapeHtml(articleTitle(article))}</h3><p>${escapeHtml(textPair('Open the article inside the site shell, with related documentation kept one step away.', '喺網站外殼入面開文章，相關文件永遠只隔一步。'))}</p><div class="card-footer"><button class="text-button" type="button" data-article="${escapeHtml(article.slug)}">${escapeHtml(textPair('Read article', '閱讀文章'))}</button><button class="icon-button" data-appearance-trigger="feature-card" aria-label="Edit feature card appearance">⌁</button></div></article>`).join('') || `<div class="empty-state">${escapeHtml(text('noResults'))}</div>`}</div></section>`;
}
function renderDocs() {
  if (openArticle) return renderArticle(openArticle);
  const matcher = getMatcher('docs'); const visible = visibleArticles().filter((article) => matcher.test(`${articleTitle(article)} ${article.slug} ${article.path}`));
  return `<section id="view-docs" class="view" role="tabpanel" aria-labelledby="tab-docs" tabindex="-1"><div class="section-heading"><div><h2>${bilingual('In-site documentation', '站內說明文件')}</h2><p>${escapeHtml(textPair('Markdown is the canonical source, rendered safely inside this workspace instead of sending readers to raw files.', 'Markdown 依然係正式來源，但會喺工作台裡安全渲染，唔使叫讀者跳去原始檔。'))}</p></div></div>${renderSearch('docs', text('docSearch'), 'Search article titles and paths')}<ol class="article-list">${visible.map((article) => `<li data-article-row="${escapeHtml(article.slug)}"><div><div class="article-title">${escapeHtml(articleTitle(article))}</div><div class="article-meta">${escapeHtml(article.path)} · ${article.related?.length || 0} related article${(article.related?.length || 0) === 1 ? '' : 's'}</div></div><button class="text-button" type="button" data-article="${escapeHtml(article.slug)}">${escapeHtml(textPair('Read in site', '站內閱讀'))}</button></li>`).join('') || `<li>${escapeHtml(text('noResults'))}</li>`}</ol></section>`;
}
function canonicalArticlePath(path) { return String(path || '').replace(/^(?:\.\.\/)+/, '').replace(/^\.\//, ''); }
function siteBasePath() { const pathname = location.pathname || '/'; return pathname.endsWith('/') ? pathname : pathname.replace(/[^/]*$/, ''); }
function articleFetchPath(path) { return `${siteBasePath()}${canonicalArticlePath(path)}`; }
function renderArticleRoutes() { return visibleArticles().map((article) => ({ path: canonicalArticlePath(article.path), route: article.route || `#/articles/${article.slug}` })); }
function renderArticle(article) {
  const content = article.loading ? '<p class="muted">Loading canonical Markdown…</p>' : article.error ? `<div class="notice error"><strong>Article unavailable</strong><p>${escapeHtml(article.error)}</p></div>` : article.html || '<p class="muted">No article body was returned.</p>';
  const related = (article.related || []).map((slug) => visibleArticles().find((candidate) => candidate.slug === slug)).filter(Boolean);
  return `<section id="view-docs" class="view" role="tabpanel" aria-labelledby="tab-docs" tabindex="-1"><div class="toolbar"><button class="outlined" type="button" data-action="close-article">${escapeHtml(textPair('Back to documentation', '返回文件列表'))}</button><button class="text-button" type="button" data-action="copy-article-link">${escapeHtml(textPair('Copy article link', '複製文章連結'))}</button></div><article class="article" aria-label="${escapeHtml(articleTitle(article))}">${content}</article><aside class="suggested"><h2>${escapeHtml(textPair('Suggested articles', '建議文章'))}</h2><p>${escapeHtml(textPair('Related features and the natural next step stay inside the documentation workspace.', '相關功能同自然下一步都會留喺說明文件工作台。'))}</p><div class="action-row">${related.map((item) => `<button class="tonal" type="button" data-article="${escapeHtml(item.slug)}">${escapeHtml(articleTitle(item))}</button>`).join('') || '<span class="muted">Related articles are being mapped in the local catalog.</span>'}</div></aside></section>`;
}

function setControl(key, value, eventName = 'settings changed') {
  state[key] = value; record(eventName, key, value); saveState(); applyPresentation(); notify(funny(`${key.replace(/([A-Z])/g, ' $1').replace(/^./, (char) => char.toUpperCase())} saved locally.`, activeLanguage() === 'zh' ? 'zh' : 'en'), 'success'); renderApp();
}
function settingMatches(key, title, detail, zh = '') { const matcher = getMatcher('settings'); return matcher.test(`${key} ${title} ${zh} ${detail}`); }
function settingCard(key, title, zh, detail, content) { if (state.school.enabled && ['language', 'funny', 'schedules'].includes(key)) return ''; return settingMatches(key, title, detail, zh) ? `<article id="setting-${key}" class="setting" data-appearance="setting-card"><h3>${bilingual(title, zh)}</h3><p>${escapeHtml(textPair(detail, SETTING_DETAIL_ZH[key] || detail))}</p>${content}<div class="provenance">${escapeHtml(textPair('Current value comes from this browser’s local preference store.', '目前數值來自呢個瀏覽器嘅本機偏好儲存。'))}</div></article>` : ''; }
const SETTING_DETAIL_ZH = {
  theme: '選擇淺色或深色 Material 色彩角色。', language: '將整個外殼切換到英文、廣東話或緊湊雙語文案。', funny: '英文和廣東話各自保留已儲存的語氣程度。', emoji: '裝飾性 emoji 只會出現在訊息；標籤和動作保持事實文字。', seed: '使用連續色彩輸入並查看相等的色彩空間數值。', typography: '選擇安全的本機字體組合、大小比例和粗幼。', density: '選擇緊湊、標準或寬鬆間距，以及減少動畫偏好。', tabs: '預設靠左停靠；可從各自動作釘選、重新排序、分組、搜尋和設定分頁樣式。', school: '強制專注英文顯示，但不聲稱有共用程式憑證。', schedules: '儲存受限制的本機顯示規則；遠端來源保持在此靜態邊界之外。', appearance: '針對色彩、文字、形狀、間距、預設、匯入、匯出、每目標重設和全域重設。', export: '將所有本機顯示數值匯出或匯入為受限制 JSON 檔案。',
};
function settingsSectionFor(setting) { return setting === 'appName' ? 'theme' : SETTINGS_SECTION_IDS.includes(setting) ? setting : 'theme'; }
function renderSettingsTabs() {
  const grid = byId('view-settings')?.querySelector('.settings-grid');
  if (!grid) return;
  const sections = SETTINGS_SECTIONS.filter(([id]) => byId(`setting-${id}`));
  if (!sections.length) return;
  const active = sections.some(([id]) => id === state.settingsSection) ? state.settingsSection : sections[0][0];
  grid.querySelectorAll('.setting').forEach((panel) => { const id = panel.id.replace(/^setting-/, ''); const selected = id === active; panel.setAttribute('role', 'tabpanel'); panel.setAttribute('aria-labelledby', `settings-tab-${id}`); panel.tabIndex = -1; panel.hidden = !selected; });
  const strip = document.createElement('div'); strip.className = 'settings-tab-strip'; strip.setAttribute('role', 'tablist'); strip.setAttribute('aria-label', textPair('Presentation setting sections', '顯示設定分類'));
  strip.innerHTML = sections.map(([id, en, zh]) => `<button id="settings-tab-${id}" class="settings-tab" type="button" role="tab" aria-selected="${id === active}" aria-controls="setting-${id}" tabindex="${id === active ? 0 : -1}" data-settings-tab="${id}">${escapeHtml(textPair(en, zh))}</button>`).join('');
  grid.before(strip);
  requestAnimationFrame(() => byId(`settings-tab-${active}`)?.scrollIntoView({ block: 'nearest', inline: 'nearest' }));
}
function openSettingsSection(id, origin = null) {
  if (!SETTINGS_SECTION_IDS.includes(id)) return;
  state.settingsSection = id; saveState(); renderApp();
  requestAnimationFrame(() => { const tab = byId(`settings-tab-${id}`); tab?.focus({ preventScroll: true }); tab?.scrollIntoView({ block: 'nearest', inline: 'nearest' }); });
}
function renderScheduleRule(item) {
  const settings = item.settings || {}; const dayMode = item.days === 'Every day' ? 'every' : 'selected'; const selectedDays = Array.isArray(item.days) ? item.days : ['Mon', 'Tue', 'Wed', 'Thu', 'Fri'];
  const languageField = state.school.enabled ? '' : `<label>${escapeHtml(textPair('Language', '語言'))}<select data-schedule-field="language" data-schedule-id="${escapeHtml(item.id)}"><option value="en" ${settings.language === 'en' ? 'selected' : ''}>English</option><option value="zh" ${settings.language === 'zh' ? 'selected' : ''}>香港粵語</option><option value="bi" ${settings.language === 'bi' ? 'selected' : ''}>English · 香港粵語</option></select></label>`;
  return `<li class="schedule-rule"><div class="field"><label>${escapeHtml(textPair('Rule name', '規則名稱'))}<input type="text" value="${escapeHtml(item.label)}" maxlength="80" data-schedule-field="label" data-schedule-id="${escapeHtml(item.id)}"></label></div><div class="field-row"><label>${escapeHtml(textPair('Start date (optional)', '開始日期（可選）'))}<input type="date" value="${escapeHtml(item.startDate || '')}" data-schedule-field="startDate" data-schedule-id="${escapeHtml(item.id)}"></label><label>${escapeHtml(textPair('End date (optional)', '結束日期（可選）'))}<input type="date" value="${escapeHtml(item.endDate || '')}" data-schedule-field="endDate" data-schedule-id="${escapeHtml(item.id)}"></label></div><div class="field-row"><label>${escapeHtml(textPair('Start', '開始'))}<input type="time" value="${escapeHtml(item.start)}" data-schedule-field="start" data-schedule-id="${escapeHtml(item.id)}"></label><label>${escapeHtml(textPair('End', '結束'))}<input type="time" value="${escapeHtml(item.end)}" data-schedule-field="end" data-schedule-id="${escapeHtml(item.id)}"></label></div><div class="field"><label>${escapeHtml(textPair('Days', '日子'))}<select data-schedule-field="days" data-schedule-id="${escapeHtml(item.id)}"><option value="every" ${dayMode === 'every' ? 'selected' : ''}>${escapeHtml(textPair('Every day', '每日'))}</option><option value="selected" ${dayMode === 'selected' ? 'selected' : ''}>${escapeHtml(textPair('Selected weekdays', '選擇星期'))}</option></select></label></div>${dayMode === 'selected' ? `<fieldset class="weekday-set"><legend>${escapeHtml(textPair('Weekdays', '星期'))}</legend>${['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'].map((day) => `<label><input type="checkbox" value="${day}" ${selectedDays.includes(day) ? 'checked' : ''} data-schedule-day data-schedule-id="${escapeHtml(item.id)}">${day}</label>`).join('')}</fieldset>` : ''}<div class="field-row"><label>${escapeHtml(textPair('Theme', '主題'))}<select data-schedule-field="theme" data-schedule-id="${escapeHtml(item.id)}"><option value="light" ${settings.theme === 'light' ? 'selected' : ''}>${escapeHtml(textPair('Light', '淺色'))}</option><option value="dark" ${settings.theme === 'dark' ? 'selected' : ''}>${escapeHtml(textPair('Dark', '深色'))}</option></select></label><label>${escapeHtml(textPair('Density', '密度'))}<select data-schedule-field="density" data-schedule-id="${escapeHtml(item.id)}"><option value="compact" ${settings.density === 'compact' ? 'selected' : ''}>${escapeHtml(textPair('Compact', '緊湊'))}</option><option value="standard" ${settings.density === 'standard' ? 'selected' : ''}>${escapeHtml(textPair('Standard', '標準'))}</option><option value="comfortable" ${settings.density === 'comfortable' ? 'selected' : ''}>${escapeHtml(textPair('Comfortable', '寬鬆'))}</option></select></label>${languageField}</div><div class="action-row"><button class="text-button" type="button" data-delete-schedule="${escapeHtml(item.id)}">${escapeHtml(textPair('Remove rule', '移除規則'))}</button></div></li>`;
}
function renderSettings() {
  const themeContent = `<div id="setting-appName" class="field"><label for="app-name">${escapeHtml(textPair('Display name', '顯示名稱'))}</label><input id="app-name" type="text" maxlength="60" value="${escapeHtml(state.appName)}" data-setting="appName"><small>${escapeHtml(textPair('This changes only the local site title and messages; repository and application identifiers stay fixed.', '呢個只會改本機網站標題同訊息；儲存庫及程式識別碼保持不變。'))}</small></div><div class="field"><label for="theme-select">${escapeHtml(textPair('Color mode', '色彩模式'))}</label><select id="theme-select" data-setting="theme"><option value="light" ${state.theme === 'light' ? 'selected' : ''}>${escapeHtml(textPair('Light', '淺色'))}</option><option value="dark" ${state.theme === 'dark' ? 'selected' : ''}>${escapeHtml(textPair('Dark', '深色'))}</option></select></div>`;
  const languageContent = state.school.enabled ? `<div class="notice warning"><strong>${escapeHtml(state.school.label)}</strong><p>${escapeHtml(textPair('This locally named mode forces English and omits Cantonese, bilingual, funny-level, and dim-sum controls until unlocked.', '呢個本機改名模式會強制英文，亦會隱藏廣東話、雙語、幽默程度同點心控制，直到解鎖。'))}</p><button class="outlined" type="button" data-action="school-unlock">${escapeHtml(textPair('Unlock', '解鎖'))}</button></div>` : `<div class="field"><label for="language-select">${escapeHtml(textPair('Language mode', '語言模式'))}</label><select id="language-select" data-setting="language"><option value="en" ${state.language === 'en' ? 'selected' : ''}>English</option><option value="zh" ${state.language === 'zh' ? 'selected' : ''}>香港粵語</option><option value="bi" ${state.language === 'bi' ? 'selected' : ''}>English · 香港粵語</option></select></div>`;
  const funnyContent = state.school.enabled ? '' : `<div class="field"><label for="funny-en">${escapeHtml(textPair('English funny level', '英文幽默程度'))}: ${state.funnyEnglish}</label><input id="funny-en" type="range" min="1" max="5" value="${state.funnyEnglish}" data-setting-range="funnyEnglish"></div><div class="field"><label for="funny-zh">${escapeHtml(textPair('Cantonese funny level', '廣東話幽默程度'))}: ${state.funnyCantonese}</label><input id="funny-zh" type="range" min="1" max="5" value="${state.funnyCantonese}" data-setting-range="funnyCantonese"></div><small>${escapeHtml(textPair('Voice changes every message category—including warnings—without changing the facts.', '所有訊息類別包括警告都會改語氣，但唔會改變事實。'))}</small>`;
  const seedContent = `<div class="field-row"><input type="color" value="${escapeHtml(state.seed)}" data-seed-color aria-label="Choose seed color"><input type="text" value="${escapeHtml(state.seed)}" data-seed-text aria-label="Seed color as hex"></div>${renderColorValues(state.seed)}`;
  const typographyContent = `<div class="field"><label for="font-family">${escapeHtml(textPair('Font family', '字型'))}</label><select id="font-family" data-setting="fontFamily"><option value="system" ${state.fontFamily === 'system' ? 'selected' : ''}>System UI</option><option value="serif" ${state.fontFamily === 'serif' ? 'selected' : ''}>Serif</option><option value="mono" ${state.fontFamily === 'mono' ? 'selected' : ''}>Monospace</option></select></div><div class="field"><label for="font-scale">${escapeHtml(textPair('Size scale', '字體比例'))}: ${state.fontScale}%</label><input id="font-scale" type="range" min="85" max="130" value="${state.fontScale}" data-setting-range="fontScale"></div><div class="field"><label for="font-weight">${escapeHtml(textPair('Weight', '粗幼'))}</label><select id="font-weight" data-setting="fontWeight"><option value="400" ${Number(state.fontWeight) === 400 ? 'selected' : ''}>Regular</option><option value="500" ${Number(state.fontWeight) === 500 ? 'selected' : ''}>Medium</option><option value="700" ${Number(state.fontWeight) === 700 ? 'selected' : ''}>Bold</option></select></div>`;
  const dockContent = `<div class="field"><label for="tab-dock">${escapeHtml(textPair('Dock edge', '停靠位置'))}</label><select id="tab-dock" data-setting="tabDock"><option value="left" ${state.tabDock === 'left' ? 'selected' : ''}>${escapeHtml(textPair('Left (default)', '左邊（預設）'))}</option><option value="right" ${state.tabDock === 'right' ? 'selected' : ''}>${escapeHtml(textPair('Right', '右邊'))}</option><option value="top" ${state.tabDock === 'top' ? 'selected' : ''}>${escapeHtml(textPair('Top', '上面'))}</option><option value="bottom" ${state.tabDock === 'bottom' ? 'selected' : ''}>${escapeHtml(textPair('Bottom', '下面'))}</option></select></div><button class="outlined" type="button" data-action="open-tab-groups">${escapeHtml(textPair('Manage tabs and groups', '管理分頁同群組'))}</button>`;
  const schoolContent = `<div class="field"><label for="school-label">${escapeHtml(textPair('Your name for this mode', '你為此模式改嘅名稱'))}</label><input id="school-label" type="text" value="${escapeHtml(state.school.label)}" data-school-label maxlength="60"></div><p>${escapeHtml(textPair('This Pages-local mode is a user-experience lock, not an account-security boundary. Reset it by deleting this site’s local data.', '呢個 Pages 本機模式係使用體驗鎖，唔係帳戶安全邊界。刪除本網站本機資料便可以重設。'))}</p><button class="${state.school.enabled ? 'danger' : 'outlined'}" type="button" data-action="${state.school.enabled ? 'school-unlock' : 'school-enable'}">${escapeHtml(state.school.enabled ? textPair('Unlock mode', '解鎖模式') : textPair('Turn on mode', '開啟模式'))}</button>`;
  const scheduleContent = `<p>${escapeHtml(textPair('Local rules are evaluated every 30 seconds in this browser’s local timezone. Later matching rules win; start/end dates are inclusive, and cross-midnight windows keep the originating weekday until their end time. External API and Home Assistant sources remain outside static Pages.', '本機規則會按此瀏覽器本地時區每 30 秒評估；之後符合的規則優先；開始／結束日期包括當日，跨午夜時段會沿用開始嗰日，直到結束時間。外部 API 同 Home Assistant 來源仍然不會喺靜態 Pages 使用。'))}</p><button class="outlined" type="button" data-action="add-schedule">${escapeHtml(textPair('Add local schedule', '加入本機排程'))}</button>${state.schedules.length ? `<ul class="article-list">${state.schedules.map(renderScheduleRule).join('')}</ul>` : ''}`;
  const presetRows = Object.keys(state.presets).map((name) => `<div class="switch-row"><span>${escapeHtml(name)}</span><div class="action-row"><button class="text-button" type="button" data-apply-preset="${escapeHtml(name)}">${escapeHtml(textPair('Apply', '套用'))}</button><button class="text-button" type="button" data-delete-preset="${escapeHtml(name)}">${escapeHtml(textPair('Delete', '刪除'))}</button></div></div>`).join('');
  const appearanceContent = `<p>${escapeHtml(textPair('Right-click any marked card or tab—or use the visible edit buttons—to change its local surface, text, radius, and spacing. Presets are exportable, applicable, and resettable.', '喺任何標記卡片或分頁按右鍵，或者用顯示編輯掣，就可以調校本機背景、文字、圓角同間距。預設可以匯出、套用同重設。'))}</p><div class="action-row"><button class="tonal" type="button" data-action="open-appearance" data-target="hero">${escapeHtml(text('appearance'))}</button><button class="outlined" type="button" data-action="save-preset">${escapeHtml(textPair('Save preset', '儲存預設'))}</button></div>${presetRows ? `<div class="stack">${presetRows}</div>` : ''}`;
  const exportContent = `<div class="action-row"><button class="tonal" type="button" data-action="export-preferences">${escapeHtml(textPair('Export preferences', '匯出偏好'))}</button><button class="outlined" type="button" data-action="open-import-preferences">${escapeHtml(textPair('Import preferences', '匯入偏好'))}</button><input id="import-preferences" type="file" accept="application/json" data-action="import-preferences" hidden></div><button class="text-button" type="button" data-action="reset-preferences">${escapeHtml(textPair('Reset all local preferences', '重設所有本機偏好'))}</button>`;
  return `<section id="view-settings" class="view" role="tabpanel" aria-labelledby="tab-settings" tabindex="-1"><div class="section-heading"><div><h2>${bilingual('Presentation settings', '顯示設定')}</h2><p>${escapeHtml(textPair('Every control explains its behavior and records where the current value comes from. This site’s values are device-local by design.', '每個控制都會解釋行為，同時說明目前數值由邊度來。網站數值按設計只留喺本機。'))}</p></div></div>${renderSearch('settings', text('settingsSearch'), 'Search this settings surface')}<div class="settings-grid">${settingCard('theme', 'Theme', '主題', 'Choose light or dark Material color roles.', themeContent)}${settingCard('language', 'Language', '語言', 'Switch the entire shell between English, Cantonese, and compact bilingual copy.', languageContent)}${settingCard('funny', 'Funny levels', '幽默程度', 'English and Cantonese each keep their own persisted voice level.', funnyContent)}${settingCard('emoji', 'Message decoration', '訊息裝飾', 'Decorative emoji appear in messages only; labels and actions remain factual text.', `<div class="switch-row"><span>${escapeHtml(textPair('Show emojis in dialogs and messages', '喺對話框同訊息顯示 emoji'))}</span><button class="switch" type="button" aria-pressed="${state.emoji}" data-action="toggle-emoji" aria-label="Toggle dialog and message emoji"></button></div>`)}${settingCard('seed', 'Seed color and translator', '種子顏色同轉換器', 'Use a continuous color input and inspect equivalent color-space values.', seedContent)}${settingCard('typography', 'Typography', '文字樣式', 'Choose a safe local font stack, size scale, and weight.', typographyContent)}${settingCard('density', 'Density and motion', '密度同動畫', 'Choose compact, standard, or comfortable spacing and a reduced-motion preference.', `<div class="field"><label for="density">${escapeHtml(textPair('Density', '密度'))}</label><select id="density" data-setting="density"><option value="compact" ${state.density === 'compact' ? 'selected' : ''}>${escapeHtml(textPair('Compact', '緊湊'))}</option><option value="standard" ${state.density === 'standard' ? 'selected' : ''}>${escapeHtml(textPair('Standard', '標準'))}</option><option value="comfortable" ${state.density === 'comfortable' ? 'selected' : ''}>${escapeHtml(textPair('Comfortable', '寬鬆'))}</option></select></div><div class="switch-row"><span>${escapeHtml(textPair('Reduce motion', '減少動畫'))}</span><button class="switch" type="button" aria-pressed="${state.motion === 'reduced'}" data-action="toggle-motion" aria-label="Toggle reduced motion"></button></div>`)}${settingCard('tabs', 'Tabs and groups', '分頁同群組', 'Left-docked by default. Pin, reorder, group, search, and style tabs from their own actions.', dockContent)}${settingCard('school', state.school.label || 'School mode', state.school.label || '校園模式', 'Force a focused English-only presentation without claiming a shared app credential.', schoolContent)}${settingCard('schedules', 'Scheduled presentation', '排程顯示', 'Store bounded local display rules and keep remote sources outside this static boundary.', scheduleContent)}${settingCard('appearance', 'Appearance editor', '外觀編輯器', 'Targeted colors, typography, shape, spacing, presets, import, export, per-target reset, and global reset.', appearanceContent)}${settingCard('export', 'Portability', '可攜性', 'Export or import all local presentation values as a bounded JSON file.', exportContent)}</div></section>`;
}
function renderColorValues(hex) {
  const rgb = hexToRgb(hex); if (!rgb) return '<p class="error-text">Enter a valid #RRGGBB color.</p>';
  const hsl = rgbToHsl(rgb); const hsv = hslToHsv(hsl); const cmyk = rgbToCmyk(rgb); const oklab = rgbToOklab(rgb); const contrast = contrastRatio(rgb, state.theme === 'dark' ? { r: 20, g: 18, b: 24 } : { r: 255, g: 251, b: 254 });
  return `<div class="color-preview"><div class="color-swatch" style="background:${escapeHtml(hex)}" aria-label="Current color ${escapeHtml(hex)}"></div><div class="color-values"><span>HEX ${escapeHtml(hex.toUpperCase())}</span><span>RGB ${rgb.r}, ${rgb.g}, ${rgb.b}</span><span>HSL ${hsl.h}° ${hsl.s}% ${hsl.l}%</span><span>HSV ${hsv.h}° ${hsv.s}% ${hsv.v}%</span><span>HWB ${hsl.h}° ${hsv.w}% ${hsv.b}%</span><span>CMYK ${cmyk.c}% ${cmyk.m}% ${cmyk.y}% ${cmyk.k}%</span><span>OKLab ${oklab.l} ${oklab.a} ${oklab.b}</span><span>Contrast ${contrast}:1</span></div></div>`;
}
function renderHistory() {
  const matcher = getMatcher('history'); const day = ensureSearch('history').day || ''; const action = ensureSearch('history').action || 'all';
  const entries = state.settingsHistory.map((entry) => state.school.enabled && String(entry.field).toLowerCase() === 'school mode' ? { ...entry, field: focusedModeLabel() } : entry);
  const visible = entries.filter((entry) => matcher.test(`${entry.action} ${entry.field} ${entry.value}`) && (!day || entry.at.startsWith(day)) && (action === 'all' || entry.action === action));
  const actions = [...new Set(entries.map((entry) => entry.action))];
  return `<section id="view-history" class="view" role="tabpanel" aria-labelledby="tab-history" tabindex="-1"><div class="section-heading"><div><h2>${bilingual('Local settings history', '本機設定歷史')}</h2><p>${escapeHtml(textPair('Each meaningful change becomes an append-only local revision. Restoring a snapshot records another revision rather than rewriting history.', '每次有意義改動都會成為本機追加式修訂。回復快照會新增一個修訂，而唔係改寫歷史。'))}</p></div><button class="outlined" type="button" data-action="export-history">${escapeHtml(textPair('Export filtered history', '匯出已篩選歷史'))}</button></div>${renderSearch('history', text('historySearch'), 'Search action, field, or value')}<div class="field-row"><div class="field"><label for="history-day">${escapeHtml(textPair('Date', '日期'))}</label><input id="history-day" type="date" data-history-day value="${escapeHtml(day)}"></div><div class="field"><label for="history-action">${escapeHtml(textPair('Action', '動作'))}</label><select id="history-action" data-history-action><option value="all">${escapeHtml(text('all'))}</option>${actions.map((item) => `<option value="${escapeHtml(item)}" ${action === item ? 'selected' : ''}>${escapeHtml(item)}</option>`).join('')}</select></div></div>${visible.length ? `<div class="table-wrap"><table><thead><tr><th>${escapeHtml(textPair('When', '時間'))}</th><th>${escapeHtml(textPair('Action', '動作'))}</th><th>${escapeHtml(textPair('Setting', '設定'))}</th><th>${escapeHtml(textPair('Value', '數值'))}</th></tr></thead><tbody>${visible.map((entry) => `<tr><td>${escapeHtml(formatDate(entry.at))}</td><td>${escapeHtml(entry.action)}</td><td>${escapeHtml(entry.field)}</td><td>${escapeHtml(entry.value)}</td></tr>`).join('')}</tbody></table></div>` : `<div class="empty-state">${escapeHtml(textPair('No local revisions match this filter.', '未有本機修訂符合呢個篩選。'))}</div>`}</section>`;
}
function renderNotifications() {
  const matcher = getMatcher('notifications'); const visible = visibleNotifications().filter((item) => !item.dismissed && matcher.test(`${item.title} ${item.message} ${item.kind}`)); const selected = new Set(state.notificationSelection);
  return `<section id="view-notifications" class="view" role="tabpanel" aria-labelledby="tab-notifications" tabindex="-1"><div class="section-heading"><div><h2>${bilingual('Notification center', '通知中心')}</h2><p>${escapeHtml(textPair('Messages stack without blocking work. Warning and error records persist until dismissed; history can be searched, selected, exported, and reviewed.', '訊息會疊住顯示而唔會阻住工作。警告同錯誤會留到手動關閉；記錄可以搜尋、選取、匯出同查看。'))}</p></div><div class="action-row"><button class="outlined" type="button" data-action="notification-select-all">${escapeHtml(textPair('Select all shown', '選取所有顯示項目'))}</button><button class="outlined" type="button" data-action="notification-invert">${escapeHtml(textPair('Invert', '反選'))}</button><button class="danger" type="button" data-action="notification-dismiss-selected">${escapeHtml(textPair('Dismiss selected', '關閉已選項目'))}</button><button class="tonal" type="button" data-action="notification-export">${escapeHtml(textPair('Export selection', '匯出選取項目'))}</button></div></div>${renderSearch('notifications', text('notificationSearch'), 'Search notification history')}${visible.length ? `<div class="table-wrap"><table><thead><tr><th class="select-cell"><input type="checkbox" aria-label="Select all visible notifications" data-action="notification-select-all-check" ${visible.length && visible.every((item) => selected.has(item.id)) ? 'checked' : ''}></th><th>${escapeHtml(textPair('Message', '訊息'))}</th><th>${escapeHtml(textPair('Kind', '種類'))}</th><th>${escapeHtml(textPair('When', '時間'))}</th><th>${escapeHtml(textPair('Action', '動作'))}</th></tr></thead><tbody>${visible.map((item) => `<tr><td><input type="checkbox" data-notification-select="${item.id}" ${selected.has(item.id) ? 'checked' : ''} aria-label="Select notification"></td><td><strong>${escapeHtml(state.emoji ? notificationGlyph(item.kind) : '')} ${escapeHtml(item.title)}</strong><br><span class="muted">${escapeHtml(item.message)}</span></td><td>${escapeHtml(item.kind)}</td><td>${escapeHtml(formatDate(item.at))}</td><td><button class="text-button" type="button" data-dismiss-notification="${item.id}">${escapeHtml(textPair('Dismiss', '關閉'))}</button></td></tr>`).join('')}</tbody></table></div>` : `<div class="empty-state">${escapeHtml(textPair('No active notifications match this filter.', '未有已啟用通知符合呢個篩選。'))}</div>`}</section>`;
}
function renderStatus() {
  const dimSumStatus = state.school.enabled ? '' : `<section class="section"><article class="notice"><strong>${escapeHtml(textPair('Dim sum surprise', '點心驚喜'))}</strong><p>${escapeHtml(textPair('After the first visit, this site makes one 10% non-blocking draw per launch. Any photo comes directly from the public dim-sum catalog release; it is neither generated nor copied into this repository.', '首次造訪之後，網站每次開啟只作一次 10% 非阻塞抽選。圖片直接來自公開點心目錄發行版，唔會生成或複製入此儲存庫。'))}</p><button class="outlined" type="button" data-action="show-dim-sum">${escapeHtml(textPair('Preview the public-catalog boundary', '預覽公開目錄邊界'))}</button></article></section>`;
  return `<section id="view-status" class="view" role="tabpanel" aria-labelledby="tab-status" tabindex="-1"><div class="section-heading"><div><h2>${bilingual('Evidence status', '證據狀態')}</h2><p>${escapeHtml(textPair('The site distinguishes what its static surface proves from native or release evidence it cannot truthfully invent.', '網站會清楚分開靜態介面證明咗嘅事，同原生或發行證據未能老作嘅事。'))}</p></div></div><div class="grid"><article class="card"><span class="badge"><span class="status-dot"></span>${escapeHtml(textPair('Static Pages source', '靜態 Pages 來源'))}</span><h3>${bilingual('This workspace', '呢個工作台')}</h3><p>${escapeHtml(textPair('The hosted static documentation, browser-local preferences, and visible controls are within this site’s evidence boundary.', '託管靜態文件、瀏覽器本機偏好同可見控制都喺網站證據邊界之內。'))}</p></article><article class="card"><span class="badge">${escapeHtml(textPair('No verified installer', '未驗證安裝程式'))}</span><h3>${bilingual('Downloads stay absent', '下載保持不顯示')}</h3><p>${escapeHtml(textPair('No installer is advertised until CI supplies an immutable, validated release manifest and asset URL.', '直到 CI 提供不可變、已驗證嘅發行清單同資產 URL，網站都唔會展示安裝程式。'))}</p></article><article class="card"><span class="badge">${escapeHtml(textPair('Runtime is separate', '執行時係另一回事'))}</span><h3>${bilingual('Native application proof', '原生程式證明')}</h3><p>${escapeHtml(textPair('A Pages deployment is not presented as proof that Qt/MSVC produced or ran a current desktop application.', 'Pages 部署唔會當成 Qt/MSVC 已經成功建立或執行目前桌面程式嘅證明。'))}</p></article></div>${dimSumStatus}</section>`;
}

function renderMain() {
  const views = [renderOverview(), renderFeatures(), renderDocs(), renderSettings(), renderHistory(), renderNotifications(), renderStatus()];
  return views.map((view) => view.replace(/class="view"/, `class="view" ${view.includes(`id="view-${activeTab()}"`) ? '' : 'hidden'}`)).join('');
}
function renderApp() {
  const dialogToRestore = activeDialogId;
  const preserveDialogFocus = dialogToRestore && preserveDialogFocusId === dialogToRestore;
  preserveDialogFocusId = '';
  document.documentElement.lang = activeLanguage() === 'zh' ? 'zh-Hant' : 'en';
  document.title = `${state.appName} · ${textPair('Material Design 3 workspace', 'Material Design 3 工作台')}`;
  app.innerHTML = `<main class="app-shell"><header class="top-app-bar" data-appearance="app-bar"><a class="brand" href="#overview" data-tab="overview" aria-label="${escapeHtml(state.appName)} home"><span class="brand-mark" aria-hidden="true">S</span><span><span class="brand-name">${escapeHtml(state.appName)}</span><span class="brand-subtitle">${bilingual('Material Design 3 documentation workspace', 'Material Design 3 說明文件工作台')}</span></span></a><div class="top-actions"><span class="badge">${escapeHtml(textPair('Local-first', '本機優先'))}</span><button class="outlined" type="button" data-action="toggle-theme">${escapeHtml(state.theme === 'dark' ? textPair('Light theme', '淺色主題') : textPair('Dark theme', '深色主題'))}</button><button class="tonal" type="button" data-action="open-palette">${escapeHtml(textPair('Command palette', '指令面板'))} <span class="shortcut">Ctrl+Shift+F</span></button></div></header><div class="workspace" data-dock="${escapeHtml(state.tabDock)}">${renderTabRail()}<div id="main-content" class="main-pane">${renderMain()}</div></div><footer class="footer">${escapeHtml(textPair('Static documentation workspace · no analytics · browser-local preferences only.', '靜態說明文件工作台 · 無分析追蹤 · 偏好只留瀏覽器本機。'))}</footer></main>${renderDialogs()}<div id="floating-menu" class="floating-menu" hidden></div><div id="toast-stack" class="toast-stack" role="status" aria-live="polite" aria-label="Notifications"></div>`;
  const floatingMenu = byId('floating-menu'); floatingMenu?.setAttribute('role', 'dialog'); floatingMenu?.setAttribute('aria-modal', 'false'); floatingMenu?.setAttribute('aria-label', textPair('Context actions', '操作選單'));
  const extraDialogs = document.createElement('div'); extraDialogs.innerHTML = `${renderGroupDialog()}${renderResetDialog()}${renderDestructiveDialog()}`; while (extraDialogs.firstChild) app.append(extraDialogs.firstChild);
  const dialogLabels = { 'appearance-dialog': textPair('Edit appearance', '編輯外觀'), 'school-dialog': focusedModeDialogTitle(), 'tabs-dialog': textPair('Tabs and groups', '分頁同群組'), 'bulk-close-dialog': textPair('Review tab close', '檢查關閉分頁'), 'dim-dialog': textPair('Dim-sum surprise', '點心驚喜') };
  Object.entries(dialogLabels).forEach(([id, label]) => byId(id)?.setAttribute('aria-label', label));
  applyPresentation(); renderSettingsTabs(); localizeStaticCopy(app); bindEvents(); attachDialogSearches(); patchFocusedModeDialog(); renderToasts(); applyAppearance();
  restorePostRenderFocus();
  if (dialogToRestore) requestAnimationFrame(() => presentDialog(dialogToRestore, { preserveFocus: preserveDialogFocus }));
}
function renderGroupDialog() {
  if (!groupDialogState) return '';
  const isMove = groupDialogState.mode === 'move'; const isRename = groupDialogState.mode === 'rename'; const current = state.tabGroups[groupDialogState.groupId];
  const matcher = getMatcher('group-picker'); const groups = Object.entries(state.tabGroups).filter(([id, group]) => matcher.test(`${group.name} ${groupLabel(id, group)}`));
  const title = isMove ? textPair('Move tab into group', '移動分頁到群組') : isRename ? textPair('Rename tab group', '重新命名分頁群組') : textPair('Create tab group', '建立分頁群組');
  const body = isMove ? `${renderSearch('group-picker', textPair('Search groups', '搜尋群組'), textPair('Search destination groups', '搜尋目標群組'))}<div class="palette-results">${groups.map(([id, group]) => `<button class="palette-result" type="button" data-group-choice="${escapeHtml(id)}"><span>${escapeHtml(groupLabel(id, group))}<span class="palette-context">${escapeHtml(textPair(`${group.tabs.length} members`, `${group.tabs.length} 個成員`))}</span></span><span aria-hidden="true">→</span></button>`).join('') || `<div class="empty-state">${escapeHtml(text('noResults'))}</div>`}</div><div class="dialog-actions"><button class="outlined" type="button" data-action="create-group-from-picker">${escapeHtml(textPair('Create a new group', '建立新群組'))}</button><button class="text-button" type="button" data-action="close-group-dialog">${escapeHtml(text('close'))}</button></div>` : `<form id="group-form" class="stack"><div class="field"><label for="group-name">${escapeHtml(textPair('Group name', '群組名稱'))}</label><input id="group-name" type="text" maxlength="60" required value="${escapeHtml(current?.name || '')}"></div><div class="dialog-actions"><button class="outlined" type="button" data-action="close-group-dialog">${escapeHtml(text('close'))}</button><button class="tonal" type="submit">${escapeHtml(isRename ? textPair('Save name', '儲存名稱') : textPair('Create group', '建立群組'))}</button></div></form>`;
  return `<dialog id="group-dialog" aria-label="${escapeHtml(title)}"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(title)}</h2><p class="muted">${escapeHtml(textPair('This local picker has its own search and regex builder.', '呢個本機選擇器有自己嘅搜尋同正則建構器。'))}</p></div><button class="icon-button" type="button" data-action="close-group-dialog" aria-label="Close group dialog">×</button></div>${body}</div></dialog>`;
}
function renderResetDialog() {
  const ready = resetConfirmation.keyA && resetConfirmation.keyB && resetConfirmation.range >= 100;
  return `<dialog id="reset-dialog" aria-label="${escapeHtml(textPair('Confirm local preference reset', '確認重設本機偏好'))}"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(textPair('Reset browser-local preferences', '重設瀏覽器本機偏好'))}</h2><p class="muted">${escapeHtml(textPair('This clears this site’s stored presentation, history, notifications, schedules, and local unlock hash. It never changes the app, repository, or account.', '呢個會清除本網站儲存嘅顯示設定、歷史、通知、排程同本機解鎖雜湊；絕對唔會改到程式、儲存庫或者帳戶。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="reset-dialog" aria-label="Close reset confirmation">×</button></div><div class="super-confirm"><p>${escapeHtml(textPair('Operate both confirmation keys, then move the slider to the end.', '請操作兩個確認鍵，然後將滑桿拉到最右。'))}</p><div class="action-row"><button id="reset-key-a" class="outlined" type="button" data-action="reset-key-a" aria-pressed="${resetConfirmation.keyA}">${escapeHtml(textPair('Key A', '確認鍵 A'))}</button><button id="reset-key-b" class="outlined" type="button" data-action="reset-key-b" aria-pressed="${resetConfirmation.keyB}">${escapeHtml(textPair('Key B', '確認鍵 B'))}</button></div><label class="field" for="reset-slider">${escapeHtml(textPair('Full-range confirmation slider', '全程確認滑桿'))}<input id="reset-slider" type="range" min="0" max="100" value="${resetConfirmation.range}" ${resetConfirmation.keyA && resetConfirmation.keyB ? '' : 'disabled'}></label></div><div class="dialog-actions"><button class="outlined" type="button" data-action="close-dialog" data-dialog="reset-dialog">${escapeHtml(text('close'))}</button><button id="reset-confirm" class="danger" type="button" data-action="confirm-reset-preferences" ${ready ? '' : 'disabled'}>${escapeHtml(textPair('Reset local preferences', '重設本機偏好'))}</button></div></div></dialog>`;
}
function renderDestructiveDialog() {
  if (!destructiveRequest) return '';
  const ready = destructiveConfirmation.keyA && destructiveConfirmation.keyB && destructiveConfirmation.range >= 100;
  const labels = { 'bulk-close': textPair('Close reviewed tabs', '關閉已檢查分頁'), 'delete-schedule': textPair('Remove local schedule rule', '移除本機排程規則'), 'dismiss-notifications': textPair('Dismiss selected notifications', '關閉已選通知'), 'delete-preset': textPair('Delete saved preset', '刪除已儲存預設'), 'reset-appearance-target': textPair('Reset selected appearance', '重設選定外觀'), 'import-preferences': textPair('Replace local preferences', '取代本機偏好') };
  const label = labels[destructiveRequest.type] || textPair('Confirm action', '確認動作');
  return `<dialog id="destructive-dialog" aria-label="${escapeHtml(label)}"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(label)}</h2><p class="muted">${escapeHtml(textPair('This action changes browser-local records. Review the target, operate both keys, and move the slider to authorize it.', '此動作會更改瀏覽器本機記錄。請檢查目標、操作兩個確認鍵，再拉動滑桿授權。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="destructive-dialog" aria-label="Close confirmation">×</button></div><div class="super-confirm"><p>${escapeHtml(destructiveRequest.summary)}</p><div class="action-row"><button id="destructive-key-a" class="outlined" type="button" data-action="destructive-key-a" aria-pressed="${destructiveConfirmation.keyA}">${escapeHtml(textPair('Key A', '確認鍵 A'))}</button><button id="destructive-key-b" class="outlined" type="button" data-action="destructive-key-b" aria-pressed="${destructiveConfirmation.keyB}">${escapeHtml(textPair('Key B', '確認鍵 B'))}</button></div><label class="field" for="destructive-slider">${escapeHtml(textPair('Full-range confirmation slider', '全程確認滑桿'))}<input id="destructive-slider" type="range" min="0" max="100" value="${destructiveConfirmation.range}" ${destructiveConfirmation.keyA && destructiveConfirmation.keyB ? '' : 'disabled'}></label></div><div class="dialog-actions"><button class="outlined" type="button" data-action="close-dialog" data-dialog="destructive-dialog">${escapeHtml(text('close'))}</button><button id="destructive-confirm" class="danger" type="button" data-action="confirm-destructive" ${ready ? '' : 'disabled'}>${escapeHtml(label)}</button></div></div></dialog>`;
}
function renderDialogs() {
  const pendingCount = pendingBulkClose?.ids?.length || 0;
  return `<dialog id="palette-dialog" class="palette ${state.paletteSize === 'full' ? 'full' : ''}" aria-label="${escapeHtml(text('palette'))}"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(text('palette'))}</h2><p class="muted">${escapeHtml(textPair('Find every tab, article, setting, and appearance action.', '尋找每個分頁、文章、設定同外觀動作。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="palette-dialog" aria-label="Close command palette">×</button></div>${renderSearch('palette', text('palette'), 'Search commands, pages, settings, and articles')}<div class="action-row"><button class="text-button" type="button" data-action="toggle-palette-size">${escapeHtml(state.paletteSize === 'full' ? textPair('Use card size', '使用卡片大小') : textPair('Use full window', '使用全視窗'))}</button></div><div class="palette-results">${renderPaletteResults()}</div></div></dialog><dialog id="appearance-dialog"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(text('appearance'))}</h2><p class="muted">${escapeHtml(textPair('This local editor changes the selected rendered target and records the change in local history.', '呢個本機編輯器會改選定渲染目標，亦會記錄喺本機歷史。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="appearance-dialog" aria-label="Close appearance editor">×</button></div><form id="appearance-form" class="stack"><input type="hidden" id="appearance-target" value="hero"><div class="field"><label for="appearance-background">${escapeHtml(textPair('Background color', '背景顏色'))}</label><div class="field-row"><input id="appearance-background" type="color" value="#e9ddff"><input id="appearance-background-text" type="text" value="#e9ddff"></div></div><div class="field"><label for="appearance-foreground">${escapeHtml(textPair('Text color', '文字顏色'))}</label><div class="field-row"><input id="appearance-foreground" type="color" value="#24005a"><input id="appearance-foreground-text" type="text" value="#24005a"></div></div><div class="field"><label for="appearance-radius">${escapeHtml(textPair('Corner radius', '圓角'))}</label><input id="appearance-radius" type="range" min="0" max="40" value="28"><output id="appearance-radius-output">28px</output></div><div class="field"><label for="appearance-spacing">${escapeHtml(textPair('Internal spacing', '內部間距'))}</label><input id="appearance-spacing" type="range" min="8" max="64" value="28"><output id="appearance-spacing-output">28px</output></div><div id="appearance-color-values"></div><div class="dialog-actions"><button class="text-button" type="button" data-action="reset-appearance-target">${escapeHtml(textPair('Reset target', '重設目標'))}</button><button class="tonal" type="submit">${escapeHtml(text('save'))}</button></div></form></div></dialog><dialog id="school-dialog"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(focusedModeDialogTitle())}</h2><p class="muted">${escapeHtml(textPair('This is a local experience lock. It does not protect an account or the native application.', '呢個係本機使用體驗鎖，唔會保護帳戶或者原生程式。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="school-dialog" aria-label="${escapeHtml(textPair(`Close ${focusedModeLabel()}`, `關閉 ${focusedModeLabel()}`))}">×</button></div><form id="school-form" class="stack"><div class="field"><label for="school-name">${escapeHtml(textPair('Mode name', '模式名稱'))}</label><input id="school-name" type="text" maxlength="60" value="${escapeHtml(state.school.label)}"></div><div class="field"><label for="school-pin">${escapeHtml(textPair(state.school.enabled ? 'Unlock code' : 'Set an unlock code', state.school.enabled ? '解鎖密碼' : '設定解鎖密碼'))}</label><input id="school-pin" type="password" inputmode="numeric" minlength="4" maxlength="128" required><small>${escapeHtml(textPair('The hash stays only in this browser. Deleting local site data resets it deliberately.', '雜湊只留喺呢個瀏覽器。刪除本機網站資料就可以刻意重設。'))}</small></div><div class="dialog-actions"><button class="outlined" type="button" data-action="close-dialog" data-dialog="school-dialog">${escapeHtml(text('close'))}</button><button class="tonal" type="submit">${escapeHtml(state.school.enabled ? textPair('Unlock', '解鎖') : textPair('Turn on', '開啟'))}</button></div></form></div></dialog><dialog id="tabs-dialog"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(textPair('Tabs and groups', '分頁同群組'))}</h2><p class="muted">${escapeHtml(textPair('Move tabs with keyboard-safe controls, manage groups, and keep the structure across reloads.', '用鍵盤安全控制移動分頁、管理群組，結構會跨重新載入保留。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="tabs-dialog" aria-label="Close tabs and groups">×</button></div>${renderSearch('tab-groups', text('groupNameSearch'), 'Search group names')}<div class="stack">${renderTabManager()}</div><div class="dialog-actions"><button class="outlined" type="button" data-action="new-group">${escapeHtml(textPair('Create group', '建立群組'))}</button><button class="tonal" type="button" data-action="close-dialog" data-dialog="tabs-dialog">${escapeHtml(text('close'))}</button></div></div></dialog><dialog id="bulk-close-dialog"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(textPair('Review tab close', '檢查關閉分頁'))}</h2><p class="muted">${escapeHtml(textPair('Tabs are matched only by their visible labels. Pinned tabs stay protected unless you explicitly include them.', '分頁只按可見標籤比對。除非你明確包括，釘選分頁會保持受保護。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="bulk-close-dialog" aria-label="Close tab review">×</button></div><p id="bulk-close-summary">${pendingCount} ${escapeHtml(textPair('tabs are ready for review.', '個分頁準備好檢查。'))}</p><label class="switch-row"><span>${escapeHtml(textPair('Include pinned tabs', '包括釘選分頁'))}</span><input id="bulk-close-include-pinned" type="checkbox"></label><div class="dialog-actions"><button class="outlined" type="button" data-action="close-dialog" data-dialog="bulk-close-dialog">${escapeHtml(text('close'))}</button><button class="danger" type="button" data-action="confirm-bulk-close">${escapeHtml(textPair('Close reviewed tabs', '關閉已檢查分頁'))}</button></div></div></dialog><dialog id="dim-dialog"><div class="dialog-content"><div class="dialog-head"><div><h2>${escapeHtml(textPair('A small dim-sum hello', '一個小小點心招呼'))}</h2><p class="muted">${escapeHtml(textPair('A public-catalog asset is loaded only when this non-blocking surprise is shown.', '只會喺出現非阻塞驚喜時先讀取公開目錄資產。'))}</p></div><button class="icon-button" type="button" data-action="close-dialog" data-dialog="dim-dialog" aria-label="Close dim sum surprise">×</button></div><figure><img id="dim-image" alt="Matcha Har Gow · 抹茶蝦餃" style="width:min(100%,460px);border-radius:18px" loading="lazy"><figcaption><strong>Matcha Har Gow · 抹茶蝦餃</strong><br><span class="muted">${escapeHtml(textPair('Public catalog release asset; not copied into this project.', '公開目錄發行資產；並沒有複製入本專案。'))}</span></figcaption></figure><div class="dialog-actions"><button class="tonal" type="button" data-action="close-dialog" data-dialog="dim-dialog">${escapeHtml(text('close'))}</button></div></div></dialog>`;
}
function renderPaletteResults() {
  const matcher = getMatcher('palette'); const settings = [
    ['appName', 'Display name', '顯示名稱'], ['theme', 'Theme', '主題'], ['language', 'Language mode', '語言模式'], ['funny', 'Funny levels', '幽默程度'], ['emoji', 'Message emoji', '訊息 emoji'], ['seed', 'Seed color', '種子顏色'], ['typography', 'Typography', '文字樣式'], ['density', 'Density and motion', '密度及動畫'], ['tabs', 'Tabs and groups', '分頁及群組'], ['school', state.school.label || 'School mode', state.school.label || '校園模式'], ['schedules', 'Scheduled presentation', '排程顯示'], ['appearance', 'Appearance editor', '外觀編輯器'], ['export', 'Portability', '可攜性'],
  ].filter(([id]) => !(state.school.enabled && ['language', 'funny', 'schedules'].includes(id))).map(([id, en, zh]) => ({ label: textPair(en, zh), context: textPair('Live setting', '即時設定'), action: `setting:${id}` }));
  const commands = [
    ...TAB_DEFS.map(([id, en, zh]) => ({ label: textPair(en, zh), context: textPair('Workspace tab', '工作台分頁'), action: `tab:${id}` })),
    ...visibleArticles().map((article) => ({ label: articleTitle(article), context: textPair('Documentation article', '說明文件文章'), action: `article:${article.slug}` })),
    ...settings,
    { label: textPair('Appearance editor', '外觀編輯器'), context: textPair('Appearance', '外觀'), action: 'appearance:hero' },
    ...TAB_DEFS.map(([id, en, zh]) => ({ label: textPair(`${en} tab appearance`, `${zh} 分頁外觀`), context: textPair('Appearance', '外觀'), action: `appearance:tab-${id}` })),
    { label: textPair('Notification center', '通知中心'), context: textPair('Review messages', '查看訊息'), action: 'tab:notifications' },
  ].filter((item) => matcher.test(`${item.label} ${item.context}`)).slice(0, 40);
  return commands.length ? commands.map((command) => `<button class="palette-result" type="button" data-palette-action="${escapeHtml(command.action)}"><span>${escapeHtml(command.label)}<span class="palette-context">${escapeHtml(command.context)}</span></span><span aria-hidden="true">↗</span></button>`).join('') : `<div class="empty-state">${escapeHtml(text('noResults'))}</div>`;
}
function renderTabManager() {
  const matcher = getMatcher('tab-groups');
  const master = getMatcher('master-tabs');
  const bulk = ensureSearch('tab-bulk');
  const masterRows = state.tabOrder.filter((tab) => !state.closedTabs.includes(tab) && master.test(`${tabText(TAB_BY_ID[tab])} ${TAB_BY_ID[tab].detail}`)).map((tab) => {
    const group = Object.entries(state.tabGroups).find(([, candidate]) => candidate.tabs.includes(tab)); const groupName = group ? groupLabel(group[0], group[1]) : textPair('Ungrouped', '未分組');
    return `<button class="palette-result" type="button" data-tab="${tab}"><span>${escapeHtml(tabText(TAB_BY_ID[tab]))}<span class="palette-context">${escapeHtml(textPair(`Workspace · ${groupName}${state.pinnedTabs.includes(tab) ? ' · pinned' : ''}`, `工作台 · ${groupName}${state.pinnedTabs.includes(tab) ? ' · 已釘選' : ''}`))}</span></span><span aria-hidden="true">↗</span></button>`;
  }).join('') || `<div class="empty-state">${escapeHtml(text('noResults'))}</div>`;
  const groups = Object.entries(state.tabGroups).filter(([id, group]) => matcher.test(`${group.name} ${groupLabel(id, group)}`)).map(([id, group]) => `<article class="control-card"><div class="section-heading"><div><h3>${escapeHtml(groupLabel(id, group))}</h3><p>${group.tabs.length} ${escapeHtml(textPair('tabs', '個分頁'))}</p></div><button class="text-button" type="button" data-rename-group="${id}">${escapeHtml(textPair('Rename', '改名'))}</button></div><div class="stack">${group.tabs.filter((tab) => !state.closedTabs.includes(tab)).map((tab) => `<div class="switch-row"><span>${escapeHtml(tabText(TAB_BY_ID[tab]))}</span><div class="action-row"><button class="text-button" type="button" data-move-tab="${tab}" data-direction="up">↑ <span class="sr-only">Move up</span></button><button class="text-button" type="button" data-move-tab="${tab}" data-direction="down">↓ <span class="sr-only">Move down</span></button><button class="text-button" type="button" data-remove-from-group="${tab}">${escapeHtml(textPair('Remove', '移除'))}</button></div></div>`).join('') || `<small>${escapeHtml(textPair('No open tabs in this group.', '呢個群組未有開啟分頁。'))}</small>`}</div></article>`).join('') || `<div class="empty-state">${escapeHtml(text('noResults'))}</div>`;
  const closed = state.closedTabs.map((tab) => `<button class="text-button" type="button" data-reopen-tab="${tab}">${escapeHtml(textPair(`Reopen ${tabText(TAB_BY_ID[tab])}`, `重新開啟 ${tabText(TAB_BY_ID[tab])}`))}</button>`).join('');
  return `${renderSearch('master-tabs', textPair('Search every open tab', '搜尋每個開啟分頁'), 'Search all workspace tabs')}<section class="control-card"><h3>${escapeHtml(textPair('Master tab search', '主分頁搜尋'))}</h3><div class="palette-results">${masterRows}</div></section><section class="control-card"><h3>${escapeHtml(textPair('Bulk close by visible title', '按可見標題批量關閉'))}</h3><p>${escapeHtml(textPair('Plain text is the default. Both actions use this same predicate; pinned tabs need an explicit review choice.', '預設使用普通文字。兩個動作都用同一個比對；釘選分頁需要明確檢查選擇。'))}</p>${renderSearch('tab-bulk', textPair('Find tab titles to close', '尋找要關閉嘅分頁標題'), 'Enter title text to review')}<div class="action-row"><button class="outlined" type="button" data-action="review-bulk-close-containing" ${(!bulk.plain && !bulk.pattern) ? 'disabled title="Enter text or a valid pattern first"' : ''}>${escapeHtml(textPair('Close tabs containing text', '關閉包含文字嘅分頁'))}</button><button class="outlined" type="button" data-action="review-bulk-close-not-containing" ${(!bulk.plain && !bulk.pattern) ? 'disabled title="Enter text or a valid pattern first"' : ''}>${escapeHtml(textPair('Close tabs not containing text', '關閉不包含文字嘅分頁'))}</button></div></section>${groups}${closed ? `<section class="control-card"><h3>${escapeHtml(textPair('Recently closed', '最近關閉'))}</h3><div class="action-row">${closed}</div></section>` : ''}`;
}
function renderToasts() { const container = byId('toast-stack'); if (!container) return; const visible = state.school.enabled ? runtimeToasts.filter((item) => !isDimSumNotice(item)) : runtimeToasts; container.innerHTML = visible.map((item) => `<article class="toast ${escapeHtml(item.kind)}"><div><strong>${escapeHtml(state.emoji ? notificationGlyph(item.kind) : '')} ${escapeHtml(item.title)}</strong><p>${escapeHtml(item.message)}</p>${item.image ? `<img class="toast-image" src="${escapeHtml(item.image)}" alt="Matcha Har Gow · 抹茶蝦餃">` : ''}</div><button type="button" data-dismiss-toast="${item.id}" aria-label="Dismiss message">×</button></article>`).join(''); localizeStaticCopy(container); container.querySelectorAll('[data-dismiss-toast]').forEach((button) => button.addEventListener('click', () => { runtimeToasts = runtimeToasts.filter((item) => item.id !== button.dataset.dismissToast); renderToasts(); })); }

function bindEvents() {
  app.querySelectorAll('[data-tab]').forEach((element) => element.addEventListener('click', (event) => { if (event.target.closest('[data-tab-menu]')) return; openTab(event.currentTarget.dataset.tab, event.currentTarget); }));
  app.querySelectorAll('[data-tab]').forEach((tab) => tab.addEventListener('keydown', (event) => { const all = [...document.querySelectorAll('.tab-button[role="tab"]')]; const index = all.indexOf(tab); const vertical = tabRailIsVertical(); const previous = vertical ? 'ArrowUp' : 'ArrowLeft'; const next = vertical ? 'ArrowDown' : 'ArrowRight'; let target = null; if (event.key === next) target = all[(index + 1) % all.length]; if (event.key === previous) target = all[(index - 1 + all.length) % all.length]; if (event.key === 'Home') target = all[0]; if (event.key === 'End') target = all.at(-1); if (target) { event.preventDefault(); target.focus(); } if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); openTab(tab.dataset.tab, tab); } }));
  app.querySelectorAll('[data-settings-tab]').forEach((tab) => { tab.addEventListener('click', () => openSettingsSection(tab.dataset.settingsTab, tab)); tab.addEventListener('keydown', (event) => { const tabs = [...app.querySelectorAll('[data-settings-tab]')]; const index = tabs.indexOf(tab); let target = null; if (event.key === 'ArrowLeft') target = tabs[(index - 1 + tabs.length) % tabs.length]; if (event.key === 'ArrowRight') target = tabs[(index + 1) % tabs.length]; if (event.key === 'Home') target = tabs[0]; if (event.key === 'End') target = tabs.at(-1); if (target) { event.preventDefault(); openSettingsSection(target.dataset.settingsTab, target); } }); });
  app.querySelectorAll('[data-tab-menu]').forEach((button) => button.addEventListener('click', (event) => { event.stopPropagation(); openTabMenu(button.dataset.tabMenu, button); }));
  app.querySelectorAll('[data-group-toggle]').forEach((summary) => summary.addEventListener('click', () => { const group = state.tabGroups[summary.dataset.groupToggle]; setTimeout(() => { group.collapsed = !summary.parentElement.open; saveState(); }, 0); }));
  app.querySelectorAll('[data-article]').forEach((button) => button.addEventListener('click', () => showArticle(button.dataset.article)));
  app.querySelectorAll('[data-action]').forEach((element) => element.addEventListener('click', (event) => handleAction(element.dataset.action, element, event)));
  app.querySelectorAll('[data-search]').forEach((input) => input.addEventListener('input', () => updateSearch(input.dataset.search, 'plain', input.value, input)));
  app.querySelectorAll('[data-pattern]').forEach((input) => input.addEventListener('input', () => updateSearch(input.dataset.pattern, 'pattern', input.value, input)));
  app.querySelectorAll('[data-flags]').forEach((input) => input.addEventListener('input', () => updateSearch(input.dataset.flags, 'flags', input.value, input)));
  app.querySelectorAll('[data-sample]').forEach((input) => input.addEventListener('input', () => updateSearch(input.dataset.sample, 'sample', input.value.slice(0, MAX_SAMPLE), input)));
  app.querySelectorAll('[data-token]').forEach((button) => button.addEventListener('click', () => { const search = ensureSearch(button.dataset.token); search.pattern += button.dataset.value; search.regex = true; saveState(); renderApp(); requestAnimationFrame(() => byId(`pattern-${button.dataset.token}`)?.focus()); }));
  app.querySelectorAll('[data-setting]').forEach((input) => input.addEventListener('change', () => setControl(input.dataset.setting, input.value)));
  app.querySelectorAll('[data-setting-range]').forEach((input) => input.addEventListener('change', () => setControl(input.dataset.settingRange, Number(input.value))));
  app.querySelectorAll('[data-seed-color]').forEach((input) => input.addEventListener('input', () => setSeed(input.value, input)));
  app.querySelectorAll('[data-seed-text]').forEach((input) => input.addEventListener('change', () => setSeed(input.value, input)));
  app.querySelectorAll('[data-school-label]').forEach((input) => input.addEventListener('change', () => { state.school.label = input.value.trim().slice(0, 60) || 'School mode'; record('settings changed', 'school label', state.school.label); saveState(); renderApp(); }));
  app.querySelectorAll('[data-history-day]').forEach((input) => input.addEventListener('change', () => { ensureSearch('history').day = input.value; saveState(); renderApp(); }));
  app.querySelectorAll('[data-history-action]').forEach((input) => input.addEventListener('change', () => { ensureSearch('history').action = input.value; saveState(); renderApp(); }));
  app.querySelectorAll('[data-schedule-field]').forEach((input) => input.addEventListener('change', () => updateScheduleField(input.dataset.scheduleId, input.dataset.scheduleField, input.value)));
  app.querySelectorAll('[data-schedule-day]').forEach((input) => input.addEventListener('change', () => updateScheduleDays(input.dataset.scheduleId)));
  app.querySelectorAll('[data-notification-select]').forEach((input) => input.addEventListener('change', () => { state.notificationSelection = input.checked ? [...new Set([...state.notificationSelection, input.dataset.notificationSelect])] : state.notificationSelection.filter((id) => id !== input.dataset.notificationSelect); saveState(); renderApp(); }));
  app.querySelectorAll('[data-dismiss-notification]').forEach((button) => button.addEventListener('click', () => dismissNotifications([button.dataset.dismissNotification])));
  app.querySelectorAll('[data-delete-schedule]').forEach((button) => button.addEventListener('click', () => requestDestructive('delete-schedule', button, { id: button.dataset.deleteSchedule })));
  app.querySelectorAll('[data-palette-action]').forEach((button) => button.addEventListener('click', () => applyPaletteAction(button.dataset.paletteAction)));
  app.querySelectorAll('[data-move-tab]').forEach((button) => button.addEventListener('click', () => moveTab(button.dataset.moveTab, button.dataset.direction)));
  app.querySelectorAll('[data-remove-from-group]').forEach((button) => button.addEventListener('click', () => removeFromGroup(button.dataset.removeFromGroup)));
  app.querySelectorAll('[data-rename-group]').forEach((button) => button.addEventListener('click', () => openGroupDialog('rename', button, '', button.dataset.renameGroup)));
  app.querySelectorAll('[data-reopen-tab]').forEach((button) => button.addEventListener('click', () => reopenTab(button.dataset.reopenTab)));
  app.querySelectorAll('[data-apply-preset]').forEach((button) => button.addEventListener('click', () => applyPreset(button.dataset.applyPreset)));
  app.querySelectorAll('[data-delete-preset]').forEach((button) => button.addEventListener('click', () => requestDestructive('delete-preset', button, { name: button.dataset.deletePreset })));
  app.querySelectorAll('[data-appearance-trigger]').forEach((button) => button.addEventListener('click', () => openAppearance(button.dataset.appearanceTrigger, button)));
  byId('appearance-form')?.addEventListener('submit', saveAppearance);
  byId('appearance-background')?.addEventListener('input', updateAppearanceColorPreview);
  byId('appearance-background-text')?.addEventListener('input', updateAppearanceColorPreview);
  byId('appearance-radius')?.addEventListener('input', updateAppearanceNumbers);
  byId('appearance-spacing')?.addEventListener('input', updateAppearanceNumbers);
  byId('school-form')?.addEventListener('submit', submitSchool);
  byId('group-form')?.addEventListener('submit', submitGroupDialog);
  const groupChoices = [...app.querySelectorAll('[data-group-choice]')];
  groupChoices.forEach((button, index) => { button.addEventListener('click', () => moveIntoGroup(groupDialogState?.tab, button.dataset.groupChoice)); button.addEventListener('keydown', (event) => { if (!['ArrowUp', 'ArrowDown', 'Enter'].includes(event.key)) return; event.preventDefault(); if (event.key === 'Enter') return button.click(); const offset = event.key === 'ArrowDown' ? 1 : -1; groupChoices[(index + offset + groupChoices.length) % groupChoices.length]?.focus(); }); });
  byId('reset-slider')?.addEventListener('input', () => { resetConfirmation.range = Number(byId('reset-slider').value); updateResetConfirmation(); });
  byId('destructive-slider')?.addEventListener('input', () => { destructiveConfirmation.range = Number(byId('destructive-slider').value); updateDestructiveConfirmation(); });
  byId('import-preferences')?.addEventListener('change', importPreferences);
  app.querySelectorAll('dialog').forEach((dialog) => dialog.addEventListener('cancel', (event) => { if (dialog.id === 'group-dialog') { event.preventDefault(); closeGroupDialog(); return; } if (dialog.id === 'destructive-dialog') { event.preventDefault(); closeDialog(dialog.id); return; } if (activeDialogId === dialog.id) activeDialogId = null; restoreReturnFocus(); }));
}
function globalKeyHandler(event) { if (event.ctrlKey && event.shiftKey && event.key.toLowerCase() === 'f') { event.preventDefault(); openDialog('palette-dialog', event.target); } if (event.key === 'Escape') { if (menuState) { event.preventDefault(); closeMenu(true); return; } const dialog = activeDialogId ? byId(activeDialogId) : null; if (dialog?.open && !dialog.matches(':modal')) { event.preventDefault(); if (activeDialogId === 'group-dialog') closeGroupDialog(); else closeDialog(activeDialogId); } } }
function closeMenuWhenOutside(event) { if (!event.target.closest('#floating-menu') && !event.target.closest('[data-tab-menu]')) closeMenu(); }
function contextMenuHandler(event) { const target = event.target.closest('[data-appearance]'); if (!target) return; event.preventDefault(); if (event.shiftKey) { openAppearance(target.dataset.appearance, target); return; } openAppearanceMenu(target.dataset.appearance, event.clientX, event.clientY, target); }
function updateSearch(id, key, value, input) { ensureSearch(id)[key] = value; if (key === 'pattern') ensureSearch(id).regex = true; saveState(); const focusId = input.id; const start = input.selectionStart; const end = input.selectionEnd; preserveDialogFocusId = input.closest('dialog')?.id || ''; renderApp(); requestAnimationFrame(() => { const next = byId(focusId); if (next) { next.focus({ preventScroll: true }); if (typeof next.setSelectionRange === 'function') next.setSelectionRange(start, end); } }); }
function setSeed(value, input) { const normalized = normalizeHex(value); if (!normalized) { input.setAttribute('aria-invalid', 'true'); return; } state.seed = normalized; record('settings changed', 'seed color', normalized); saveState(); applyPresentation(); renderApp(); }
function updateScheduleField(id, field, value) {
  const rule = state.schedules.find((item) => item.id === id); if (!rule) return;
  if (field === 'label') rule.label = boundedText(value, rule.label, 80) || rule.label;
  else if (field === 'start' || field === 'end') { if (timeToMinutes(value) !== null) rule[field] = value; }
  else if (field === 'startDate' || field === 'endDate') rule[field] = /^\d{4}-\d{2}-\d{2}$/.test(value) ? value : '';
  else if (field === 'days') rule.days = value === 'every' ? 'Every day' : (Array.isArray(rule.days) ? rule.days : ['Mon', 'Tue', 'Wed', 'Thu', 'Fri']);
  else if (['theme', 'density', 'language'].includes(field)) rule.settings = { ...(rule.settings || {}), [field]: value };
  record('settings changed', 'schedule', `${rule.label} ${field}`); saveState(); applyPresentation(); renderApp();
}
function updateScheduleDays(id) { const rule = state.schedules.find((item) => item.id === id); if (!rule) return; rule.days = [...app.querySelectorAll(`[data-schedule-day][data-schedule-id="${CSS.escape(id)}"]:checked`)].map((input) => input.value); record('settings changed', 'schedule', `${rule.label} weekdays`); saveState(); applyPresentation(); renderApp(); }
function openTab(id, origin = null) { if (TAB_BY_ID[id]) state.closedTabs = state.closedTabs.filter((tab) => tab !== id); state.activeTab = TAB_BY_ID[id] ? id : 'overview'; openArticle = null; saveState(); renderApp(); history.replaceState(null, '', `#${state.activeTab}`); requestAnimationFrame(() => { const target = byId(`view-${state.activeTab}`); target?.focus({ preventScroll: true }); if (origin) target?.scrollIntoView({ behavior: state.motion === 'reduced' ? 'auto' : 'smooth', block: 'start' }); }); }
function showArticle(slug, fromHash = false) { const meta = visibleArticles().find((article) => article.slug === slug); if (!meta) { if (state.school.enabled && FOCUSED_HIDDEN_ARTICLES.has(slug)) { openArticle = null; state.activeTab = 'overview'; saveState(); if (fromHash) history.replaceState(null, '', '#overview'); renderApp(); } return; } state.activeTab = 'docs'; openArticle = { ...meta, loading: true }; saveState(); renderApp(); if (!fromHash) history.replaceState(null, '', `#/articles/${encodeURIComponent(slug)}`); fetchArticle(meta).then((article) => { openArticle = article; renderApp(); requestAnimationFrame(() => byId('view-docs')?.focus()); }); }
async function fetchArticle(meta) {
  try { const response = await fetch(articleFetchPath(meta.path), { cache: 'no-store' }); if (!response.ok) throw new Error(`HTTP ${response.status}`); const markdown = await response.text(); const renderer = window.MaterialSandboxMarkdown?.render; const html = renderer ? renderer(markdown, { sourcePath: canonicalArticlePath(meta.path), basePath: siteBasePath(), articleRoutes: renderArticleRoutes() }) : `<pre>${escapeHtml(markdown)}</pre>`; return { ...meta, html, loading: false }; } catch (error) { return { ...meta, error: `The canonical article could not be loaded (${error.message}).`, loading: false }; }
}
function handleAction(action, element, event) {
  if (action === 'toggle-theme') return setControl('theme', state.theme === 'dark' ? 'light' : 'dark');
  if (action === 'open-palette') return openDialog('palette-dialog', element);
  if (action === 'close-dialog') return closeDialog(element.dataset.dialog);
  if (action === 'close-group-dialog') return closeGroupDialog();
  if (action === 'toggle-palette-size') { state.paletteSize = state.paletteSize === 'full' ? 'card' : 'full'; saveState(); const dialog = byId('palette-dialog'); dialog?.close(); renderApp(); requestAnimationFrame(() => openDialog('palette-dialog', element)); return; }
  if (action === 'toggle-regex') { const search = ensureSearch(element.dataset.searchId); search.regex = !search.regex; saveState(); renderApp(); requestAnimationFrame(() => byId(search.regex ? `pattern-${element.dataset.searchId}` : `search-${element.dataset.searchId}`)?.focus()); return; }
  if (action === 'regex-plain') { const search = ensureSearch(element.dataset.searchId); search.regex = false; search.plain = search.pattern; saveState(); renderApp(); return; }
  if (action === 'copy-regex') return navigator.clipboard?.writeText(`${ensureSearch(element.dataset.searchId).pattern}\n${ensureSearch(element.dataset.searchId).flags}`).then(() => notify(text('copied'), 'success'));
  if (action === 'export-regex') { const s = ensureSearch(element.dataset.searchId); return download(`sandboxie-${element.dataset.searchId}-regex.json`, JSON.stringify({ engine: 'ECMAScript', pattern: s.pattern, flags: s.flags, sample: s.sample }, null, 2)); }
  if (action === 'toggle-emoji') { state.emoji = !state.emoji; record('settings changed', 'message emoji', state.emoji); saveState(); notify('Message decoration preference saved locally.', 'success'); renderApp(); return; }
  if (action === 'toggle-motion') return setControl('motion', state.motion === 'reduced' ? 'full' : 'reduced');
  if (action === 'school-enable' || action === 'school-unlock') return openDialog('school-dialog', element);
  if (action === 'add-schedule') { state.schedules.push({ id: uid(), label: 'Local presentation rule', startDate: '', endDate: '', start: '09:00', end: '17:00', days: 'Every day', settings: { theme: state.theme, density: state.density, language: state.language } }); record('settings changed', 'schedule', 'local presentation rule'); saveState(); notify(textPair('Local schedule added.', '已加入本機排程。'), 'success'); renderApp(); return; }
  if (action === 'open-appearance') return openAppearance(element.dataset.target || 'hero', element);
  if (action === 'open-import-preferences') { importOrigin = element; byId('import-preferences')?.click(); return; }
  if (action === 'reset-appearance-target') return requestDestructive('reset-appearance-target', element, { target: byId('appearance-target')?.value || 'hero' });
  if (action === 'save-preset') { const name = `Preset ${Object.keys(state.presets).length + 1}`; state.presets[name] = { seed: state.seed, appearance: structuredClone(state.appearance), theme: state.theme, density: state.density, fontScale: state.fontScale, fontFamily: state.fontFamily, fontWeight: state.fontWeight }; record('settings changed', 'appearance preset', name); saveState(); notify(`${name} saved locally.`, 'success'); renderApp(); return; }
  if (action === 'export-preferences') return download('sandboxie-pages-preferences.json', JSON.stringify({ schema: 'material-sandbox-pages/v3', exportedAt: now(), preferences: exportablePreferences() }, null, 2));
  if (action === 'reset-preferences') return openResetDialog(element);
  if (action === 'reset-key-a') { resetConfirmation.keyA = !resetConfirmation.keyA; updateResetConfirmation(); return; }
  if (action === 'reset-key-b') { resetConfirmation.keyB = !resetConfirmation.keyB; updateResetConfirmation(); return; }
  if (action === 'confirm-reset-preferences') return completeResetPreferences();
  if (action === 'destructive-key-a') { destructiveConfirmation.keyA = !destructiveConfirmation.keyA; updateDestructiveConfirmation(); return; }
  if (action === 'destructive-key-b') { destructiveConfirmation.keyB = !destructiveConfirmation.keyB; updateDestructiveConfirmation(); return; }
  if (action === 'confirm-destructive') return completeDestructive();
  if (action === 'open-tab-groups') return openDialog('tabs-dialog', element);
  if (action === 'new-group') return openGroupDialog('create', element);
  if (action === 'create-group-from-picker') return openGroupDialog('create', element);
  if (action === 'review-bulk-close-containing') return reviewBulkClose(false, element);
  if (action === 'review-bulk-close-not-containing') return reviewBulkClose(true, element);
  if (action === 'confirm-bulk-close') return requestBulkCloseConfirmation(Boolean(byId('bulk-close-include-pinned')?.checked), element);
  if (action === 'notification-select-all' || action === 'notification-select-all-check') { const ids = state.notifications.filter((item) => !item.dismissed && getMatcher('notifications').test(`${item.title} ${item.message} ${item.kind}`)).map((item) => item.id); state.notificationSelection = ids; saveState(); renderApp(); return; }
  if (action === 'notification-invert') { const ids = state.notifications.filter((item) => !item.dismissed && getMatcher('notifications').test(`${item.title} ${item.message} ${item.kind}`)).map((item) => item.id); state.notificationSelection = ids.filter((id) => !state.notificationSelection.includes(id)); saveState(); renderApp(); return; }
  if (action === 'notification-dismiss-selected') return requestDestructive('dismiss-notifications', element, { ids: state.notificationSelection });
  if (action === 'notification-export') { const selected = state.notifications.filter((item) => state.notificationSelection.includes(item.id)); return download('sandboxie-pages-notifications.json', JSON.stringify({ schema: 'material-sandbox-pages/notifications-v1', exportedAt: now(), records: selected }, null, 2)); }
  if (action === 'export-history') { const matcher = getMatcher('history'); const s = ensureSearch('history'); const records = state.settingsHistory.filter((item) => matcher.test(`${item.action} ${item.field} ${item.value}`) && (!s.day || item.at.startsWith(s.day)) && (!s.action || s.action === 'all' || s.action === item.action)); return download('sandboxie-pages-history.json', JSON.stringify({ schema: 'material-sandbox-pages/history-v1', exportedAt: now(), records }, null, 2)); }
  if (action === 'close-article') { openArticle = null; renderApp(); return; }
  if (action === 'copy-article-link') { const value = `${location.origin}${location.pathname}#/articles/${encodeURIComponent(openArticle.slug)}`; return navigator.clipboard?.writeText(value).then(() => notify(text('copied'), 'success')); }
  if (action === 'show-dim-sum') return showDimSum();
}
function applyPaletteAction(action) { closeDialog('palette-dialog'); const [type, value] = action.split(':'); if (type === 'tab') return openTab(value); if (type === 'article') return showArticle(value); if (type === 'setting') { state.settingsSection = settingsSectionFor(value); openTab('settings'); setTimeout(() => { const target = document.getElementById(`setting-${value}`) || document.getElementById(`setting-${state.settingsSection}`) || document.getElementById('setting-theme'); target?.scrollIntoView({ behavior: state.motion === 'reduced' ? 'auto' : 'smooth', block: 'center' }); target?.classList.add('highlight-target'); target?.querySelector('input,select,button')?.focus({ preventScroll: true }); setTimeout(() => target?.classList.remove('highlight-target'), 1600); }, 60); return; } if (type === 'appearance') return openAppearance(value); }
function dismissNotifications(ids) { if (!ids.length) { notify('Select one or more notifications first.', 'warning', true); return; } state.notifications.forEach((item) => { if (ids.includes(item.id)) item.dismissed = true; }); state.notificationSelection = []; record('notifications dismissed', 'notification center', `${ids.length} record(s)`); saveState(); notify(`${ids.length} notification${ids.length === 1 ? '' : 's'} dismissed.`, 'success'); renderApp(); }
function dialogNeedsModal(id) { return ['school-dialog', 'bulk-close-dialog', 'reset-dialog', 'destructive-dialog'].includes(id); }
function presentDialog(id, { preserveFocus = false } = {}) { const dialog = byId(id); if (dialog && !dialog.open) { if (dialogNeedsModal(id)) dialog.showModal(); else dialog.show(); } requestAnimationFrame(() => { positionAnchoredDialog(id); if (!preserveFocus) dialog?.querySelector('input,button,select,textarea')?.focus(); }); }
function openDialog(id, origin = document.activeElement) { setReturnFocus(origin); setDialogAnchor(id, origin); activeDialogId = id; presentDialog(id); }
function closeDialog(id) { const resume = id === 'destructive-dialog' ? destructiveReturnDialog : ''; if (id === 'destructive-dialog') destructiveReturnDialog = ''; byId(id)?.close(); if (activeDialogId === id) activeDialogId = null; if (['appearance-dialog', 'group-dialog'].includes(id)) dialogAnchorSelector = ''; if (resume) { activeDialogId = resume; requestAnimationFrame(() => { presentDialog(resume); requestAnimationFrame(restoreReturnFocus); }); return; } restoreReturnFocus(); }
function bindDetachedSearch(scope, id, refresh) {
  const refreshFrom = (source) => {
    const fieldId = source?.id || '';
    const start = Number.isInteger(source?.selectionStart) ? source.selectionStart : null;
    const end = Number.isInteger(source?.selectionEnd) ? source.selectionEnd : start;
    refresh();
    if (!fieldId) return;
    requestAnimationFrame(() => {
      const replacement = byId(fieldId);
      replacement?.focus({ preventScroll: true });
      if (replacement && start !== null && typeof replacement.setSelectionRange === 'function') replacement.setSelectionRange(start, end ?? start);
    });
  };
  scope.querySelector(`[data-search="${id}"]`)?.addEventListener('input', (event) => { ensureSearch(id).plain = event.target.value; saveState(); refreshFrom(event.currentTarget); });
  scope.querySelector(`[data-pattern="${id}"]`)?.addEventListener('input', (event) => { ensureSearch(id).pattern = event.target.value; ensureSearch(id).regex = true; saveState(); refreshFrom(event.currentTarget); });
  scope.querySelector(`[data-flags="${id}"]`)?.addEventListener('input', (event) => { ensureSearch(id).flags = event.target.value; saveState(); refreshFrom(event.currentTarget); });
  scope.querySelector(`[data-sample="${id}"]`)?.addEventListener('input', (event) => { ensureSearch(id).sample = event.target.value.slice(0, MAX_SAMPLE); saveState(); refreshFrom(event.currentTarget); });
  scope.querySelector(`[data-action="toggle-regex"][data-search-id="${id}"]`)?.addEventListener('click', (event) => { ensureSearch(id).regex = !ensureSearch(id).regex; saveState(); refreshFrom(event.currentTarget); });
  scope.querySelector(`[data-action="regex-plain"][data-search-id="${id}"]`)?.addEventListener('click', (event) => { ensureSearch(id).regex = false; ensureSearch(id).plain = ensureSearch(id).pattern; saveState(); refreshFrom(event.currentTarget); });
  scope.querySelectorAll(`[data-token="${id}"]`).forEach((button) => button.addEventListener('click', (event) => { ensureSearch(id).pattern += button.dataset.value; ensureSearch(id).regex = true; saveState(); refreshFrom(event.currentTarget); }));
  scope.querySelector(`[data-action="copy-regex"][data-search-id="${id}"]`)?.addEventListener('click', () => navigator.clipboard?.writeText(`${ensureSearch(id).pattern}\n${ensureSearch(id).flags}`).then(() => notify(text('copied'), 'success')));
  scope.querySelector(`[data-action="export-regex"][data-search-id="${id}"]`)?.addEventListener('click', () => download(`sandboxie-${id}-regex.json`, JSON.stringify({ engine: 'ECMAScript', ...ensureSearch(id) }, null, 2)));
}
function attachDialogSearch(dialogId, searchId, label, placeholder) {
  const dialog = byId(dialogId); const content = dialog?.querySelector('.dialog-content'); if (!dialog || !content) return;
  let host = content.querySelector(`[data-dialog-search="${searchId}"]`);
  if (!host) { host = document.createElement('div'); host.dataset.dialogSearch = searchId; content.querySelector('.dialog-head')?.after(host); }
  const paint = () => {
    host.innerHTML = renderSearch(searchId, label, placeholder); bindDetachedSearch(host, searchId, paint);
    const matcher = getMatcher(searchId);
    content.querySelectorAll('.field, .field-row, .weekday-set').forEach((item) => { if (!host.contains(item)) item.hidden = Boolean(!matcher.error && !matcher.test(item.textContent)); });
  };
  paint();
}
function attachDialogSearches() {
  attachDialogSearch('appearance-dialog', 'appearance-dialog-search', textPair('Search appearance editor', '搜尋外觀編輯器'), 'Search this appearance editor');
  const name = focusedModeLabel(); attachDialogSearch('school-dialog', 'school-dialog-search', textPair(`Search ${name} controls`, `搜尋 ${name} 控制`), `Search ${name} controls`);
}
function patchFocusedModeDialog() {
  const dialog = byId('school-dialog'); if (!dialog) return;
  const name = focusedModeLabel(); const title = focusedModeDialogTitle(); dialog.setAttribute('aria-label', title);
  const heading = dialog.querySelector('.dialog-head h2'); if (heading) heading.textContent = title;
  dialog.querySelector('[data-dialog="school-dialog"]')?.setAttribute('aria-label', textPair(`Close ${name}`, `關閉 ${name}`));
  byId('search-school-dialog-search')?.setAttribute('aria-label', textPair(`Search ${name} controls`, `搜尋 ${name} 控制`));
}
function openTabMenu(id, origin) {
  menuState = { id, origin }; const menu = byId('floating-menu'); const pinned = state.pinnedTabs.includes(id); const actions = [
    { id: 'pin', label: pinned ? textPair('Unpin tab', '取消釘選分頁') : textPair('Pin tab', '釘選分頁'), shortcut: pinned ? '' : 'Alt+P' }, { id: 'up', label: textPair('Move earlier', '向前移動'), shortcut: 'Alt+↑' }, { id: 'down', label: textPair('Move later', '向後移動'), shortcut: 'Alt+↓' }, { id: 'group', label: textPair('Move… into group…', '移動…到群組…') }, { id: 'appearance', label: text('appearance') },
  ];
  const paint = () => { const matcher = getMatcher('tab-menu'); const visible = actions.filter((item) => matcher.test(item.label)); menu.innerHTML = `${renderSearch('tab-menu', textPair('Search tab actions', '搜尋分頁動作'), 'Search tab actions')}${visible.map((item) => `<button type="button" data-menu-action="${item.id}">${escapeHtml(item.label)}${item.shortcut ? `<span class="shortcut">${item.shortcut}</span>` : ''}</button>`).join('') || `<div class="empty-state">${escapeHtml(text('noResults'))}</div>`}`; bindDetachedSearch(menu, 'tab-menu', paint); menu.querySelectorAll('[data-menu-action]').forEach((button) => button.addEventListener('click', () => { const action = button.dataset.menuAction; closeMenu(); if (action === 'pin') togglePin(id); if (action === 'up') moveTab(id, 'up'); if (action === 'down') moveTab(id, 'down'); if (action === 'group') chooseGroup(id, origin); if (action === 'appearance') openAppearance(`tab-${id}`, origin); })); };
  const rect = origin.getBoundingClientRect(); menu.style.left = `${Math.max(8, Math.min(rect.left, innerWidth - 360))}px`; menu.style.top = `${Math.max(8, Math.min(rect.bottom + 6, innerHeight - 300))}px`; menu.hidden = false; paint(); requestAnimationFrame(() => menu.querySelector('#search-tab-menu')?.focus({ preventScroll: true }));
}
function closeMenu(restoreFocus = false) { const origin = menuState?.origin; byId('floating-menu')?.setAttribute('hidden', ''); menuState = null; if (restoreFocus && origin) requestAnimationFrame(() => { if (origin.isConnected) origin.focus({ preventScroll: true }); else focusSelector(selectorValue(origin)); }); }
function openAppearanceMenu(target, x, y, origin = document.activeElement) { menuState = { id: `appearance-${target}`, origin }; const menu = byId('floating-menu'); const paint = () => { const matcher = getMatcher('appearance-menu'); const label = text('appearance'); menu.innerHTML = `${renderSearch('appearance-menu', textPair('Search appearance actions', '搜尋外觀動作'), 'Search appearance actions')}${matcher.test(label) ? `<button type="button" data-menu-action="appearance">${escapeHtml(label)}</button>` : `<div class="empty-state">${escapeHtml(text('noResults'))}</div>`}`; bindDetachedSearch(menu, 'appearance-menu', paint); menu.querySelector('[data-menu-action="appearance"]')?.addEventListener('click', () => { closeMenu(false); openAppearance(target, origin); }); }; menu.style.left = `${Math.max(8, Math.min(x, innerWidth - 360))}px`; menu.style.top = `${Math.max(8, Math.min(y, innerHeight - 300))}px`; menu.hidden = false; paint(); requestAnimationFrame(() => menu.querySelector('#search-appearance-menu')?.focus({ preventScroll: true })); }
function togglePin(id) { state.pinnedTabs = state.pinnedTabs.includes(id) ? state.pinnedTabs.filter((tab) => tab !== id) : [...state.pinnedTabs, id]; record('settings changed', 'pinned tabs', id); saveState(); renderApp(); }
function moveTab(id, direction) { const index = state.tabOrder.indexOf(id); const target = direction === 'up' ? index - 1 : index + 1; if (target < 0 || target >= state.tabOrder.length) return; [state.tabOrder[index], state.tabOrder[target]] = [state.tabOrder[target], state.tabOrder[index]]; record('settings changed', 'tab order', state.tabOrder.join(', ')); saveState(); renderApp(); }
function reopenTab(id) { state.closedTabs = state.closedTabs.filter((tab) => tab !== id); record('settings changed', 'tab reopened', id); saveState(); renderApp(); requestAnimationFrame(() => openDialog('tabs-dialog')); }
function reviewBulkClose(invert, origin) { const matcher = getMatcher('tab-bulk'); const search = ensureSearch('tab-bulk'); const query = search.regex ? search.pattern : search.plain; if (!query || matcher.error) { notify(matcher.error || 'Enter text or a valid pattern before reviewing tabs.', 'warning', true); return; } const ids = state.tabOrder.filter((tab) => !state.closedTabs.includes(tab) && (invert ? !matcher.test(tabText(TAB_BY_ID[tab])) : matcher.test(tabText(TAB_BY_ID[tab])))); pendingBulkClose = { ids, invert }; openDialog('bulk-close-dialog', origin); }
function applyBulkClose(includePinned) { if (!pendingBulkClose) return; const candidates = pendingBulkClose.ids.filter((id) => includePinned || !state.pinnedTabs.includes(id)); if (!candidates.length) { notify('No eligible tabs matched this review.', 'warning', true); closeDialog('bulk-close-dialog'); pendingBulkClose = null; return; } state.closedTabs = [...new Set([...state.closedTabs, ...candidates])]; record('tabs closed', pendingBulkClose.invert ? 'titles not containing text' : 'titles containing text', candidates.join(', ')); saveState(); pendingBulkClose = null; closeDialog('bulk-close-dialog'); notify(`${candidates.length} tab${candidates.length === 1 ? '' : 's'} closed from this workspace.`, 'success'); renderApp(); }
function openGroupDialog(mode, origin = document.activeElement, tab = '', groupId = '') { groupReturnDialog = byId('tabs-dialog')?.open ? 'tabs-dialog' : null; byId('tabs-dialog')?.close(); groupDialogState = { mode, tab, groupId }; setReturnFocus(origin); setDialogAnchor('group-dialog', origin); activeDialogId = 'group-dialog'; renderApp(); }
function closeGroupDialog() { const reopen = groupReturnDialog; const origin = returnFocus; groupReturnDialog = null; groupDialogState = null; activeDialogId = null; dialogAnchorSelector = ''; renderApp(); if (reopen) requestAnimationFrame(() => openDialog(reopen, origin)); else restoreReturnFocus(); }
function chooseGroup(tab, origin) { openGroupDialog('move', origin, tab); }
function moveIntoGroup(tab, groupId) { if (!tab || !state.tabGroups[groupId]) return; Object.values(state.tabGroups).forEach((group) => group.tabs = group.tabs.filter((item) => item !== tab)); state.tabGroups[groupId].tabs.push(tab); record('settings changed', 'tab group', `${tab} → ${state.tabGroups[groupId].name}`); saveState(); closeGroupDialog(); }
function removeFromGroup(tab) { Object.values(state.tabGroups).forEach((group) => group.tabs = group.tabs.filter((item) => item !== tab)); record('settings changed', 'tab group', `${tab} removed`); saveState(); renderApp(); }
function submitGroupDialog(event) { event.preventDefault(); const name = boundedText(byId('group-name')?.value, '', 60); if (!name) return; if (groupDialogState?.mode === 'rename' && state.tabGroups[groupDialogState.groupId]) { state.tabGroups[groupDialogState.groupId].name = name; } else { state.tabGroups[`group-${uid()}`] = { name, tabs: [], collapsed: false }; } record('settings changed', 'tab group', name); saveState(); closeGroupDialog(); }
function openResetDialog(origin) { resetConfirmation = { keyA: false, keyB: false, range: 0 }; openDialog('reset-dialog', origin); }
function updateResetConfirmation() { const readyKeys = resetConfirmation.keyA && resetConfirmation.keyB; const slider = byId('reset-slider'); const keyA = byId('reset-key-a'); const keyB = byId('reset-key-b'); const confirm = byId('reset-confirm'); const panel = slider?.closest('.super-confirm'); if (keyA) keyA.setAttribute('aria-pressed', String(resetConfirmation.keyA)); if (keyB) keyB.setAttribute('aria-pressed', String(resetConfirmation.keyB)); if (slider) slider.disabled = !readyKeys; if (confirm) confirm.disabled = !(readyKeys && resetConfirmation.range >= 100); if (panel) { panel.style.setProperty('--confirmation-progress', String(resetConfirmation.range / 100)); panel.dataset.authorized = String(readyKeys && resetConfirmation.range >= 100); } }
function completeResetPreferences() { if (!(resetConfirmation.keyA && resetConfirmation.keyB && resetConfirmation.range >= 100)) return; state = structuredClone(DEFAULTS); saveState(); closeDialog('reset-dialog'); resetConfirmation = { keyA: false, keyB: false, range: 0 }; notify('Local preferences reset.', 'success'); renderApp(); }
function requestDestructive(type, origin, payload = {}) {
  if (type === 'dismiss-notifications' && !payload.ids?.length) { notify('Select one or more notifications first.', 'warning', true); return; }
  const summary = type === 'bulk-close' ? `${pendingBulkClose?.ids?.length || 0} matched tabs are queued; pinned tabs ${payload.includePinned ? 'are included' : 'remain protected'}.` : type === 'delete-schedule' ? `The rule “${state.schedules.find((item) => item.id === payload.id)?.label || 'unknown'}” will be removed from this browser.` : type === 'delete-preset' ? `The saved preset “${payload.name || 'unknown'}” will be removed from this browser.` : type === 'reset-appearance-target' ? `The appearance override for “${payload.target || 'selected target'}” will be reset to the shipped local values.` : type === 'import-preferences' ? `The selected file “${payload.fileName || 'preferences'}” will replace the browser-local presentation preferences; the focused-mode unlock remains off.` : `${payload.ids?.length || 0} selected notification records will be dismissed.`;
  destructiveRequest = { type, payload, summary }; destructiveConfirmation = { keyA: false, keyB: false, range: 0 }; destructiveReturnDialog = type === 'reset-appearance-target' ? origin?.closest?.('dialog')?.id || '' : ''; closeDialog('bulk-close-dialog'); openDialog('destructive-dialog', origin); renderApp();
}
function requestBulkCloseConfirmation(includePinned, origin) { requestDestructive('bulk-close', origin, { includePinned }); }
function updateDestructiveConfirmation() { const readyKeys = destructiveConfirmation.keyA && destructiveConfirmation.keyB; const slider = byId('destructive-slider'); const keyA = byId('destructive-key-a'); const keyB = byId('destructive-key-b'); const confirm = byId('destructive-confirm'); const panel = slider?.closest('.super-confirm'); if (keyA) keyA.setAttribute('aria-pressed', String(destructiveConfirmation.keyA)); if (keyB) keyB.setAttribute('aria-pressed', String(destructiveConfirmation.keyB)); if (slider) slider.disabled = !readyKeys; if (confirm) confirm.disabled = !(readyKeys && destructiveConfirmation.range >= 100); if (panel) { panel.style.setProperty('--confirmation-progress', String(destructiveConfirmation.range / 100)); panel.dataset.authorized = String(readyKeys && destructiveConfirmation.range >= 100); } }
function completeDestructive() { if (!destructiveRequest || !(destructiveConfirmation.keyA && destructiveConfirmation.keyB && destructiveConfirmation.range >= 100)) return; const { type, payload } = destructiveRequest; const focusAfter = returnFocusSelector; const queueFocus = (fallback = '') => { queuePostRenderFocus(focusAfter, fallback); }; destructiveRequest = null; destructiveConfirmation = { keyA: false, keyB: false, range: 0 }; if (type === 'bulk-close') { queueFocus('[data-action="open-tab-groups"]'); closeDialog('destructive-dialog'); return applyBulkClose(payload.includePinned); } if (type === 'delete-schedule') { state.schedules = state.schedules.filter((item) => item.id !== payload.id); record('settings changed', 'schedule', 'removed'); saveState(); queueFocus('[data-action="add-schedule"]'); closeDialog('destructive-dialog'); notify('Local schedule removed.', 'success'); renderApp(); return; } if (type === 'delete-preset') { queueFocus('[data-action="save-preset"]'); closeDialog('destructive-dialog'); return deletePreset(payload.name); } if (type === 'reset-appearance-target') { delete state.appearance[payload.target]; record('settings changed', 'appearance target', 'reset'); saveState(); queueFocus('[data-action="reset-appearance-target"]'); closeDialog('destructive-dialog'); notify(textPair('Selected appearance reset locally.', '已重設選定外觀。'), 'success'); renderApp(); return; } if (type === 'import-preferences') { state = sanitizeState(payload.preferences, false); state.activeTab = TAB_BY_ID[payload.activeTab] ? payload.activeTab : 'overview'; state.school.enabled = false; state.school.hash = ''; record('preferences imported', 'preferences', payload.fileName || 'preferences'); saveState(); queueFocus('[data-action="open-import-preferences"]'); closeDialog('destructive-dialog'); notify(textPair('Local preferences imported; focused mode remains off until explicitly enabled.', '已匯入本機偏好；專注模式會保持關閉，直到明確啟用。'), 'success'); renderApp(); return; } if (type === 'dismiss-notifications') { queueFocus('[data-action="notification-select-all"]'); closeDialog('destructive-dialog'); return dismissNotifications(payload.ids); } }
function openAppearance(target, origin) { const dialog = byId('appearance-dialog'); const values = state.appearance[target] || defaultAppearance(target); byId('appearance-target').value = target; byId('appearance-background').value = values.background; byId('appearance-background-text').value = values.background; byId('appearance-foreground').value = values.foreground; byId('appearance-foreground-text').value = values.foreground; byId('appearance-radius').value = values.radius; byId('appearance-spacing').value = values.spacing; updateAppearanceNumbers(); updateAppearanceColorPreview(); openDialog('appearance-dialog', origin); }
function defaultAppearance(target) { if (target === 'hero') return { background: state.theme === 'dark' ? '#4f378b' : '#e9ddff', foreground: state.theme === 'dark' ? '#eaddff' : '#24005a', radius: 28, spacing: 28 }; if (target.startsWith('tab-')) return { background: state.theme === 'dark' ? '#2b2930' : '#f3edf7', foreground: state.theme === 'dark' ? '#e6e0e9' : '#1d1b20', radius: 12, spacing: 10 }; return { background: state.theme === 'dark' ? '#211f26' : '#f3edf7', foreground: state.theme === 'dark' ? '#e6e0e9' : '#1d1b20', radius: 18, spacing: 20 }; }
function updateAppearanceNumbers() { const radius = byId('appearance-radius'); const spacing = byId('appearance-spacing'); if (radius) byId('appearance-radius-output').value = `${radius.value}px`; if (spacing) byId('appearance-spacing-output').value = `${spacing.value}px`; }
function updateAppearanceColorPreview() { const value = normalizeHex(byId('appearance-background-text')?.value || byId('appearance-background')?.value || ''); if (!value) return; byId('appearance-background').value = value; byId('appearance-background-text').value = value; byId('appearance-color-values').innerHTML = renderColorValues(value); }
function saveAppearance(event) { event.preventDefault(); const target = byId('appearance-target').value; const background = normalizeHex(byId('appearance-background-text').value); const foreground = normalizeHex(byId('appearance-foreground-text').value); if (!background || !foreground) { notify('Use six-digit hexadecimal colors in the appearance editor.', 'warning', true); return; } state.appearance[target] = { background, foreground, radius: Number(byId('appearance-radius').value), spacing: Number(byId('appearance-spacing').value) }; record('settings changed', 'appearance target', target); saveState(); closeDialog('appearance-dialog'); notify('Appearance saved locally.', 'success'); renderApp(); }
function applyPreset(name) { const preset = state.presets[name]; if (!preset) return; state = { ...state, seed: preset.seed, theme: preset.theme, density: preset.density, fontScale: preset.fontScale, fontFamily: preset.fontFamily, fontWeight: preset.fontWeight, appearance: structuredClone(preset.appearance || {}) }; record('settings changed', 'appearance preset', `${name} applied`); saveState(); applyPresentation(); notify(`${name} applied locally.`, 'success'); renderApp(); }
function deletePreset(name) { if (!state.presets[name]) return; delete state.presets[name]; record('settings changed', 'appearance preset', `${name} deleted`); saveState(); notify(`${name} deleted locally.`, 'success'); renderApp(); }
async function sha256(value) { const bytes = new TextEncoder().encode(value); const hash = await crypto.subtle.digest('SHA-256', bytes); return [...new Uint8Array(hash)].map((byte) => byte.toString(16).padStart(2, '0')).join(''); }
async function submitSchool(event) { event.preventDefault(); const pin = byId('school-pin').value; if (pin.length < 4) { notify(textPair('Use at least four characters for the local unlock code.', '本機解鎖密碼最少要四個字元。'), 'warning', true); return; } const hashed = await sha256(pin); if (state.school.enabled && state.school.hash && hashed !== state.school.hash) { notify(textPair('The local unlock code does not match.', '本機解鎖密碼唔相符。'), 'error', true); return; } state.school.label = byId('school-name').value.trim().slice(0, 60) || 'School mode'; state.school.hash = state.school.hash || hashed; state.school.enabled = !state.school.enabled; if (state.school.enabled) { if (FOCUSED_HIDDEN_ARTICLES.has(openArticle?.slug)) openArticle = null; runtimeToasts = runtimeToasts.filter((item) => !isDimSumNotice(item)); state.notificationSelection = state.notificationSelection.filter((id) => !isDimSumNotice(state.notifications.find((item) => item.id === id))); if (FOCUSED_HIDDEN_ARTICLES.has(/^#\/articles\/([^/?#]+)/.exec(location.hash)?.[1])) history.replaceState(null, '', '#overview'); } record('settings changed', focusedModeLabel(), state.school.enabled ? 'enabled' : 'disabled'); saveState(); closeDialog('school-dialog'); notify(state.school.enabled ? `${state.school.label} is active.` : `${state.school.label} is unlocked.`, 'success'); renderApp(); }
function exportablePreferences() { const exported = sanitizeState(state, false); exported.school = { ...exported.school, enabled: false, hash: '' }; return exported; }
async function importPreferences(event) { const file = event.target.files?.[0]; if (!file || file.size > 256_000) { notify('Choose a JSON preferences file under 256 KB.', 'warning', true); return; } try { const imported = JSON.parse(await file.text()); if (imported.schema !== 'material-sandbox-pages/v3' || !isRecord(imported.preferences)) throw new Error('This is not a compatible preference export.'); const preferences = sanitizeState(imported.preferences, false); preferences.school.enabled = false; preferences.school.hash = ''; event.target.value = ''; const origin = importOrigin || event.target; importOrigin = null; requestDestructive('import-preferences', origin, { preferences, fileName: boundedText(file.name, 'preferences', 120), activeTab: activeTab() }); } catch (error) { importOrigin = null; notify(`Import failed: ${error.message}`, 'error', true); } }
function applyAppearance() { document.querySelectorAll('[data-appearance]').forEach((element) => { const value = state.appearance[element.dataset.appearance]; if (!value) return; element.style.background = value.background; element.style.color = value.foreground; element.style.borderRadius = `${value.radius}px`; element.style.padding = `${value.spacing}px`; element.querySelectorAll('h1,h2,h3,h4,p,strong,small,.muted,.brand-subtitle').forEach((node) => { node.style.color = value.foreground; }); }); }
function applyPresentation() { const effective = effectivePresentation(); const root = document.documentElement; root.dataset.theme = effective.theme; root.style.setProperty('--seed', effective.seed); root.style.setProperty('--page-scale', `${effective.fontScale}%`); root.style.setProperty('--page-weight', String(effective.fontWeight)); root.style.setProperty('--page-font', effective.fontFamily === 'serif' ? 'Georgia, "Times New Roman", serif' : effective.fontFamily === 'mono' ? 'ui-monospace, SFMono-Regular, Consolas, monospace' : 'Inter, "Segoe UI", system-ui, sans-serif'); document.body.dataset.density = effective.density; document.body.dataset.motion = effective.motion; lastPresentationSignature = JSON.stringify(effective); }
function maybeDimSum() { if (!state.hasVisited) { state.hasVisited = true; saveState(); return; } if (!state.school.enabled && Math.random() < .1) setTimeout(showDimSum, 700); }
function showDimSum() { if (state.school.enabled) return; notify(textPair('Matcha Har Gow · 抹茶蝦餃', '抹茶蝦餃 · Matcha Har Gow'), 'info', false, 'https://github.com/Ding-Ding-Projects/dim-sum-photos/releases/download/catalog-v1-part-003/hk-dish-3031-matcha-har-gow.png'); }
function rgbToHsl({ r, g, b }) { r /= 255; g /= 255; b /= 255; const max = Math.max(r, g, b); const min = Math.min(r, g, b); const delta = max - min; let h = 0; if (delta) { if (max === r) h = ((g - b) / delta) % 6; else if (max === g) h = (b - r) / delta + 2; else h = (r - g) / delta + 4; h *= 60; if (h < 0) h += 360; } const l = (max + min) / 2; const s = delta ? delta / (1 - Math.abs(2 * l - 1)) : 0; return { h: Math.round(h), s: Math.round(s * 100), l: Math.round(l * 100) }; }
function hslToHsv({ h, s, l }) { s /= 100; l /= 100; const v = l + s * Math.min(l, 1 - l); const sat = v ? 2 * (1 - l / v) : 0; const c = (1 - Math.abs(2 * l - 1)) * s; const x = c * (1 - Math.abs(((h / 60) % 2) - 1)); const m = l - c / 2; const rgb = h < 60 ? [c, x, 0] : h < 120 ? [x, c, 0] : h < 180 ? [0, c, x] : h < 240 ? [0, x, c] : h < 300 ? [x, 0, c] : [c, 0, x]; const min = Math.min(...rgb.map((part) => part + m)); return { h, s: Math.round(sat * 100), v: Math.round(v * 100), w: Math.round(min * 100), b: Math.round((1 - v) * 100) }; }
function rgbToCmyk({ r, g, b }) { r /= 255; g /= 255; b /= 255; const k = 1 - Math.max(r, g, b); return { c: Math.round((k === 1 ? 0 : (1 - r - k) / (1 - k)) * 100), m: Math.round((k === 1 ? 0 : (1 - g - k) / (1 - k)) * 100), y: Math.round((k === 1 ? 0 : (1 - b - k) / (1 - k)) * 100), k: Math.round(k * 100) }; }
function rgbToOklab({ r, g, b }) { let [lr, lg, lb] = [r, g, b].map((value) => { value /= 255; return value <= .04045 ? value / 12.92 : ((value + .055) / 1.055) ** 2.4; }); const l = Math.cbrt(.4122214708 * lr + .5363325363 * lg + .0514459929 * lb); const m = Math.cbrt(.2119034982 * lr + .6806995451 * lg + .1073969566 * lb); const s = Math.cbrt(.0883024619 * lr + .2817188376 * lg + .6299787005 * lb); return { l: (.2104542553 * l + .793617785 * m - .0040720468 * s).toFixed(3), a: (1.9779984951 * l - 2.428592205 * m + .4505937099 * s).toFixed(3), b: (.0259040371 * l + .7827717662 * m - .808675766 * s).toFixed(3) }; }
function hexToRgb(hex) { const normalized = normalizeHex(hex); if (!normalized) return null; const number = Number.parseInt(normalized.slice(1), 16); return { r: number >> 16, g: (number >> 8) & 255, b: number & 255 }; }
function normalizeHex(value) { const match = /^#?([0-9a-f]{6})$/i.exec(String(value).trim()); return match ? `#${match[1].toLowerCase()}` : null; }
function contrastRatio(first, second) { const lum = ({ r, g, b }) => [r, g, b].map((value) => { value /= 255; return value <= .03928 ? value / 12.92 : ((value + .055) / 1.055) ** 2.4; }).reduce((sum, value, index) => sum + value * [0.2126, .7152, .0722][index], 0); const [a, b] = [lum(first), lum(second)].sort((x, y) => y - x); return ((a + .05) / (b + .05)).toFixed(2); }
function notificationGlyph(kind) { return ({ info: '●', success: '✓', warning: '!', error: '×' }[kind] || '●'); }
function formatDate(value) { try { return new Intl.DateTimeFormat(activeLanguage() === 'zh' ? 'zh-HK' : 'en-CA', { dateStyle: 'medium', timeStyle: 'short' }).format(new Date(value)); } catch { return value; } }
async function loadCatalog() {
  try { const [manifestResponse, routeResponse] = await Promise.all([fetch('articles/index.json'), fetch('assets/article-routes.json').catch(() => null)]); const manifest = await manifestResponse.json(); if (!Array.isArray(manifest.articles)) throw new Error('Manifest has no articles'); const supplemental = [manifest.changelog?.path ? { slug: 'changelog', path: manifest.changelog.path, related: ['changelog-viewer', 'native-ci-evidence'], kind: 'supplemental' } : null, { slug: 'screenshots', path: 'screenshots.md', related: ['material-design', 'm3-shell-boundary', 'native-ci-evidence'], kind: 'supplemental' }].filter(Boolean); articles = [...manifest.articles, ...supplemental].map((item) => ({ ...item, slug: item.slug || item.path.replace(/\.md$/, '').split('/').at(-1) })); if (routeResponse?.ok) { const routeJson = await routeResponse.json(); articleRoutes = Array.isArray(routeJson.articles) ? routeJson.articles : Array.isArray(routeJson.routes) ? routeJson.routes : []; const bySlug = new Map(articleRoutes.map((entry) => [entry.slug, entry])); articles = articles.map((article) => { const route = bySlug.get(article.slug); return route ? { ...article, route: route.route, related: route.relatedArticles?.map((item) => item.slug) || article.related } : article; }); } } catch (error) { notify(`Documentation catalog fallback: ${error.message}`, 'warning', true); } finally { renderApp(); routeFromHash(); maybeDimSum(); }
}
function routeFromHash() { const articleMatch = /^#\/articles\/([^/?#]+)/.exec(location.hash); if (articleMatch) return showArticle(decodeURIComponent(articleMatch[1]), true); const tab = location.hash.replace(/^#/, ''); if (TAB_BY_ID[tab]) openTab(tab); }
document.addEventListener('keydown', globalKeyHandler);
document.addEventListener('click', closeMenuWhenOutside);
document.addEventListener('contextmenu', contextMenuHandler);
window.addEventListener('hashchange', routeFromHash);
window.matchMedia('(max-width: 900px)').addEventListener?.('change', () => renderApp());
window.addEventListener('resize', () => positionAnchoredDialog(activeDialogId));
renderApp();
loadCatalog();
setInterval(() => { if (JSON.stringify(effectivePresentation()) !== lastPresentationSignature) renderApp(); }, 30_000);
