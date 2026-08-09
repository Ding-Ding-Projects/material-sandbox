import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const workflowPath = path.join(root, '.github', 'workflows', 'main.yml');
const smokeScriptPath = path.join(root, 'scripts', 'smoke-sandman-runtime.ps1');
if (!fs.existsSync(workflowPath)) throw new Error('missing .github/workflows/main.yml');
if (!fs.existsSync(smokeScriptPath)) throw new Error('missing scripts/smoke-sandman-runtime.ps1');

const workflow = fs.readFileSync(workflowPath, 'utf8');
const smokeScript = fs.readFileSync(smokeScriptPath, 'utf8');
const required = [
  ['x64 smoke invocation', 'scripts\\smoke-sandman-runtime.ps1 -ArtifactDirectory Installer/SbiePlus_x64'],
  ['headless Qt platform', "EnvironmentVariables['QT_QPA_PLATFORM'] = 'minimal'"],
  ['hidden process startup', '$startInfo.CreateNoWindow = $true'],
  ['bounded startup interval', 'Start-Sleep -Seconds $StartupSeconds'],
  ['runtime evidence upload', 'Upload SandMan x64 runtime smoke evidence'],
  ['runtime evidence retention', 'Installer/SbiePlus_x64/ci-runtime-smoke.json'],
];

const failures = required.filter(([label, token]) => {
  const text = label.includes('invocation') || label.includes('upload') || label.includes('retention') ? workflow : smokeScript;
  return !text.includes(token);
});
if (failures.length) {
  console.error('ci-runtime-smoke-contract failed');
  for (const [label, token] of failures) console.error(`- ${label}: ${token}`);
  process.exit(1);
}

const x64SmokeIndex = workflow.indexOf('scripts\\smoke-sandman-runtime.ps1 -ArtifactDirectory Installer/SbiePlus_x64');
const x64ArtifactIndex = workflow.indexOf('name: Sandboxie_x64', x64SmokeIndex);
if (x64SmokeIndex < 0 || x64ArtifactIndex < x64SmokeIndex) {
  throw new Error('x64 runtime smoke must run before the ordinary x64 artifact upload');
}
if (workflow.includes('smoke-sandman-runtime.ps1 -ArtifactDirectory Installer/SbiePlus_a64')) {
  throw new Error('ARM64 runtime smoke is not valid on the x64 Windows runner');
}

console.log('ci-runtime-smoke-contract checks=8');
