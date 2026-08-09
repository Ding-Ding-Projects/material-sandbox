import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

const PAGE_PATH = fileURLToPath(new URL('../docs/index.html', import.meta.url));
const PAGE_HTML = readFileSync(PAGE_PATH, 'utf8');

function decodeEntities(value) {
  return value.replace(/&(#x[\da-f]+|#\d+|amp|apos|gt|lt|quot);/gi, (match, entity) => {
    const normalized = entity.toLowerCase();
    if (normalized === 'amp') return '&';
    if (normalized === 'apos') return "'";
    if (normalized === 'gt') return '>';
    if (normalized === 'lt') return '<';
    if (normalized === 'quot') return '"';
    const radix = normalized.startsWith('#x') ? 16 : 10;
    const digits = normalized.replace(/^#x?/, '');
    return String.fromCodePoint(Number.parseInt(digits, radix));
  });
}

function dataKey(name) {
  return name.slice(5).replace(/-([a-z])/g, (_, character) => character.toUpperCase());
}

function dataAttribute(key) {
  return 'data-' + String(key).replace(/[A-Z]/g, (character) => '-' + character.toLowerCase());
}

class MicroStyle {
  constructor() {
    this.values = new Map();
  }

  setProperty(name, value) {
    this.values.set(String(name), String(value));
  }

  getPropertyValue(name) {
    return this.values.get(String(name)) ?? '';
  }
}

class MicroClassList {
  constructor(owner) {
    this.owner = owner;
    this.values = new Set();
  }

  replace(value) {
    this.values = new Set(String(value).split(/\s+/).filter(Boolean));
    this.commit();
  }

  commit() {
    this.owner.attributesMap.set('class', [...this.values].join(' '));
  }

  add(...tokens) {
    tokens.forEach((token) => this.values.add(String(token)));
    this.commit();
  }

  remove(...tokens) {
    tokens.forEach((token) => this.values.delete(String(token)));
    this.commit();
  }

  contains(token) {
    return this.values.has(String(token));
  }

  toggle(token, force) {
    const normalized = String(token);
    const enabled = force === undefined ? !this.values.has(normalized) : Boolean(force);
    if (enabled) this.values.add(normalized);
    else this.values.delete(normalized);
    this.commit();
    return enabled;
  }

  toString() {
    return [...this.values].join(' ');
  }
}

class MicroText {
  constructor(ownerDocument, data) {
    this.ownerDocument = ownerDocument;
    this.parentNode = null;
    this.nodeType = 3;
    this.data = String(data);
  }

  get parentElement() {
    return this.parentNode instanceof MicroElement ? this.parentNode : null;
  }

  get textContent() {
    return this.data;
  }

  set textContent(value) {
    this.data = String(value);
  }
}

function splitSelector(selector, separator) {
  const parts = [];
  let start = 0;
  let bracketDepth = 0;
  for (let index = 0; index < selector.length; index += 1) {
    const character = selector[index];
    if (character === '[') bracketDepth += 1;
    if (character === ']') bracketDepth -= 1;
    if (bracketDepth === 0 && separator(character)) {
      const part = selector.slice(start, index).trim();
      if (part) parts.push(part);
      start = index + 1;
    }
  }
  const tail = selector.slice(start).trim();
  if (tail) parts.push(tail);
  return parts;
}

function parseSimpleSelector(selector) {
  let rest = selector.trim();
  const parsed = { tag: null, id: null, classes: [], attributes: [] };
  const tag = /^[a-z][\w-]*/i.exec(rest);
  if (tag) {
    parsed.tag = tag[0].toUpperCase();
    rest = rest.slice(tag[0].length);
  }
  while (rest) {
    const classMatch = /^\.([\w-]+)/.exec(rest);
    if (classMatch) {
      parsed.classes.push(classMatch[1]);
      rest = rest.slice(classMatch[0].length);
      continue;
    }
    const idMatch = /^#([\w-]+)/.exec(rest);
    if (idMatch) {
      parsed.id = idMatch[1];
      rest = rest.slice(idMatch[0].length);
      continue;
    }
    if (rest.startsWith('[')) {
      const end = rest.indexOf(']');
      if (end === -1) throw new Error(`Unsupported selector: ${selector}`);
      const expression = rest.slice(1, end).trim();
      const attributeMatch = /^([^\s=]+)(?:\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s]+)))?$/.exec(expression);
      if (!attributeMatch) throw new Error(`Unsupported selector: ${selector}`);
      parsed.attributes.push({
        name: attributeMatch[1].toLowerCase(),
        value: attributeMatch[2] ?? attributeMatch[3] ?? attributeMatch[4] ?? null
      });
      rest = rest.slice(end + 1);
      continue;
    }
    throw new Error(`Unsupported selector: ${selector}`);
  }
  if (!parsed.tag && !parsed.id && !parsed.classes.length && !parsed.attributes.length) {
    throw new Error(`Unsupported selector: ${selector}`);
  }
  return parsed;
}

