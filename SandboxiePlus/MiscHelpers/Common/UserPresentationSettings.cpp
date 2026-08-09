#include "UserPresentationSettings.h"

#include "Settings.h"

namespace {
const char* languageKey = "Options/LanguageMode";
const char* funnyEnglishKey = "Options/FunnyLevelEnglish";
const char* funnyCantoneseKey = "Options/FunnyLevelCantonese";
const char* emojiKey = "Options/ShowDialogEmojis";

int clampFunny(int level)
{
    return qBound(1, level, 5);
}
}

namespace UserPresentationSettings {

LanguageMode languageMode(CSettings* settings)
{
    if (!settings)
        return LanguageMode::English;
    const QString value = settings->GetString(QString::fromLatin1(languageKey), QStringLiteral("english")).toLower();
    if (value == "cantonese" || value == "zh_hant" || value == "zh-tw")
        return LanguageMode::Cantonese;
    if (value == "bilingual" || value == "both")
        return LanguageMode::Bilingual;
    return LanguageMode::English;
}

void setLanguageMode(CSettings* settings, LanguageMode mode)
{
    if (!settings)
        return;
    const char* value = mode == LanguageMode::Cantonese ? "cantonese" : mode == LanguageMode::Bilingual ? "bilingual" : "english";
    settings->SetValue(QString::fromLatin1(languageKey), value);
}

int funnyEnglish(CSettings* settings)
{
    return settings ? clampFunny(settings->GetInt(CSettings::SStrRef(funnyEnglishKey), 1)) : 1;
}

void setFunnyEnglish(CSettings* settings, int level)
{
    if (settings)
        settings->SetValue(QString::fromLatin1(funnyEnglishKey), clampFunny(level));
}

int funnyCantonese(CSettings* settings)
{
    return settings ? clampFunny(settings->GetInt(CSettings::SStrRef(funnyCantoneseKey), 1)) : 1;
}

void setFunnyCantonese(CSettings* settings, int level)
{
    if (settings)
        settings->SetValue(QString::fromLatin1(funnyCantoneseKey), clampFunny(level));
}

bool showDialogEmojis(CSettings* settings)
{
    return settings ? settings->GetBool(CSettings::SStrRef(emojiKey), true) : true;
}

void setShowDialogEmojis(CSettings* settings, bool enabled)
{
    if (settings)
        settings->SetValue(QString::fromLatin1(emojiKey), enabled);
}

QString formatMessage(CSettings* settings, const QString& english, const QString& cantonese)
{
    const LanguageMode mode = languageMode(settings);
    const QString zh = cantonese.isEmpty() ? english : cantonese;
    const int level = mode == LanguageMode::Cantonese ? funnyCantonese(settings) : funnyEnglish(settings);
    QString styledEnglish = english;
    QString styledCantonese = zh;
    if (level >= 4 && showDialogEmojis(settings)) {
        styledEnglish.prepend(QStringLiteral("✨ "));
        styledCantonese.prepend(QStringLiteral("✨ "));
    }
    if (level == 5) {
        styledEnglish.append(QStringLiteral(" (The facts remain firmly supervised.)"));
        styledCantonese.append(QStringLiteral("（資料照樣由沙盒睇實，放心。）"));
    }
    if (mode == LanguageMode::Cantonese)
        return styledCantonese;
    if (mode == LanguageMode::Bilingual)
        return styledEnglish + QStringLiteral("\n") + styledCantonese;
    return styledEnglish;
}

}
