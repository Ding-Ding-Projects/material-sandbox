import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');

const sources = {
  header: read('SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.h'),
  implementation: read('SandboxiePlus/MiscHelpers/Common/LocalSettingsHistory.cpp'),
  settings: read('SandboxiePlus/MiscHelpers/Common/Settings.cpp'),
  window: read('SandboxiePlus/SandMan/Windows/SettingsWindow.cpp'),
  qmake: read('SandboxiePlus/MiscHelpers/MiscHelpers.pri'),
  msbuild: read('SandboxiePlus/MiscHelpers/MiscHelpers.vcxproj'),
  qrc: read('SandboxiePlus/SandMan/Resources/SandMan.qrc'),
  article: read('docs/features/settings-history.md'),
};

const requiredImplementationTokens = [
  '#include <QProcess>',
  'QStandardPaths::findExecutable(QStringLiteral("git"))',
  'GIT_TERMINAL_PROMPT',
  'GIT_CONFIG_NOSYSTEM',
  'GIT_CONFIG_GLOBAL',
  'GIT_ATTR_NOSYSTEM',
  'CREATE_NO_WINDOW',
  'QStringLiteral("init")',
  'QStringLiteral("symbolic-ref")',
  'QStringLiteral("config")',
  'QStringLiteral("remote")',
  'QStringLiteral("add")',
  'QStringLiteral("commit")',
  'QStringLiteral("log")',
  'QStringLiteral("show")',
  'QStringLiteral("rev-parse")',
  'QStringLiteral("bundle")',
  'QStringLiteral("--no-gpg-sign")',
  'QStringLiteral("--no-verify")',
  'QStringLiteral("--allow-empty")',
  'settings.snapshot',
  'settings restore checkpoint',
  'settings restored',
  'ApplySnapshot(target)',
  'const QVariantMap values = settings->SnapshotValues();',
];

function validateSourceContract(candidate) {
  for (const token of requiredImplementationTokens)
    assert.ok(candidate.includes(token), `LocalSettingsHistory.cpp is missing ${token}`);
  assert.ok(!candidate.includes('#include <QJsonDocument>'), 'Git history backend must not retain the JSONL document writer');
  assert.ok(!candidate.includes('m_filePath'), 'Git history backend must not retain the JSONL file-path state');
}

validateSourceContract(sources.implementation);
assert.ok(
  sources.implementation.indexOf('QMutexLocker locker(&m_mutex);') <
    sources.implementation.indexOf('const QVariantMap values = settings->SnapshotValues();'),
  'history mutex must be acquired before snapshot capture so concurrent writes cannot append stale state',
);
assert.throws(
  () => validateSourceContract(sources.implementation.replace('QStringLiteral("commit")', 'QStringLiteral("disabled")')),
  /missing QStringLiteral\("commit"\)/,
  'the Git-command completeness guard must fail when commit execution disappears',
);

const checks = [
  [sources.header, 'QString repositoryPath() const', 'repository path API'],
  [sources.header, 'bool exportBundle(', 'complete Git bundle export API'],
  [sources.header, 'bool restore(', 'restore API'],
  [sources.settings, 'm_ConfigDir + "/history/settings-history",', 'isolated Git repository path'],
  [sources.settings, 'm_ConfigDir + "/history/settings-history.jsonl");', 'bounded legacy migration input'],
  [sources.settings, 'm_History->initialize(this)', 'initial full-state Git revision'],
  [sources.settings, 'm_History->record(this, key', 'full-state revision after each setting mutation'],
  [sources.window, 'Local Git repository:', 'factual repository status'],
  [sources.window, 'Export Git bundle', 're-importable Git export'],
  [sources.window, 'entry.id.left(8)', 'visible commit identity'],
  [sources.window, 'M3DialogHost::Install(dialog)', 'Material history dialog host'],
  [sources.qmake, './Common/LocalSettingsHistory.cpp', 'qmake implementation registration'],
  [sources.msbuild, String.raw`Common\LocalSettingsHistory.cpp`, 'MSBuild implementation registration'],
  [sources.qrc, 'Docs/settings-history.md', 'offline article registration'],
  [sources.article, 'git init', 'documented initialization command'],
  [sources.article, 'git commit', 'documented commit command'],
  [sources.article, 'git log', 'documented browse command'],
  [sources.article, 'git show', 'documented restore-read command'],
  [sources.article, 'git bundle create', 'documented full export command'],
];
for (const [content, token, label] of checks)
  assert.ok(content.includes(token), `settings history contract is missing ${label}`);

