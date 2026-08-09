# Dim-sum startup surprise

Sandboxie-Plus makes one fresh, per-launch random draw with a 10% probability after the normal desktop has become usable. When the draw succeeds, a small Material-styled, non-blocking toast shows one catalog dish in the active language mode, including the authoritative English and Traditional Chinese names and meaningful alt text.

## Source and offline behavior

The catalog authority is the public [`Ding-Ding-Projects/dim-sum-photos` catalog](https://raw.githubusercontent.com/Ding-Ding-Projects/dim-sum-photos/main/catalog/index.json). Published photos are accepted only from its immutable `catalog-v1*` release assets. The desktop app does not fetch a photo during startup and does not ship or vendor catalog images. Instead, an optional application-data cache at `<config>/dim-sum/catalog-cache.json` may contain a bounded, validated subset with:

- `sourceUrl`, exactly the public catalog URL;
- a non-empty `catalogRevision`;
- `dishes[].name.en` and `dishes[].name.zhHant`;
- `dishes[].image.alt.en` and `dishes[].image.alt.yue` (the catalog's canonical image-alt shape);
- the catalog's canonical `dishes[].image.path`;
- a local `dishes[].image.localPath` inside the same cache directory; and
- a pinned `dishes[].image.url` matching the public `catalog-v1` or `catalog-v1-part-###` release URL.

If the cache is missing, malformed, outside its directory, or has no decodable local image, the draw is skipped. Startup remains usable and no network request is made. Existing local caches may be refreshed from the public catalog by a separately reviewed packaging/update path; this feature never downloads or stores a substitute image.

## Presentation and suppression

The toast is delayed until after startup, uses `WA_ShowWithoutActivating` and `Qt::WindowDoesNotAcceptFocus`, remains at the lower-right of the available screen, and dismisses automatically after nine seconds. It does not add a setting or opt-out switch. It is suppressed for `-autorun`, pending command-line work, School mode, an active modal dialog or wizard, and unavailable/invalid cache data. Dish names remain factual; the existing language and funny-level formatter styles only the surrounding message.

## Verification

Run `node scripts/validate-dim-sum-surprise.mjs`. The validator checks the single 10% draw, one-launch scheduling, delayed non-blocking flags, suppression paths, bilingual names/alt text, public source URLs, cache validation, qmake/MSVC registration, documentation, and the absence of tracked image assets. Runtime screenshot verification still requires a built Qt artifact and the cheap headless route; no local Qt toolchain is assumed by this contract.

Suggested articles: [Material 3 appearance](material-design.md), [Notifications](notifications.md), [Settings history](settings-history.md).
