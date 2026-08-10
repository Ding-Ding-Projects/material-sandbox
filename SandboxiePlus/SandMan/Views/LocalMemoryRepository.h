#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

class CLocalMemoryRepository
{
public:
    enum Error {
        NoError,
        Unconfigured,
        RootNotFound,
        NotFound,
        AccessDenied,
        EscapedRoot,
        TooLarge,
        TooManyEntries,
        InvalidPath,
        ReadFailed
    };

    struct TextResult {
        Error error = NoError;
        QString text;
        QString absolutePath;
        QString message;
        bool ok() const { return error == NoError; }
    };

    struct Entry {
        QString name;
        QString relativePath;
        QString absolutePath;
        bool directory = false;
        qint64 size = 0;
        QDateTime modified;
    };

    struct ListResult {
        Error error = NoError;
        QList<Entry> entries;
        QString message;
        bool ok() const { return error == NoError; }
    };

    explicit CLocalMemoryRepository(const QString& root = QString());

    void setRoot(const QString& root);
    QString root() const;
    QString canonicalRoot() const;
    bool isConfigured() const;

    TextResult readText(const QString& relativePath, qint64 maximumBytes = 1024 * 1024) const;
    ListResult list(const QString& relativeDirectory = QString(),
                    const QStringList& nameFilters = QStringList(),
                    int maximumEntries = 2000) const;
    QString safeExistingPath(const QString& relativePath, Error* error = nullptr) const;

    static QString discoverDefaultRoot();
    static QString errorText(Error error);

private:
    QString m_root;
};