const selectorCache = new Map();
function compileSelector(selector) {
  if (selectorCache.has(selector)) return selectorCache.get(selector);
  const groups = splitSelector(selector, (character) => character === ',').map((group) =>
    splitSelector(group, (character) => /\s/.test(character)).map(parseSimpleSelector)
  );
  selectorCache.set(selector, groups);
  return groups;
}

function matchesSimple(element, selector) {
  if (selector.tag && element.tagName !== selector.tag) return false;
  if (selector.id && element.id !== selector.id) return false;
  if (selector.classes.some((name) => !element.classList.contains(name))) return false;
  return selector.attributes.every(({ name, value }) => {
    if (!element.hasAttribute(name)) return false;
    return value === null || element.getAttribute(name) === value;
  });
}

function matchesSelectorGroup(element, selectors) {
  if (!matchesSimple(element, selectors.at(-1))) return false;
  let ancestor = element.parentElement;
  for (let index = selectors.length - 2; index >= 0; index -= 1) {
    while (ancestor && !matchesSimple(ancestor, selectors[index])) ancestor = ancestor.parentElement;
    if (!ancestor) return false;
    ancestor = ancestor.parentElement;
  }
  return true;
}

function descendants(element, includeSelf = false) {
  const result = [];
  const visit = (node) => {
    if (node instanceof MicroElement) result.push(node);
    if (node?.childNodes) node.childNodes.forEach(visit);
  };
  if (includeSelf) visit(element);
  else element.childNodes.forEach(visit);
  return result;
}

class MicroElement extends EventTarget {
  constructor(ownerDocument, tagName) {
    super();
    this.ownerDocument = ownerDocument;
    this.parentNode = null;
    this.nodeType = 1;
    this.tagName = String(tagName).toUpperCase();
    this.childNodes = [];
    this.attributesMap = new Map();
    this._id = '';
    this._value = '';
    this._valueWasSet = false;
    this.classList = new MicroClassList(this);
    this.style = new MicroStyle();
    this.hidden = false;
    this.checked = false;
    this.disabled = false;
    this.inert = false;
    this.tabIndex = -1;
    this.selectionStart = 0;
    this.selectionEnd = 0;
    const store = Object.create(null);
    this.dataset = new Proxy(store, {
      set: (target, key, value) => {
        const normalized = String(value);
        target[key] = normalized;
        this.attributesMap.set(dataAttribute(key), normalized);
        return true;
      }
    });
  }

  get parentElement() {
    return this.parentNode instanceof MicroElement ? this.parentNode : null;
  }

  get children() {
    return this.childNodes.filter((node) => node instanceof MicroElement);
  }

  get id() {
    return this._id;
  }

