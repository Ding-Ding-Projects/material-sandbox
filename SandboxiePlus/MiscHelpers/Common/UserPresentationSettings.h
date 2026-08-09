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

// School mode is a shared, user-experience mode: it forces English copy and
// temporarily omits playful presentation choices without changing identity or
// deleting the user's saved preferences.
bool schoolModeEnabled(CSettings* settings);
void setSchoolModeEnabled(CSettings* settings, bool enabled);
QString schoolModeName(CSettings* settings);
void setSchoolModeName(CSettings* settings, const QString& name);
void resetSchoolModeName(CSettings* settings);

QString formatMessage(CSettings* settings, const QString& english, const QString& cantonese = QString());

}
