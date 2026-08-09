# SandMan Material 3 — UI rewrite handoff

Target repo: `material-sandbox` (Sandboxie‑Plus desktop manager, Qt/MSVC)
Feature source: `agent-global-memory` (managed instructions, skills, sync, status hub)
Design source of truth: **`SandMan M3.dc.html`** in this project — a working, clickable build of every screen and overlay described below. Where this document and the design disagree, the design is correct.

Everything in the design was recreated from files in the two attached checkouts. Nothing is invented from memory of the product. The provenance table at the end of each section names the file each surface came from.

---

## 1. What changed, in one paragraph

The desktop manager keeps its menu bar and its Qt data model, and gains a Material 3 shell: an M3 top app bar that also hosts the four menus and the global search, a primary tab strip for browser‑style workspace tabs, an 80 px M3 navigation rail with eleven destinations, and a rounded content pane on `surface‑container‑low`. Five of those destinations are new and come from `agent-global-memory`: Sync, Skills, Memory, Ops, Status. Every menu in the product — including every right‑click context menu — now opens with a search field at the top that filters the menu live and carries the mandatory regex‑builder button. Colour, type, shape, elevation and state layers come from one token set with a light and a dark map; the memory surfaces use a second seed (teal) inside the same token structure.

---

## 2. Colour tokens

Two seeds. Purple `#6750A4` is the app seed (unchanged from `MaterialTheme.cpp`). Teal `#006A63` is the memory seed and is used **only** on the five memory destinations and on memory‑owned chrome (rail pills, sync state chips, session cards).

### 2.1 App palette (purple seed)

| M3 role | Light | Dark |
| --- | --- | --- |
| `surface` | `#FEF7FF` | `#141218` |
| `surface-container-lowest` | `#FFFFFF` | `#0F0D13` |
| `surface-container-low` | `#F7F2FA` | `#1D1B20` |
| `surface-container` | `#F3EDF7` | `#211F26` |
| `surface-container-high` | `#ECE6F0` | `#2B2930` |
| `surface-container-highest` | `#E6E0E9` | `#36343B` |
| `on-surface` | `#1D1B20` | `#E6E0E9` |
| `on-surface-variant` | `#49454F` | `#CAC4D0` |
| `outline` | `#79747E` | `#938F99` |
| `outline-variant` | `#CAC4D0` | `#49454F` |
| `primary` | `#6750A4` | `#D0BCFF` |
| `on-primary` | `#FFFFFF` | `#381E72` |
| `primary-container` | `#EADDFF` | `#4F378B` |
| `on-primary-container` | `#21005D` | `#EADDFF` |
| `secondary` | `#625B71` | `#CCC2DC` |
| `secondary-container` | `#E8DEF8` | `#4A4458` |
| `on-secondary-container` | `#1D192B` | `#E8DEF8` |
| `tertiary` | `#7D5260` | `#EFB8C8` |
| `tertiary-container` | `#FFD8E4` | `#633B48` |
| `on-tertiary-container` | `#31111D` | `#FFD8E4` |
| `error` | `#B3261E` | `#F2B8B5` |
| `error-container` | `#F9DEDC` | `#8C1D18` |
| `on-error-container` | `#410E0B` | `#F9DEDC` |
| `inverse-surface` | `#322F35` | `#E6E0E9` |
| `inverse-on-surface` | `#F5EFF7` | `#322F35` |

### 2.2 Memory palette (teal seed) and semantic extras

| Role | Light | Dark | Used by |
| --- | --- | --- | --- |
| `mem` (primary) | `#006A63` | `#82D5CC` | Memory rail labels, sync CTA, session accents |
| `on-mem` | `#FFFFFF` | `#003733` | Text on filled memory buttons |
| `mem-container` | `#9EF2E6` | `#00504A` | Rail indicator, "current" chips, skill avatars |
| `on-mem-container` | `#00201D` | `#9EF2E6` | Text inside those containers |
| `ok` / `ok-container` | `#2E6B12` / `#D7F5C4` | `#B7F397` / `#254D14` | Running processes, green CI, valid regex |
| `warn-container` / `on-warn-container` | `#FFE7C2` / `#2A1800` | `#5C3D00` / `#FFDDB0` | Unsigned‑release chips, unsupported‑property notice |
| `state` (hover layer) | `rgba(29,27,32,.08)` | `rgba(230,224,233,.08)` | 8 % state layer |
| `state-strong` (pressed) | `rgba(29,27,32,.12)` | `rgba(230,224,233,.12)` | 12 % state layer |
| `scrim` | `rgba(0,0,0,.32)` | `rgba(0,0,0,.55)` | Dialog scrim |

### 2.3 Elevation

M3 levels, expressed as shadows because Qt does not tint by elevation:

| Level | Light | Dark | Applied to |
| --- | --- | --- | --- |
| 1 | `0 1px 3px 1px rgba(0,0,0,.15), 0 1px 2px rgba(0,0,0,.30)` | same with `.30/.40` | Filled buttons on hover, list rows lifted |
| 2 | `0 2px 6px 2px …` | idem | Menus, context menus |
| 3 | `0 4px 8px 3px …` | idem | Dialogs, FAB‑weight New Box button, snackbar |

---

## 3. Typography, shape, density, spacing

### 3.1 Type scale (Roboto; `Roboto Mono` for paths, ids, logs)