  set id(value) {
    const normalized = String(value);
    if (this._id) this.ownerDocument.unregisterId(this._id, this);
    this._id = normalized;
    if (normalized) {
      this.attributesMap.set('id', normalized);
      this.ownerDocument.registerId(normalized, this);
    } else {
      this.attributesMap.delete('id');
    }
  }

  get className() {
    return this.classList.toString();
  }

  set className(value) {
    this.classList.replace(value);
  }

  get value() {
    if (this.tagName === 'OPTION' && !this._valueWasSet) return this.textContent;
    return this._value;
  }

  set value(value) {
    this._value = String(value);
    this._valueWasSet = true;
    if (this.tagName === 'OUTPUT') this.textContent = this._value;
  }

  get href() {
    const raw = this.getAttribute('href') ?? '';
    return new URL(raw, this.ownerDocument.location.href).href;
  }

  set href(value) {
    this.setAttribute('href', value);
  }

  get textContent() {
    return this.childNodes.map((node) => node.textContent).join('');
  }

  set textContent(value) {
    const text = String(value);
    this.replaceChildren(...(text ? [this.ownerDocument.createTextNode(text)] : []));
  }

  get innerText() {
    return this.textContent;
  }

  set innerText(value) {
    this.textContent = value;
  }

  get offsetParent() {
    let node = this;
    while (node) {
      if (node.hidden) return null;
      if (node.classList.contains('builder') && !node.classList.contains('open')) return null;
      if (node.classList.contains('dialog-surface') && !node.classList.contains('open')) return null;
      node = node.parentElement;
    }
    return this.ownerDocument.body;
  }

  append(...nodes) {
    nodes.forEach((node) => this.appendChild(node));
  }

  appendChild(node) {
    const child = node instanceof MicroElement || node instanceof MicroText
      ? node
      : this.ownerDocument.createTextNode(node);
    if (child.parentNode) {
      child.parentNode.childNodes = child.parentNode.childNodes.filter((candidate) => candidate !== child);
    }
    child.parentNode = this;
    this.childNodes.push(child);
    if (this.tagName === 'TEXTAREA' && child instanceof MicroText && !this._valueWasSet) this._value += child.data;
    if (this.tagName === 'SELECT' && child instanceof MicroElement && child.tagName === 'OPTION' && !this._valueWasSet) {
      this._value = child.value;
      this._valueWasSet = true;
    }
    return child;
  }

  replaceChildren(...nodes) {
    this.childNodes.forEach((node) => { node.parentNode = null; });
    this.childNodes = [];
    nodes.forEach((node) => this.appendChild(node));
  }

  remove() {
    if (!this.parentNode) return;
    this.parentNode.childNodes = this.parentNode.childNodes.filter((node) => node !== this);
    this.parentNode = null;
  }

  setAttribute(name, value) {
    const normalizedName = String(name).toLowerCase();
    const normalizedValue = String(value);
    if (normalizedName === 'id') {
      this.id = normalizedValue;
      return;
    }
    if (normalizedName === 'class') {
      this.className = normalizedValue;
      return;
    }
    this.attributesMap.set(normalizedName, normalizedValue);
    if (normalizedName.startsWith('data-')) this.dataset[dataKey(normalizedName)] = normalizedValue;
    if (normalizedName === 'value') this.value = normalizedValue;
    if (normalizedName === 'checked') this.checked = true;
    if (normalizedName === 'disabled') this.disabled = true;
    if (normalizedName === 'hidden') this.hidden = true;
    if (normalizedName === 'tabindex') this.tabIndex = Number(normalizedValue);
    if (normalizedName === 'placeholder') this.placeholder = normalizedValue;
    if (normalizedName === 'type') this.type = normalizedValue;
    if (normalizedName === 'lang') this.lang = normalizedValue;
  }

  getAttribute(name) {
    return this.attributesMap.get(String(name).toLowerCase()) ?? null;
  }

  hasAttribute(name) {
    return this.attributesMap.has(String(name).toLowerCase());
  }

