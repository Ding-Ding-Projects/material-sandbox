# GitHub Pages language, tone, and appearance controls

## Implemented page behaviour

The landing page provides English, Hong Kong Cantonese, and bilingual modes for
its hand-written page-copy inventory. That inventory includes visible labels,
select options, placeholders, dialog names, search controls, and other
accessible names; adding a new user-facing control requires adding it to the
inventory in the same change. Bilingual text identifies its Cantonese span so a
screen reader does not pronounce the whole line with an English voice. Native
HTML option rows cannot contain separately tagged spans, so bilingual mode keeps
their visible row in English and attaches a separately language-tagged bilingual
description of the available choices to each select.

English and Cantonese each have an independent five-position tone control for
the dynamic hero and notification copy currently wired to it. The levels change
voice only: status, action meaning, version, source revision, and verification
facts remain unchanged. Generated article routes inherit the same interactive
site shell and add their own bounded article search and adjacent regex builder,
but the source article prose remains English until a reviewed translation is
available. This is not a claim that every desktop message already has five
rewritten variants. The emoji preference changes only an `aria-hidden`
decoration.

Theme, density, bounded font scale, validated font family, motion, accent seed,
and tab position apply to this page and persist in this browser. The accent
control synchronizes the browser color field with six-digit HEX, RGB, and HSL
feedback. Contrast-safe foreground and primary roles are derived from the seed,
while invalid HEX input is rejected before application. Storing a seed is not
treated as proof that its text remains readable. During publication, the site
is copied to a separate staging directory and its marker is replaced with the
exact 40-character source revision before that staged directory is uploaded.

## Storage, regex, and network boundaries

Preferences use a versioned, page-specific storage namespace. Reads and writes
handle unavailable browser storage without aborting page initialization.
Migration accepts only known keys, enum values are allowlisted, scalar values
are bounded, and reset removes only this page's namespace. The page stores no
credentials, browsing history, clipboard contents, or private font inventory.

Search patterns and samples remain local. Length is enforced in code as well as
HTML attributes, including guided insertion. Before evaluation, a conservative
safety check rejects constructs that can cause unbounded backtracking; actual
search execution uses the page's bounded evaluation path. Invalid or rejected
patterns leave an actionable message without running the expression. Clipboard
access is write-only and starts only from the user's Copy action. Clipboard and
other error notifications remain visible until dismissed.

Localized and user-entered text reaches the DOM through `textContent` or created
text nodes, not HTML evaluation. Accent input is validated before reaching a CSS
custom property, and font input is assigned only through the `fontFamily`
property. The source contract rejects remote CSS imports/URLs, frames, embedded
media or objects, remote scripts/styles/images, and outbound `fetch`, beacon,
XHR, WebSocket, or EventSource code. The page has no analytics or third-party
runtime asset path.

## Documented inventory, not universal implementation

Feature cards form a hand-written index of detailed project articles. The Pages
workflow renders the canonical manifest into local HTML routes that clone the
stamped interactive site shell, preserve internal article links, and add a
Suggested articles section. They do not mean every article's desktop capability
is reimplemented inside the static landing page, nor do they replace
Sandboxie's full core-product inventory. The desktop application, offline
documentation browser, README, release evidence, and runtime captures remain
separate verification surfaces.

## Verification

Run:

```powershell
node scripts/validate-pages-material.mjs
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/validate-pages-a11y.ps1
node scripts/validate-pages-material.mjs --self-test
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/validate-pages-a11y.ps1 -SelfTest
node --test scripts/test-pages-stamp.mjs
node --test scripts/test-pages-interactions.mjs
node --test scripts/test-pages-article-renderer.mjs
```

The Material validator checks the explicit copy/accessibility inventory, safe
storage contract, conservative regex path, persistent errors, contrast roles,
source revision, local-only resource boundary, public-copy vocabulary boundary,
feature-article index, unique IDs, referenced control targets, and JavaScript
syntax. Its self-test mode applies deliberate in-memory violations to prove
critical checks fail. Browser verification is still required for rendered
contrast, language pronunciation, all tone levels, 320-pixel layout, 200% zoom,
high contrast, reduced motion, and storage-disabled behavior.

## Suggested articles

- [GitHub Pages keyboard and overlay boundary](pages-a11y-boundary.md)
- [Material Design 3 desktop shell](../material-design.md)
- [Color translator](color-translator.md)
- [Command palette](command-palette.md)
