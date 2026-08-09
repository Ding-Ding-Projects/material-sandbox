import fs from 'node:fs';

const header = fs.readFileSync('SandboxiePlus/MiscHelpers/Common/ScheduledSettings.h', 'utf8');
const source = fs.readFileSync('SandboxiePlus/MiscHelpers/Common/ScheduledSettings.cpp', 'utf8');
const settingsWindow = fs.readFileSync('SandboxiePlus/SandMan/Windows/SettingsWindow.cpp', 'utf8');
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
for (const token of ['struct Source', 'https-api', 'home-assistant', 'os-vault://', 'unsupported-external-source', 'refreshSeconds', 'binary_sensor|input_boolean']) {
  if (!header.includes(token) && !source.includes(token)) throw new Error(`scheduled-settings-contract missing source contract ${token}`);
}
if (!source.includes('url.scheme() != QStringLiteral("https")') || !source.includes('url.userName()') || !source.includes('url.password()')) {
  throw new Error('scheduled-settings-contract missing HTTPS credential/URL validation');
}
if (!source.includes('source.url.size() > 2048') || !source.includes('source.refreshSeconds < 15') || !source.includes('source.refreshSeconds > 86400')) {
  throw new Error('scheduled-settings-contract missing source bounds');
}
if (!source.includes('External sources are deliberately inert') && !source.includes('External source metadata is deliberately inert')) {
  throw new Error('scheduled-settings-contract missing fail-safe external-source behavior');
}
for (const control of ['addSchedule', 'editSchedule', 'deleteSchedule']) {
  const pattern = new RegExp(`connect\\(${control}[^\\n]+\\[this,`);
  if (!pattern.test(settingsWindow)) throw new Error(`scheduled-settings-contract missing safe ${control} callback capture`);
}
console.log(`scheduled-settings-contract checks=${required.length + 14}`);
