import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const workflowPath = path.join(root, '.github', 'workflows', 'main.yml');
if (!fs.existsSync(workflowPath)) throw new Error('missing .github/workflows/main.yml');
const workflow = fs.readFileSync(workflowPath, 'utf8');

const required = [
  ['concurrency group', 'group: sandboxie-ci-${{ github.workflow }}-${{ github.ref }}'],
  ['stale-run cancellation', 'cancel-in-progress: true'],
];
const failures = required.filter(([, token]) => !workflow.includes(token));
if (failures.length) {
  console.error('ci-concurrency-contract failed');
  for (const [label, token] of failures) console.error(`- ${label}: ${token}`);
  process.exit(1);
}

// Publishing workflows intentionally do not inherit this contract: cancelling
// a deployment or release halfway through can strand external state.
for (const publishingWorkflow of ['.github/workflows/pages.yml', '.github/workflows/announcements.yml', '.github/workflows/release.yml']) {
  const text = fs.existsSync(path.join(root, publishingWorkflow))
    ? fs.readFileSync(path.join(root, publishingWorkflow), 'utf8')
    : '';
  if (text.includes('cancel-in-progress: true')) {
    throw new Error(`${publishingWorkflow}: publishing workflow must not cancel side effects`);
  }
}

console.log('ci-concurrency-contract checks=3');