| Token | Size / line | Weight | Tracking | Used for |
| --- | --- | --- | --- | --- |
| `display-small` | 32 / 40 | 400 | 0 | Page titles (“Sandboxes”, “Settings”) |
| `headline-small` | 24 / 32 | 400 | 0 | Dialog titles |
| `title-large` | 22 / 28 | 400 | 0 | App name, panel titles |
| `title-medium` | 16 / 24 | 500 | +.15 | Card headings, selected‑item name |
| `body-large` | 16 / 24 | 400 | +.5 | Search fields, select values |
| `body-medium` | 14 / 20 | 400 | +.25 | Table cells, body copy |
| `label-large` | 14 / 20 | 500 | +.1 | Buttons, menu items, tabs |
| `label-medium` | 12 / 16 | 500 | +.5 | Column headers, status bar, rail labels |
| `label-small` | 11 / 16 | 500 | +1.5 | Eyebrows (uppercase), chips |

`Options/FontScaling` still multiplies the whole scale. The design keeps 14 px as the base so a 100 % profile equals today's 10 pt Qt default at 96 dpi.

### 3.2 Shape

| Token | Radius | Applied to |
| --- | --- | --- |
| extra‑small | 4 px | Text fields (M3 filled/outlined), snackbar |
| small | 8 px | Filter/assist chips, small selects |
| medium | 12 px | Cards, tables, menus, banners, list rows |
| large | 16 px | Rail indicator (pill 56×32 → r16), FAB‑weight button, toolbars |
| extra‑large | 28 px | Dialogs, search bars (56 px tall pill) |
| full | 999 px | Icon buttons (40 px), pill buttons (40 px h / r20) |

### 3.3 Density and control heights

`Options/Density` maps to a spacing multiplier, not to a second stylesheet.

| Density | Row height | Control height | Page padding | Gap |
| --- | --- | --- | --- | --- |
| Compact (−4) | 40 px | 36 px | 16 px | 8 px |
| Comfortable (0) — default | 48 px | 40 px | 24 px | 16 px |
| Spacious (+4) | 56 px | 48 px | 32 px | 24 px |

Fixed regardless of density: top app bar 64 px, tab strip 48 px, rail width 80 px, rail indicator 56×32, status bar 40 px, menu row 48 px, search bar 56 px, dialog padding 24 px. Minimum hit target is 40×40; nothing in the design is smaller.

---

## 4. Screen‑by‑screen mapping (Qt widget tree → M3)

### 4.1 Shell

| Design element | Today | Change |
| --- | --- | --- |
| Top app bar, 64 px | `M3TitleBar` (40 px) + `QMenuBar` on a separate row | Merge. `M3ShellHost::Install` builds one 64 px bar: app icon → title + version + contributor chip → the four `QMenu` triggers as flat 40 px buttons → global search → language/theme/notifications → window controls. Keeps the frameless drag behaviour and `windowTitleChanged` hook. |
| Menus | `m_pMenuBar->addMenu(tr("&Sandbox"))` etc. | Same four menus, same actions, same order. Rendered by a new `M3Menu` popup (see §5). |
| Workspace tabs, 48 px | none in SandMan (exists in Memory Console) | New `M3TabStrip`: M3 primary tabs, 3 px indicator, close icon per tab, pin marker in `mem`, drag to reorder, context menu. Persist order/active/pin/group like `docs/features/tab-discovery.md` requires. |
| Navigation rail, 80 px | none (toolbar only) | New `M3NavigationRail`. Eleven destinations in three labelled groups: **SANDBOX** Boxes · Recovery · Trace · Snapshots · Docs — **MEMORY** Sync · Skills · Memory · Ops · Status — **SYSTEM** Settings. Selected item: 56×32 `secondary-container` pill (`mem-container` for memory items), filled icon, label in `primary`/`mem`. |
| Toolbar | `CSandMan::CreateToolBar` + `GetToolBarItemsConfig` | **Removed as a separate strip.** Its actions live on: the page header (Create New Box, Sandbox Options), the selection bar (Run, Recover, Snapshots, Remove), the status bar (Trace logging, Forcing paused, Recovery), and the menus. The scriptable toolbar config keeps working; it now populates an overflow menu. Decide before implementation whether to keep the configurable strip as an option — see §10.1. |
| Content pane | `QMainWindow` central widget | `surface-container-low`, `border-radius: 16px 0 0 0`. Only the top‑left corner is rounded (M3 pane treatment). |
| Status bar, 40 px | `statusBar()` + `m_pSummaryInfo`, `m_pTraceInfo`, `m_pDisabledForce`, `m_pDisabledRecovery`, `m_pDisabledMessages`, `m_pRamDiskInfo` | Same five permanent widgets, now 28 px pill **buttons** that toggle their setting on click (they were read‑only labels). Summary text keeps the left stretch. |

### 4.2 Boxes (default destination)

Source: `Views/SbieView.cpp`, `Models/SbieModel.cpp`.