  removeAttribute(name) {
    const normalizedName = String(name).toLowerCase();
    if (normalizedName === 'id') this.id = '';
    else if (normalizedName === 'class') this.className = '';
    else {
      this.attributesMap.delete(normalizedName);
      if (normalizedName.startsWith('data-')) delete this.dataset[dataKey(normalizedName)];
      if (normalizedName === 'checked') this.checked = false;
      if (normalizedName === 'disabled') this.disabled = false;
      if (normalizedName === 'hidden') this.hidden = false;
    }
  }

  matches(selector) {
    return compileSelector(selector).some((group) => matchesSelectorGroup(this, group));
  }

  querySelectorAll(selector) {
    const groups = compileSelector(selector);
    return descendants(this).filter((element) => groups.some((group) => matchesSelectorGroup(element, group)));
  }

  querySelector(selector) {
    return this.querySelectorAll(selector)[0] ?? null;
  }

  closest(selector) {
    let element = this;
    while (element) {
      if (element.matches(selector)) return element;
      element = element.parentElement;
    }
    return null;
  }

  focus() {
    this.ownerDocument.activeElement = this;
  }

  scrollIntoView(options) {
    this.lastScrollOptions = options;
  }

  setRangeText(replacement, start, end) {
    this.value = this.value.slice(0, start) + replacement + this.value.slice(end);
    this.selectionStart = this.selectionEnd = start + replacement.length;
  }

  click() {
    this.dispatchEvent(new Event('click', { bubbles: true, cancelable: true }));
  }
}

class MicroDocument extends EventTarget {
  constructor(location) {
    super();
    this.location = location;
    this.ids = new Map();
    this.documentElement = new MicroElement(this, 'html');
    this.body = new MicroElement(this, 'body');
    this.documentElement.append(this.body);
    this.activeElement = this.body;
  }

  registerId(id, element) {
    this.ids.set(id, element);
  }

  unregisterId(id, element) {
    if (this.ids.get(id) === element) this.ids.delete(id);
  }

  createElement(tagName) {
    return new MicroElement(this, tagName);
  }

  createTextNode(value) {
    return new MicroText(this, value);
  }

  getElementById(id) {
    return this.ids.get(String(id)) ?? null;
  }

  querySelectorAll(selector) {
    const groups = compileSelector(selector);
    return descendants(this.documentElement, true).filter((element) =>
      groups.some((group) => matchesSelectorGroup(element, group))
    );
  }

  querySelector(selector) {
    return this.querySelectorAll(selector)[0] ?? null;
  }
}

class MicroStorage {
  constructor(seed = {}) {
    this.values = new Map(Object.entries(seed).map(([key, value]) => [key, String(value)]));
  }

  getItem(key) {
    return this.values.get(String(key)) ?? null;
  }

  setItem(key, value) {
    this.values.set(String(key), String(value));
  }

  removeItem(key) {
    this.values.delete(String(key));
  }
}

class MicroLocation {
  constructor(hash = '') {
    this.url = new URL('https://example.test/docs/index.html');
    this.reloadCount = 0;
    if (hash) this.hash = hash;
  }

  get href() {
    return this.url.href;
  }

  set href(value) {
    this.url = new URL(value, this.url);
  }

  get hash() {
    return this.url.hash;
  }

  set hash(value) {
    this.url.hash = String(value);
  }

  reload() {
    this.reloadCount += 1;
  }
}

class MicroWindow extends EventTarget {}

