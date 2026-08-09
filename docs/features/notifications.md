# Notification center

Sandboxie routes informational `OnLogMessage(..., bNotify=true)` events into a non-blocking Notifications tab. The center keeps a bounded local history, supports extended multi-selection, dismiss-selected, dismiss-all, clear-history, plain-text search, and an opt-in `QRegularExpression` builder with a 512-character pattern bound. Double-clicking a notification link opens only an explicit HTTP(S) link.

Decision prompts and destructive confirmations remain modal because they require an answer before work can continue. Payment, donation, supporter, and certificate reminders are not emitted by the contributor capability path.

History is stored in the profile under `UIConfig/NotificationHistory`; it is local-only and capped at 100 visible records. A malformed or empty history simply starts empty. Static contract validation covers the source registration, non-blocking integration, bulk actions, bounded regex input, and README visual gallery. Qt/MSVC runtime capture remains pending until the configured build toolchain is available.

Suggested articles: [Local settings history](settings-history.md), [Material Design](../material-design.md), [Contributor build](../contributor-build.md).
