#!/usr/bin/env node

import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const workflowPath = path.join(root, '.github', 'workflows', 'release.yml');
const counterPath = path.join(root, 'scripts', 'count-lines.mjs');
const docsPath = path.join(root, 'docs', 'features', 'native-ci-evidence.md');
const read = (file) => fs.readFileSync(file, 'utf8');

function countMatches(text, expression) {
  return [...text.matchAll(expression)].length;
}

function validateBundle({ workflow, counter, docs }) {
  const errors = [];
  const requireWorkflow = (label, token) => {
    if (!workflow.includes(token)) errors.push('workflow missing ' + label + ': ' + token);
  };
  const requireCounter = (label, token) => {
    if (!counter.includes(token)) errors.push('counter missing ' + label + ': ' + token);
  };
  const requireDocs = (label, token) => {
    if (!docs.includes(token)) errors.push('documentation missing ' + label + ': ' + token);
  };

  requireWorkflow('main push trigger', 'branches: [main]');
  requireWorkflow('manual trigger', 'workflow_dispatch:');
  requireWorkflow('pinned Windows runner image', 'runs-on: windows-2022');
  requireWorkflow('exact event revision checkout', 'ref: ${{ github.sha }}');
  requireWorkflow('full-history checkout', 'fetch-depth: 0');
  requireWorkflow('credential-free checkout', 'persist-credentials: false');
  requireWorkflow('pinned Node runtime', 'node-version: 24.19.0');
  requireWorkflow('release token fallback chain', 'secrets.RELEASE_TOKEN || secrets.ORG_TOKEN || secrets.GITHUB_TOKEN');
  requireWorkflow('monotonic run-number tag', 'RELEASE_TAG: v0.0.0-build.${{ github.run_number }}');
  requireWorkflow('monotonic prior-tag comparison', 'is not greater than the prior monotonic tag');
  requireWorkflow('root build entrypoint', "'build.bat'");
  requireWorkflow('root installer entrypoint', "'build-installer.bat'");
  requireWorkflow('line counter execution', 'node scripts/count-lines.mjs --json');
  requireWorkflow('line counter evidence verification', 'node scripts/count-lines.mjs --verify');
  requireWorkflow('unsigned installer verification', 'Get-AuthenticodeSignature');
  requireWorkflow('required unsigned status', 'SignatureStatus]::NotSigned');
  requireWorkflow('SHA-256 verification', 'Get-FileHash');
  requireWorkflow('authoritative workflow start', 'run_started_at');
  requireWorkflow('authoritative publication completion', 'published_at');
  requireWorkflow('workflow start notes', 'Workflow started:');
  requireWorkflow('workflow completion notes', 'Workflow completed:');
  requireWorkflow('workflow duration notes', 'Workflow duration:');
  requireWorkflow('exact source commit notes', 'Commit: $env:GITHUB_SHA');
  requireWorkflow('exact checked-out HEAD proof', 'Checked-out HEAD $head does not match release source $env:GITHUB_SHA');
  requireWorkflow('exact release target', '--target $env:GITHUB_SHA');
  requireWorkflow('published tag commit proof', 'git rev-list -n 1 $env:RELEASE_TAG');
  requireWorkflow('project total table marker', 'Project total');
  requireWorkflow('grand total table marker', 'Grand total');
  requireWorkflow('line-table start marker', '<!-- line-count:start -->');
  requireWorkflow('line-table end marker', '<!-- line-count:end -->');
  requireWorkflow('line-table exact comparison', 'lineBlock.Groups[1].Value -cne $lineTable');
  requireWorkflow('unsigned user warning', 'unknown-publisher or SmartScreen warning');
  requireWorkflow('public dim-sum catalog link', 'https://github.com/Ding-Ding-Projects/dim-sum-photos/releases');
  requireWorkflow('no-copy dim-sum statement', 'does not copy or attach a dim-sum image');
  requireWorkflow('draft staging', '--draft');
  requireWorkflow('authenticated paginated release inventory', 'gh api --paginate --slurp');
  requireWorkflow('numeric draft release identity', '$releaseId = [int64]$draft.id');
  requireWorkflow('numeric release API', 'repos/$env:GITHUB_REPOSITORY/releases/$releaseId');
  requireWorkflow('non-draft publication', '-F draft=false');
  requireWorkflow('file-backed final release body', '-F "body=@$finalNotes"');
  requireWorkflow('release download verification', 'gh release download');
  requireWorkflow('safe evidence collection', 'Collect safe release evidence');
  requireWorkflow('always-run artifact upload', 'if: ${{ always() }}');
  requireWorkflow('non-masking artifact handling', 'continue-on-error: true');
  requireWorkflow('bounded artifact retention', 'retention-days: 30');
  requireWorkflow('missing evidence warning', 'if-no-files-found: warn');
  if (countMatches(workflow, /-Filter 'Sandboxie-Plus-x64-v\*\.exe' -File -Recurse/g) !== 2) {
    errors.push('installer discovery and evidence collection must recurse into run-scoped output directories');
  }

  if (/^concurrency:/m.test(workflow) || /cancel-in-progress\s*:/m.test(workflow)) {
    errors.push('release workflow must not use workflow concurrency or cancellation');
  }
  if (countMatches(workflow, /\bgh release create\b/g) !== 1) {
    errors.push('workflow must contain exactly one gh release create invocation');
  }
  if (countMatches(workflow, /secrets[.]RELEASE_TOKEN \|\| secrets[.]ORG_TOKEN \|\| secrets[.]GITHUB_TOKEN/g) !== 2) {
    errors.push('release token fallback must be scoped exactly to preflight and publication');
  }
  const createAt = workflow.indexOf('gh release create');
  const unsignedAt = workflow.indexOf('SignatureStatus]::NotSigned');
  const inventoryAt = workflow.indexOf('$drafts = @(Get-RepositoryReleases');
  const numericIdAt = workflow.indexOf('$releaseId = [int64]$draft.id');
  const publishAt = workflow.indexOf('-F draft=false');
  if (unsignedAt < 0 || createAt < unsignedAt) errors.push('release creation must occur after unsigned-installer verification');
  if (inventoryAt < createAt || numericIdAt < inventoryAt || publishAt < numericIdAt) {
    errors.push('draft publication must follow authenticated inventory and numeric-ID verification');
  }

  const assetBlock = /\$assets\s*=\s*@\(([\s\S]*?)\n\s*\)/.exec(workflow)?.[1] || '';
  const requiredAssets = [
    '$env:INSTALLER_PATH',
    "'sha256-checksums.txt'",
    "'release-provenance.json'",
    "'line-count.json'",
    "'line-count.md'",
  ];
  for (const asset of requiredAssets) {
    if (!assetBlock.includes(asset)) errors.push('release asset allowlist missing ' + asset);
  }
  if (assetBlock && assetBlock.split(/\r?\n/).filter((line) => line.trim()).length !== requiredAssets.length) {
    errors.push('release asset allowlist must contain exactly five entries');
  }
  if (/[.](?:png|jpe?g|gif|webp|bmp|tiff?)\b/i.test(assetBlock)) {
    errors.push('consumer release must not attach image assets');
  }

  for (const bucket of ['source', 'tests', 'styles', 'generated', 'excluded']) {
    requireCounter(bucket + ' classification', "'" + bucket + "'");
  }
  requireCounter('project total arithmetic', "key: 'project'");
  requireCounter('grand total arithmetic', "key: 'grand'");
  requireCounter('nonblank count', 'nonblank');
  requireCounter('surviving-line attribution', "'blame', '--incremental'");
  requireCounter('agent co-author detection', 'Co-Authored-By');
  requireCounter('arithmetic mismatch failure', 'arithmetic self-check failed');
  requireCounter('full-history refusal', '--is-shallow-repository');
  requireCounter('fail-closed unclassified path handling', 'review-required tracked paths=');

  requireDocs('release workflow name', 'Windows Release');
  requireDocs('root build scripts', 'build.bat /s');
  requireDocs('root installer script', 'build-installer.bat /s');
  requireDocs('line counter command', 'node scripts/count-lines.mjs');
  requireDocs('unsigned release warning', 'unsigned');
  requireDocs('public catalog link', 'Ding-Ding-Projects/dim-sum-photos');
  requireDocs('no current publication claim', 'does not prove that a release has run');

  return errors;
}

