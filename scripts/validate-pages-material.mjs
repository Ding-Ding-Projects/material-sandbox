import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const pagePath = path.join(root, 'docs', 'index.html');
const source = fs.readFileSync(pagePath, 'utf8');
const regexWorkerPath = path.join(root, 'docs', 'regex-worker.js');
const regexWorkerSource = fs.existsSync(regexWorkerPath) ? fs.readFileSync(regexWorkerPath, 'utf8') : '';
const revisionStampPath = path.join(root, 'scripts', 'stamp-pages-revision.mjs');
const revisionStampSource = fs.existsSync(revisionStampPath) ? fs.readFileSync(revisionStampPath, 'utf8') : '';
const revisionStampTestPath = path.join(root, 'scripts', 'test-pages-stamp.mjs');
const revisionStampTestSource = fs.existsSync(revisionStampTestPath) ? fs.readFileSync(revisionStampTestPath, 'utf8') : '';
const pagesWorkflowPath = path.join(root, '.github', 'workflows', 'pages.yml');
const pagesWorkflow = fs.existsSync(pagesWorkflowPath) ? fs.readFileSync(pagesWorkflowPath, 'utf8') : '';
const interactionTestPath = path.join(root, 'scripts', 'test-pages-interactions.mjs');
const interactionTestSource = fs.existsSync(interactionTestPath) ? fs.readFileSync(interactionTestPath, 'utf8') : '';
const articleBuilderPath = path.join(root, 'scripts', 'build-pages-articles.mjs');
const articleBuilderSource = fs.existsSync(articleBuilderPath) ? fs.readFileSync(articleBuilderPath, 'utf8') : '';
const articleTestPath = path.join(root, 'scripts', 'test-pages-article-renderer.mjs');
const articleTestSource = fs.existsSync(articleTestPath) ? fs.readFileSync(articleTestPath, 'utf8') : '';

const documentedFeatureSlugs = [
  'material-design.md', 'features/appearance-editor.md', 'features/scheduled-settings.md',
  'features/school-mode.md', 'features/tab-discovery.md', 'features/settings-history.md',
  'features/notifications.md', 'features/external-editor.md', 'features/command-palette.md',
  'features/dim-sum-surprise.md', 'features/color-translator.md', 'features/contributor-build-audit.md',
  'features/settings-provenance.md', 'features/build-entrypoints.md', 'features/destructive-confirmation.md',
  'features/native-ci-evidence.md', 'features/ui/m3-shell-boundary.md', 'features/editor-settings.md',
  'features/changelog-viewer.md', 'features/pages-language-tone.md', 'features/pages-a11y-boundary.md',
];

const localizedAccessibleTargets = [
  { name: 'brand', selector: '.brand' },
  { name: 'language', id: 'language' },
  { name: 'themeButton', id: 'themeButton' },
  { name: 'paletteButton', id: 'paletteButton' },
  { name: 'publicationStatus', id: 'publicationStatus' },
  { name: 'siteTabs', id: 'siteTabs' },
  { name: 'featureSearch', id: 'featureSearch' },
  { name: 'featureRegexBuilder', id: 'featureRegexBuilder' },
  { name: 'settingsSearch', id: 'settingsSearch' },
  { name: 'settingsRegexBuilder', id: 'settingsRegexBuilder' },
  { name: 'accentColor', id: 'accentColor' },
  { name: 'accentHex', id: 'accentHex' },
  { name: 'resetSettings', id: 'resetSettings' },
  { name: 'notificationStack', id: 'notificationStack' },
  { name: 'paletteSearch', id: 'paletteSearch' },
  { name: 'paletteRegexBuilder', id: 'paletteRegexBuilder' },
  { name: 'paletteClose', id: 'paletteClose' },
];

const localizedOptionSelectIds = [
  'language',
  'featureConstruct',
  'settingsConstruct',
  'paletteConstruct',
  'density',
  'tabDock',
  'motion',
];