function git(cwd, args, options = {}) {
  const result = spawnSync('git', args, {
    cwd,
    encoding: Object.prototype.hasOwnProperty.call(options, 'encoding') ? options.encoding : 'utf8',
    input: options.input,
    env: { ...process.env, GIT_TERMINAL_PROMPT: '0', LC_ALL: 'C', LANG: 'C' },
    windowsHide: true,
    timeout: 10_000,
    maxBuffer: 4 * 1024 * 1024,
  });
  assert.equal(result.error, undefined, `git ${args[0]} could not start: ${result.error?.message ?? ''}`);
  assert.equal(result.status, 0, `git ${args.join(' ')} failed: ${String(result.stderr).trim()}`);
  return result.stdout;
}

const fixtureRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'sandboxie-settings-git-'));
try {
  git(fixtureRoot, ['init', '--quiet']);
  git(fixtureRoot, ['symbolic-ref', 'HEAD', 'refs/heads/main']);
  git(fixtureRoot, ['config', '--local', 'user.name', 'Sandboxie Local History']);
  git(fixtureRoot, ['config', '--local', 'user.email', 'local-history@sandboxie.invalid']);
  git(fixtureRoot, ['config', '--local', 'commit.gpgSign', 'false']);
  git(fixtureRoot, ['config', '--local', 'core.autocrlf', 'false']);

  const snapshotPath = path.join(fixtureRoot, 'settings.snapshot');
  const commitSnapshot = (body) => {
    fs.writeFileSync(snapshotPath, body);
    git(fixtureRoot, ['add', '--', 'settings.snapshot']);
    git(fixtureRoot, ['commit', '--quiet', '--allow-empty', '--no-gpg-sign', '--no-verify', '-m', 'Record local settings revision']);
    return git(fixtureRoot, ['rev-parse', 'HEAD']).trim();
  };

  const first = commitSnapshot(Buffer.from('snapshot-one'));
  commitSnapshot(Buffer.from('snapshot-two'));
  commitSnapshot(Buffer.from('pre-restore-checkpoint'));
  const firstSnapshot = git(fixtureRoot, ['show', '--no-ext-diff', '--no-textconv', `${first}:settings.snapshot`], { encoding: null });
  assert.deepEqual(firstSnapshot, Buffer.from('snapshot-one'), 'git show must recover the exact selected snapshot bytes');
  commitSnapshot(firstSnapshot);

  const revisions = git(fixtureRoot, ['log', '--format=%H']).trim().split(/\r?\n/);
  assert.equal(revisions.length, 4, 'restore must append pre-restore and restored-state commits without rewriting history');
  assert.equal(git(fixtureRoot, ['remote']).trim(), '', 'local settings history must have no remote');
  const bundle = path.join(fixtureRoot, 'settings-history.bundle');
  git(fixtureRoot, ['bundle', 'create', bundle, '--all']);
  assert.ok(fs.statSync(bundle).size > 0, 'git bundle create must produce a non-empty export');
  git(fixtureRoot, ['bundle', 'verify', bundle]);
} finally {
  fs.rmSync(fixtureRoot, { recursive: true, force: true });
}

console.log(`settings-history-git-contract checks=${requiredImplementationTokens.length + checks.length + 6}`);
