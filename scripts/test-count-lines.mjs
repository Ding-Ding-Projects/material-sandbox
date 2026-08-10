import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import test from 'node:test';

const counter = path.resolve('scripts/count-lines.mjs');

function run(command, args, cwd, expected = 0) {
  const result = spawnSync(command, args, {
    cwd,
    encoding: 'utf8',
    env: { ...process.env, LANG: 'C', LC_ALL: 'C', TZ: 'UTC' },
    windowsHide: true,
  });
  assert.equal(result.error, undefined, result.error?.message);
  assert.equal(result.status, expected, 'command: ' + command + ' ' + args.join(' ') + '\nstdout:\n' + result.stdout + '\nstderr:\n' + result.stderr);
  return result;
}

function write(root, relative, content) {
  const target = path.join(root, ...relative.split('/'));
  fs.mkdirSync(path.dirname(target), { recursive: true });
  fs.writeFileSync(target, content);
}

function adjustAttribution(report, field, amount) {
  for (const row of [report.categories.find(({ key }) => key === 'source'), report.project, report.grand]) {
    row.attribution[field] += amount;
    row.attribution[field + 'Nonblank'] += amount;
  }
}

test('counts immutable classifications and exact surviving blame attribution with mutation checks', async () => {
  const fixture = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-line-counter-'));
  const shallowFixture = fixture + '-shallow';
  try {
    run('git', ['init', '-b', 'main'], fixture);
    run('git', ['config', 'commit.gpgsign', 'false'], fixture);
    run('git', ['config', 'core.autocrlf', 'false'], fixture);
    run('git', ['config', 'user.name', 'Example Person'], fixture);
    run('git', ['config', 'user.email', 'person@example.test'], fixture);

    write(fixture, 'Sandboxie/src/main.cpp', 'int person_line = 1;\n\nint retained_line = 2;\n');
    write(fixture, 'Sandboxie/src/no-final.cpp', 'int one_line = 1;');
    write(fixture, 'Sandboxie/src/lone-cr.cpp', 'left\rright');
    write(fixture, 'SandboxiePlus/SandMan/TestStyle.ui', ':root {\r\n\r\n  color: black;\r\n}\r\n');
    write(fixture, 'SandboxiePlus/SandMan/TestUtf16.rc', Buffer.from('\uFEFFalpha\r\nbeta\r\n', 'utf16le'));
    write(fixture, 'Installer/isl/Swedish.isl', Buffer.from([0x50, 0x72, 0x69, 0x73, 0x20, 0x80, 0x0a]));
    write(fixture, 'Sandboxie/common/Detours/vendor.txt', 'vendored record\n');
    write(fixture, 'SandboxiePlus/SandMan/logo.png', Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x00]));
    run('git', ['add', '.'], fixture);
    run('git', ['commit', '-m', 'Add person-authored fixture'], fixture);

    run('git', ['config', 'user.name', 'Release Maintainer'], fixture);
    run('git', ['config', 'user.email', 'maintainer@example.test'], fixture);
    write(fixture, 'Sandboxie/src/main.cpp', 'int agent_line = 7;\n\nint retained_line = 2;\n');
    write(fixture, 'scripts/test-fixture.mjs', 'export const tested = true;\n');
    write(fixture, 'SandboxiePlus/SandMan/resource.h', '#define GENERATED_FIXTURE 1\n');
    run('git', ['add', '.'], fixture);
    run('git', ['commit', '-m', 'Add coauthored release fixture', '-m', 'Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>'], fixture);

    run('git', ['config', 'user.name', 'Automation Researcher'], fixture);
    run('git', ['config', 'user.email', 'human@example.test'], fixture);
    write(fixture, 'Sandboxie/src/human.cpp', 'int human_line = 3;\n');
    run('git', ['add', '.'], fixture);
    run('git', ['commit', '-m', 'Add misleading human fixture', '-m', 'Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>', '-m', 'This final prose line makes the earlier text non-canonical.'], fixture);

    const first = run(process.execPath, [counter, '--root', fixture, '--json', 'evidence/report.json', '--markdown', 'evidence/report.md', '--jobs', '1'], fixture);
    assert.match(first.stdout, /Project total/);
    const firstJson = fs.readFileSync(path.join(fixture, 'evidence/report.json'), 'utf8');
    const report = JSON.parse(firstJson);
    const categories = Object.fromEntries(report.categories.map((bucket) => [bucket.key, bucket]));

    assert.deepEqual(report.categories.map(({ key }) => key), ['source', 'tests', 'styles', 'generated', 'excluded']);
    assert.equal(report.schema, 3);
    assert.match(report.revision, /^[0-9a-f]{40,64}$/);
    assert.match(report.tree, /^[0-9a-f]{40,64}$/);
    assert.equal(report.shallow, false);
    assert.equal(report.clean, true);
    assert.equal(categories.tests.files, 1);
    assert.equal(categories.generated.files, 1);
    assert.equal(categories.excluded.files, 3);
    assert.equal(categories.source.attribution.agent, 1);
    assert.ok(categories.source.attribution.people >= 5);
    assert.equal(categories.source.attribution.unattributed, 0);
    assert.equal(categories.styles.total, 7);
    assert.equal(categories.styles.nonblank, 5);
    assert.deepEqual(report.encodingWarnings, [
      { path: 'Installer/isl/Swedish.isl', encoding: 'windows-1252', lineSemantics: 'Git LF byte records' },
      { path: 'SandboxiePlus/SandMan/TestUtf16.rc', encoding: 'utf-16le', lineSemantics: 'Git LF byte records' },
    ]);
    assert.equal(report.project.total, categories.source.total + categories.tests.total + categories.styles.total);
    assert.equal(report.grand.total, report.categories.reduce((sum, bucket) => sum + bucket.total, 0));
    assert.equal(report.grand.total, report.grand.nonblank + report.grand.blank);
    assert.equal(report.grand.total, report.grand.attribution.agent + report.grand.attribution.people);
    assert.equal(report.grand.attribution.agent, report.grand.attribution.agentNonblank + report.grand.attribution.agentBlank);
    assert.equal(report.grand.attribution.people, report.grand.attribution.peopleNonblank + report.grand.attribution.peopleBlank);
    assert.equal(report.grand.nonblank, report.grand.attribution.agentNonblank + report.grand.attribution.peopleNonblank);
    assert.equal(report.grand.blank, report.grand.attribution.agentBlank + report.grand.attribution.peopleBlank);
    assert.equal(report.grand.attribution.blamed, report.grand.total);

    run(process.execPath, [counter, '--root', fixture, '--json', 'evidence/report-jobs2.json', '--markdown', 'evidence/report-jobs2.md', '--jobs', '2'], fixture);
    assert.equal(fs.readFileSync(path.join(fixture, 'evidence/report-jobs2.json'), 'utf8'), firstJson);
    assert.equal(fs.readFileSync(path.join(fixture, 'evidence/report-jobs2.md'), 'utf8'), fs.readFileSync(path.join(fixture, 'evidence/report.md'), 'utf8'));

    run(process.execPath, [counter, '--root', fixture, '--verify', 'evidence/report.json'], fixture);

    const arithmeticMutation = structuredClone(report);
    arithmeticMutation.project.total += 1;
    write(fixture, 'evidence/arithmetic-mutation.json', JSON.stringify(arithmeticMutation, null, 2) + '\n');
    const arithmeticFailure = run(process.execPath, [counter, '--root', fixture, '--verify', 'evidence/arithmetic-mutation.json'], fixture, 1);
    assert.match(arithmeticFailure.stderr, /project[.]total arithmetic mismatch/);

    const arithmeticMutations = [
      ['schema', (candidate) => { candidate.schema = 99; }],
      ['category-order', (candidate) => { candidate.categories.reverse(); }],
      ['source-files', (candidate) => { candidate.categories[0].files += 1; }],
      ['source-bytes', (candidate) => { candidate.categories[0].bytes += 1; }],
      ['source-total', (candidate) => { candidate.categories[0].total += 1; }],
      ['source-nonblank', (candidate) => { candidate.categories[0].nonblank += 1; }],
      ['source-blank', (candidate) => { candidate.categories[0].blank += 1; }],
      ['source-agent', (candidate) => { candidate.categories[0].attribution.agent += 1; }],
      ['source-agent-nonblank', (candidate) => { candidate.categories[0].attribution.agentNonblank += 1; }],
      ['source-agent-blank', (candidate) => { candidate.categories[0].attribution.agentBlank += 1; }],
      ['source-people', (candidate) => { candidate.categories[0].attribution.people += 1; }],
      ['source-people-nonblank', (candidate) => { candidate.categories[0].attribution.peopleNonblank += 1; }],
      ['source-people-blank', (candidate) => { candidate.categories[0].attribution.peopleBlank += 1; }],
      ['source-blamed', (candidate) => { candidate.categories[0].attribution.blamed -= 1; }],
      ['source-unattributed', (candidate) => { candidate.categories[0].attribution.unattributed += 1; }],
      ['project-files', (candidate) => { candidate.project.files += 1; }],
      ['grand-bytes', (candidate) => { candidate.grand.bytes += 1; }],
      ['policy-hash', (candidate) => { candidate.policyHash = '0'.repeat(64); }],
      ['shallow', (candidate) => { candidate.shallow = true; }],
    ];
    for (const [label, mutate] of arithmeticMutations) {
      const candidate = structuredClone(report);
      mutate(candidate);
      const relative = 'evidence/mutation-' + label + '.json';
      write(fixture, relative, JSON.stringify(candidate, null, 2) + '\n');
      const failure = run(process.execPath, [counter, '--root', fixture, '--verify', relative], fixture, 1);
      assert.match(failure.stderr, /verification failed|mismatch|must be|does not equal|invalid|inventory/);
    }

    const balancedForgery = structuredClone(report);
    adjustAttribution(balancedForgery, 'agent', -1);
    adjustAttribution(balancedForgery, 'people', 1);
    write(fixture, 'evidence/balanced-forgery.json', JSON.stringify(balancedForgery, null, 2) + '\n');
    const forgeryFailure = run(process.execPath, [counter, '--root', fixture, '--verify', 'evidence/balanced-forgery.json'], fixture, 1);
    assert.match(forgeryFailure.stderr, /does not exactly match a fresh count/);

    write(fixture, 'Sandboxie/src/main.cpp', 'int dirty_worktree_line = 9;\n');
    const dirtyFailure = run(process.execPath, [counter, '--root', fixture], fixture, 1);
    assert.match(dirtyFailure.stderr, /tracked worktree or index changes are present/);
    run(process.execPath, [counter, '--root', fixture, '--allow-dirty', '--json', 'evidence/dirty.json', '--jobs', '2'], fixture);
    const dirty = JSON.parse(fs.readFileSync(path.join(fixture, 'evidence/dirty.json'), 'utf8'));
    assert.equal(dirty.clean, false);
    assert.equal(dirty.revision, report.revision);
    assert.deepEqual(dirty.categories, report.categories);

    run('git', ['add', 'Sandboxie/src/main.cpp'], fixture);
    run('git', ['commit', '-m', 'Commit dirty fixture for classifier mutation'], fixture);
    write(fixture, 'Sandboxie/generated/moc_unreviewed.cpp', 'int generated_marker = 1;\n');
    write(fixture, 'Sandboxie/unknown.blob', Buffer.from([0x00, 0xff, 0x10]));
    run('git', ['add', 'Sandboxie/generated/moc_unreviewed.cpp', 'Sandboxie/unknown.blob'], fixture);
    run('git', ['commit', '-m', 'Add review-required classifier mutations'], fixture);
    const reviewFailure = run(process.execPath, [counter, '--root', fixture], fixture, 1);
    assert.match(reviewFailure.stderr, /review-required tracked paths=2/);
    assert.match(reviewFailure.stderr, /moc_unreviewed[.]cpp/);
    assert.match(reviewFailure.stderr, /unknown[.]blob/);

    run('git', ['clone', '--quiet', '--depth', '1', 'file:///' + fixture.replaceAll('\\', '/'), shallowFixture], fixture);
    const shallowFailure = run(process.execPath, [counter, '--root', shallowFixture], shallowFixture, 1);
    assert.match(shallowFailure.stderr, /requires a full Git history/);
  } finally {
    fs.rmSync(fixture, { recursive: true, force: true });
    fs.rmSync(shallowFixture, { recursive: true, force: true });
  }
});
