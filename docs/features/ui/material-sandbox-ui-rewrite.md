# Material Sandbox UI rewrite overlay

## Behaviour

The rewrite adds a shared Material 3 shell for SandMan: a frameless app bar,
searchable menus, a bounded regex builder, a navigation rail, a tab strip,
snackbars, and read-only local Memory/Skills/Operations/Status destinations.
It rehosts the existing Boxes central widget and adapts the live Settings and
Box Options `QTabWidget` in place, preserving page objects, indexes, signals,
and `CTabStateManager` ownership.

The search field and its anchored builder share `M3RegexExecutionPolicy`.
Invalid expressions resolve to an invalid sentinel rather than an empty
match-all expression. The builder keeps Qt's normal literals, classes,
anchors, groups, alternation, quantifiers, and `i`/`m`/`s`/`x`/`U` flags, while
making pattern, flags, sample, match, capture, and preview-output bounds
visible. The shared expression prefix uses PCRE `LIMIT_MATCH=100000` and
`LIMIT_DEPTH=1000` verbs before the user's unmodified pattern, so execution
is bounded by the actual Qt regex engine without a second incomplete grammar
parser or altered capture numbering. Search accessibility names and state
descriptions are forwarded to the focused editor; menu-originated builders
retain Escape, focus return, and menu restoration behavior.

The builder uses the shared dialog host exactly once and reflows its fields,
guided tokens, and actions below the narrow-layout threshold. It remains
anchored to its source when room permits, otherwise uses a viewport-bounded,
scrollable fallback instead of painting off-screen.

## Configuration and integration

`tools/apply_ui_rewrite.py` copies the overlay, patches `SandMan.pri`, updates
the tracked MSVC project when present, and installs the shell/workspace at the
existing SandMan lifecycle seams. Reapplying is idempotent. The overlay does
not change sandbox, service, driver, IPC, certificate, installer, or security
logic.

## Failure and security boundaries

The local repository reader canonicalizes the configured root and candidate
paths, rejects traversal, bounds text to 1 MiB and listings to 2,000 entries,
and does not launch processes or make network requests. Native compilation,
startup, driver communication, high-DPI behavior, bilingual layout, and real
window captures are not implied by the static validator; failures in those
areas must remain visible until Windows evidence exists.

The MSVC project contains one `MaterialTheme.cpp` and one `M3Tokens.cpp`
compile item, each with `PrecompiledHeader` disabled because those external
sources do not include `stdafx.h`. The qmake and MSVC registrations also carry
the shared regex policy.

## Verification

Run `node scripts/validate-m3-ui-rewrite.mjs --root .`, `node --check
scripts/validate-m3-ui-rewrite.mjs`, `py -3 -m py_compile` on the materializer
scripts, and `git diff --check`. Follow `CODEX_HANDOFF_UI_REWRITE.md` and
`NATIVE_VERIFICATION.md` for the required x64/ARM64 Qt/MSVC and runtime matrix.

## Suggested articles

- [Material 3 shell boundary](m3-shell-boundary.md)
- [Tab discovery](../tab-discovery.md)
- [Native CI evidence](../native-ci-evidence.md)