function applyAttributes(element, source) {
  const attributePattern = /([^\s=/>]+)(?:\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s"'=<>`]+)))?/g;
  let match;
  while ((match = attributePattern.exec(source))) {
    const value = decodeEntities(match[2] ?? match[3] ?? match[4] ?? '');
    element.setAttribute(match[1], value);
  }
}

function parsePage(document) {
  const htmlTag = /<html\b([^>]*)>/i.exec(PAGE_HTML);
  const body = /<body\b([^>]*)>([\s\S]*?)<script>/i.exec(PAGE_HTML);
  assert.ok(htmlTag, 'docs/index.html must have an html element');
  assert.ok(body, 'docs/index.html must have a body followed by its inline script');
  applyAttributes(document.documentElement, htmlTag[1]);
  applyAttributes(document.body, body[1]);

  const voidElements = new Set(['AREA', 'BASE', 'BR', 'COL', 'EMBED', 'HR', 'IMG', 'INPUT', 'LINK', 'META', 'PARAM', 'SOURCE', 'TRACK', 'WBR']);
  const stack = [document.body];
  const tokens = body[2].match(/<!--[\s\S]*?-->|<\/?[a-z][^>]*>|[^<]+/gi) ?? [];
  for (const token of tokens) {
    if (token.startsWith('<!--')) continue;
    if (token.startsWith('</')) {
      const tagName = /^<\/\s*([a-z][\w-]*)/i.exec(token)?.[1].toUpperCase();
      assert.equal(stack.at(-1).tagName, tagName, `micro-DOM parser found mismatched closing tag ${token}`);
      stack.pop();
      continue;
    }
    if (token.startsWith('<')) {
      const opening = /^<\s*([a-z][\w-]*)([\s\S]*?)\/?\s*>$/i.exec(token);
      assert.ok(opening, `micro-DOM parser could not parse ${token}`);
      const element = document.createElement(opening[1]);
      applyAttributes(element, opening[2]);
      stack.at(-1).append(element);
      if (!voidElements.has(element.tagName) && !token.endsWith('/>')) stack.push(element);
      continue;
    }
    stack.at(-1).append(document.createTextNode(decodeEntities(token)));
  }
  assert.equal(stack.length, 1, 'micro-DOM parser must finish at the body element');
}

function extractInlineScript() {
  const scripts = [...PAGE_HTML.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/gi)];
  assert.equal(scripts.length, 1, 'the interaction gate expects one inline page script');
  return scripts[0][1];
}

function createPage({
  storageSeed = {},
  hash = '',
  width = 1200,
  clipboard = { writeText: async () => {} },
  controlledWorkers = false
} = {}) {
  const location = new MicroLocation(hash);
  const document = new MicroDocument(location);
  parsePage(document);
  const localStorage = new MicroStorage(storageSeed);
  const window = new MicroWindow();
  const pendingTimers = new Map();
  let nextTimer = 1;
  const setTimeout = (callback, delay) => {
    const id = nextTimer++;
    pendingTimers.set(id, { callback, delay });
    return id;
  };
  const clearTimeout = (id) => pendingTimers.delete(id);
  const history = {
    calls: [],
    replaceState(state, title, url) {
      this.calls.push({ state, title, url });
      location.href = url;
    }
  };
  const matchMedia = (query) => ({
    matches: query.includes('max-width: 760px') ? width <= 760 : false,
    media: query,
    addEventListener() {},
    removeEventListener() {}
  });
  class UnexpectedWorker {
    constructor(url) {
      throw new Error(`Unexpected worker construction in micro-DOM interaction: ${url}`);
    }
  }
  const workerController = { workers: [] };
  class ControlledWorker extends EventTarget {
    constructor(url) {
      super();
      this.url = url;
      this.payload = null;
      this.terminated = false;
      workerController.workers.push(this);
    }

    postMessage(payload) {
      this.payload = payload;
      if (!Object.hasOwn(payload, 'sample')) {
        queueMicrotask(() => {
          if (!this.terminated) {
            this.emitMessage({
              id: payload.id,
              ok: true,
              results: (payload.values ?? []).map(() => true),
              matches: []
            });
          }
        });
      }
    }

    terminate() {
      this.terminated = true;
    }

    emitMessage(data) {
      const event = new Event('message');
      Object.defineProperty(event, 'data', { value: data });
      this.dispatchEvent(event);
    }
  }
  window.document = document;
  window.location = location;
  window.history = history;
  window.localStorage = localStorage;
  window.setTimeout = setTimeout;
  window.clearTimeout = clearTimeout;
  window.matchMedia = matchMedia;

  const context = vm.createContext({
    console,
    document,
    window,
    location,
    history,
    localStorage,
    navigator: { clipboard },
    HTMLElement: MicroElement,
    Event,
    Worker: controlledWorkers ? ControlledWorker : UnexpectedWorker,
    URL,
    setTimeout,
    clearTimeout,
    matchMedia,
    addEventListener: window.addEventListener.bind(window),
    removeEventListener: window.removeEventListener.bind(window)
  });
  vm.runInContext(extractInlineScript(), context, { filename: 'docs/index.html:inline-script' });
  return {
    document,
    history,
    localStorage,
    location,
    pendingTimers,
    workerController,
    window,
    byId: (id) => {
      const element = document.getElementById(id);
      assert.ok(element, `expected #${id} in docs/index.html`);
      return element;
    }
  };
}