* Columns are unchanged and in the same order: **Name · Status · Info · Path / Command Line** (`SbieModel::headerData`, cases `eName`, `eStatus`, `eInfo`, `ePath`). Header cells become sort buttons with an M3 arrow.
* Tree structure unchanged: group → box → process, 28 px indent per level, chevron button, then the type icon.
* Row height 48 px, selection uses `secondary-container`, hover uses the 8 % state layer instead of alternating row colours. `Options/AlternateRowColors` still applies when the user asks for it.
* Box icons: `inbox` (empty), `inventory_2` (in use), `lock` + a teal “encrypted” chip (encrypted box), `folder` (group), `memory` (process). The existing `sandbox-empty.png` / `sandbox-full.png` / `Boxes/*` assets remain valid; the design uses Material Symbols so the icon set is one family — pick one before implementation (§10.2).
* Filter chips above the table (All boxes / Has processes / Empty / Encrypted) are new and map to existing model filters plus `m_pShowHidden`.
* A per‑row `more_vert` button opens the same context menu as right‑click — needed for keyboard and touch parity.
* Selection bar under the table: Run · Recover Files · Snapshots · Remove Sandbox, mapping to `m_pMenuRun`, `m_pMenuRecover`, `m_pMenuSnapshots`, `m_pMenuRemove`.

### 4.3 Recovery

Source: `Windows/RecoveryWindow.cpp`. Was a separate window; becomes a destination (it can still open as a dialog from a box context menu).

Preserved verbatim: columns **File Name · File Size · Full Path** (`m_pFileModel->AddColumn`), `Show All Files` / `Show Ignored Files`, the `Recover target:` combo with its three entries (`Original location`, `Browse for location`, `Clear folder list`) plus remembered folders, `Add Folder`, `Refresh`, and the button row `Recover / Delete / Delete Content / Close`. The two close‑menu options (“Close until all programs stop in this box”, “Close and Disable Immediate Recovery for this box”) are shown as a footer note; keep them on the Close split button.

### 4.4 Trace

Source: `Views/TraceView.cpp`, `Models/TraceModel.cpp`. Columns **Process · Type · Status · Value** exactly as `TraceModel::headerData`. The trace toolbar keeps every control: Monitor mode, Show as task tree, Show NT Object Tree, PID, TID, Type, Status (`Open/Closed/Trace/Other`), Show All Boxes, Show Stack Trace, Save to file, Cleanup Trace Log, Auto Scroll. Monospace rows at 32 px, status colour‑coded (`ok` open, `error` closed).

### 4.5 Snapshots

Source: `Windows/SnapshotsWindow.cpp`. Two columns **Snapshot · Creation Time**, indented to show the snapshot tree. Right pane keeps the two group boxes: *Selected Snapshot Details* (Name, Description, Default snapshot) and *Snapshot Actions* (Take Snapshot, Go to Snapshot, Revert to empty box, Remove Snapshot). Revert and Remove route through the destructive gate.

### 4.6 Docs

Source: `Windows/DocumentationBrowser.cpp`. Two tabs — Articles and Changelog — a search field with the regex button, `Copy` / `Export…`, and the status line (`%1 bundled articles · offline and searchable`). Changelog keeps its From/To ISO date fields and the exact validation strings (“Use ISO dates like 2026-08-09”, “Start date must not be after end date”).

### 4.7 Memory destinations (new)

| Destination | Content | Source |
| --- | --- | --- |
| Sync | Managed‑block state, four stat cards, the three‑runtime target table (exact paths from the README’s *Installed targets* table), attestation block, `status` / dry‑run / `install -Yes` buttons, conflict banner with exit code 2 | `agent-global-memory/README.md`, `scripts/sync-agent-memory.ps1`, `contracts/agent-memory-sync-*.schema.json` |
| Skills | Card per owned skill with scope, install badge, install path, Open SKILL.md, Reinstall | `skills/`, `docs/features/memory-sync/convenience-skills.md` |
| Memory | File list (SHARED_INSTRUCTIONS.md, HOST_INVENTORY.md, `projects/*.md`, PERSONAL_VOCABULARY.json, CLAUDE.md) with a reader pane, Open in VS Code, Export | `memory/`, `docs/features/memory-sync/project-profiles.md` |
| Ops | Failing‑run banner using the exact phrase “the hui is red”, workflow runs, releases with an `unsigned` chip, rolling Discussion / pinned changelog / project item | `README.md` (release + CI evidence), `docs/features/operations/*` |
| Status | One card per session: agent, session id, state dot, headline, monospace evidence block, progress, Reply to session, Reveal card | `apps/status-hub/`, `docs/features/memory-sync/status-hub.md` |

### 4.8 Settings

Source: `Forms/SettingsWindow.ui`. The six Qt pages keep their titles and their fields — *General Config*, *Shell Integration*, *Interface Config*, *Add-Ons Manager*, *Support && Updates*, *Advanced Config* — and four pages are added for the global‑memory contract: *Presentation*, *Scheduled settings*, *School mode*, *Notifications*. Layout is an M3 two‑pane: 56 px pill list on the left, sectioned cards on the right, one 56 px row per setting with the control right‑aligned.

The settings search is cross‑page: typing filters **every** page and each left‑hand entry shows its hit count. That is the behaviour the memory contract asks for (“searchable settings”), and it is new.

Provenance line under the title is the settings‑provenance feature (`docs/features/settings-provenance.md`): it states whether values come from the user profile or the compiled‑in fallback and refreshes after edit and reset.

### 4.9 Box Options

Source: `Windows/OptionsWindow.cpp` + `Options*.cpp` per page. Eight pages — General, File options, Security, Network, Recovery, Start / Run, Stop behaviour, Advanced — in the same two‑pane form, with its own search field and regex button. `Options/UsePageTree` chooses between the pill list (page tree) and nested M3 secondary tabs.