const requiredTokens = [
  ['M3 color roles', '--md-sys-color-primary'],
  ['runtime accent seed', '--seed: #6750a4'],
  ['M3 shape tokens', '--md-sys-shape-extra-large'],
  ['M3 elevation', '--md-sys-elevation-1'],
  ['reduced motion', 'prefers-reduced-motion'],
  ['light/dark theme', 'data-theme="dark"'],
  ['vertical-default tablist', 'aria-orientation="vertical"'],
  ['persisted four-edge docking', 'id="tabDock"'],
  ['axis-aware tab keyboard handling', 'effectiveOrientation'],
  ['feature full regex builder', 'id="featureConstruct"'],
  ['settings full regex builder', 'id="settingsConstruct"'],
  ['palette full regex builder', 'id="paletteConstruct"'],
  ['regex sample and capture feedback', 'SampleResult'],
  ['guided literal insertion', 'AddLiteral'],
  ['regex copy action', 'CopyPattern'],
  ['command palette shortcut', 'Ctrl+Shift+F'],
  ['command palette settings inventory', 'settings.forEach((setting)'],
  ['command palette feature-name inventory', '.feature-card h3'],
  ['command palette exact-target teleport', 'teleport-target'],
  ['non-blocking notification host', 'id="notificationStack"'],
  ['notification emoji preference wiring', "$('emojiToggle').checked"],
  ['notification history', 'id="notificationHistory"'],
  ['conditional installer status', 'verified installer button remains absent'],
  ['documented feature article inventory', 'feature-card'],
  ['settings search', 'id="settingsSearch"'],
  ['settings regex builder', 'id="settingsRegexBuilder"'],
  ['settings search wiring', 'filterSettings'],
  ['dynamic document language', 'root.lang'],
  ['five-level English tone', 'tiny party hat'],
  ['five-level Cantonese tone', '迷你派對帽'],
  ['density token consumers', 'padding: var(--card-padding)'],
  ['continuous accent control', 'type="color"'],
  ['accent format translator', 'rgbToHsl'],
  ['honest Designer boundary', 'two Designer forms remain'],
  ['48-pixel notification action', '.notification button { min-width: 48px; min-height: 48px;'],
];

