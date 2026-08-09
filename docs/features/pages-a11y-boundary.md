# GitHub Pages keyboard and overlay boundary

## Behaviour

The documentation tab strip now exposes its horizontal orientation, keeps one
roving tab stop, supports Left/Right/Home/End navigation, and marks inactive
panels hidden. The command palette overlay reports its open state with
`aria-hidden`, locks background scrolling while open, traps Tab focus inside
the bounded dialog, and restores focus to the invoking control on close.

## Responsive and motion rules

The palette remains viewport-bounded with an internal scroll region, while the
existing narrow-width grid and reduced-motion media query remain active. The
overlay never depends on pointer-only dismissal: Escape, the close button, and
the backdrop all remain available.

## Verification

Run `scripts/validate-pages-a11y.ps1` for structural checks. Manual verification
should exercise 320px and 200% zoom layouts, keyboard-only tab navigation,
palette focus cycling, Escape/backdrop close, and `prefers-reduced-motion`.