---

## 5. Menus and the search requirement

**Every menu in the product opens with a search field.** That includes the four menu‑bar menus and all four context menus (box, group, process, tab). The pattern:

```
┌ menu, surface-container, r12, elevation 2, width 304 ────┐
│  [ search  ⌕  Search sandbox actions       ✕   .*  ]  40px│  ← surface-container-high, r20
├──────────────────────────────────────────────────────────┤
│  ⟨icon 24⟩  Label                          Shortcut   48px│
│  ───────── separator (hidden while filtering) ───────────│
│  ⟨icon 24⟩  Label                          ▸          48px│
└──────────────────────────────────────────────────────────┘
```

Rules:

1. The field is focused on open; typing filters by visible label, case‑insensitive, plain text.
2. Separators are suppressed while a filter is active (otherwise the filtered list is full of stray rules).
3. Empty state reads “Nothing in this menu matches that search.”
4. The `.*` button opens the full regex builder for that field — this is the mandatory per‑search‑bar builder from the memory contract, and the menu search is a search bar.
5. Escape closes; the menu closes on outside click and on right‑click elsewhere.

Menu contents are unchanged from source: `SandMan.cpp:1002‑1160` for the four menu‑bar menus, `SbieView.cpp:176‑312` for the box/process menus (advanced layout) and `:345‑472` for the simple layout and group menu.

---

## 6. New and changed files

### 6.1 New

| File | Contents |
| --- | --- |
| `MiscHelpers/Common/M3Tokens.h/.cpp` | The token tables in §2/§3 as `struct M3Palette` with `light()` / `dark()` and a `seed()` overload for the memory palette. One source for colour, type, shape, density. |
| `SandMan/Windows/M3Menu.h/.cpp` | Searchable menu popup. Wraps a `QMenu`’s actions, or takes an action list, and renders the §5 pattern. Used by the menu bar and every context menu. |
| `SandMan/Windows/M3NavigationRail.h/.cpp` | The 80 px rail, group headers, indicator pill, destination signal. |
| `SandMan/Windows/M3TabStrip.h/.cpp` | Workspace tabs: order, pin, group, drag, context menu, persistence. |
| `SandMan/Windows/M3SearchField.h/.cpp` | 56 px / 48 px / 40 px search bar with clear button and the `.*` regex‑builder button. Every search in the app uses this one widget — that is how the “every search bar opens the builder” contract stays true. |
| `SandMan/Windows/RegexBuilderDialog.h/.cpp` | Full builder: token palette, pattern, flags (`i m s x U`), sample, live validation and capture preview, Apply / Keep plain text. Mirrors `docs/regex-builder.html` but on `QRegularExpression`. |
| `SandMan/Windows/SnackBar.h/.cpp` | M3 snackbar on `inverse-surface`, 4 s timeout, one action + dismiss. Replaces transient `statusBar()->showMessage`. |
| `SandMan/Views/MemorySyncView.*`, `SkillsView.*`, `MemoryInventoryView.*`, `OperationsView.*`, `StatusHubView.*` | The five memory destinations. Each reads the local repo checkout; none of them mutate a host. |

### 6.2 Changed

| File | Change |
| --- | --- |
| `MiscHelpers/Common/MaterialTheme.cpp` | Replace the ad‑hoc palette/stylesheet with the token tables. Drop‑in source in §7. |
| `SandMan/Windows/M3ShellHost.cpp` | Title bar grows to 64 px and absorbs the menu bar, search, and the language/theme/notification actions. `InstallDialog` gains the 28 px radius and 24 px padding. |
| `SandMan/SandMan.cpp` | `CreateMenus` / `CreateOldMenus` unchanged in content; the menus are shown through `M3Menu`. `CreateToolBar` is retired or moved behind the overflow (§10.1). `statusBar()` widgets become toggle buttons. Add rail + tab strip to the central layout. |
| `SandMan/Views/SbieView.cpp` | Context menus routed through `M3Menu`; add the per‑row `more_vert` trigger and the filter chips. |
| `SandMan/Windows/RecoveryWindow.cpp`, `SnapshotsWindow.cpp`, `TraceView.cpp`, `DocumentationBrowser.cpp` | Re‑host as destination widgets; keep every control and every string. Replace their local search fields with `M3SearchField`. |
| `SandMan/Windows/NotificationCenter.cpp` | Same actions (Dismiss selected / Dismiss all / Clear history / Export JSON / Export Markdown), M3 sheet layout, `M3SearchField`. |
| `SandMan/Windows/AppearanceEditorDialog.cpp` | Same fields (family, size, weight, style, underline, strikeout, overline, capitalization, letter/word spacing, accent, text colour, highlight, preview, unsupported disclosure). Layout becomes the M3 dialog; the live preview updates on every change. |
| `SandMan/Windows/ColorTranslatorDialog.cpp` | Same four rows (HEX/HEX8, RGB/RGBA, HSL/HSLA, Contrast) and the same validation strings. |
| `SandMan/Windows/DestructiveConfirmationDialog.cpp` | Same two‑key + full‑range gate and the same state strings; restyled. |
| `Forms/SettingsWindow.ui`, `Forms/OptionsWindow.ui` | These two Designer forms are the tracked migration boundary in `docs/features/ui/m3-shell-boundary.md`. Replace with the two‑pane M3 layouts; keep every widget’s object name so existing slots keep binding. |

