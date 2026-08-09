# Material color translator

The Material appearance editor accepts a seed color as HEX/HEX8, RGB/RGBA, or HSL/HSLA. Editing any field updates the other representations, preserves alpha, previews the color, and reports contrast ratios against white and black. Plain keyboard editing and labelled fields work without a pointer; Cancel leaves the stored seed unchanged, while OK persists the new seed and refreshes the live Material theme.

Invalid or out-of-range values remain in the edited field and do not overwrite the last valid color. The editor uses Qt's `QColor` conversion and bounded numeric inputs; it does not claim a broader gamut conversion than Qt exposes.

Suggested articles: [Material Design](../material-design.md), [Local settings history](settings-history.md), [Notification center](notifications.md).
