import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { after, test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { buildHeadingSlugs, canonicalScreenshotAssets, resolveInternalDocumentationLink } from './validate-docs.mjs';

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const validatorPath = path.join(repositoryRoot, 'scripts', 'validate-docs.mjs');
const temporaryRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-offline-docs-'));

after(() => fs.rmSync(temporaryRoot, { recursive: true, force: true }));

function copyFile(fixture, relativePath) {
  const source = path.join(repositoryRoot, relativePath);
  const target = path.join(fixture, relativePath);
  fs.mkdirSync(path.dirname(target), { recursive: true });
  fs.copyFileSync(source, target);
}

function makeFixture(name) {
  const fixture = path.join(temporaryRoot, name);
  fs.mkdirSync(fixture, { recursive: true });
  fs.cpSync(path.join(repositoryRoot, 'docs'), path.join(fixture, 'docs'), { recursive: true });
  for (const relativePath of [
    'SandboxiePlus/SandMan/Resources/SandMan.qrc',
    'SandboxiePlus/SandMan/Windows/DocumentationBrowser.cpp',
    'SandboxiePlus/SandMan/Windows/DocumentationBrowser.h',
    'SandboxiePlus/SandMan/SandMan.cpp',
    'SandboxiePlus/SandMan/SandMan.pri',
    'SandboxiePlus/SandMan/SandMan.vcxproj',
  ]) copyFile(fixture, relativePath);
  for (const asset of canonicalScreenshotAssets)
    copyFile(fixture, `SandboxiePlus/SandMan/Resources/${asset.resource}`);
  return fixture;
}

function runValidator(fixture) {
  return spawnSync(process.execPath, [validatorPath, '--root', fixture, '--skip-git-history'], {
    cwd: fixture,
    encoding: 'utf8',
    windowsHide: true,
  });
}

function replaceOnce(file, before, afterText) {
  const original = fs.readFileSync(file, 'utf8');
  assert.equal(original.includes(before), true, `fixture mutation anchor is missing: ${before}`);
  fs.writeFileSync(file, original.replace(before, afterText), 'utf8');
}

function insertBeforeSuggestedArticles(file, text) {
  const marker = '\nSuggested articles:';
  const original = fs.readFileSync(file, 'utf8');
  assert.equal(original.includes(marker), true, 'fixture Suggested articles anchor is missing');
  fs.writeFileSync(file, original.replace(marker, `\n${text}\n${marker}`), 'utf8');
}

test('complete repository contract validates 23 feature articles and one supplemental document', () => {
  const result = runValidator(makeFixture('complete'));
  assert.equal(result.status, 0, result.stderr || result.stdout);
  assert.match(result.stdout, /articles=23 supplemental=1 suggested=23 images=24 internal-links=64 external-links=138/);
});

test('oversized and excessive changelog inventories fail before offline splitting or rendering', () => {
  const oversizedFixture = makeFixture('oversized-changelog');
  const oversizedChangelog = path.join(oversizedFixture, 'docs', 'changelog.md');
  fs.appendFileSync(oversizedChangelog, `\n${'x'.repeat(512 * 1024)}\n`, 'utf8');
  const oversizedResult = runValidator(oversizedFixture);
  assert.notEqual(oversizedResult.status, 0);
  assert.match(oversizedResult.stderr, /documentation changelog exceeds the 524288-byte offline limit/);

  const entryFixture = makeFixture('excessive-changelog-entries');
  const entryChangelog = path.join(entryFixture, 'docs', 'changelog.md');
  fs.appendFileSync(entryChangelog,
    `\n${Array.from({ length: 513 }, (_, index) => `## Synthetic entry ${index + 1}\n`).join('\n')}`,
    'utf8');
  const entryResult = runValidator(entryFixture);
  assert.notEqual(entryResult.status, 0);
  assert.match(entryResult.stderr, /documentation changelog exceeds the 512-entry offline limit/);
});

test('removing a Qt article resource fails the exact inventory guard', () => {
  const fixture = makeFixture('missing-qrc-article');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  const entry = '        <file alias="Docs/articles/pages-a11y-boundary.md">../../../docs/features/pages-a11y-boundary.md</file>';
  replaceOnce(qrc, entry, '');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /Qt documentation resource set must contain exactly 50 entries; found 49/);
});

test('commenting out a Qt article resource fails the comment-aware inventory guard', () => {
  const fixture = makeFixture('commented-qrc-article');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  const entry = '        <file alias="Docs/articles/pages-a11y-boundary.md">../../../docs/features/pages-a11y-boundary.md</file>';
  replaceOnce(qrc, entry, `        <!-- ${entry.trim()} -->`);
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /Qt documentation resource set must contain exactly 50 entries; found 49/);
});