---

## 7. Drop‑in `MaterialTheme.cpp`

Replaces the current file. Same public surface — `MaterialTheme::Apply(app, dark, accentSeed, density)` — so `CSandMan::SetUITheme()` needs no change.

```cpp
#include "MaterialTheme.h"
#include <QPalette>

namespace {

struct Roles {
    QString surface, surfaceLow, surfaceCont, surfaceHigh, surfaceHighest;
    QString onSurface, onSurfaceVar, outline, outlineVar;
    QString primary, onPrimary, primaryCont, onPrimaryCont;
    QString secondaryCont, onSecondaryCont;
    QString error, errorCont, onErrorCont;
    QString inverseSurface, inverseOnSurface;
};

Roles RolesFor(bool dark)
{
    if (dark) return {
        "#141218", "#1D1B20", "#211F26", "#2B2930", "#36343B",
        "#E6E0E9", "#CAC4D0", "#938F99", "#49454F",
        "#D0BCFF", "#381E72", "#4F378B", "#EADDFF",
        "#4A4458", "#E8DEF8",
        "#F2B8B5", "#8C1D18", "#F9DEDC",
        "#E6E0E9", "#322F35" };
    return {
        "#FEF7FF", "#F7F2FA", "#F3EDF7", "#ECE6F0", "#E6E0E9",
        "#1D1B20", "#49454F", "#79747E", "#CAC4D0",
        "#6750A4", "#FFFFFF", "#EADDFF", "#21005D",
        "#E8DEF8", "#1D192B",
        "#B3261E", "#F9DEDC", "#410E0B",
        "#322F35", "#F5EFF7" };
}

QPalette BuildPalette(bool dark, const QColor& accentSeed)
{
    const Roles r = RolesFor(dark);
    const QColor primary = accentSeed.isValid() ? accentSeed : QColor(r.primary);
    const QColor onPrimary = primary.lightnessF() > 0.62 ? QColor("#1D1B20") : QColor("#FFFFFF");

    QPalette p;
    p.setColor(QPalette::Window,          QColor(r.surface));
    p.setColor(QPalette::WindowText,      QColor(r.onSurface));
    p.setColor(QPalette::Base,            QColor(r.surface));
    p.setColor(QPalette::AlternateBase,   QColor(r.surfaceCont));
    p.setColor(QPalette::Text,            QColor(r.onSurface));
    p.setColor(QPalette::Button,          QColor(r.surfaceCont));
    p.setColor(QPalette::ButtonText,      QColor(r.onSurface));
    p.setColor(QPalette::Highlight,       primary);
    p.setColor(QPalette::HighlightedText, onPrimary);
    p.setColor(QPalette::Link,            primary);
    p.setColor(QPalette::PlaceholderText, QColor(r.onSurfaceVar));
    p.setColor(QPalette::Mid,             QColor(r.outlineVar));
    p.setColor(QPalette::ToolTipBase,     QColor(r.inverseSurface));
    p.setColor(QPalette::ToolTipText,     QColor(r.inverseOnSurface));
    return p;
}

// density: -4 compact, 0 comfortable, +4 spacious
QString BuildStyleSheet(bool dark, const QColor& accentSeed, int density)
{
    const Roles r = RolesFor(dark);
    const QColor accent = accentSeed.isValid() ? accentSeed : QColor(r.primary);
    const QString primary = accent.name();
    const QString onPrimary = accent.lightnessF() > 0.62 ? "#1D1B20" : "#FFFFFF";

    const int row     = density <= -4 ? 40 : density >= 4 ? 56 : 48;
    const int control = density <= -4 ? 36 : density >= 4 ? 48 : 40;
    const int padV    = (control - 20) / 2;

    return QString::fromUtf8(R"(
        QWidget { color: %(onSurface)s; font-family: "Roboto", "Segoe UI"; font-size: 10pt; }
        QMainWindow, QDialog, QWizard { background: %(surface)s; }

        /* Top app bar and menus */
        QMenuBar { background: %(surface)s; padding: 4px 8px; }
        QMenuBar::item { padding: 10px 12px; border-radius: 20px; }
        QMenuBar::item:selected { background: %(stateLayer)s; }
        QMenu { background: %(surfaceCont)s; border: 0; border-radius: 12px; padding: 8px 0; }
        QMenu::item { min-height: 48px; padding: 0 24px 0 12px; border-radius: 0; }
        QMenu::item:selected { background: %(stateLayer)s; color: %(onSurface)s; }
        QMenu::separator { height: 1px; background: %(outlineVar)s; margin: 8px 0; }
        QMenu::icon { padding-left: 12px; }

        /* Buttons — filled / tonal / outlined / text */
        QPushButton { min-height: %(control)dpx; padding: 0 24px; border-radius: %(controlR)dpx;
                      border: 1px solid %(outline)s; background: transparent; color: %(onSurface)s;
                      font-weight: 500; }
        QPushButton:hover { background: %(stateLayer)s; }
        QPushButton:pressed { background: %(statePressed)s; }
        QPushButton:disabled { color: %(onSurfaceVar)s; border-color: %(outlineVar)s; }
        QPushButton[m3="filled"] { background: %(primary)s; color: %(onPrimary)s; border: 0; }
        QPushButton[m3="tonal"] { background: %(secondaryCont)s; color: %(onSecondaryCont)s; border: 0; }
        QPushButton[m3="danger"] { background: %(errorCont)s; color: %(onErrorCont)s; border: 0; }
        QPushButton[m3="text"] { border: 0; color: %(primary)s; padding: 0 12px; }
        QToolButton { min-width: %(control)dpx; min-height: %(control)dpx;
                      border: 0; border-radius: %(controlR)dpx; }
        QToolButton:hover { background: %(stateLayer)s; }

        /* Fields */
        QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            min-height: %(control)dpx; background: %(surface)s; border: 1px solid %(outline)s;
            border-radius: 4px; padding: 0 12px; selection-background-color: %(primary)s;
            selection-color: %(onPrimary)s; }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus,
        QSpinBox:focus, QDoubleSpinBox:focus { border: 2px solid %(primary)s; padding: 0 11px; }
        QLineEdit[m3="search"] { min-height: 56px; border: 0; border-radius: 28px;
                                 background: %(surfaceCont)s; padding: 0 16px; }

        /* Tabs */
        QTabWidget::pane { border: 0; border-top: 1px solid %(outlineVar)s; background: %(surface)s; }
        QTabBar::tab { background: transparent; color: %(onSurfaceVar)s; min-height: 48px;
                       padding: 0 24px; border: 0; font-weight: 500; }
        QTabBar::tab:selected { color: %(primary)s; border-bottom: 3px solid %(primary)s; }
        QTabBar::tab:hover { background: %(stateLayer)s; }

        /* Lists, trees, tables */
        QTreeView, QListView, QTableView {
            background: %(surfaceCont)s; border: 0; border-radius: 12px;
            selection-background-color: %(secondaryCont)s; selection-color: %(onSecondaryCont)s;
            alternate-background-color: %(surfaceHigh)s; }
        QTreeView::item, QListView::item, QTableView::item { min-height: %(row)dpx; }
        QTreeView::item:hover, QListView::item:hover { background: %(stateLayer)s; }
        QHeaderView::section { background: %(surfaceCont)s; color: %(onSurfaceVar)s; border: 0;
                               border-bottom: 1px solid %(outlineVar)s; padding: 14px 16px;
                               font-size: 9pt; font-weight: 500; }

        /* Containers */
        QGroupBox { border: 0; border-radius: 12px; background: %(surfaceCont)s;
                    margin-top: 16px; padding: 16px; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 4px;
                           color: %(onSurface)s; font-weight: 500; }
        QToolBar { background: %(surface)s; border: 0; spacing: 8px; padding: 8px; }
        QStatusBar { background: %(surfaceCont)s; min-height: 40px; color: %(onSurfaceVar)s; }
        QStatusBar::item { border: 0; }

        /* Selection controls */
        QCheckBox, QRadioButton { spacing: 12px; min-height: %(control)dpx; }
        QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid %(outline)s;
                               border-radius: 2px; }
        QCheckBox::indicator:checked { background: %(primary)s; border-color: %(primary)s; }
        QSlider::groove:horizontal { height: 4px; background: %(surfaceHighest)s; border-radius: 2px; }
        QSlider::handle:horizontal { width: 20px; height: 20px; margin: -8px 0;
                                     border-radius: 10px; background: %(primary)s; }
        QProgressBar { border: 0; border-radius: 2px; background: %(surfaceHighest)s;
                       height: 4px; text-align: center; }
        QProgressBar::chunk { background: %(primary)s; border-radius: 2px; }

        /* Scrollbars */
        QScrollBar:vertical { background: transparent; width: 12px; margin: 4px; }
        QScrollBar::handle:vertical { background: %(outline)s; min-height: 40px; border-radius: 4px; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }

        /* Focus — never remove this */
        *:focus { outline: 3px solid %(primary)s; outline-offset: 2px; }
    )")
    .replace("%(surface)s",          r.surface)
    .replace("%(surfaceLow)s",       r.surfaceLow)
    .replace("%(surfaceCont)s",      r.surfaceCont)
    .replace("%(surfaceHigh)s",      r.surfaceHigh)
    .replace("%(surfaceHighest)s",   r.surfaceHighest)
    .replace("%(onSurface)s",        r.onSurface)
    .replace("%(onSurfaceVar)s",     r.onSurfaceVar)
    .replace("%(outline)s",          r.outline)
    .replace("%(outlineVar)s",       r.outlineVar)
    .replace("%(secondaryCont)s",    r.secondaryCont)
    .replace("%(onSecondaryCont)s",  r.onSecondaryCont)
    .replace("%(errorCont)s",        r.errorCont)
    .replace("%(onErrorCont)s",      r.onErrorCont)
    .replace("%(primary)s",          primary)
    .replace("%(onPrimary)s",        onPrimary)
    .replace("%(stateLayer)s",       dark ? "rgba(230,224,233,0.08)" : "rgba(29,27,32,0.08)")
    .replace("%(statePressed)s",     dark ? "rgba(230,224,233,0.12)" : "rgba(29,27,32,0.12)")
    .replace("%(row)d",              QString::number(row))
    .replace("%(control)d",          QString::number(control))
    .replace("%(controlR)d",         QString::number(control / 2))
    .replace("%(padV)d",             QString::number(padV));
}

}

namespace MaterialTheme {

void Apply(QApplication* app, bool dark, const QColor& accentSeed, int density)
{
    if (!app) return;
    app->setPalette(BuildPalette(dark, accentSeed));
    app->setStyleSheet(BuildStyleSheet(dark, accentSeed, density));
}

}
```

