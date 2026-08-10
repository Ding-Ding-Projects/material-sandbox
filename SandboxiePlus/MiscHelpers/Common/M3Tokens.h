#pragma once

#include <QColor>

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
    enum Level { Compact, Comfortable, Spacious };
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

struct FixedGeometry
{
    enum {
        AppBarHeight = 64,
        TabStripHeight = 48,
        NavigationRailWidth = 80,
        RailIndicatorWidth = 56,
        RailIndicatorHeight = 32,
        StatusBarHeight = 40,
        MenuRowHeight = 48,
        SearchHeight = 56,
        MenuSearchHeight = 40,
        MinimumTarget = 40,
        DialogPadding = 24,
        ContentRadius = 16
    };
};

Colors colors(bool dark, const QColor& accentSeed = QColor());
Density densityFromSettingIndex(int index);
Density densityFromOffset(int offset);

} // namespace M3Tokens
