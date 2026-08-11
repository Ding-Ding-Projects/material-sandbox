#pragma once

#include <QDateTime>
#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
#include <functional>
#endif

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
        ReadFailed,
        InvalidLimit,
        RemoteRoot,
        ReparsePoint,
        InvalidRepository
    };

    struct TextResult {
        Error error = NoError;
        QString text;
        QString absolutePath;
        QString message;
        qint64 size = 0;
        QDateTime modified;
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
    ~CLocalMemoryRepository();
    CLocalMemoryRepository(const CLocalMemoryRepository&) = delete;
    CLocalMemoryRepository& operator=(const CLocalMemoryRepository&) = delete;

    void setRoot(const QString& root);
    QString root() const;
    QString canonicalRoot() const;
    bool isConfigured() const;

    TextResult readText(const QString& relativePath, qint64 maximumBytes = 1024 * 1024) const;
    ListResult list(const QString& relativeDirectory = QString(),
                    const QStringList& nameFilters = QStringList(),
                    int maximumEntries = 2000) const;
    // Returns display metadata only. Callers must never reopen this path as a trusted capability.
    QString safeExistingPath(const QString& relativePath, Error* error = nullptr) const;

    static QString discoverDefaultRoot();
    static QString errorText(Error error);

#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
    // Runs after a file handle has been securely opened and before bytes are read.
    // Tests use this to prove a pathname replacement cannot redirect the read.
    static void setTestBeforeCandidateOpenHook(const std::function<void()>& hook);
    static void setTestAfterDirectoryOpenHook(const std::function<void()>& hook);
    static void setTestAfterOpenHook(const std::function<void()>& hook);
    static void setTestEnumerationHook(const std::function<void(int)>& hook);
#endif

private:
    void configureRoot(const QString& root);
    QString m_root;
    QString m_canonicalRoot;
    quint64 m_rootVolumeSerial = 0;
    QByteArray m_rootFileId;
    Error m_configurationError = Unconfigured;
    quintptr m_rootHandle = 0;
};