function currentBundle() {
  for (const file of [workflowPath, counterPath, docsPath]) {
    if (!fs.existsSync(file)) throw new Error('missing release-contract file: ' + path.relative(root, file));
  }
  return { workflow: read(workflowPath), counter: read(counterPath), docs: read(docsPath) };
}

function runSelfTest(bundle) {
  const mutations = [
    ['main trigger', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace('branches: [main]', 'branches: [master]') })],
    ['token chain', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replaceAll('secrets.RELEASE_TOKEN || secrets.ORG_TOKEN || secrets.GITHUB_TOKEN', 'secrets.GITHUB_TOKEN') })],
    ['non-cancelling workflow', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow + '\nconcurrency:\n  cancel-in-progress: true\n' })],
    ['full history', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace('fetch-depth: 0', 'fetch-depth: 1') })],
    ['exact checkout', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace('persist-credentials: false', 'persist-credentials: true') })],
    ['root installer script', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace("'build-installer.bat'", "'Installer\\\\make.bat'") })],
    ['unsigned status', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replaceAll('SignatureStatus]::NotSigned', 'SignatureStatus]::Valid') })],
    ['one release', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace('gh release create', 'gh release create\ngh release create') })],
    ['draft inventory', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replaceAll('gh api --paginate --slurp', 'gh api') })],
    ['numeric release id', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace('$releaseId = [int64]$draft.id', '$releaseId = 1') })],
    ['monotonic tag', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace('RELEASE_TAG: v0.0.0-build.${{ github.run_number }}', 'RELEASE_TAG: v0.0.0-build.1') })],
    ['no image attachment', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replace("(Join-Path $env:RELEASE_EVIDENCE 'line-count.md')", "(Join-Path $env:RELEASE_EVIDENCE 'dim-sum.png')") })],
    ['timing evidence', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replaceAll('Workflow duration:', 'Elapsed:') })],
    ['exact line-table block', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replaceAll('<!-- line-count:start -->', '<!-- line-count-removed -->') })],
    ['failure evidence', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replaceAll('if: ${{ always() }}', 'if: ${{ success() }}') })],
    ['run-scoped installer discovery', ({ workflow, ...rest }) => ({ ...rest, workflow: workflow.replaceAll("-Filter 'Sandboxie-Plus-x64-v*.exe' -File -Recurse", "-Filter 'Sandboxie-Plus-x64-v*.exe' -File") })],
    ['full-history counter', ({ counter, ...rest }) => ({ ...rest, counter: counter.replace("'--is-shallow-repository'", "'--show-toplevel'") })],
    ['fail-closed classifier', ({ counter, ...rest }) => ({ ...rest, counter: counter.replace('review-required tracked paths=', 'unclassified paths ignored=') })],
    ['documentation boundary', ({ docs, ...rest }) => ({ ...rest, docs: docs.replace('does not prove that a release has run', 'proves a release ran') })],
  ];
  for (const [label, mutate] of mutations) {
    const candidate = mutate(bundle);
    const failures = validateBundle(candidate);
    if (!failures.length) throw new Error('mutation survived release-contract validation: ' + label);
  }
  console.log('release-contract self-test mutations=' + mutations.length);
}

const bundle = currentBundle();
const failures = validateBundle(bundle);
if (failures.length) {
  console.error('release-contract validation failed');
  for (const failure of failures) console.error('- ' + failure);
  process.exit(1);
}
if (process.argv.includes('--self-test')) runSelfTest(bundle);
else console.log('release-contract checks=52');
