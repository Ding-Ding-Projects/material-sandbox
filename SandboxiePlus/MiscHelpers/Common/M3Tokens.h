#pragma once

#include <QColor>

// Canonical Material 3 design tokens for the native SandMan surface.  Keep
// these values independent from widget construction so every shell, page and
// overlay consumes the same roles and density measurements.
namespace M3Tokens {

struct Colors
{
    QColor surface;
    QColor surfaceContainerLowest;
    QColor surfaceContainerLow;
    QColor surfaceContainer;
    QColor surfaceContainerHigh;
    QColor surfaceContainerHighest;
    QColor onSurface;
    QColor onSurfaceVariant;
    QColor outline;
    QColor outlineVariant;

    QColor primary;
    QColor onPrimary;
    QColor primaryContainer;
    QColor onPrimaryContainer;
    QColor secondary;
    QColor secondaryContainer;
    QColor onSecondaryContainer;
    QColor tertiary;
    QColor tertiaryContainer;
    QColor onTertiaryContainer;

    QColor error;
    QColor errorContainer;
    QColor onErrorContainer;
    QColor inverseSurface;
    QColor inverseOnSurface;

    QColor memory;
    QColor onMemory;
    QColor memoryContainer;
    QColor onMemoryContainer;
    QColor ok;
    QColor okContainer;
    QColor onOkContainer;
    QColor warningContainer;
    QColor onWarningContainer;

    QColor stateLayer;
    QColor stateLayerStrong;
    QColor scrim;
};

struct Density
{
    enum Level {
        Compact,
        Comfortable,
        Spacious
    };

    Level level;
    int rowHeight;
    int controlHeight;
    int pagePadding;
    int gap;
};

struct Shape
{
    enum {
        ExtraSmall = 4,
        Small = 8,
        Medium = 12,
        Large = 16,
        ExtraLarge = 28,
        Full = 999
    };
};

// accentSeed is a user-selected seed.  The canonical #6750A4 seed resolves
// to the exact light/dark design roles instead of being reused verbatim in
// both themes.
Colors colors(bool dark, const QColor& accentSeed = QColor());

// Existing profiles persist density as 0/1/2.  The design specification also
// describes the same levels as -4/0/+4 offsets, so both encodings are explicit
// rather than overloading the ambiguous value 0.
Density densityFromSettingIndex(int index);
Density densityFromOffset(int offset);

} // namespace M3Tokens
