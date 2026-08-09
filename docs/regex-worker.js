'use strict';

const REGEX_MATCH_LIMIT = 20;
const REGEX_PATTERN_LIMIT = 256;
const REGEX_SAMPLE_LIMIT = 512;
const REGEX_VALUE_LIMIT = 256;

self.addEventListener('message', (event) => {
  const { id, pattern, flags, values = [], sample = '', maxMatches = REGEX_MATCH_LIMIT } = event.data || {};
  try {
    if (typeof pattern !== 'string' || pattern.length > REGEX_PATTERN_LIMIT) throw new Error(`Pattern must be at most ${REGEX_PATTERN_LIMIT} characters.`);
    if (typeof flags !== 'string' || flags.length > 8 || /[^dgimsuvy]/.test(flags)) throw new Error('Flags are not supported by this worker.');
    const expression = new RegExp(pattern, flags);
    const results = values.slice(0, REGEX_VALUE_LIMIT).map((value) => {
      expression.lastIndex = 0;
      return expression.test(String(value).slice(0, REGEX_SAMPLE_LIMIT));
    });

    const sampleFlags = flags.includes('g') ? flags : flags + 'g';
    const sampleExpression = new RegExp(pattern, sampleFlags);
    const boundedSample = String(sample).slice(0, REGEX_SAMPLE_LIMIT);
    const matches = [];
    let match;
    const matchLimit = Math.min(REGEX_MATCH_LIMIT, Math.max(1, Number(maxMatches) || REGEX_MATCH_LIMIT));
    while (matches.length < matchLimit && (match = sampleExpression.exec(boundedSample)) !== null) {
      matches.push({ value: match[0], captures: match.slice(1) });
      if (match[0] === '') sampleExpression.lastIndex += 1;
    }
    self.postMessage({ id, ok: true, results, matches });
  } catch (error) {
    self.postMessage({ id, ok: false, error: error instanceof Error ? error.message : String(error) });
  }
});