async function settle() {
  for (let turn = 0; turn < 4; turn += 1) await Promise.resolve();
  await new Promise((resolve) => setImmediate(resolve));
}

async function enter(element, value) {
  element.value = value;
  element.dispatchEvent(new Event('input', { bubbles: true }));
  await settle();
}

function paletteResult(page, label) {
  const result = page.byId('paletteResults').children.find((candidate) =>
    candidate.querySelector('strong')?.textContent.trim() === label
  );
  assert.ok(result, `expected command palette result ${label}`);
  return result;
}

test('palette setting teleport keeps focus on the exact target', async () => {
  const page = createPage();
  await settle();
  page.byId('paletteButton').focus();
  page.byId('paletteButton').click();
  await enter(page.byId('paletteSearch'), 'Font scale');

  paletteResult(page, 'Font scale 100%').click();

  assert.equal(page.document.activeElement, page.byId('fontScale'));
  assert.equal(page.byId('settings').hidden, false);
  assert.equal(page.byId('tab-settings').getAttribute('aria-selected'), 'true');
  assert.equal(page.byId('paletteSurface').getAttribute('aria-hidden'), 'true');
  assert.notEqual(page.document.activeElement, page.byId('paletteButton'));
});

test('feature heading is indexed as an article command', async () => {
  const page = createPage();
  await settle();
  page.byId('paletteButton').click();
  await enter(page.byId('paletteSearch'), 'Appearance editor');

  const result = paletteResult(page, 'Appearance editor');
  assert.equal(result.querySelector('small').textContent, 'Article');
  result.click();

  assert.match(page.location.href, /\/docs\/articles\/appearance-editor\.html$/);
  assert.equal(page.byId('paletteSurface').getAttribute('aria-hidden'), 'true');
});

test('invalid regex clears prior feature and settings results', async () => {
  const page = createPage();
  await settle();
  const featureCards = page.document.querySelectorAll('.feature-card');
  const settings = page.document.querySelectorAll('.setting');

  await enter(page.byId('featureSearch'), 'Appearance editor');
  assert.equal(featureCards.filter((card) => !card.hidden).length, 1);
  page.byId('featureSearch').value = '[';
  page.byId('featurePattern').value = '[';
  page.byId('featureRegexMode').checked = true;
  page.byId('featureRegexMode').dispatchEvent(new Event('change', { bubbles: true }));
  await settle();
  assert.equal(featureCards.filter((card) => !card.hidden).length, 0);
  assert.match(page.byId('featureRegexStatus').textContent, /Invalid pattern:/);
  assert.equal(page.byId('featureCount').textContent, 'No results until the pattern is valid.');

  await enter(page.byId('settingsSearch'), 'Font scale');
  assert.equal(settings.filter((setting) => !setting.hidden).length, 1);
  page.byId('settingsSearch').value = '[';
  page.byId('settingsPattern').value = '[';
  page.byId('settingsRegexMode').checked = true;
  page.byId('settingsRegexMode').dispatchEvent(new Event('change', { bubbles: true }));
  await settle();
  assert.equal(settings.filter((setting) => !setting.hidden).length, 0);
  assert.match(page.byId('settingsRegexStatus').textContent, /Invalid pattern:/);
  assert.equal(page.byId('settingsCount').textContent, 'No results until the pattern is valid.');
});

