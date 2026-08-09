# GitHub Pages language and tone controls

## Behaviour

The Pages workspace renders English, playful Hong Kong Cantonese, and a compact
bilingual presentation from one bounded copy layer. English and Cantonese each
have an independently persisted funny-level slider. The level changes the
voice of information, warnings, and errors without changing the action, the
affected setting, or any verification claim. The dialogs-and-messages emoji
preference only decorates messages; controls keep factual accessible names.

The browser-local focused mode is user-renamable. While enabled, it forces
English and removes Cantonese, bilingual, funny-level, and dim-sum controls
from the rendered workspace. Turning it off requires the locally stored hash
of the code chosen by the visitor. This is an experience lock rather than an
account-security boundary: deleting the site’s local data deliberately resets
it. Existing dim-sum notices disappear from the active toast stack and the
visible notification list while focused mode is on; the stored history returns
when the visitor unlocks the mode.

## Documentation route and configuration

Canonical Markdown remains the source of truth, but every catalogued article
is rendered inside the Pages shell at `#/articles/<slug>`. Article links are
resolved only when the local route registry names them; unknown Markdown or
desktop-resource links render an honest unavailable state instead of a guessed
destination. The deployment also emits stable `articles/<slug>.html` entry
pages for all 22 catalogued articles plus changelog and screenshot routes;
each entry redirects locally into the same rendered shell. Related routes
appear in the **Suggested articles** area at the end of each rendered article.

Preferences, notification history, schedules, appearance choices, tab state,
and language choices stay in the current browser’s local storage. No
preferences, account tokens, personal vocabulary, or Home Assistant credential
are fetched or sent by this static site.

## Failure modes and security

An unknown language value falls back to English. Invalid regular expressions,
oversize patterns, backreferences, lookarounds, nested quantifiers, and
ambiguous repeated wildcards produce a local validation message rather than
running a potentially unbounded search. A missing article or route keeps the
user inside the documentation workspace with a factual error state. Remote
settings sources remain deliberately out of the static-site boundary: they
require the desktop application’s privileged credential handling.

## Verification

Run `node scripts/validate-pages-material.mjs`,
`node scripts/validate-pages-article-completeness.mjs`, and
`node --test scripts/test-pages-article-renderer.mjs`, and
`pwsh -NoProfile -File scripts/validate-pages-contract.ps1`. Manual browser
verification covers all three language modes, both sliders at levels 1/3/5,
focused-mode enable/unlock, a rendered article and suggested route, and a
narrow layout with long bilingual labels.

## Suggested articles

- [School mode](school-mode.md)
- [Scheduled settings](scheduled-settings.md)
- [Notification center](notifications.md)
