# Destructive-action confirmation

The **Remove Sandbox** command uses a native super-confirmation before it starts deleting content. The gate names every selected sandbox, requires two independent confirmations, then requires a full-range slider. The final authorization button cannot enable early, and Escape or **Emergency exit** closes the gate without changing anything.

The bounded progress bar reports the exact slider percentage. Completing the range shows a separate completion state and a short opacity animation. `UIConfig/ReducedMotion` keeps all states available without animation. Focus returns to the control that opened the gate after cancellation or authorization.

The existing `NeverRemove` protection, unmount handling, and status checks remain in the removal path after the gate. This integration intentionally covers only **Remove Sandbox**; cleanup, recovery-file deletion, snapshot deletion, and other destructive commands retain their existing safeguards until each flow receives an equivalent review.

The gate is local-only and stores no keys, slider values, credentials, or telemetry. It is a decision prompt, so it is modal; progress and completion stay inside that prompt rather than becoming a nagging notification.

Suggested articles: [Notification center](qrc:/Docs/notifications.md), [Local settings history](qrc:/Docs/settings-history.md), and [Material appearance editor](qrc:/Docs/appearance-editor.md).
