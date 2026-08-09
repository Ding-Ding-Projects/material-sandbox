/* Conservative, dependency-free guard for the browser's ECMAScript regex engine. */
(() => {
  const MAX_PATTERN = 256;
  const MAX_SAMPLE = 512;
  const MAX_VARIABLE_QUANTIFIERS = 2;
  const MAX_OPTIONAL_QUANTIFIERS = 8;

  function stripEscapesAndClasses(value) {
    let output = ''; let escaped = false; let inClass = false;
    for (const char of String(value || '')) {
      if (escaped) { output += 'x'; escaped = false; continue; }
      if (char === '\\') { escaped = true; output += 'x'; continue; }
      if (inClass) { if (char === ']') inClass = false; output += 'x'; continue; }
      if (char === '[') { inClass = true; output += 'x'; continue; }
      output += char;
    }
    return output;
  }

  function countQuantifiers(pattern) {
    let variable = 0; let optional = 0;
    for (let index = 0; index < pattern.length; index += 1) {
      const char = pattern[index];
      if (char === '*' || char === '+') variable += 1;
      else if (char === '?') optional += 1;
      else if (char === '{') {
        const match = /^\{\d+(?:,\d*)?\}/.exec(pattern.slice(index));
        if (match) { variable += 1; index += match[0].length - 1; }
      }
    }
    return { variable, optional };
  }

  function isRiskyPattern(value) {
    const raw = String(value || '');
    if (!raw || raw.length > MAX_PATTERN) return raw.length > MAX_PATTERN;
    if (/\\(?:[1-9]|k<)/.test(raw)) return true; // backreferences
    const pattern = stripEscapesAndClasses(raw);
    if (/\(\?/.test(pattern)) return true; // lookaround, named, and non-capturing forms
    if (/\)(?:[+*?]|\{\d+(?:,\d*)?\})/.test(pattern)) return true; // quantified group
    if (/\.[*+][\s\S]*\.[*+]/.test(pattern)) return true; // overlapping wildcards
    const { variable, optional } = countQuantifiers(pattern);
    return variable > MAX_VARIABLE_QUANTIFIERS || optional > MAX_OPTIONAL_QUANTIFIERS;
  }

  window.MaterialSandboxRegexSafety = Object.freeze({ MAX_PATTERN, MAX_SAMPLE, isRiskyPattern });
})();
