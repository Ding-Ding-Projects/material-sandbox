#pragma once

#include <QString>

class CSettings;

namespace UserPresentationSettings {

enum class LanguageMode {
    English,
    Cantonese,
    Bilingual
};

LanguageMode languageMode(CSettings* settings);
void setLanguageMode(CSettings* settings, LanguageMode mode);

int funnyEnglish(CSettings* settings);
void setFunnyEnglish(CSettings* settings, int level);
int funnyCantonese(CSettings* settings);
void setFunnyCantonese(CSettings* settings, int level);

bool showDialogEmojis(CSettings* settings);
void setShowDialogEmojis(CSettings* settings, bool enabled);

QString formatMessage(CSettings* settings, const QString& english, const QString& cantonese = QString());

}
