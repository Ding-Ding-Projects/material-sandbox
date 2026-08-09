# Material Design 3 desktop shell

## Behavior

`SandboxiePlus/MiscHelpers/Common/MaterialTheme.*` defines the shared Material 3 baseline. `CSandMan::SetUITheme()` keeps the existing persisted light/dark choice, applies Material surface and primary roles, and then restores the custom tab/menu proxy style.

The stylesheet covers the desktop shell's common controls: menus, buttons, fields, tabs, tree/list/table views, group boxes, toolbars, status bars, focus states, and scrollbars. It uses a continuous palette rather than platform-specific colors so dialogs inherit the same roles.

## Configuration

The existing `Options/UseDarkTheme`, `Options/UseFusionTheme`, `Options/UIFont`, and `Options/FontScaling` values remain the source of truth. Material styling is applied at runtime and does not move the application's data directory or package identity.

## Failure modes and security

If the Qt Fusion style cannot be created, Qt falls back to its normal style object; the application remains usable. No network resources, fonts, analytics, or credentials are introduced by the theme service.

## Verification

Run `git diff --check`, configure `SandboxiePlus/SandMan/SandMan-Qt6.qc.pro` with the repository Qt toolchain, build SandMan, and inspect light/dark Settings and Options windows at 100% and 200% scale. A missing `qmake` executable is an environment blocker, not a green build result.

## Suggested articles

- [Contributor capability mode](contributor-build.md)