test('wrapping a Qt article resource in a processing instruction fails the structural inventory guard', () => {
  const fixture = makeFixture('pi-wrapped-qrc-article');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  const entry = '        <file alias="Docs/articles/pages-a11y-boundary.md">../../../docs/features/pages-a11y-boundary.md</file>';
  replaceOnce(qrc, entry, `        <?ignored ${entry.trim()} ?>`);
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /Qt documentation resource set must contain exactly 50 entries; found 49/);
});

test('Qt documentation resources reject attributes that change RCC payload semantics', () => {
  const fixture = makeFixture('qrc-empty-attribute');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  replaceOnce(qrc,
    '<file alias="Docs/articles/pages-a11y-boundary.md">',
    '<file alias="Docs/articles/pages-a11y-boundary.md" empty="true">');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /must use only its exact alias attribute/);
});

test('Qt documentation resources require the exact root prefix without a language override', () => {
  const fixture = makeFixture('qrc-prefix-drift');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  replaceOnce(qrc, '<qresource prefix="/">', '<qresource prefix="/moved" lang="en">');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /must use exactly qresource prefix="\/" with no language override/);
});

test('Qt resource structure rejects wrappers that rcc does not accept', () => {
  const fixture = makeFixture('qrc-invalid-wrapper');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  replaceOnce(qrc, '    <qresource prefix="/">', '    <ignored>\n    <qresource prefix="/">');
  replaceOnce(qrc, '    </qresource>', '    </qresource>\n    </ignored>');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /unsupported element ignored|allows qresource only as a direct RCC child/);
});

test('commented qmake and MSBuild registrations cannot satisfy the active project inventory', () => {
  const qmakeFixture = makeFixture('commented-qmake-registration');
  const pri = path.join(qmakeFixture, 'SandboxiePlus', 'SandMan', 'SandMan.pri');
  replaceOnce(pri,
    '    ./Windows/DocumentationBrowser.cpp \\',
    '    # ./Windows/DocumentationBrowser.cpp \\');
  const qmakeResult = runValidator(qmakeFixture);
  assert.notEqual(qmakeResult.status, 0);
  assert.match(qmakeResult.stderr, /SandMan\.pri SOURCES must register \.\/Windows\/DocumentationBrowser\.cpp exactly once; found 0/);

  const msbuildFixture = makeFixture('commented-msbuild-registration');
  const vcxproj = path.join(msbuildFixture, 'SandboxiePlus', 'SandMan', 'SandMan.vcxproj');
  const element = '    <ClCompile Include="Windows\\DocumentationBrowser.cpp" />';
  replaceOnce(vcxproj, element, `    <!-- ${element.trim()} -->`);
  const msbuildResult = runValidator(msbuildFixture);
  assert.notEqual(msbuildResult.status, 0);
  assert.match(msbuildResult.stderr, /SandMan\.vcxproj must register <ClCompile Include="Windows\\DocumentationBrowser\.cpp" \/> exactly once; found 0/);
});

test('project registration must be effective in qmake and every MSBuild configuration', () => {
  const qmakeFixture = makeFixture('unused-qmake-registration');
  const pri = path.join(qmakeFixture, 'SandboxiePlus', 'SandMan', 'SandMan.pri');
  replaceOnce(pri, './Windows/DocumentationBrowser.cpp', './Windows/DocumentationBrowserMoved.cpp');
  fs.appendFileSync(pri, '\nUNUSED_TOKEN = ./Windows/DocumentationBrowser.cpp\n', 'utf8');
  const qmakeResult = runValidator(qmakeFixture);
  assert.notEqual(qmakeResult.status, 0);
  assert.match(qmakeResult.stderr, /SandMan\.pri SOURCES must register \.\/Windows\/DocumentationBrowser\.cpp exactly once; found 0/);

  const msbuildFixture = makeFixture('false-msbuild-item-groups');
  const vcxproj = path.join(msbuildFixture, 'SandboxiePlus', 'SandMan', 'SandMan.vcxproj');
  const project = fs.readFileSync(vcxproj, 'utf8');
  assert.equal(project.includes('<ItemGroup>'), true);
  fs.writeFileSync(vcxproj, project.replaceAll('<ItemGroup>', '<ItemGroup Condition="false">'), 'utf8');
  const msbuildResult = runValidator(msbuildFixture);
  assert.notEqual(msbuildResult.status, 0);
  assert.match(msbuildResult.stderr, /SandMan\.vcxproj must register <ClCompile Include="Windows\\DocumentationBrowser\.cpp" \/> exactly once; found 0/);
});

test('removing the browser manifest hook fails the runtime inventory guard', () => {
  const fixture = makeFixture('missing-browser-hook');
  const browser = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Windows', 'DocumentationBrowser.cpp');
  replaceOnce(browser,
    'QFile manifestFile(QStringLiteral(":/Docs/articles/index.json"))',
    'QFile manifestFile(QStringLiteral(":/Docs/articles/missing.json"))');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /DocumentationBrowser contract missing: QFile manifestFile/);
});