Notes for whoever lands this:

* `QString::replace` chains are used instead of `.arg()` because the sheet has far more than nine substitutions and positional args become unreadable. If you prefer, swap in a `QHash<QString,QString>` loop.
* Qt style sheets have no `box-shadow`. Elevation is carried by `QGraphicsDropShadowEffect` on menus, dialogs and the snackbar — set it once in `M3Menu`, `M3DialogHost` and `SnackBar` rather than per call site.
* Qt has no `:hover` state layer over an image background; the state layer is a solid `rgba`, which is what M3 specifies anyway.
* The `m3` dynamic property drives button variants. After setting it at runtime you must call `style()->unpolish(w); style()->polish(w);`.

---

## 8. Keyboard and focus

| Screen | Tab order | Shortcuts |
| --- | --- | --- |
| Shell | app icon → menu bar (S, V, O, H) → global search → language → theme → notifications → window controls → tab strip → rail → content → status bar | `Ctrl+Shift+F` palette · `F5` refresh · `Ctrl+D` file panel · `Ctrl+Shift+R` reset GUI · `Alt+S/V/O/H` menus |
| Menus (all) | search field is focused on open → ↓ moves into the list → Enter activates → Esc closes and returns focus to the trigger | typing filters; Home/End jump the list |
| Boxes | filter chips → search → regex button → table (arrow keys walk the tree, ←/→ collapse/expand) → selection bar | `Menu` key or `Shift+F10` opens the context menu on the focused row; `Shift+RMB` opens the appearance editor |
| Tab strip | tabs left→right, close button inside each tab | `Ctrl+Tab` next tab · `Ctrl+W` close · `Ctrl+Shift+T` reopen |
| Dialogs | title → content in reading order → actions right‑to‑left, primary last | Esc = Cancel / Emergency exit; Enter = primary, except in the destructive gate where Enter never authorizes |
| Destructive gate | Press A → Press L → slider → Emergency exit → Authorize | slider is keyboard‑operable with ←/→; Authorize stays disabled until both keys and 100 % |
| Palette | search → results | ↑/↓ move, Enter teleports, live controls are reachable with Tab without leaving the row |

