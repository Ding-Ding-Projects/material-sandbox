# GitHub Pages language and tone controls

## Behaviour

The page preference controls now have a real rendered effect. English, playful
Hong Kong Cantonese, and bilingual mode update the hero and publication-status
copy in place. The English and Cantonese funny-level sliders independently add
voice styling at levels 3–5 while preserving the factual status and feature
claims. Preferences remain local to the browser and survive reloads.

## Failure modes and security

The page uses a bounded in-memory copy table and `textContent`; it does not
evaluate user-provided text as HTML or send preferences over the network. A
missing or unknown language value falls back to English. The controls do not
claim to change the desktop application's translation catalog.

## Verification and remaining work

The Pages contract validator checks that the copy table, `applyPresentation`,
and both funny-level listeners exist. Manual browser verification should check
English, Cantonese, bilingual, and levels 1/3/5 at narrow widths. This slice
does not yet translate every static article/card label or implement the
desktop-only School-mode unlock; those remain explicit follow-up gaps.