test('resource copies cannot replace the canonical documentation source', () => {
  const fixture = makeFixture('copied-resource-drift');
  const qrc = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.qrc');
  replaceOnce(qrc,
    '../../../docs/material-design.md</file>',
    'Docs/material-design.md</file>');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /Qt documentation resource source for Docs\/articles\/material-design\.md must be/);
});

test('an unresolved or escaping Markdown destination fails the link guard', () => {
  const fixture = makeFixture('bad-markdown-link');
  const article = path.join(fixture, 'docs', 'features', 'native-ci-evidence.md');
  replaceOnce(article, '../screenshots.md', '../../outside.md');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /local documentation link escapes docs/);
});

test('a Markdown fragment must match a real canonical heading slug', () => {
  const fixture = makeFixture('bad-heading-fragment');
  const article = path.join(fixture, 'docs', 'features', 'native-ci-evidence.md');
  replaceOnce(article, '../screenshots.md', '../screenshots.md#missing-heading');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /documentation fragment does not match a canonical heading: screenshots\.md#missing-heading/);
});

test('reference-style and angle ordinary links cannot evade destination validation', () => {
  for (const mutation of [
    { name: 'missing-reference', text: '[Missing][target]\n\n[target]: missing.md', expected: /local documentation destination is not bundled/ },
    { name: 'traversal-collapsed', text: '[Traversal][]\n\n[Traversal]: ..\/..\/outside.md', expected: /local documentation link escapes docs/ },
    { name: 'fragment-shortcut', text: '[Fragment]\n\n[Fragment]: ..\/material-design.md#not-real', expected: /fragment does not match a canonical heading/ },
    { name: 'scheme-reference', text: '[Unsafe][target]\n\n[target]: javascript:alert(1)', expected: /unsupported external documentation scheme: javascript/ },
    { name: 'angle-inline', text: '[Unsafe](<file:\/\/\/C:\/outside.md>)', expected: /unsupported external documentation scheme: file/ },
  ]) {
    const fixture = makeFixture(mutation.name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), mutation.text);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${mutation.name} link mutation unexpectedly validated`);
    assert.match(result.stderr, mutation.expected);
  }
});

test('multiline reference definitions cannot evade destination validation', () => {
  for (const [name, text] of [
    ['multiline-reference-indented', '[Unsafe][target]\n\n[target]:\n  file:///C:/outside.md'],
    ['multiline-reference-column-zero', '[Unsafe][target]\n\n[target]:\nfile:///C:/outside.md'],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} mutation unexpectedly validated`);
    assert.match(result.stderr, /unsupported external documentation scheme: file/);
  }
});

