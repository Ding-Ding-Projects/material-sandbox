# GitHub Pages navigation and overlay boundary

## Implemented page behaviour

The documentation landing page has one persistent tab strip whose shipped
position is the left edge. A page preference can move that strip to the right,
top, or bottom. The strip reports a vertical orientation at either side and a
horizontal orientation at the top, bottom, and narrow-width fallback. Arrow
keys follow that axis; Home and End move focus; Enter and Space activate the
focused tab; and inactive panels use the `hidden` attribute.

The command palette is a bounded modal surface with Escape, backdrop, and close
button paths. Its focus loop stays inside the open palette. A normal close
returns focus to the invoking control, while a setting-teleport command keeps
focus on the selected live setting instead of immediately moving it back to the
palette invoker. Palette indexing covers tabs, linked documentation, feature
names, and settings.

Feature, settings, and palette searches each own a separate adjacent regular-
expression builder. This is a page-level search facility, not a claim that the
site already implements the desktop application's complete tab-management,
appearance-editor, history, export, or scheduling contracts.

## Responsive and motion boundary

Buttons and text-entry controls use the page's Material minimum target sizing,
and focusable page panels have their own visible focus treatment. The palette
paints its own surface and scrolls within a viewport bound. CSS honors the
operating system's reduced-motion request; scripted movement must consult the
same preference rather than relying on CSS alone.

## Source verification and remaining proof

Run:

```powershell
node scripts/validate-pages-material.mjs
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/validate-pages-a11y.ps1
node scripts/validate-pages-material.mjs --self-test
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/validate-pages-a11y.ps1 -SelfTest
```

The source validators check the hand-written accessibility inventory, including
axis-aware tabs, focusable panels, modal containment, post-command focus,
feature-name discovery, localized accessible names and options, persistent
error feedback, reduced-motion handling, and minimum target rules. They also
exercise deliberate negative source mutations in their self-test modes.

These checks parse committed source; they do not render a browser accessibility
tree or prove pixel geometry. Real browser verification remains required at
320/360/390/414 CSS pixels, 100/125/150/200% zoom, touch input, long bilingual
copy, high contrast, and full focus cycling. Pinning, grouping, reordering, the
four desktop tab-discovery scopes, per-element appearance editing, and bulk tab
actions remain documented product work rather than implemented landing-page
controls.

## Suggested articles

- [GitHub Pages language, tone, and appearance controls](pages-language-tone.md)
- [Command palette](command-palette.md)
- [Tab discovery](tab-discovery.md)
- [Material Design 3 desktop shell](../material-design.md)
