# Destructive-action confirmation

The **Remove Sandbox** command now uses a native Material-styled super-confirmation before it starts deleting content. The gate names every selected sandbox, requires two independently operated confirmations, then requires the user to move a full-range slider. The final authorization button cannot enable early, and Escape or **Emergency exit** closes the gate without changing anything.

While the slider moves, a bounded progress bar reports the exact percentage. Completing the range shows a separate completion state and a short opacity animation. When `UIConfig/ReducedMotion` is enabled, the same states are shown without animation. Focus returns to the control that opened the gate after either cancellation or authorization.

The existing sandbox-level `NeverRemove` protection, unmount handling, and status checks remain in the removal path after the gate. The aggregate **Cleanup** toolbar action now uses the same gate and names the message, trace, recovery, and process entries it will clear. Individual log actions remain unchanged and retain their existing safeguards until each flow receives an equivalent review.

The gate is local-only and does not store keys, slider values, credentials, or telemetry. It is a decision prompt, so it is modal; progress and completion are kept inside that prompt rather than emitted as a nagging notification.

Verification: run `node scripts/validate-destructive-confirmation.mjs`. Hosted Qt/MSVC build and hidden-desktop capture remain the authoritative runtime checks. The static contract checks source registration, both independent keys, the full-range slider, reduced-motion path, focus return, and the real Remove Sandbox wiring.

## GitHub Pages workspace

The GitHub Pages workspace uses the same two independently operated keys and
full-range slider before it removes browser-local records or replaces them.
The gate names the exact tab review, schedule rule, notification selection,
saved appearance preset, selected appearance override, or parsed preferences
file that will change. A cancellation or completed authorization returns focus
to the originating control; progress and completion remain reduced-motion aware
inside the decision dialog. Import parses and allowlists the file before review,
but does not replace local preferences until this gate completes.

Run `node scripts/validate-pages-material.mjs` and manually test cancel and
authorization for a preset deletion, appearance reset, and preference import.

Suggested articles: [Notification center](notifications.md), [Local settings history](settings-history.md), and [Material appearance editor](appearance-editor.md).