test('blockquote and list reference definitions cannot evade destination validation', () => {
  for (const [name, text, expected] of [
    ['blockquote-reference-link', '[Unsafe][target]\n\n> [target]: file:///C:/outside.md', /unsupported external documentation scheme: file/],
    ['list-reference-link', '[Unsafe][target]\n\n- [target]: file:///C:/outside.md', /unsupported external documentation scheme: file/],
    ['blockquote-reference-image', '![Unsafe][target]\n\n> [target]: qrc:/SandMan.png', /must not render Markdown images.*qrc:\/SandMan\.png/],
    ['list-reference-image', '![Unsafe][target]\n\n- [target]: file:///C:/outside.png', /must not render Markdown images.*file:\/\//],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} mutation unexpectedly validated`);
    assert.match(result.stderr, expected);
  }
});

test('nested-list continuation links and images remain inside the rendered security boundary', () => {
  for (const [name, text, expected] of [
    ['nested-list-link', '- nested item\n    [Unsafe](file:///C:/outside.md)', /unsupported external documentation scheme: file/],
    ['nested-list-image', '- nested item\n    ![Unsafe](file:///C:/outside.png)', /must not render Markdown images.*file:\/\//],
    ['nested-list-qrc-image', '- nested item\n    ![Unsafe](qrc:/SandMan.png)', /must not render Markdown images.*qrc:\/SandMan\.png/],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} mutation unexpectedly validated`);
    assert.match(result.stderr, expected);
  }
});

test('external HTTPS links require a host and reject embedded credentials', () => {
  for (const [name, text, expected] of [
    ['https-userinfo', '[Credential link](https://user@example.test/path?view=1#section)', /must not contain embedded credentials/],
    ['https-hostless', '[Hostless link](https:local-path)', /must include \/\/ and a host/],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} mutation unexpectedly validated`);
    assert.match(result.stderr, expected);
  }
});

test('Qt-rendered bare email and FTP autolinks pass through the scheme allowlist', () => {
  for (const [name, text, expected] of [
    ['bare-email-autolink', 'Contact test@example.com now.', /unsupported external documentation scheme: mailto/],
    ['bare-ftp-autolink', 'Visit ftp://example.com/path now.', /unsupported external documentation scheme: ftp/],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} mutation unexpectedly validated`);
    assert.match(result.stderr, expected);
  }
});

test('fragment and changelog routing stays aligned with the runtime article map', () => {
  const multipleFixture = makeFixture('multiple-fragment-delimiters');
  insertBeforeSuggestedArticles(path.join(multipleFixture, 'docs', 'features', 'native-ci-evidence.md'),
    '[Ambiguous fragment](../material-design.md#verification#extra)');
  const multipleResult = runValidator(multipleFixture);
  assert.notEqual(multipleResult.status, 0);
  assert.match(multipleResult.stderr, /more than one fragment delimiter/);

  const featureFixture = makeFixture('feature-to-changelog');
  insertBeforeSuggestedArticles(path.join(featureFixture, 'docs', 'features', 'native-ci-evidence.md'),
    '[Changelog](../changelog.md)');
  const featureResult = runValidator(featureFixture);
  assert.notEqual(featureResult.status, 0);
  assert.match(featureResult.stderr, /links between feature articles and changelog are not supported/);

  const changelogFixture = makeFixture('changelog-to-feature');
  fs.appendFileSync(path.join(changelogFixture, 'docs', 'changelog.md'),
    '\n\n[Feature article](features/native-ci-evidence.md)\n', 'utf8');
  const changelogResult = runValidator(changelogFixture);
  assert.notEqual(changelogResult.status, 0);
  assert.match(changelogResult.stderr, /links between feature articles and changelog are not supported/);
});

test('duplicate reference definitions and tab-indented pseudo-fences fail closed', () => {
  const duplicateFixture = makeFixture('duplicate-reference-definition');
  insertBeforeSuggestedArticles(path.join(duplicateFixture, 'docs', 'features', 'native-ci-evidence.md'),
    '[Target][duplicate]\n\n[duplicate]: file:///C:/outside.md\n[duplicate]: ../material-design.md');
  const duplicateResult = runValidator(duplicateFixture);
  assert.notEqual(duplicateResult.status, 0);
  assert.match(duplicateResult.stderr, /Markdown reference definition is duplicated: duplicate/);

  const fenceFixture = makeFixture('tab-indented-pseudo-fence');
  insertBeforeSuggestedArticles(path.join(fenceFixture, 'docs', 'features', 'native-ci-evidence.md'),
    '\t```\n![Unsafe image](file:///C:/outside.png)');
  const fenceResult = runValidator(fenceFixture);
  assert.notEqual(fenceResult.status, 0);
  assert.match(fenceResult.stderr, /feature article native-ci-evidence must not render Markdown images.*file:\/\/\//);
});

test('code-masked links and headings do not enter the rendered destination or anchor inventory', () => {
  const fixture = makeFixture('code-masked-markdown');
  insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'),
    '```markdown\n## Fake heading\n[Unsafe][target]\n[target]: file:///C:/outside.md\n```\n\n[Fake heading](#fake-heading)');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /fragment does not match a canonical heading.*#fake-heading/);
});

test('inline-code masking requires equal backtick delimiter runs', () => {
  for (const [name, text] of [
    ['one-to-two-backticks', '`[Unsafe](file:///C:/outside.md)``'],
    ['two-to-three-backticks', '``[Unsafe](file:///C:/outside.md)```'],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} unequal delimiter mutation unexpectedly validated`);
    assert.match(result.stderr, /unsupported external documentation scheme: file/);
  }

  for (const [name, text] of [
    ['balanced-one-backtick', '`[Inert](file:///C:/outside.md)`'],
    ['balanced-two-backticks', '``[Inert](file:///C:/outside.md)``'],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.equal(result.status, 0, result.stderr || `${name} balanced delimiter control failed`);
  }
});

test('backtick fence info strings cannot contain backticks', () => {
  const invalidFixture = makeFixture('invalid-backtick-fence-info');
  insertBeforeSuggestedArticles(path.join(invalidFixture, 'docs', 'features', 'native-ci-evidence.md'),
    '```bad`info\n[Unsafe](file:///C:/outside.md)\n```\n```');
  const invalidResult = runValidator(invalidFixture);
  assert.notEqual(invalidResult.status, 0);
  assert.match(invalidResult.stderr, /unsupported external documentation scheme: file/);

  for (const [name, text] of [
    ['normal-backtick-fence', '```text\n[Inert](file:///C:/outside.md)\n```'],
    ['normal-tilde-fence', '~~~bad`info\n[Inert](file:///C:/outside.md)\n~~~'],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.equal(result.status, 0, result.stderr || `${name} control failed`);
  }
});

test('Markdown opening escapes use odd-even backslash parity for links and images', () => {
  for (const count of [2, 4]) {
    for (const [kind, token, expected] of [
      ['link', '[Unsafe](file:///C:/outside.md)', /unsupported external documentation scheme: file/],
      ['image', '![Unsafe](file:///C:/outside.png)', /must not render Markdown images.*file:\/\//],
    ]) {
      const fixture = makeFixture(`even-${count}-${kind}-escape`);
      insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'),
        `${'\\'.repeat(count)}${token}`);
      const result = runValidator(fixture);
      assert.notEqual(result.status, 0, `even ${count}-${kind} escape mutation unexpectedly validated`);
      assert.match(result.stderr, expected);
    }
  }
  for (const count of [1, 3]) {
    for (const [kind, token] of [
      ['link', '[Inert](file:///C:/outside.md)'],
      ['image', '![Inert](file:///C:/outside.png)'],
    ]) {
      const fixture = makeFixture(`odd-${count}-${kind}-escape`);
      insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'),
        `${'\\'.repeat(count)}${token}`);
      const result = runValidator(fixture);
      assert.equal(result.status, 0, result.stderr || `odd ${count}-${kind} escape control failed`);
    }
  }
});

test('unsupported Setext and blockquote heading forms fail the canonical anchor contract', () => {
  for (const [name, heading, expected] of [
    ['setext-heading', 'Unsupported heading\n-------------------', /Setext heading/],
    ['blockquote-heading', '> ## Unsupported heading', /blockquote heading/],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), heading);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} mutation unexpectedly validated`);
    assert.match(result.stderr, expected);
  }
});

test('a fenced fake H1 cannot replace the one rendered canonical article title', () => {
  const fixture = makeFixture('fenced-fake-title');
  const article = path.join(fixture, 'docs', 'features', 'native-ci-evidence.md');
  const original = fs.readFileSync(article, 'utf8');
  const title = '# Native CI evidence and stale-run control';
  assert.equal(original.startsWith(title), true);
  fs.writeFileSync(article,
    `\`\`\`markdown\n${title}\n\`\`\`\n\n${original.replace(title, '## Native CI evidence and stale-run control')}`, 'utf8');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /must contain exactly one rendered ATX H1 heading; found 0/);
});

test('canonical heading slugs use rendered entities, inline code, and reference-link text', () => {
  for (const [name, text] of [
    ['entity-heading', '## Encoded &amp; heading\n\n[Jump](#encoded-heading)'],
    ['formatted-heading', '## **Bold** *Italic* `Code` [Link](https://example.com)\n\n[Jump](#bold-italic-code-link)'],
    ['reference-heading', '## [Reference heading][heading-ref]\n\n[Jump](#reference-heading)\n\n[heading-ref]: https://example.com/reference'],
  ]) {
    const fixture = makeFixture(name);
    insertBeforeSuggestedArticles(path.join(fixture, 'docs', 'features', 'native-ci-evidence.md'), text);
    const result = runValidator(fixture);
    assert.equal(result.status, 0, result.stderr || `${name} rendered-heading control failed`);
  }
});

test('all screenshot references must retain exact offline asset coverage', () => {
  const fixture = makeFixture('screenshot-reference-drift');
  const screenshots = path.join(fixture, 'docs', 'screenshots.md');
  replaceOnce(screenshots,
    '../SandboxiePlus/SandMan/Resources/SandMan.png',
    '../SandboxiePlus/SandMan/Resources/sandboxie-logo.png');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /screenshot image\[0\] destination spelling must be exactly/);
});

test('bundled screenshot assets reject corrupt signatures and oversized dimensions', () => {
  const corruptFixture = makeFixture('corrupt-png-signature');
  fs.writeFileSync(path.join(corruptFixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.png'),
    Buffer.from('not a PNG resource', 'utf8'));
  const corruptResult = runValidator(corruptFixture);
  assert.notEqual(corruptResult.status, 0);
  assert.match(corruptResult.stderr, /PNG size must be between|invalid PNG signature/);

  const dimensionsFixture = makeFixture('oversized-png-dimensions');
  const imagePath = path.join(dimensionsFixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.png');
  const image = fs.readFileSync(imagePath);
  image.writeUInt32BE(20000, 16);
  image.writeUInt32BE(20000, 20);
  fs.writeFileSync(imagePath, image);
  const dimensionsResult = runValidator(dimensionsFixture);
  assert.notEqual(dimensionsResult.status, 0);
  assert.match(dimensionsResult.stderr, /PNG dimensions are invalid or exceed the 100-megapixel bound: 20000x20000/);
});

test('bundled screenshot assets reject hard links and duplicated payloads', () => {
  const hardLinkFixture = makeFixture('hard-linked-png-assets');
  const hardLinkSource = path.join(hardLinkFixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.png');
  const hardLinkTarget = path.join(hardLinkFixture, 'SandboxiePlus', 'SandMan', 'Resources', 'sandboxie-logo.png');
  fs.unlinkSync(hardLinkTarget);
  fs.linkSync(hardLinkSource, hardLinkTarget);
  const hardLinkResult = runValidator(hardLinkFixture);
  assert.notEqual(hardLinkResult.status, 0);
  assert.match(hardLinkResult.stderr, /must have exactly one hard link|screenshot assets must not be hard-linked to one another/);

  const duplicateFixture = makeFixture('duplicate-png-payload');
  fs.copyFileSync(
    path.join(duplicateFixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.png'),
    path.join(duplicateFixture, 'SandboxiePlus', 'SandMan', 'Resources', 'sandboxie-logo.png'));
  const duplicateResult = runValidator(duplicateFixture);
  assert.notEqual(duplicateResult.status, 0);
  assert.match(duplicateResult.stderr, /byte length must be|SHA-256 must be/);
});

test('canonical sources reject hard links that escape the validation inventory', () => {
  const fixture = makeFixture('outside-inventory-hard-link');
  const source = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Resources', 'SandMan.png');
  const outside = path.join(temporaryRoot, 'outside-inventory-SandMan.png');
  fs.linkSync(source, outside);
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /must have exactly one hard link; found 2/);
});

test('feature and changelog documents reject inline and reference-style images', () => {
  const inlineFixture = makeFixture('feature-inline-image');
  fs.appendFileSync(path.join(inlineFixture, 'docs', 'features', 'native-ci-evidence.md'),
    '\n\n![Unsafe image](file:///C:/outside.png)\n', 'utf8');
  const inlineResult = runValidator(inlineFixture);
  assert.notEqual(inlineResult.status, 0);
  assert.match(inlineResult.stderr, /feature article native-ci-evidence must not render Markdown images.*file:\/\/\//);

  const referenceFixture = makeFixture('changelog-reference-image');
  fs.appendFileSync(path.join(referenceFixture, 'docs', 'changelog.md'),
    '\n\n![Unsafe image][outside]\n\n[outside]: ../../outside.png\n', 'utf8');
  const referenceResult = runValidator(referenceFixture);
  assert.notEqual(referenceResult.status, 0);
  assert.match(referenceResult.stderr, /documentation changelog must not render Markdown images.*\.\.\/\.\.\/outside\.png/);
});

test('supplemental images reject remote, data, qrc, and noncanonical traversal destinations', () => {
  for (const [name, destination, expected] of [
    ['http', 'http://example.test/image.png', /forbidden scheme: http/],
    ['data', 'data:image/png;base64,AAAA', /forbidden scheme: data/],
    ['qrc', 'qrc:\/Docs\/assets\/SandMan.png', /forbidden scheme: qrc/],
    ['traversal', '../../outside.png', /screenshot image\[0\] destination spelling must be exactly/],
  ]) {
    const fixture = makeFixture(`supplemental-${name}-image`);
    const screenshots = path.join(fixture, 'docs', 'screenshots.md');
    replaceOnce(screenshots, '../SandboxiePlus/SandMan/Resources/SandMan.png', destination);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} image mutation unexpectedly validated`);
    assert.match(result.stderr, expected);
  }
});

test('supplemental image syntax must remain exact inline, non-angle, and rewrite-compatible', () => {
  const mutations = [
    {
      name: 'escaped-alt',
      before: '![Sandboxie desktop mark](',
      after: '![Sandboxie desktop \\] mark](',
      expected: /exact inline, non-angle syntax; found escaped-inline/,
    },
    {
      name: 'angle-destination',
      before: '(../SandboxiePlus/SandMan/Resources/SandMan.png)',
      after: '(<../SandboxiePlus/SandMan/Resources/SandMan.png>)',
      expected: /exact inline, non-angle syntax; found angle-inline/,
    },
    {
      name: 'escaped-destination',
      before: '../SandboxiePlus/SandMan/Resources/SandMan.png',
      after: '..\\/SandboxiePlus/SandMan/Resources/SandMan.png',
      expected: /destination spelling must be exactly/,
    },
    {
      name: 'entity-destination',
      before: '../SandboxiePlus/SandMan/Resources/SandMan.png',
      after: '&#x2e;&#x2e;/SandboxiePlus/SandMan/Resources/SandMan.png',
      expected: /destination spelling must be exactly/,
    },
    {
      name: 'full-reference',
      before: '![Sandboxie desktop mark](../SandboxiePlus/SandMan/Resources/SandMan.png)',
      after: '![Sandboxie desktop mark][desktop-mark]',
      suffix: '\n\n[desktop-mark]: ../SandboxiePlus/SandMan/Resources/SandMan.png\n',
      expected: /exact inline, non-angle syntax; found full-reference/,
    },
    {
      name: 'shortcut-reference',
      before: '![Sandboxie desktop mark](../SandboxiePlus/SandMan/Resources/SandMan.png)',
      after: '![desktop-mark]',
      suffix: '\n\n[desktop-mark]: ../SandboxiePlus/SandMan/Resources/SandMan.png\n',
      expected: /exact inline, non-angle syntax; found shortcut-reference/,
    },
  ];
  for (const mutation of mutations) {
    const fixture = makeFixture(`supplemental-${mutation.name}`);
    const screenshots = path.join(fixture, 'docs', 'screenshots.md');
    replaceOnce(screenshots, mutation.before, mutation.after);
    if (mutation.suffix) fs.appendFileSync(screenshots, mutation.suffix, 'utf8');
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${mutation.name} image syntax unexpectedly validated`);
    assert.match(result.stderr, mutation.expected);
  }
});

test('all 23 feature articles require a nonempty final Suggested articles section', () => {
  for (const mutation of [
    { name: 'missing-suggestions', before: '## Suggested articles', after: '## Related reading', expected: /must end with a Suggested articles section/ },
    {
      name: 'empty-suggestions',
      before: '- [Material Design 3 desktop shell](../../material-design.md)\n- [Tab discovery](../tab-discovery.md)\n- [Material appearance editor](../appearance-editor.md)',
      after: 'No related articles are listed.',
      expected: /Suggested articles section must contain a rendered link/,
    },
    {
      name: 'nonfinal-suggestions',
      before: '- [Material appearance editor](../appearance-editor.md)',
      after: '- [Material appearance editor](../appearance-editor.md)\n\n## Later section\n\nThis content appears after the suggestions.',
      expected: /must keep Suggested articles as its final section/,
    },
  ]) {
    const fixture = makeFixture(mutation.name);
    const article = path.join(fixture, 'docs', 'features', 'ui', 'm3-shell-boundary.md');
    replaceOnce(article, mutation.before, mutation.after);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${mutation.name} mutation unexpectedly validated`);
    assert.match(result.stderr, mutation.expected);
  }
});

test('a fenced fake Suggested articles block cannot satisfy the trailing body guard', () => {
  const fixture = makeFixture('fenced-fake-suggestions');
  const article = path.join(fixture, 'docs', 'features', 'ui', 'm3-shell-boundary.md');
  const original = fs.readFileSync(article, 'utf8');
  const marker = original.indexOf('\n## Suggested articles');
  assert.notEqual(marker, -1);
  fs.writeFileSync(article,
    `${original.slice(0, marker)}\n\n\`\`\`markdown\n## Suggested articles\n\n- [Fake](../../material-design.md)\n\`\`\`\n`, 'utf8');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /must end with a Suggested articles section/);
});

test('Suggested articles require a bundled non-self feature destination', () => {
  const canonicalSuggestions = '- [Material Design 3 desktop shell](../../material-design.md)\n'
    + '- [Tab discovery](../tab-discovery.md)\n'
    + '- [Material appearance editor](../appearance-editor.md)';
  for (const [name, replacement] of [
    ['external-only-suggestions', '- [External documentation](https://example.test/docs)'],
    ['self-only-suggestions', '- [This article](m3-shell-boundary.md)'],
    ['supplemental-only-suggestions', '- [Screenshot gallery](../../screenshots.md)'],
  ]) {
    const fixture = makeFixture(name);
    const article = path.join(fixture, 'docs', 'features', 'ui', 'm3-shell-boundary.md');
    replaceOnce(article, canonicalSuggestions, replacement);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${name} mutation unexpectedly validated`);
    assert.match(result.stderr, /Suggested articles must link to at least one bundled non-self feature article/);
  }
});

test('changelog parsing, regex flags, and accessible status contracts fail closed when removed', () => {
  const mutations = [
    {
      name: 'changelog-multiline',
      before: 'QRegularExpression::MultilineOption), Qt::SkipEmptyParts',
      after: 'QRegularExpression::NoPatternOption), Qt::SkipEmptyParts',
      expected: /DocumentationBrowser contract missing: QRegularExpression::MultilineOption/,
    },
    {
      name: 'runtime-changelog-byte-bound',
      before: 'changelogFile.size() > kMaximumChangelogBytes',
      after: 'false /* changelog byte bound removed */',
      expected: /DocumentationBrowser contract missing: changelogFile\.size\(\) > kMaximumChangelogBytes/,
    },
    {
      name: 'runtime-changelog-entry-bound',
      before: 'sections.size() > kMaximumChangelogEntries + 1',
      after: 'false /* changelog entry bound removed */',
      expected: /DocumentationBrowser contract missing: sections\.size\(\) > kMaximumChangelogEntries \+ 1/,
    },
    {
      name: 'runtime-code-masked-title',
      before: 'extractCanonicalAtxTitle(article.body, &canonicalTitle)',
      after: 'true /* canonical title validation removed */',
      expected: /DocumentationBrowser contract missing: extractCanonicalAtxTitle\(article\.body, &canonicalTitle\)/,
    },
    {
      name: 'regex-flag-validation',
      before: 'if (!supportedRegexOptions(flags->text(), &options)) {',
      after: 'if (false) {',
      expected: /both regex builders must validate the exact supported flag set/,
    },
    {
      name: 'accessible-status-name',
      before: 'QAccessibleEvent event(label, QAccessible::NameChanged);',
      after: 'QAccessibleEvent event(label, QAccessible::ValueChanged);',
      expected: /DocumentationBrowser contract missing: QAccessible::NameChanged/,
    },
    {
      name: 'hidden-target-search-reset',
      before: 'clearArticleSearchState();',
      after: 'm_search->clear();',
      expected: /both hidden-article navigation paths must clear regex search state/,
    },
    {
      name: 'regex-flag-rehydrate',
      before: 'activeRegexFlags(m_searchExpression,',
      after: 'QStringLiteral("i"), /* stale flags */',
      expected: /DocumentationBrowser contract missing: activeRegexFlags\(m_searchExpression/,
    },
  ];
  for (const mutation of mutations) {
    const fixture = makeFixture(mutation.name);
    const browser = path.join(fixture, 'SandboxiePlus', 'SandMan', 'Windows', 'DocumentationBrowser.cpp');
    replaceOnce(browser, mutation.before, mutation.after);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${mutation.name} mutation unexpectedly validated`);
    assert.match(result.stderr, mutation.expected);
  }
});

test('palette inventory includes supplemental articles and an exact Changelog teleport', () => {
  for (const mutation of [
    {
      name: 'palette-supplemental',
      before: 'appendDocumentationCommands(inventory.value(QStringLiteral("supplemental")).toArray());',
      after: '/* supplemental inventory omitted */',
      expected: /command palette documentation contract missing: inventory\.value\(QStringLiteral\("supplemental"\)/,
    },
    {
      name: 'palette-changelog',
      before: 'browser->openChangelog();',
      after: 'browser->show();',
      expected: /command palette documentation contract missing: browser->openChangelog\(\)/,
    },
  ]) {
    const fixture = makeFixture(mutation.name);
    const sandMan = path.join(fixture, 'SandboxiePlus', 'SandMan', 'SandMan.cpp');
    replaceOnce(sandMan, mutation.before, mutation.after);
    const result = runValidator(fixture);
    assert.notEqual(result.status, 0, `${mutation.name} mutation unexpectedly validated`);
    assert.match(result.stderr, mutation.expected);
  }
});

test('command palette validation and matching reject raw unbounded regex construction', () => {
  const fixture = makeFixture('palette-unbounded-regex');
  const sandMan = path.join(fixture, 'SandboxiePlus', 'SandMan', 'SandMan.cpp');
  replaceOnce(sandMan,
    'const QRegularExpression expression = BoundedPaletteExpression(pattern, options);',
    'const QRegularExpression expression(pattern, options);');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /command palette validation and matching must both use the bounded expression helper|must not synchronously construct an unbounded user expression/);
});

test('ordered manifest identity cannot be replaced while retaining the count', () => {
  const fixture = makeFixture('manifest-identity-drift');
  const manifest = path.join(fixture, 'docs', 'articles', 'index.json');
  replaceOnce(manifest, '"slug":"material-design"', '"slug":"replacement-design"');
  const result = runValidator(fixture);
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /manifest articles\[0\] slug must be material-design/);
});

