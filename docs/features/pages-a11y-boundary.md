# GitHub Pages keyboard and overlay boundary

## Behaviour

The Pages workspace is a browser-style tab surface rather than one long page.
The tab strip starts on the left and can dock on every edge. It exposes the
correct `aria-orientation`: vertical strips use Up/Down and horizontal strips
use Left/Right, with Home and End available in both. Tabs retain `tablist`,
`tab`, and `tabpanel` semantics, a roving tab stop, visible focus, an explicit
selected state, and a controlled panel relationship.

Every search field—including strip, per-group, group-name, master-tab,
settings, history, notifications, command palette, and bulk-close search—has
its own adjacent regex builder. Plain text remains the default. The builder
limits input size and rejects a small high-risk nested-quantifier set before
evaluation. Regex sample feedback runs in a local worker with a bounded
timeout; an overdue preview is terminated rather than holding the page thread.

## Responsive and motion rules

Controls meet a 44px target baseline. The narrow layout moves the left/right
rail into a scrollable row instead of clipping its labels. The site honors
both the visitor’s reduced-motion setting and `prefers-reduced-motion`.

The tab and appearance context menus paint their own labelled surface, move
focus to their local search field, and return focus through a stable selector
after an internal rerender. The move-into-group picker supports Arrow Up,
Arrow Down, Enter, and Escape. Appearance and group dialogs anchor beside the
originating control where space permits, scroll that origin back into view if
needed, and clamp to the viewport instead of appearing off-screen. The
appearance and focused-mode dialogs each carry their own local regex builder.

Native dialog elements provide the command palette, appearance editor,
focused-mode control, tab manager, and bulk-close review. They have explicit
close actions, respond to Escape, retain a return-focus path, paint their own
surface, and are restored across an internal rerender so typing in a search
field does not silently close the dialog.

Browser-local destructive actions use two independently operated keys before
enabling a full-range slider. The slider paints non-blocking progress, and
reaching the end triggers a distinct reduced-motion-aware authorization pulse.

## Verification

Run `pwsh -NoProfile -File scripts/validate-pages-a11y.ps1`,
`node --test scripts/test-pages-interactions.mjs`, and manually
exercise all four dock edges, keyboard-only tab navigation, command-palette
open/close and return focus, regex validation, group-picker arrows, an
anchored appearance editor, the two-key slider gate, a bulk-close review with
pinned tabs excluded, and 320px/200% layouts.

## Suggested articles

- [Tab discovery](tab-discovery.md)
- [Command palette](command-palette.md)
- [Material appearance editor](appearance-editor.md)