Focus visible everywhere: 3 px `primary` ring, 2 px offset. Focus returns to the invoking control when any dialog closes — this is already required by `docs/features/destructive-confirmation.md` and now applies to every overlay.

---

## 9. Accessibility and contrast

* Body text on `surface`: 13.9:1 dark, 15.8:1 light. `on-surface-variant` on `surface-container`: 7.0:1 dark, 8.1:1 light. Both clear AA and AAA for body copy.
* `primary` on `surface`: 8.9:1 dark, 7.4:1 light — safe for the 12 px rail labels and eyebrows.
* `on-primary` on `primary`: 9.6:1 light, 11.4:1 dark.
* `error` text on `surface` is only used at ≥14 px 500; the red **fill** pairs `error-container`/`on-error-container` (9.7:1 light, 8.4:1 dark).
* The one contrast risk found in the source is real and unfixed upstream: the Pages changelog tab measured 3.9:1 against `surface-container` (CI run 31304058732). The design uses `on-surface-variant` there, which measures 7.0:1. Fix the Pages CSS at the same time or the a11y contract stays red.
* Every icon‑only button has `toolTip` + `accessibleName` (the existing `M3ShellHost` pattern — keep it).
* Status colour is never the only signal: running processes are green **and** say “Running”; the failing CI card is red **and** carries an `error` icon **and** the words “the hui is red”.
* `prefers-reduced-motion` / the `Respect the system reduced-motion setting` toggle disables every transition, including the destructive‑gate progress animation, which the feature article already requires.
* Minimum target 40×40. Row chevrons and per‑row overflow buttons are 24 px glyphs inside 40 px hit areas.

---

## 10. Decisions you must make before implementing

1. **The toolbar.** The design removes the strip and redistributes its actions. `GetToolBarItemsConfig` / `CreateToolBarConfigMenu` let users customise it today, so removing it is a user‑visible regression for anyone who configured one. Options: keep the strip as an opt‑in row under the tab strip; or move the configurable set into an overflow menu on the app bar. The design assumes the second. **Not decided.**
2. **Icon family.** The design uses Material Symbols Outlined for UI glyphs while the repo ships ~190 bespoke PNGs under `Resources/Actions/`. Mixing them looks wrong. Either commission/derive an M3 set for the box‑type and action icons, or keep the PNGs everywhere and drop Symbols. **Not decided** — the design is currently mixed on purpose so both are visible side by side (app icon is the PNG; everything else is Symbols).
3. **The two Designer forms.** `SettingsWindow.ui` and `OptionsWindow.ui` are the tracked M3 migration boundary. The design replaces both layouts. That is a large mechanical change and should be its own phase with object names preserved.
4. **Where the memory features actually read from.** The five memory destinations are drawn against the local `agent-global-memory` checkout. SandMan has no such dependency today. Decide whether it shells out to `sync-agent-memory.ps1`, reads the repo directly, or talks to a local service. Nothing in the design assumes network access.

