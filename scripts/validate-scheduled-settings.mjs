import fs from 'node:fs';

const header = fs.readFileSync('SandboxiePlus/MiscHelpers/Common/ScheduledSettings.h', 'utf8');
const source = fs.readFileSync('SandboxiePlus/MiscHelpers/Common/ScheduledSettings.cpp', 'utf8');
const required = [
  'schemaVersion', 'startDate', 'endDate', 'startTime', 'endTime', 'weekdays',
  'priority', 'effectiveRule', 'schoolModeEnabled', 'UIConfig/ScheduledRules',
  'Options/UseDarkTheme', 'light', 'dark', 'system', 'english', 'cantonese', 'bilingual'
];
for (const token of required) {
  if (!header.includes(token) && !source.includes(token)) throw new Error(`scheduled-settings-contract missing ${token}`);
}
if (!source.includes('candidate.priority > best.priority') || !source.includes('candidate.id < best.id')) {
  throw new Error('scheduled-settings-contract missing deterministic precedence');
}
if (!source.includes('crossesMidnight')) throw new Error('scheduled-settings-contract missing cross-midnight semantics');
if (!source.includes('!UserPresentationSettings::schoolModeEnabled')) throw new Error('scheduled-settings-contract missing School mode precedence');
if (!source.includes('Options/UseDarkTheme')) throw new Error('scheduled-settings-contract missing real theme persistence');
console.log(`scheduled-settings-contract checks=${required.length + 3}`);
