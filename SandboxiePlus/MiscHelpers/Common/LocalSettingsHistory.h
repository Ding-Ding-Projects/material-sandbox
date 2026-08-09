#pragma once

#include <QDateTime>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QVector>

#include "../mischelpers_global.h"

class CSettings;
class QJsonObject;

// Append-only, local settings history. The history file lives beside the
// profile, never inside the user's project and never leaves the machine.
class MISCHELPERS_EXPORT CLocalSettingsHistory
{
public:
    struct Entry
    {
        QString id;
        QDateTime timestamp;
        QString key;
        QString action;
        bool hadBefore = false;
        QVariant before;
        bool hadAfter = false;
        QVariant after;
    };

    explicit CLocalSettingsHistory(const QString& filePath, int maxEntries = 500);

    void record(const QString& key, bool hadBefore, const QVariant& before,
        bool hadAfter, const QVariant& after, const QString& action = QStringLiteral("settings changed"));
    QVector<Entry> entries() const;
    bool restore(const QString& id, CSettings* settings, QString* error = nullptr) const;
    QString filePath() const { return m_filePath; }

private:
    void load();
    bool write() const;
    static QJsonObject toJson(const Entry& entry);
    static Entry fromJson(const QJsonObject& object);

    QString m_filePath;
    int m_maxEntries;
    mutable QMutex m_mutex;
    QVector<Entry> m_entries;
};