test('legacy preferences migrate once and current values win', async () => {
  const migrated = createPage({
    storageSeed: {
      'sandboxie-pages-theme': 'dark',
      'sandboxie-pages-tabDock': 'right'
    }
  });
  await settle();
  assert.equal(migrated.document.documentElement.dataset.theme, 'dark');
  assert.equal(migrated.document.documentElement.dataset.tabDock, 'right');
  assert.equal(migrated.localStorage.getItem('sandboxie-pages-v3-theme'), 'dark');
  assert.equal(migrated.localStorage.getItem('sandboxie-pages-v3-tabDock'), 'right');
  assert.equal(migrated.localStorage.getItem('sandboxie-pages-theme'), null);
  assert.equal(migrated.localStorage.getItem('sandboxie-pages-tabDock'), null);

  const currentWins = createPage({
    storageSeed: {
      'sandboxie-pages-v3-theme': 'light',
      'sandboxie-pages-theme': 'dark',
      'sandboxie-pages-v3-tabDock': 'top',
      'sandboxie-pages-tabDock': 'right'
    }
  });
  await settle();
  assert.equal(currentWins.document.documentElement.dataset.theme, 'light');
  assert.equal(currentWins.document.documentElement.dataset.tabDock, 'top');
  assert.equal(currentWins.localStorage.getItem('sandboxie-pages-v3-theme'), 'light');
  assert.equal(currentWins.localStorage.getItem('sandboxie-pages-v3-tabDock'), 'top');
});

test('initial and changed hashes activate the matching tab', async () => {
  const page = createPage({ hash: '#settings' });
  await settle();
  assert.equal(page.byId('settings').hidden, false);
  assert.equal(page.byId('tab-settings').getAttribute('aria-selected'), 'true');

  page.location.hash = '#overview';
  page.window.dispatchEvent(new Event('hashchange'));

  assert.equal(page.byId('overview').hidden, false);
  assert.equal(page.byId('settings').hidden, true);
  assert.equal(page.byId('tab-overview').getAttribute('aria-selected'), 'true');
  assert.equal(page.location.hash, '#overview');
});

test('palette reopen resets hidden regex state', async () => {
  const page = createPage();
  await settle();
  page.byId('paletteButton').click();
  page.byId('paletteBuilderButton').click();
  page.byId('paletteSearch').value = '[';
  page.byId('palettePattern').value = '[';
  page.byId('paletteFlags').value = 'm';
  page.byId('paletteRegexMode').checked = true;
  page.byId('paletteRegexMode').dispatchEvent(new Event('change', { bubbles: true }));
  await settle();
  assert.equal(page.byId('paletteRegexBuilder').classList.contains('open'), true);
  assert.equal(page.byId('paletteRegexMode').checked, true);

  page.byId('paletteClose').click();
  page.byId('paletteButton').click();
  await settle();

  assert.equal(page.byId('paletteSearch').value, '');
  assert.equal(page.byId('palettePattern').value, '');
  assert.equal(page.byId('paletteFlags').value, 'i');
  assert.equal(page.byId('paletteRegexMode').checked, false);
  assert.equal(page.byId('paletteRegexBuilder').classList.contains('open'), false);
  assert.equal(page.byId('paletteBuilderButton').getAttribute('aria-expanded'), 'false');
  assert.equal(page.byId('paletteRegexStatus').textContent, 'Plain text is active.');
});

