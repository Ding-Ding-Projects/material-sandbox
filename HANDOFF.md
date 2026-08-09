# Handoff

## Scope

The GitHub Pages source has been rebuilt as a local-first Material Design 3
documentation workspace under `docs/`. The page shell, application logic,
styles, renderer, article route registry, and focused validation scripts are
kept together so a later change does not reintroduce a raw-Markdown or
project-subpath deployment failure.

## What to verify after a Pages change

Run:

```powershell
node scripts/validate-pages-material.mjs
node scripts/validate-pages-article-completeness.mjs
node scripts/validate-pages-markdown-renderer.mjs
node scripts/validate-pages-regex-safety.mjs
node --check docs/regex-worker.js
pwsh -NoProfile -File scripts/validate-pages-a11y.ps1
pwsh -NoProfile -File scripts/validate-pages-contract.ps1
node --test scripts/test-pages-interactions.mjs
node --test scripts/test-pages-article-renderer.mjs
```

Then load the deployed project-subpath URL, open a canonical article at
`#/articles/<slug>`, test the `Ctrl+Shift+F` command palette, switch the tab
rail at a narrow width, and confirm browser-local import filtering. Do not use
a Pages deployment to claim native desktop or installer verification. Also
open `articles/<slug>.html` and confirm it enters the same local article route.

## Static-site safety notes

- Canonical article fetches are built from the current project base path.
- Preferences are allowlisted and bounded before they reach state or CSS.
- Exported preferences exclude the focused-mode unlock hash; import resets the
  focused mode until the visitor intentionally enables it again.
- The dim-sum delight is an auto-dismissing toast, not a modal dialog.
- Destructive browser-local actions require two keys and a full-range slider.
- Regex previews run in a local worker with a short cancellation timeout; the
  main search path keeps bounded pattern/sample inputs and conservative guards.

## Ongoing ownership

When changing a feature article, update its route record, the appropriate
Pages contract, and the in-site documentation behavior in the same change.