test('link resolution retains fragments and rejects unsafe destinations', () => {
  const known = new Set(['material-design.md', 'features/pages-a11y-boundary.md']);
  const headings = new Map([
    ['material-design.md', buildHeadingSlugs('# Material Design 3 desktop shell\n\n## Failure modes and security\n')],
    ['features/pages-a11y-boundary.md', buildHeadingSlugs('# GitHub Pages navigation and overlay boundary\n\n## Verification\n')],
  ]);
  assert.deepEqual(
    resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '#verification', known, headings),
    { source: 'features/pages-a11y-boundary.md', fragment: 'verification' });
  assert.deepEqual(
    resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '../material-design.md#failure-modes-and-security', known, headings),
    { source: 'material-design.md', fragment: 'failure-modes-and-security' });
  assert.deepEqual(
    resolveInternalDocumentationLink('material-design.md', 'https://example.test/docs', known),
    { external: 'https://example.test/docs' });
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'http://example.test', known), /unsupported external documentation scheme/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'javascript:alert(1)', known), /unsupported external documentation scheme/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'file:///tmp/readme.md', known), /unsupported external documentation scheme/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', '../outside.md', known), /local documentation link escapes docs/);
  assert.throws(() => resolveInternalDocumentationLink('material-design.md', 'missing.md', known), /local documentation destination is not bundled/);
  assert.throws(() => resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '#source-verification-and-remaining-proof', known, headings), /fragment does not match a canonical heading/);
  assert.throws(() => resolveInternalDocumentationLink('features/pages-a11y-boundary.md', '../material-design.md#failure-modes', known, headings), /fragment does not match a canonical heading/);
  assert.deepEqual([...buildHeadingSlugs('## Repeated\n\n## Repeated\n\n## Repeated\n')], ['repeated', 'repeated-1', 'repeated-2']);
});