test('a deferred valid sample cannot overwrite a newer invalid sample state', async () => {
  const page = createPage({ controlledWorkers: true });
  await settle();
  page.byId('featureSearch').value = 'Appearance';
  page.byId('featurePattern').value = 'Appearance';
  page.byId('featureRegexMode').checked = true;
  page.byId('featureRegexMode').dispatchEvent(new Event('change', { bubbles: true }));
  await settle();

  const deferredSample = page.workerController.workers.find((worker) =>
    Object.hasOwn(worker.payload ?? {}, 'sample')
  );
  assert.ok(deferredSample, 'expected a deferred sample worker request');
  assert.equal(
    page.byId('featureSampleResult').textContent,
    'Enable regex mode and enter a pattern to inspect sample matches.'
  );

  page.byId('featurePattern').value = '[';
  page.byId('featurePattern').dispatchEvent(new Event('input', { bubbles: true }));
  await settle();
  const invalidState = page.byId('featureSampleResult').textContent;
  assert.equal(invalidState, 'Sample not evaluated until the pattern and flags are safe and valid.');

  deferredSample.emitMessage({
    id: deferredSample.payload.id,
    ok: true,
    matches: [{ captures: [] }],
    results: []
  });
  await settle();

  assert.equal(page.byId('featureSampleResult').textContent, invalidState);
  assert.match(page.byId('featureRegexStatus').textContent, /Invalid pattern:/);
});

test('palette clipboard errors stay visible and survive later information notices', async () => {
  let clipboardRefused = true;
  const page = createPage({
    clipboard: {
      async writeText() {
        if (clipboardRefused) throw new Error('Clipboard permission denied');
      }
    }
  });
  await settle();
  page.byId('paletteButton').click();
  page.byId('palettePattern').value = 'Appearance';
  page.byId('paletteCopyPattern').click();
  await settle();

  const paletteNotices = page.byId('paletteInlineNotifications');
  assert.equal(paletteNotices.children.length, 1);
  const persistentError = paletteNotices.children[0];
  assert.equal(persistentError.dataset.severity, 'error');
  assert.equal(persistentError.getAttribute('role'), 'alert');
  assert.match(persistentError.textContent, /The browser refused clipboard access\./);
  assert.notEqual(persistentError.offsetParent, null);
  assert.equal(page.pendingTimers.size, 0, 'persistent error must not receive an auto-dismiss timer');

  clipboardRefused = false;
  page.byId('paletteCopyPattern').click();
  await settle();

  assert.equal(paletteNotices.children.length, 2);
  assert.equal(paletteNotices.children[0], persistentError);
  assert.equal(persistentError.parentElement, paletteNotices);
  assert.match(paletteNotices.children[1].textContent, /Regular expression copied\./);
  assert.equal(page.pendingTimers.size, 1, 'only the later information notice auto-dismisses');
});

test('bilingual selects keep native options in English and describe both languages', async () => {
  const page = createPage();
  await settle();
  page.byId('language').value = 'bi';
  page.byId('language').dispatchEvent(new Event('change', { bubbles: true }));
  await settle();

  const selects = page.document.querySelectorAll('select');
  assert.ok(selects.length > 0);
  selects.forEach((select) => {
    const options = select.querySelectorAll('option');
    assert.ok(options.length > 0, `expected native options for #${select.id}`);
    options.forEach((option) => {
      assert.equal(option.textContent, option.dataset.en);
      assert.equal(option.lang, 'en');
    });

    const summaryId = select.getAttribute('aria-describedby');
    assert.equal(summaryId, `${select.id}-language-summary`);
    const summary = page.byId(summaryId);
    assert.equal(summary.children.length, 2);
    assert.equal(summary.children[0].lang, 'en');
    assert.equal(summary.children[1].lang, 'yue-Hant-HK');
    assert.match(summary.children[0].textContent, /^Options: /);
    assert.match(summary.children[1].textContent, /^選項：/);
  });
});
