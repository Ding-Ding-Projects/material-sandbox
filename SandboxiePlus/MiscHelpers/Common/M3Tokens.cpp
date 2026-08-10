#include "M3Tokens.h"

namespace {

QColor readableForeground(const QColor& background)
{
    return background.lightnessF() > 0.62
        ? QColor(QStringLiteral("#1D1B20"))
        : QColor(QStringLiteral("#FFFFFF"));
}

M3Tokens::Density densityForLevel(M3Tokens::Density::Level level)
{
    switch (level) {
    case M3Tokens::Density::Compact:
        return { level, 40, 36, 16, 8 };
    case M3Tokens::Density::Spacious:
        return { level, 56, 48, 32, 24 };
    case M3Tokens::Density::Comfortable:
    default:
        return { M3Tokens::Density::Comfortable, 48, 40, 24, 16 };
    }
}

} // namespace

namespace M3Tokens {

Colors colors(bool dark, const QColor& accentSeed)
{
    Colors r;
    if (dark) {
        r.surface = QColor(QStringLiteral("#141218"));
        r.surfaceContainerLowest = QColor(QStringLiteral("#0F0D13"));
        r.surfaceContainerLow = QColor(QStringLiteral("#1D1B20"));
        r.surfaceContainer = QColor(QStringLiteral("#211F26"));
        r.surfaceContainerHigh = QColor(QStringLiteral("#2B2930"));
        r.surfaceContainerHighest = QColor(QStringLiteral("#36343B"));
        r.onSurface = QColor(QStringLiteral("#E6E0E9"));
        r.onSurfaceVariant = QColor(QStringLiteral("#CAC4D0"));
        r.outline = QColor(QStringLiteral("#938F99"));
        r.outlineVariant = QColor(QStringLiteral("#49454F"));
        r.primary = QColor(QStringLiteral("#D0BCFF"));
        r.onPrimary = QColor(QStringLiteral("#381E72"));
        r.primaryContainer = QColor(QStringLiteral("#4F378B"));
        r.onPrimaryContainer = QColor(QStringLiteral("#EADDFF"));
        r.secondary = QColor(QStringLiteral("#CCC2DC"));
        r.secondaryContainer = QColor(QStringLiteral("#4A4458"));
        r.onSecondaryContainer = QColor(QStringLiteral("#E8DEF8"));
        r.tertiary = QColor(QStringLiteral("#EFB8C8"));
        r.tertiaryContainer = QColor(QStringLiteral("#633B48"));
        r.onTertiaryContainer = QColor(QStringLiteral("#FFD8E4"));
        r.error = QColor(QStringLiteral("#F2B8B5"));
        r.errorContainer = QColor(QStringLiteral("#8C1D18"));
        r.onErrorContainer = QColor(QStringLiteral("#F9DEDC"));
        r.inverseSurface = QColor(QStringLiteral("#E6E0E9"));
        r.inverseOnSurface = QColor(QStringLiteral("#322F35"));
        r.memory = QColor(QStringLiteral("#82D5CC"));
        r.onMemory = QColor(QStringLiteral("#003733"));
        r.memoryContainer = QColor(QStringLiteral("#00504A"));
        r.onMemoryContainer = QColor(QStringLiteral("#9EF2E6"));
        r.ok = QColor(QStringLiteral("#B7F397"));
        r.okContainer = QColor(QStringLiteral("#254D14"));
        r.onOkContainer = QColor(QStringLiteral("#CFF7B4"));
        r.warningContainer = QColor(QStringLiteral("#5C3D00"));
        r.onWarningContainer = QColor(QStringLiteral("#FFDDB0"));
        r.stateLayer = QColor(230, 224, 233, 20);
        r.stateLayerStrong = QColor(230, 224, 233, 31);
        r.scrim = QColor(0, 0, 0, 140);
    } else {
        r.surface = QColor(QStringLiteral("#FEF7FF"));
        r.surfaceContainerLowest = QColor(QStringLiteral("#FFFFFF"));
        r.surfaceContainerLow = QColor(QStringLiteral("#F7F2FA"));
        r.surfaceContainer = QColor(QStringLiteral("#F3EDF7"));
        r.surfaceContainerHigh = QColor(QStringLiteral("#ECE6F0"));
        r.surfaceContainerHighest = QColor(QStringLiteral("#E6E0E9"));
        r.onSurface = QColor(QStringLiteral("#1D1B20"));
        r.onSurfaceVariant = QColor(QStringLiteral("#49454F"));
        r.outline = QColor(QStringLiteral("#79747E"));
        r.outlineVariant = QColor(QStringLiteral("#CAC4D0"));
        r.primary = QColor(QStringLiteral("#6750A4"));
        r.onPrimary = QColor(QStringLiteral("#FFFFFF"));
        r.primaryContainer = QColor(QStringLiteral("#EADDFF"));
        r.onPrimaryContainer = QColor(QStringLiteral("#21005D"));
        r.secondary = QColor(QStringLiteral("#625B71"));
        r.secondaryContainer = QColor(QStringLiteral("#E8DEF8"));
        r.onSecondaryContainer = QColor(QStringLiteral("#1D192B"));
        r.tertiary = QColor(QStringLiteral("#7D5260"));
        r.tertiaryContainer = QColor(QStringLiteral("#FFD8E4"));
        r.onTertiaryContainer = QColor(QStringLiteral("#31111D"));
        r.error = QColor(QStringLiteral("#B3261E"));
        r.errorContainer = QColor(QStringLiteral("#F9DEDC"));
        r.onErrorContainer = QColor(QStringLiteral("#410E0B"));
        r.inverseSurface = QColor(QStringLiteral("#322F35"));
        r.inverseOnSurface = QColor(QStringLiteral("#F5EFF7"));
        r.memory = QColor(QStringLiteral("#006A63"));
        r.onMemory = QColor(QStringLiteral("#FFFFFF"));
        r.memoryContainer = QColor(QStringLiteral("#9EF2E6"));
        r.onMemoryContainer = QColor(QStringLiteral("#00201D"));
        r.ok = QColor(QStringLiteral("#2E6B12"));
        r.okContainer = QColor(QStringLiteral("#D7F5C4"));
        r.onOkContainer = QColor(QStringLiteral("#0B2000"));
        r.warningContainer = QColor(QStringLiteral("#FFE7C2"));
        r.onWarningContainer = QColor(QStringLiteral("#2A1800"));
        r.stateLayer = QColor(29, 27, 32, 20);
        r.stateLayerStrong = QColor(29, 27, 32, 31);
        r.scrim = QColor(0, 0, 0, 82);
    }

    const QColor canonicalSeed(QStringLiteral("#6750A4"));
    if (accentSeed.isValid() && accentSeed.rgb() != canonicalSeed.rgb()) {
        r.primary = accentSeed;
        r.onPrimary = readableForeground(accentSeed);
    }
    return r;
}

Density densityFromSettingIndex(int index)
{
    if (index <= 0)
        return densityForLevel(Density::Compact);
    if (index >= 2)
        return densityForLevel(Density::Spacious);
    return densityForLevel(Density::Comfortable);
}

Density densityFromOffset(int offset)
{
    if (offset <= -4)
        return densityForLevel(Density::Compact);
    if (offset >= 4)
        return densityForLevel(Density::Spacious);
    return densityForLevel(Density::Comfortable);
}

} // namespace M3Tokens
