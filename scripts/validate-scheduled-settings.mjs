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
for (const token of ['struct Source', 'https-api', 'home-assistant', 'os-vault://scheduled-settings/', 'refreshSeconds', 'binary_sensor|input_boolean', 'sourceStatusDescription', 'refreshExternalSources']) {
  if (!header.includes(token) && !source.includes(token)) throw new Error(`scheduled-settings-contract missing source contract ${token}`);
}
if (!source.includes('url.scheme() != QStringLiteral("https")') || !source.includes('url.userName()') || !source.includes('url.password()') || !source.includes('url.query().isEmpty()') || !source.includes('url.fragment().isEmpty()')) {
	throw new Error('scheduled-settings-contract missing HTTPS credential/URL validation');
}
if (!source.includes('text.size() > 2048') || !source.includes('source.refreshSeconds < 15') || !source.includes('source.refreshSeconds > 86400')) {
  throw new Error('scheduled-settings-contract missing source bounds');
}
for (const token of ['CredReadW', 'CRED_TYPE_GENERIC', 'CredentialBlobSize <= 4096', 'secret.fill', 'ManualRedirectPolicy', 'kExternalTimeoutMs', 'kExternalResponseLimit', 'sslErrors', 'reply->abort()', 'QNetworkReply::NoError', 'application/json', 'external-source-failed']) {
	if (!source.includes(token)) throw new Error(`scheduled-settings-contract missing bounded source activation ${token}`);
}
if (!source.includes('External source · ready to activate') || !settingsWindow.includes('sourceStatusDescription(rule.source)') || !settingsWindow.includes('Refresh external sources')) {
	throw new Error('scheduled-settings-contract missing visible external-source status/recovery hint');
}
for (const control of ['addSchedule', 'editSchedule', 'deleteSchedule']) {
  const pattern = new RegExp(`connect\\(${control}[^\\n]+\\[this,`);
  if (!pattern.test(settingsWindow)) throw new Error(`scheduled-settings-contract missing safe ${control} callback capture`);
}
console.log(`scheduled-settings-contract checks=${required.length + 25}`);
