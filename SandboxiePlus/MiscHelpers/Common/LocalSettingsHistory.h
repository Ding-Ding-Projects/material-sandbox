#pragma once

#include <QDateTime>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

#include "../mischelpers_global.h"

class CSettings;

// Append-only settings revisions stored in an isolated local Git repository.
// The repository lives beside the application profile, never in a user's own
// project, and is deliberately kept without a remote.
class MISCHELPERS_EXPORT CLocalSettingsHistory
{
public:
    struct Entry
    {
        QString id;
        QDateTime timestamp;
        QString key;
        QString action;
        bool isSnapshot = false;
    };

    explicit CLocalSettingsHistory(const QString& repositoryPath,
        const QString& legacyJsonlPath = QString(), int maxEntries = 500);

    bool initialize(CSettings* settings, QString* error = nullptr);
    void record(CSettings* settings, const QString& key, bool hadBefore,
        const QVariant& before, bool hadAfter, const QVariant& after,
        const QString& action = QStringLiteral("settings changed"));
    bool checkpoint(CSettings* settings, QString* id = nullptr,
        QString* error = nullptr,
        const QString& action = QStringLiteral("settings checkpoint"));
    QVector<Entry> entries() const;
    bool restore(const QString& id, CSettings* settings, QString* error = nullptr);
    bool exportBundle(const QString& path, QString* error = nullptr) const;

    QString repositoryPath() const { return m_repositoryPath; }
    QString gitExecutable() const { return m_gitExecutable; }
    bool isAvailable() const;
    QString lastError() const;

private:
    bool ensureRepository(QString* error);
    bool archiveLegacyHistory(QString* error);
    bool commitCurrentSnapshot(CSettings* settings, const QString& action,
        const QString& key, bool isSnapshot, QString* id, QString* error);
    bool writeSnapshot(const QVariantMap& values, QString* error) const;
    bool readSnapshot(const QString& id, QVariantMap* values, QString* error) const;
    bool loadEntries(QString* error);
    bool runGit(const QStringList& arguments, QByteArray* standardOutput,
        QString* error, int timeoutMs = 10000) const;
    QString m_repositoryPath;
    QString m_legacyJsonlPath;
    QString m_gitExecutable;
    int m_maxEntries;
    mutable QMutex m_mutex;
    QVector<Entry> m_entries;
    bool m_available = false;
    QString m_lastError;
};