const requiredPatterns = [
  ['explicit regex pattern limit', /\bREGEX_PATTERN_LIMIT\s*=\s*120\b/],
  ['explicit regex sample limit', /\bREGEX_SAMPLE_LIMIT\s*=\s*1200\b/],
  ['explicit regex match limit', /\bREGEX_MATCH_LIMIT\s*=\s*20\b/],
  ['programmatic pattern length enforcement', /(?:pattern|query)\.length\s*>\s*REGEX_PATTERN_LIMIT/],
  ['programmatic sample length enforcement', /(?:rawSample|sample)\.length\s*>\s*REGEX_SAMPLE_LIMIT/],
  ['guided regex insertion length enforcement', /function\s+insertAtCursor\b[\s\S]{0,800}\bREGEX_PATTERN_LIMIT\b/],
  ['conservative regex safety definition', /function\s+(?:isConservativelySafeRegex|isSafeRegexPattern|validateRegexSafety)\s*\(/],
  ['conservative regex safety call', /\b(?:isConservativelySafeRegex|isSafeRegexPattern|validateRegexSafety)\s*\(\s*(?:query|pattern)\s*\)/],
  ['bounded regex execution helper', /function\s+(?:collectBoundedMatches|runBoundedRegex|boundedRegexTest|safeRegexTest)\s*\(/],
  ['local regex worker creation', /new\s+Worker\s*\(\s*['"]regex-worker\.js['"]\s*\)/],
  ['regex worker timeout', /window\.setTimeout\s*\([\s\S]{0,300}worker\.terminate\s*\(\)[\s\S]{0,300}REGEX_TIMEOUT_MS/],
  ['regex worker completion cleanup', /window\.clearTimeout\s*\(\s*timer\s*\)[\s\S]{0,200}worker\.terminate\s*\(\)/],
  ['bounded regex payload', /worker\.postMessage\s*\(\s*\{[\s\S]{0,200}maxMatches\s*:\s*REGEX_MATCH_LIMIT/],
  ['superseded regex worker cancellation', /function\s+cancelBoundedRegex\s*\([^)]+\)[\s\S]{0,500}worker\.terminate\s*\(\)/],
  ['sample generation advances before validation', /async\s+function\s+updateSample\s*\([^)]*\)\s*\{[\s\S]{0,260}sampleGenerations\.set\([^;]+;[\s\S]{0,180}const\s+pattern\s*=/],
  ['safe storage read wrapper', /try\s*\{[\s\S]{0,500}localStorage\.getItem[\s\S]{0,500}catch(?:\s*\([^)]*\))?\s*\{/],
  ['safe storage write wrapper', /try\s*\{[\s\S]{0,500}localStorage\.setItem[\s\S]{0,500}catch(?:\s*\([^)]*\))?\s*\{/],
  ['safe storage removal wrapper', /try\s*\{[\s\S]{0,500}localStorage\.removeItem[\s\S]{0,500}catch(?:\s*\([^)]*\))?\s*\{/],
  ['versioned storage schema', /\b(?:STORAGE_SCHEMA_VERSION|storageSchemaVersion)\b|\bconst\s+storage\s*=\s*['"][^'"]*-v\d+-['"]/],
  ['known preference key inventory', /\bpreferenceKeys\s*=\s*\[[^\]]+\]/],
  ['storage migration writes then removes legacy key', /\b(?:migratePreferences|migrateStorage|runStorageMigration)\s*\(|const\s+migrated\b[\s\S]{0,500}storageWrite\(storage\s*\+\s*key,\s*migrated\)[\s\S]{0,300}storageRemove\(legacyStorage\s*\+\s*key\)/],
  ['preference enum allowlists', /\b(?:PREFERENCE_ENUMS|allowedPreferenceValues|preferenceAllowlists|allowedValues)\s*=/],
  ['persistent warning and error policy', /const\s+persistent\s*=\s*options\.persistent\s*\?\?\s*severity\s*!==\s*['"]info['"]/],
  ['only nonpersistent notifications time out', /if\s*\(\s*!persistent\s*\)\s*notify\.timer\s*=\s*window\.setTimeout/],
  ['error notification assertive semantics', /severity\s*===\s*['"]error['"][\s\S]{0,300}['"]alert['"][\s\S]{0,300}['"]assertive['"]/],
  ['independent notification stack items', /document\.createElement\s*\(\s*['"]aside['"]\s*\)[\s\S]{0,2200}\bhost\.append\s*\(\s*item\s*\)/],
  ['palette-local notification host', /body\.classList\.contains\s*\(\s*['"]dialog-open['"]\s*\)\s*\?\s*\$\(\s*['"]paletteInlineNotifications['"]\s*\)/],
  ['clipboard refusal is persistent', /navigator\.clipboard\.writeText[\s\S]{0,700}catch\s*\{[\s\S]{0,350}(?:notify|notifyCopy)\([\s\S]{0,250}(?:severity\s*:\s*['"]error['"]|persistent\s*:\s*true)/],
  ['native option bilingual accessible summary', /function\s+updateSelectLanguageSummary\s*\([^)]+\)[\s\S]{0,1200}aria-describedby/],
  ['relative luminance calculation', /\brelativeLuminance\s*\(/],
  ['contrast ratio calculation', /\bcontrastRatio\s*\(/],
  ['contrast-safe role derivation', /\b(?:deriveContrastSafeRoles|deriveAccessibleRoles|ensureAccentContrast|ensureContrast)\s*\(/],
  ['accent applies derived primary role', /const\s+primary\s*=\s*(?:deriveContrastSafeRoles|deriveAccessibleRoles|ensureAccentContrast|ensureContrast)\s*\(/],
  ['accent applies derived on-primary role', /const\s+onPrimary\s*=\s*highestContrastText\s*\(\s*primary\s*\)/],
  ['accent applies derived on-container role', /const\s+onContainer\s*=\s*(?:highestContrastText\s*\(\s*container\s*\)|onSurface)\b/],
  ['accent container contrast checks', /contrastRatio\s*\(\s*onSurface\s*,\s*candidate\s*\)\s*>=\s*4\.5[\s\S]{0,250}contrastRatio\s*\(\s*onSurfaceVariant\s*,\s*candidate\s*\)\s*>=\s*4\.5[\s\S]{0,250}contrastRatio\s*\(\s*primary\s*,\s*candidate\s*\)\s*>=\s*3/],
  ['palette teleport retains destination focus', /closePalette\s*\(\s*(?:false|\{\s*restoreFocus\s*:\s*false\s*\})\s*\)/],
];

const requiredWorkerPatterns = [
  ['worker strict mode', /^['"]use strict['"];?/],
  ['worker message boundary', /self\.addEventListener\s*\(\s*['"]message['"]/],
  ['worker value-count bound', /values\.slice\s*\(\s*0\s*,\s*(?:256|REGEX_VALUE_LIMIT)\s*\)/],
  ['worker value-length bound', /String\s*\(\s*value\s*\)\.slice\s*\(\s*0\s*,\s*(?:1200|REGEX_SAMPLE_LIMIT)\s*\)/],
  ['worker match-count default', /maxMatches\s*=\s*(?:20|REGEX_MATCH_LIMIT)/],
  ['worker clamps requested match count', /const\s+matchLimit\s*=\s*Math\.min\s*\(\s*REGEX_MATCH_LIMIT\b/],
  ['worker bounded exec loop', /while\s*\(\s*matches\.length\s*<\s*(?:maxMatches|matchLimit)[\s\S]{0,180}\.exec\s*\(/],
  ['worker zero-width progress', /match\[0\]\s*===\s*['"]['"][\s\S]{0,120}lastIndex\s*\+=\s*1/],
  ['worker structured result', /self\.postMessage\s*\(\s*\{\s*id,\s*ok:\s*true,\s*results,\s*matches\s*\}\s*\)/],
];

const forbiddenWorkerPatterns = [
  ['worker fetch transport', /\bfetch\s*\(/],
  ['worker XHR transport', /\bXMLHttpRequest\b/],
  ['worker WebSocket transport', /\bWebSocket\b/],
  ['worker EventSource transport', /\bEventSource\b/],
  ['worker dynamic import', /\b(?:import|importScripts)\s*\(/],
  ['worker beacon transport', /\bsendBeacon\s*\(/],
];

const requiredRevisionStampPatterns = [
  ['stamp requires a full commit revision', /\^\[0-9a-f\]\{40\}\$/i],
  ['stamp copies into separate staging without dereferencing links', /fs\.cpSync\s*\(\s*source\s*,\s*target\s*,\s*\{\s*recursive:\s*true\s*,\s*dereference:\s*false\s*\}\s*\)/],
  ['stamp replaces every revision marker', /replaceAll\s*\(\s*['"]PAGES_SOURCE_REVISION['"]\s*,\s*revision\.toLowerCase\s*\(\s*\)\s*\)/],
  ['stamp rejects a leftover marker', /stamped\.includes\s*\(\s*['"]PAGES_SOURCE_REVISION['"]\s*\)/],
  ['stamp rejects linked source docs', /function\s+assertSafeSourceDirectory\s*\([\s\S]{0,800}isSymbolicLink\s*\(\)/],
  ['stamp defines a recursive source-link scan', /function\s+assertNoLinkedTree\s*\(/],
  ['stamp scans source before replacing staging', /assertNoLinkedTree\s*\(\s*source\s*,[\s\S]{0,500}fs\.rmSync\s*\(\s*target/],
  ['stamp scans copied staging before reading its index', /fs\.cpSync\s*\([\s\S]{0,300}assertNoLinkedTree\s*\(\s*target\s*,[\s\S]{0,300}fs\.readFileSync\s*\(\s*indexPath/],
  ['stamp validates destructive staging target', /function\s+assertSafeStagingTarget\s*\(/],
  ['stamp validates target before recursive removal', /assertSafeStagingTarget\s*\([^)]*\)[\s\S]{0,500}fs\.rmSync\s*\(/],
  ['stamp defines a staged-tree marker scan', /function\s+findRevisionMarkers\s*\([^)]*\)\s*\{[\s\S]{0,1800}PAGES_SOURCE_REVISION/],
  ['stamp invokes the staged-tree marker scan', /remainingMarkers\s*=\s*findRevisionMarkers\s*\(\s*target\s*\)/],
];

const forbiddenPagePatterns = [
  ['CSS imports', /@import\b/i],
  ['CSS resource URLs', /url\s*\(/i],
  ['embedded frame or media surface', /<(?:iframe|frame|frameset|video|audio|source|object|embed)\b/i],
  ['remote script, style, or image asset', /<(?:script|link|img)\b[^>]*(?:https?:|\/\/)/i],
  ['fetch transport', /\bfetch\s*\(/],
  ['beacon transport', /\bsendBeacon\s*\(/],
  ['XMLHttpRequest transport', /\bXMLHttpRequest\b/],
  ['WebSocket transport', /\bWebSocket\b/],
  ['EventSource transport', /\bEventSource\b/],
  ['dynamic module transport', /\bimport\s*\(/],
  ['form submission surface', /<form\b/i],
  ['meta refresh transport', /<meta\b[^>]*http-equiv\s*=\s*['"]?refresh\b/i],
  ['link ping transport', /<a\b[^>]*\bping\s*=/i],
  ['clipboard read', /navigator\.clipboard\.(?:read|readText)\s*\(/],
  ['HTML injection sink', /\.(?:innerHTML|outerHTML)\s*=|insertAdjacentHTML\s*\(|document\.write\s*\(/],
  ['script evaluation sink', /\beval\s*\(|\bnew\s+Function\s*\(|\bDOMParser\b|createContextualFragment\s*\(/],
  ['srcdoc injection sink', /\bsrcdoc\s*=/i],
  ['javascript URL', /javascript\s*:/i],
  ['unbounded eager regex enumeration', /\[\s*\.\.\.[\s\S]{0,120}\.matchAll\s*\(/],
  ['raw Markdown page link', /href\s*=\s*['"][^'"]+\.md(?:[#?][^'"]*)?['"]/i],
  ['overstated complete feature implementation', /Complete feature inventory/i],
  ['overstated ground-up completion', /rebuilt from the ground up/i],
];

// Hashes keep private conversation-only wording out of this public validator.
// The scanner compares normalized one-to-four-word windows from public copy.
const forbiddenPublicCopyHashes = new Set([
  '7d88c59728ba99b2c9c10d3b7649914b0c35b45eb2336c89ed9cba6a386cac65',
  'de347469cb9e4f8857f163f423490c40ab11fec4b0e9febf47916d193df6c8d6',
  'ea290d5c0101746d2acdf3307e2edfbd75625f70e581f728fa8edb2df04b20ab',
  '6165bce356e843c12b11468c76daacf155e51b9b57bf8ce436a42f6add8ea961',
  '8d4101980f7aea1f57bd0c5c3d8ae5d2b3df6b5b588ba23b6bec7bf18f35320f',
  '2f87855c713d5f889d07b279a3dda3f02e1622e8f15485d7b2be397a86bd5ae2',
  'dbed88fc857037bf943a45030a6db2695265f416a39201a8b9cd228d6e0c0c51',
  '84bc89ec998e50ef4128478268f9dc5d4759bfe42cae1984ce71bbff5d04ba92',
  'eb6993e7ef74063f34ee6f66595f27235aaf7a4a3e48048429891357fd5caf01',
  'fee027b9030261b4fb0c4f8be94f5405a2233750e4bf9087522ce10a2bc5ab1d',
  'ba17a17a25ec4291a936baf526d49a5f844a26dc4e19f806ef15433a37d0be1a',
  'cfa7a27a457b7bb6fb0391077d6594cd8c2cd70584b974f6af5de1ad7618c5ab',
  'fc48aa3d0d3e402bb2dbf5c15f18095d2dea7f227ff235663ac384d5a937c6bf',
  '29dc31dea9c17f7701875dc80ee36f93dc7785dab5c47ae59548892f9f9eb25b',
  '2ac5c71840c7da3cccbfe06fcbd8662e23e7d8f767a33fcc0c299326eecd8b38',
  'c9dc55be02947a12637eeaa302cc49dd6d3a56c1e00d427efafd64d142a4275d',
  '23d62e2aee5b720dd35e52879ffb5b4792ab504e190cf6420d342d56fe8854dd',
  'f806836d7af53ba698c0ac8e9da47ad0a3a08eef93c6c441826cd95d3c1437f1',
  'f34cb69ff11dd777721e97909b9b6dd984932112d32556f59d081b97b579db6f',
  'e0bb49efd22be7abac25a2e1fd659f762439c1a373ddbc96a55bae851e2073cf',
  '759af6620b06cfd2e94837fce5a6594b3fe61f09284aed32b1a63f6aad0fe011',
  '948ad468eb5f7087f672bc575ee600aa315ce5c0fc9fc74b9aab03b387daabc1',
  '3e410dac6e32fd39cd86f255c0c3ea71aefcf2549edb1f4799d6cb7a8afacd83',
  '59fb684b567248f119c63a4f97a1b1994367625a811478d125c3a0c53773fbd7',
  'a94556ef19a97d3e587d9d419797d391e778dc90b6d6278f22ccf3d2738d78e1',
  '540c2e62e55d2305d45e683ee808750e2327d7e9404052a0159a7ce2b88c8a9b',
  'ab6c580ee143cfc7ecab073a87f641db6ea5f398f97a668615ad3eda108471ec',
  '2a6c4d16d2c973a587e500c331278cdcc06af5052f10cd2576dfc988e268de97',
  'e35867779a75d7f9263685df193c9b93f36df2fb33f4d9b5863e5e3dbf6daeb6',
  '42d1c5fa647263ff0eba22dbe7f95040589832e2dec8da593800f3091914fec6',
]);

function openingTagForId(page, id) {
  const escaped = id.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  return page.match(new RegExp(`<[^>]+\\bid="${escaped}"[^>]*>`, 'i'))?.[0] ?? '';
}

function openingTagForTarget(page, target) {
  if (target.id) return openingTagForId(page, target.id);
  const className = target.selector.slice(1).replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  return page.match(new RegExp(`<[^>]+\\bclass="[^"]*\\b${className}\\b[^"]*"[^>]*>`, 'i'))?.[0] ?? '';
}

function accessibleInventoryExpression(target) {
  const reference = target.reference
    ? target.reference.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
    : target.id
      ? `\\$\\(['"]${target.id.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}['"]\\)`
      : `document\\.querySelector\\(['"]${target.selector.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}['"]\\)`;
  return new RegExp(`\\[\\s*${reference}\\s*,\\s*['"][^'"]+['"]\\s*,\\s*['"][^'"]+['"]\\s*\\]`);
}

function localizedNameIsWired(page, target) {
  const tag = openingTagForTarget(page, target);
  if (!tag) return false;
  const hasStaticAriaLabel = /\baria-label="[^"]+"/i.test(tag);
  const hasLocalizedAriaData = /\bdata-aria-en="[^"]+"/i.test(tag) && /\bdata-aria-zh="[^"]+"/i.test(tag);
  const inventoryIsApplied = /accessibleNames\.forEach\s*\(\s*\(\[control,\s*en,\s*zh\]\)\s*=>\s*setAccessibleName\(control,\s*en,\s*zh\)\s*\)/.test(page);
  const hasLocalizedInventoryEntry = inventoryIsApplied && accessibleInventoryExpression(target).test(page);
  if (hasStaticAriaLabel) return hasLocalizedAriaData || hasLocalizedInventoryEntry;

  const idExpression = target.id?.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const label = idExpression
    ? page.match(new RegExp(`<label\\b[^>]*\\bfor="${idExpression}"[^>]*>[\\s\\S]*?<\\/label>`, 'i'))?.[0] ?? ''
    : '';
  return (/data-en="[^"]+"/i.test(label) && /data-zh="[^"]+"/i.test(label))
    || hasLocalizedAriaData || hasLocalizedInventoryEntry;
}

function sourceRevisionMarkerIsValid(page, workflow) {
  const tag = page.match(/<[^>]+\bid="sourceRevision"[^>]*>/i)?.[0] ?? '';
  const revision = tag.match(/\bdata-source-revision="([^"]+)"/i)?.[1] ?? '';
  if (/^[0-9a-f]{7,40}$/i.test(revision)) return true;
  if (revision !== 'PAGES_SOURCE_REVISION') return false;

  const stamp = workflow.indexOf('node scripts/stamp-pages-revision.mjs docs .pages-site "$GITHUB_SHA"');
  const upload = workflow.indexOf('actions/upload-pages-artifact');
  const uploadPath = workflow.indexOf('path: .pages-site', upload);
  return stamp >= 0 && upload > stamp && uploadPath > upload;
}

function pagesWorkflowPublicationGateIsValid(workflow) {
  const material = workflow.indexOf('node scripts/validate-pages-material.mjs');
  const materialSelfTest = workflow.indexOf('node scripts/validate-pages-material.mjs --self-test');
  const accessibility = workflow.indexOf('pwsh -NoProfile -File scripts/validate-pages-a11y.ps1');
  const accessibilitySelfTest = workflow.indexOf('pwsh -NoProfile -File scripts/validate-pages-a11y.ps1 -SelfTest');
  const stampTest = workflow.indexOf('node --test scripts/test-pages-stamp.mjs');
  const interaction = workflow.indexOf('node --test scripts/test-pages-interactions.mjs');
  const articleTest = workflow.indexOf('node --test scripts/test-pages-article-renderer.mjs');
  const stamp = workflow.indexOf('node scripts/stamp-pages-revision.mjs docs .pages-site "$GITHUB_SHA"');
  const articleBuild = workflow.indexOf('node scripts/build-pages-articles.mjs docs .pages-site "$GITHUB_SHA"');
  const upload = workflow.indexOf('actions/upload-pages-artifact');
  return material >= 0
    && materialSelfTest > material
    && accessibility > materialSelfTest
    && accessibilitySelfTest > accessibility
    && stampTest > accessibilitySelfTest
    && interaction > stampTest
    && articleTest > interaction
    && stamp > articleTest
    && articleBuild > stamp
    && upload > articleBuild;
}

function selectOptionsAreLocalized(page, id) {
  const idExpression = id.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const select = page.match(new RegExp(`<select\\b[^>]*\\bid="${idExpression}"[^>]*>[\\s\\S]*?<\\/select>`, 'i'))?.[0] ?? '';
  if (!select) return false;
  const options = [...select.matchAll(/<option\b[^>]*>/gi)].map((match) => match[0]);
  return options.length > 0 && options.every((option) => /\bdata-en="[^"]+"/i.test(option) && /\bdata-zh="[^"]+"/i.test(option));
}

function validatePage(page) {
  const failures = [];
  const fail = (name, evidence) => failures.push([name, evidence]);

  for (const [name, token] of requiredTokens) {
    if (!page.includes(token)) fail(name, token);
  }
  for (const [name, pattern] of requiredPatterns) {
    if (!pattern.test(page)) fail(name, pattern.source);
  }
  for (const [name, pattern] of forbiddenPagePatterns) {
    if (pattern.test(page)) fail('forbidden ' + name, pattern.source);
  }
  if (!regexWorkerSource) fail('local regex worker', 'docs/regex-worker.js');
  for (const [name, pattern] of requiredWorkerPatterns) {
    if (!pattern.test(regexWorkerSource)) fail(name, pattern.source);
  }
  for (const [name, pattern] of forbiddenWorkerPatterns) {
    if (pattern.test(regexWorkerSource)) fail('forbidden ' + name, pattern.source);
  }
  if (!revisionStampSource) fail('revision stamping helper', 'scripts/stamp-pages-revision.mjs');
  if (!revisionStampTestSource) fail('revision stamping test source', 'scripts/test-pages-stamp.mjs');
  for (const [name, pattern] of requiredRevisionStampPatterns) {
    if (!pattern.test(revisionStampSource)) fail(name, pattern.source);
  }
  if (!sourceRevisionMarkerIsValid(page, pagesWorkflow)) {
    fail('source revision marker', 'literal commit revision or staged GITHUB_SHA stamping before Pages upload');
  }
  if (!interactionTestSource) fail('Pages interaction test source', 'scripts/test-pages-interactions.mjs');
  if (!articleBuilderSource) fail('Pages article renderer source', 'scripts/build-pages-articles.mjs');
  if (!articleTestSource) fail('Pages article renderer test source', 'scripts/test-pages-article-renderer.mjs');
  if (!pagesWorkflowPublicationGateIsValid(pagesWorkflow)) {
    fail('Pages publication workflow gate', 'self-tested validation, interactions, article test, revision stamp, article build, then upload');
  }
  if (/time-limited worker/i.test(page)
      && !(/\bnew\s+Worker\s*\(/.test(page) && /\.terminate\s*\(/.test(page) && /setTimeout\s*\(/.test(page))) {
    fail('honest regex execution boundary', 'a time-limited worker claim requires Worker, timeout, and termination code');
  }
  const timeoutMs = Number(page.match(/\bREGEX_TIMEOUT_MS\s*=\s*(\d+)\b/)?.[1]);
  if (!Number.isInteger(timeoutMs) || timeoutMs < 20 || timeoutMs > 500) {
    fail('bounded regex timeout value', 'REGEX_TIMEOUT_MS must be between 20 and 500 milliseconds');
  }

  for (const prefix of ['feature', 'settings', 'palette']) {
    const searchTag = openingTagForId(page, `${prefix}Search`);
    const patternTag = openingTagForId(page, `${prefix}Pattern`);
    const sampleTag = openingTagForId(page, `${prefix}Sample`);
    if (!/\bmaxlength="120"/i.test(searchTag) || !/\bmaxlength="120"/i.test(patternTag)) {
      fail('regex pattern length inventory', prefix);
    }
    if (!/\bmaxlength="1200"/i.test(sampleTag)) fail('regex sample length inventory', prefix);
  }

  const inlineScripts = [...page.matchAll(/<script>([\s\S]*?)<\/script>/g)];
  if (inlineScripts.length !== 1) fail('inline behavior inventory', 'exactly one locally bundled script');
  for (const [, script] of inlineScripts) {
    try {
      new Function(script);
    } catch (error) {
      fail('inline script syntax', error.message);
    }
  }

  const ids = [...page.matchAll(/\bid="([^"]+)"/g)].map((match) => match[1]);
  const duplicateIds = [...new Set(ids.filter((id, index) => ids.indexOf(id) !== index))];
  if (duplicateIds.length) fail('unique element ids', duplicateIds.join(', '));
  const idSet = new Set(ids);
  for (const match of page.matchAll(/\b(?:aria-controls|aria-labelledby|for)="([^"]+)"/g)) {
    for (const target of match[1].split(/\s+/)) {
      if (target && !idSet.has(target)) fail('referenced control target', target);
    }
  }

  for (const match of page.matchAll(/<([a-z][\w-]*)\b([^>]*\baria-label="[^"]+"[^>]*)>/gi)) {
    const wholeTag = match[0];
    const id = wholeTag.match(/\bid="([^"]+)"/i)?.[1];
    const className = wholeTag.match(/\bclass="([^"]+)"/i)?.[1];
    const hasLocalizedData = /\bdata-aria-en="[^"]+"/i.test(wholeTag) && /\bdata-aria-zh="[^"]+"/i.test(wholeTag);
    const inventoryTarget = localizedAccessibleTargets.find((target) => target.id === id
      || (!target.id && className?.split(/\s+/).includes(target.selector.slice(1))));
    if (!hasLocalizedData && !(inventoryTarget && localizedNameIsWired(page, inventoryTarget))) {
      fail('localized static aria-label', id || className || wholeTag.slice(0, 80));
    }
  }

  for (const target of localizedAccessibleTargets) {
    if (!localizedNameIsWired(page, target)) fail('localized accessible-name inventory', target.name);
  }
  for (const id of localizedOptionSelectIds) {
    if (!selectOptionsAreLocalized(page, id)) fail('localized option inventory', id);
  }

  const featureCount = (page.match(/class="card feature-card"/g) || []).length;
  for (const slug of documentedFeatureSlugs) {
    const articleHref = `articles/${path.basename(slug, '.md')}.html`;
    if (!page.includes(`href="${articleHref}"`)) fail('documented feature article', articleHref);
  }
  if (featureCount !== documentedFeatureSlugs.length) {
    fail('documented feature article count', `exactly ${documentedFeatureSlugs.length} cards`);
  }

  return { failures, featureCount };
}

function collectPublicCopyFiles() {
  const files = [path.join(root, 'README.md')];
  const extensions = new Set(['.css', '.html', '.js', '.json', '.md', '.txt', '.yaml', '.yml']);
  const visit = (directory) => {
    for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
      const absolute = path.join(directory, entry.name);
      if (entry.isDirectory()) visit(absolute);
      else if (extensions.has(path.extname(entry.name).toLowerCase())) files.push(absolute);
    }
  };
  visit(path.join(root, 'docs'));
  return files;
}

function findForbiddenPublicCopy(files) {
  const findings = [];
  for (const file of files) {
    const words = fs.readFileSync(file, 'utf8').normalize('NFKC').toLocaleLowerCase('en-US').match(/[\p{L}\p{N}]+/gu) ?? [];
    let found = false;
    for (let size = 1; size <= 4 && !found; size += 1) {
      for (let index = 0; index + size <= words.length; index += 1) {
        const digest = crypto.createHash('sha256').update(words.slice(index, index + size).join(' ')).digest('hex');
        if (forbiddenPublicCopyHashes.has(digest)) {
          findings.push(path.relative(root, file).replaceAll('\\', '/'));
          found = true;
          break;
        }
      }
    }
  }
  return findings;
}

function printFailures(failures) {
  console.error('Pages Material contract failed');
  for (const [name, evidence] of failures) console.error(`- ${name}: ${evidence}`);
}

const result = validatePage(source);
for (const file of findForbiddenPublicCopy(collectPublicCopyFiles())) {
  result.failures.push(['public-copy conversation vocabulary boundary', file]);
}

if (process.argv.includes('--self-test')) {
  const mutations = [
    {
      name: 'CSS import is rejected',
      source: source.replace('</style>', '@import "network-style.css";\n</style>'),
      expected: 'forbidden CSS imports',
    },
    {
      name: 'source revision removal is rejected',
      source: source.replace('id="sourceRevision"', 'id="sourceRevisionRemoved"'),
      expected: 'source revision marker',
    },
    {
      name: 'regex safety removal is rejected',
      source: source.replace(/\b(?:isConservativelySafeRegex|isSafeRegexPattern|validateRegexSafety)\b/g, 'removedRegexSafetyContract'),
      expected: 'conservative regex safety definition',
    },
    {
      name: 'raw Markdown navigation is rejected',
      source: source.replace('articles/appearance-editor.html', 'features/appearance-editor.md'),
      expected: 'forbidden raw Markdown page link',
    },
  ];
  if (result.failures.length) {
    printFailures(result.failures);
    process.exit(1);
  }
  for (const mutation of mutations) {
    const mutated = validatePage(mutation.source);
    if (!mutated.failures.some(([name]) => name === mutation.expected)) {
      console.error(`Pages Material self-test failed: ${mutation.name}`);
      process.exit(1);
    }
    console.log(`PASS mutation: ${mutation.name}`);
  }
  const workflowWithoutArticles = pagesWorkflow.replace(
    'node scripts/build-pages-articles.mjs docs .pages-site "$GITHUB_SHA"',
    'node scripts/build-pages-articles-removed.mjs docs .pages-site "$GITHUB_SHA"',
  );
  if (pagesWorkflowPublicationGateIsValid(workflowWithoutArticles)) {
    console.error('Pages Material self-test failed: article publication removal is rejected');
    process.exit(1);
  }
  console.log('PASS mutation: article publication removal is rejected');
  console.log(`pages-material-self-test mutations=${mutations.length + 1}`);
  process.exit(0);
}

if (result.failures.length) {
  printFailures(result.failures);
  process.exit(1);
}

const publicCopyFiles = collectPublicCopyFiles();
const structuralCheckCount = requiredPatterns.length + requiredWorkerPatterns.length + requiredRevisionStampPatterns.length
  + forbiddenPagePatterns.length + forbiddenWorkerPatterns.length
  + localizedAccessibleTargets.length + localizedOptionSelectIds.length + documentedFeatureSlugs.length + 14;
console.log(`pages-material-contract checks=${requiredTokens.length + structuralCheckCount} features=${result.featureCount} publicFiles=${publicCopyFiles.length}`);
