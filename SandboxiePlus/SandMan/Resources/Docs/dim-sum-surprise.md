# Dim-sum startup surprise

Sandboxie-Plus makes one fresh random draw per launch with a 10% probability after the normal desktop is usable. A successful draw shows a small non-blocking toast with one dish's authoritative English and Traditional Chinese names plus meaningful image alt text.

The metadata authority is the public [dim-sum catalog](https://raw.githubusercontent.com/Ding-Ding-Projects/dim-sum-photos/main/catalog/index.json), and photos may only come from its immutable `catalog-v1` or `catalog-v1-part-###` release assets. The app does not fetch photos at startup or vendor images. An optional bounded application-data cache at `<config>/dim-sum/catalog-cache.json` must record the exact catalog URL, a non-empty revision, canonical `name.en`/`name.zhHant`, `image.path`, `image.alt.en`/`image.alt.yue`, a local `image.localPath` inside the cache directory, and a pinned public release URL. Missing, malformed, oversized, unsafe, or undecodable cache data skips the draw without delaying startup.

The toast uses `WA_ShowWithoutActivating` and `WindowDoesNotAcceptFocus`, is placed at the lower-right of the available screen, and dismisses after nine seconds. There is no opt-out setting. It is suppressed for School mode, `-autorun`, pending command-line work, active modal dialogs or wizards, and any unavailable local asset. Language mode and funny-level formatting styles only the surrounding copy; dish names and alt text remain factual.

Run `node scripts/validate-dim-sum-surprise.mjs` to check the 10% single draw, readiness and suppression gates, non-blocking flags, bilingual metadata, public-source contract, cache bounds, build registration, docs, and no tracked consumer images.

Suggested articles: [Material Design](qrc:/Docs/material-design.md), [Notifications](qrc:/Docs/notifications.md), [Settings history](qrc:/Docs/settings-history.md).