---

## 11. Migration order

| Phase | Ships | Risk |
| --- | --- | --- |
| 1 | `M3Tokens` + the new `MaterialTheme.cpp`. Nothing else changes. | Low. Pure restyle; every existing widget picks it up. Verify light/dark Settings and Options at 100 % and 200 %. |
| 2 | `M3SearchField` + `RegexBuilderDialog`, wired into every existing search (notifications, docs, changelog, settings). | Low. Satisfies the “every search bar opens the builder” contract on its own. |
| 3 | `M3Menu` behind every menu, including the four context menus. | Medium. Touches `SandMan.cpp` and `SbieView.cpp`; the actions themselves do not move. |
| 4 | 64 px app bar + `M3ShellHost` merge; status‑bar toggles; snackbar. | Medium. Frameless drag and `windowTitleChanged` need re‑testing on all three architectures. |
| 5 | Navigation rail + tab strip + destination hosting for Recovery, Trace, Snapshots, Docs. | High. These are windows today; hosting them as destinations changes lifetime and parenting. |
| 6 | Settings and Box Options two‑pane rebuild (the Designer‑form boundary). | High. Largest mechanical change; keep object names. |
| 7 | The five memory destinations. | Medium, but depends on decision 10.4. |

Each phase is independently shippable and independently revertible. Phases 1–3 alone deliver the pure‑M3 look and the searchable menus.

---

## 12. Known gaps — stated honestly

* **No native build was produced or run.** Everything here is a design against source. There is no evidence that a Qt/MSVC build of this shell compiles, that the driver loads under it, or that a runtime screenshot was captured. Do not cite this document as build evidence.
* **The design is HTML.** Qt style sheets cannot express every value in it — notably `box-shadow` (needs `QGraphicsDropShadowEffect`), `gap` (needs layout spacing), and per‑state layered backgrounds on images. The Qt result will be close but not pixel‑identical, and §7 already routes around the known cases.
* **Sample data is realistic but invented.** Box names, PIDs, trace lines, run numbers other than those quoted from the READMEs, and session ids are fixtures for the mock. The run/release numbers that *are* real (31304058726, 31304058732, 31301196079, 30929821528, v0.1.2810) come from the two READMEs.
* **Dim‑sum photography is a placeholder.** The public catalogue is documented as unavailable; the design shows the fail‑closed state and a striped placeholder rather than pretending an image resolved.
* **Status Hub is read‑only here.** Replies are simulated. The real inbox delivery, session keys and SSE projections live in `apps/status-hub/` and are not modelled.
* **Word‑depth typography is partial.** The appearance editor covers family, size, weight, style, decorations, capitalization, letter and word spacing. Kerning tables and OpenType stylistic sets are shown as explicitly unsupported, matching the current dialog’s disclosure.
* **Four tab searches are present but scoped to one window.** Cross‑window “every tab” search needs a real tab registry, which does not exist in SandMan yet.
* **Localisation.** The design ships English with Cantonese on the navigation, page titles and primary actions, plus the language switch. Full bilingual coverage of every string is a translation task, not a design task, and `Troubleshooting/lang_*.json` shows the existing catalogue does not cover these new surfaces yet.

---

## 13. Provenance

| Design surface | Read from |
| --- | --- |
| Shell, menus, toolbar, status bar | `SandboxiePlus/SandMan/SandMan.cpp` (`CreateMenus` 1002‑1160, `CreateOldMenus` 1159‑1270, `CreateToolBar` 1444‑1550, `statusBar` 582‑593) |
| Frameless title bar and dialog host | `SandMan/Windows/M3ShellHost.cpp` |
| Colour and stylesheet baseline | `MiscHelpers/Common/MaterialTheme.cpp`, `docs/material-design.md` |
| Box list columns and context menus | `SandMan/Models/SbieModel.cpp` (832‑843), `SandMan/Views/SbieView.cpp` (133‑472) |
| Recovery | `SandMan/Windows/RecoveryWindow.cpp` (455‑822) |
| Snapshots | `SandMan/Windows/SnapshotsWindow.cpp` (35‑296) |
| Trace | `SandMan/Views/TraceView.cpp` (250‑334), `SandMan/Models/TraceModel.cpp` (333‑337) |
| Documentation browser | `SandMan/Windows/DocumentationBrowser.cpp` (43‑299) |
| Notification centre | `SandMan/Windows/NotificationCenter.cpp` (45‑267) |
| Appearance editor | `SandMan/Windows/AppearanceEditorDialog.cpp` (30‑187) |
| Colour translator | `SandMan/Windows/ColorTranslatorDialog.cpp` (25‑145) |
| Destructive gate | `SandMan/Windows/DestructiveConfirmationDialog.cpp` (22‑165) |
| Settings pages and strings | `SandMan/Forms/SettingsWindow.ui` |
| Feature contracts (regex, tabs, palette, history, school mode, dim sum, scheduled) | `material-sandbox/README.md` feature inventory, `docs/features/*` |
| Memory sync, skills, memory files, ops, status hub | `agent-global-memory/README.md`, `memory/`, `skills/`, `apps/agent-global-memory/`, `apps/status-hub/`, `contracts/` |
| Memory Console visual reference | `agent-global-memory/apps/agent-global-memory/src/index.html`, `src/styles.css`, `DESIGN.md` |
