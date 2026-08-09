# Material 3 shell boundary

The desktop window now has one frameless Material 3 chrome host. It owns the title surface, accessible minimize/maximize/close controls, menu placement, and window dragging. Existing content remains routable beneath this host while each legacy form is replaced by native M3 components.

The host is idempotent, keyboard reachable, and sized for touch. It does not block startup or show a payment/support prompt. A migration boundary is documented so a legacy form cannot quietly add another title bar or platform chrome.

## Verification

The source contract checks qmake registration, installation after the central widget, duplicate-install protection, and the frameless flag. Native compilation remains a Windows CI check when the pinned Qt kit is available.

## Suggested articles

- [Material 3 theme](qrc:/Docs/material-design.md)
- [Appearance editor](qrc:/Docs/appearance-editor.md)
- [Tab discovery](qrc:/Docs/tab-discovery.md)
