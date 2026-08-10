#include "stdafx.h"
#include "LocalSettingsHistory.h"

#include "Settings.h"
#include <algorithm>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
constexpr quint32 kSnapshotMagic = 0x53424831; // SBH1
constexpr int kSnapshotSchemaVersion = 1;
constexpr int kMaxSnapshotBytes = 1024 * 1024;
constexpr int kMaxSnapshotKeys = 10000;
constexpr qint64 kMaxLegacyArchiveBytes = 64 * 1024 * 1024;
const char kSnapshotFileName[] = "settings.snapshot";
const char kLegacyArchiveName[] = "legacy/settings-history.jsonl";

QString EncodeMetadata(const QString& value)
{
    return QString::fromLatin1(value.toUtf8().toBase64());
}

QString DecodeMetadata(const QString& value)
{
    return QString::fromUtf8(QByteArray::fromBase64(value.toLatin1()));
}

bool SnapshotToBytes(const QVariantMap& values, QByteArray* encoded, QString* error)
{
    if (values.size() > kMaxSnapshotKeys) {
        if (error)
            *error = QStringLiteral("The settings revision has too many keys");
        return false;
    }
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        if (it.key().isEmpty() || it.key().size() > 4096 ||
            it.key().startsWith(QStringLiteral("History/"))) {
            if (error)
                *error = QStringLiteral("The settings revision contains an invalid key");
            return false;
        }
    }

    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << kSnapshotMagic << qint32(kSnapshotSchemaVersion) << values;
    if (stream.status() != QDataStream::Ok || bytes.size() > kMaxSnapshotBytes) {
        if (error)
            *error = QStringLiteral("The settings revision exceeds the local size limit");
        return false;
    }
    if (encoded)
        *encoded = bytes;
    return true;
}

bool SnapshotFromBytes(const QByteArray& bytes, QVariantMap* values, QString* error)
{
    if (bytes.isEmpty() || bytes.size() > kMaxSnapshotBytes) {
        if (error)
            *error = QStringLiteral("The selected Git revision has an invalid snapshot size");
        return false;
    }
    QDataStream stream(bytes);
    stream.setVersion(QDataStream::Qt_5_15);
    quint32 magic = 0;
    qint32 schema = 0;
    QVariantMap decoded;
    stream >> magic >> schema >> decoded;
    if (stream.status() != QDataStream::Ok || magic != kSnapshotMagic ||
        schema != kSnapshotSchemaVersion || decoded.size() > kMaxSnapshotKeys) {
        if (error)
            *error = QStringLiteral("The selected Git revision has an unsupported snapshot format");
        return false;
    }
    for (auto it = decoded.cbegin(); it != decoded.cend(); ++it) {
        if (it.key().isEmpty() || it.key().size() > 4096 ||
            it.key().startsWith(QStringLiteral("History/"))) {
            if (error)
                *error = QStringLiteral("The selected Git revision contains an invalid key");
            return false;
        }
    }
    if (values)
        *values = decoded;
    return true;
}

QString ProcessError(const QByteArray& standardError, const QString& fallback)
{
    QString detail = QString::fromUtf8(standardError).trimmed();
    detail.replace(QRegularExpression(QStringLiteral("[\\r\\n]+")), QStringLiteral(" "));
    if (detail.size() > 512)
        detail = detail.left(512) + QChar(0x2026);
    return detail.isEmpty() ? fallback : detail;
}
}

CLocalSettingsHistory::CLocalSettingsHistory(const QString& repositoryPath,
    const QString& legacyJsonlPath, int maxEntries)
    : m_repositoryPath(QDir::cleanPath(repositoryPath)),
      m_legacyJsonlPath(legacyJsonlPath.isEmpty()
          ? QString() : QDir::cleanPath(legacyJsonlPath)),
      m_maxEntries(qBound(50, maxEntries, 5000))
{
    QString error;
    m_available = ensureRepository(&error);
    m_lastError = error;
}

bool CLocalSettingsHistory::runGit(const QStringList& arguments,
    QByteArray* standardOutput, QString* error, int timeoutMs) const
{
    if (m_gitExecutable.isEmpty()) {
        if (error)
            *error = QStringLiteral("Git is not installed or is not available on PATH");
        return false;
    }

    QProcess process;
    process.setWorkingDirectory(m_repositoryPath);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    // The history repository must not inherit caller-controlled Git routing,
    // identity, configuration injection, object stores, or credential helpers.
    // Keep PATH so the already-resolved executable can load its runtime, but
    // rebuild Git's process-local contract explicitly below.
    const QStringList inheritedNames = environment.keys();
    for (const QString& name : inheritedNames) {
        if (name.startsWith(QStringLiteral("GIT_"), Qt::CaseInsensitive) ||
            name.compare(QStringLiteral("SSH_ASKPASS"), Qt::CaseInsensitive) == 0)
            environment.remove(name);
    }
    environment.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
    environment.insert(QStringLiteral("GIT_CONFIG_NOSYSTEM"), QStringLiteral("1"));
#ifdef Q_OS_WIN
    environment.insert(QStringLiteral("GIT_CONFIG_GLOBAL"), QStringLiteral("NUL"));
#else
    environment.insert(QStringLiteral("GIT_CONFIG_GLOBAL"), QStringLiteral("/dev/null"));
#endif
    environment.insert(QStringLiteral("GIT_ATTR_NOSYSTEM"), QStringLiteral("1"));
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    process.setProcessEnvironment(environment);
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* arguments) {
        arguments->flags |= CREATE_NO_WINDOW;
    });
#endif
    process.start(m_gitExecutable, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted(3000)) {
        if (error)
            *error = QStringLiteral("Git could not be started: %1").arg(process.errorString());
        return false;
    }
    process.closeWriteChannel();
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        if (error)
            *error = QStringLiteral("Git did not finish within the local history time limit");
        return false;
    }
    const QByteArray output = process.readAllStandardOutput();
    const QByteArray failure = process.readAllStandardError();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) {
            *error = ProcessError(failure,
                QStringLiteral("Git exited with code %1").arg(process.exitCode()));
        }
        return false;
    }
    if (standardOutput)
        *standardOutput = output;
    return true;
}

bool CLocalSettingsHistory::ensureRepository(QString* error)
{
    m_gitExecutable = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (m_gitExecutable.isEmpty()) {
        if (error)
            *error = QStringLiteral("Git is required for local settings history and was not found");
        return false;
    }

    const QFileInfo repositoryInfo(m_repositoryPath);
    if (repositoryInfo.exists() && repositoryInfo.isSymLink()) {
        if (error)
            *error = QStringLiteral("The local history repository path must not be a symbolic link");
        return false;
    }
    if (!QDir().mkpath(m_repositoryPath)) {
        if (error)
            *error = QStringLiteral("The local history repository directory could not be created");
        return false;
    }

    const QString gitDirectory = m_repositoryPath + QStringLiteral("/.git");
    const QFileInfo gitInfo(gitDirectory);
    if (gitInfo.exists() && (gitInfo.isSymLink() || !gitInfo.isDir())) {
        if (error)
            *error = QStringLiteral("The local history Git metadata path is not a safe directory");
        return false;
    }
    if (!gitInfo.exists()) {
        const QStringList existing = QDir(m_repositoryPath).entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        if (!existing.isEmpty()) {
            if (error)
                *error = QStringLiteral("The local history directory is not empty and is not a Git repository");
            return false;
        }
        if (!runGit({QStringLiteral("init"), QStringLiteral("--quiet")}, nullptr, error))
            return false;
        if (!runGit({QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"),
                QStringLiteral("refs/heads/main")}, nullptr, error))
            return false;
    }

    QByteArray inside;
    if (!runGit({QStringLiteral("rev-parse"), QStringLiteral("--is-inside-work-tree")},
            &inside, error) || inside.trimmed() != QByteArrayLiteral("true")) {
        if (error && error->isEmpty())
            *error = QStringLiteral("The local history path is not a Git working tree");
        return false;
    }
    QByteArray topLevel;
    if (!runGit({QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")},
            &topLevel, error))
        return false;
    const QString actualRoot = QDir::cleanPath(QString::fromUtf8(topLevel).trimmed());
    const QString expectedRoot = QDir::cleanPath(QFileInfo(m_repositoryPath).canonicalFilePath());
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
    if (actualRoot.compare(expectedRoot, pathCase) != 0) {
        if (error)
            *error = QStringLiteral("Git resolved the local history repository outside its application-data directory");
        return false;
    }

    for (const QStringList& command : {
            QStringList{QStringLiteral("config"), QStringLiteral("--local"),
                QStringLiteral("user.name"), QStringLiteral("Sandboxie Local History")},
            QStringList{QStringLiteral("config"), QStringLiteral("--local"),
                QStringLiteral("user.email"), QStringLiteral("local-history@sandboxie.invalid")},
            QStringList{QStringLiteral("config"), QStringLiteral("--local"),
                QStringLiteral("commit.gpgSign"), QStringLiteral("false")},
            QStringList{QStringLiteral("config"), QStringLiteral("--local"),
                QStringLiteral("core.autocrlf"), QStringLiteral("false")},
            QStringList{QStringLiteral("config"), QStringLiteral("--local"),
                QStringLiteral("core.fileMode"), QStringLiteral("false")}}) {
        if (!runGit(command, nullptr, error))
            return false;
    }

    QByteArray remotes;
    if (!runGit({QStringLiteral("remote")}, &remotes, error))
        return false;
    if (!remotes.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("The settings history repository must remain local and cannot have a remote");
        return false;
    }
    return true;
}

bool CLocalSettingsHistory::archiveLegacyHistory(QString* error)
{
    if (m_legacyJsonlPath.isEmpty() || !QFileInfo::exists(m_legacyJsonlPath))
        return true;
    const QFileInfo sourceInfo(m_legacyJsonlPath);
    if (!sourceInfo.isFile() || sourceInfo.isSymLink() ||
        sourceInfo.size() < 0 || sourceInfo.size() > kMaxLegacyArchiveBytes) {
        if (error)
            *error = QStringLiteral("The legacy settings history was left untouched because it is not a bounded regular file");
        return false;
    }

    QFile source(m_legacyJsonlPath);
    if (!source.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("The legacy settings history could not be read for the Git migration");
        return false;
    }
    const QByteArray contents = source.readAll();
    if (contents.size() != sourceInfo.size()) {
        if (error)
            *error = QStringLiteral("The legacy settings history changed while it was being archived");
        return false;
    }
    QDir().mkpath(m_repositoryPath + QStringLiteral("/legacy"));
    QSaveFile destination(m_repositoryPath + QLatin1Char('/') + QLatin1String(kLegacyArchiveName));
    if (!destination.open(QIODevice::WriteOnly) ||
        destination.write(contents) != contents.size() || !destination.commit()) {
        if (error)
            *error = QStringLiteral("The legacy settings history could not be archived in the Git repository");
        return false;
    }
    return true;
}

bool CLocalSettingsHistory::writeSnapshot(const QVariantMap& values, QString* error) const
{
    QByteArray bytes;
    if (!SnapshotToBytes(values, &bytes, error))
        return false;
    QSaveFile file(m_repositoryPath + QLatin1Char('/') + QLatin1String(kSnapshotFileName));
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error)
            *error = QStringLiteral("The settings snapshot could not be staged for Git");
        return false;
    }
    return true;
}

bool CLocalSettingsHistory::commitCurrentSnapshot(CSettings* settings,
    const QString& action, const QString& key, bool isSnapshot, QString* id,
    QString* error)
{
    if (!settings) {
        if (error)
            *error = QStringLiteral("Settings store is unavailable");
        return false;
    }
    QMutexLocker locker(&m_mutex);
    if (!m_available) {
        if (error)
            *error = m_lastError.isEmpty()
                ? QStringLiteral("Local Git settings history is unavailable") : m_lastError;
        return false;
    }
    // Snapshot while holding the history mutex. Without this ordering, two
    // concurrent setting writes can capture A then A+B, commit A+B first, and
    // finally append the stale A snapshot as the newest revision.
    const QVariantMap values = settings->SnapshotValues();
    QString failure;
    if (!writeSnapshot(values, &failure) ||
        !runGit({QStringLiteral("add"), QStringLiteral("--"),
                QLatin1String(kSnapshotFileName)}, nullptr, &failure)) {
        m_lastError = failure;
        if (error)
            *error = failure;
        return false;
    }
    if (QFileInfo::exists(m_repositoryPath + QLatin1Char('/') + QLatin1String(kLegacyArchiveName)) &&
        !runGit({QStringLiteral("add"), QStringLiteral("--"),
                QLatin1String(kLegacyArchiveName)}, nullptr, &failure)) {
        m_lastError = failure;
        if (error)
            *error = failure;
        return false;
    }

    const QString boundedAction = action.trimmed().left(512);
    const QString boundedKey = key.trimmed().left(4096);
    const QString body = QStringLiteral(
        "History-Schema: %1\nHistory-Action: %2\nHistory-Key: %3\nHistory-Kind: %4")
        .arg(kSnapshotSchemaVersion)
        .arg(EncodeMetadata(boundedAction.isEmpty() ? QStringLiteral("settings changed") : boundedAction))
        .arg(EncodeMetadata(boundedKey.isEmpty() ? QStringLiteral("(all settings)") : boundedKey))
        .arg(isSnapshot ? QStringLiteral("snapshot") : QStringLiteral("change"));
    if (!runGit({QStringLiteral("commit"), QStringLiteral("--quiet"),
            QStringLiteral("--allow-empty"), QStringLiteral("--no-gpg-sign"),
            QStringLiteral("--no-verify"), QStringLiteral("-m"),
            QStringLiteral("Record local settings revision"), QStringLiteral("-m"), body},
            nullptr, &failure)) {
        m_lastError = failure;
        if (error)
            *error = failure;
        return false;
    }

    QByteArray revision;
    if (!runGit({QStringLiteral("rev-parse"), QStringLiteral("HEAD")},
            &revision, &failure)) {
        m_lastError = failure;
        if (error)
            *error = failure;
        return false;
    }
    const QString revisionId = QString::fromLatin1(revision.trimmed());
    if (!QRegularExpression(QStringLiteral("^[0-9a-f]{40}$")).match(revisionId).hasMatch()) {
        failure = QStringLiteral("Git returned an invalid local revision identifier");
        m_lastError = failure;
        if (error)
            *error = failure;
        return false;
    }
    if (!loadEntries(&failure)) {
        m_lastError = failure;
        if (error)
            *error = failure;
        return false;
    }
    m_lastError.clear();
    if (id)
        *id = revisionId;
    return true;
}

bool CLocalSettingsHistory::initialize(CSettings* settings, QString* error)
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_available) {
            if (error)
                *error = m_lastError;
            return false;
        }
    }

    QByteArray head;
    QString ignored;
    if (runGit({QStringLiteral("rev-parse"), QStringLiteral("--verify"),
            QStringLiteral("HEAD")}, &head, &ignored)) {
        QMutexLocker locker(&m_mutex);
        QString failure;
        if (!loadEntries(&failure)) {
            m_lastError = failure;
            if (error)
                *error = failure;
            return false;
        }
        m_lastError.clear();
        return true;
    }

    QString migrationWarning;
    archiveLegacyHistory(&migrationWarning);
    QString id;
    QString failure;
    if (!commitCurrentSnapshot(settings, QStringLiteral("settings history initialized"),
            QStringLiteral("(all settings)"), true, &id, &failure)) {
        if (error)
            *error = failure;
        return false;
    }
    if (!migrationWarning.isEmpty()) {
        QMutexLocker locker(&m_mutex);
        m_lastError = migrationWarning;
        if (error)
            *error = migrationWarning;
    }
    return true;
}

void CLocalSettingsHistory::record(CSettings* settings, const QString& key,
    bool hadBefore, const QVariant& before, bool hadAfter, const QVariant& after,
    const QString& action)
{
    if (key.isEmpty() || key.startsWith(QStringLiteral("History/")) ||
        (hadBefore == hadAfter && (!hadBefore || before == after)))
        return;
    QString error;
    commitCurrentSnapshot(settings, action, key, false, nullptr, &error);
}

bool CLocalSettingsHistory::checkpoint(CSettings* settings, QString* id,
    QString* error, const QString& action)
{
    return commitCurrentSnapshot(settings, action, QStringLiteral("(all settings)"),
        true, id, error);
}

bool CLocalSettingsHistory::loadEntries(QString* error)
{
    QByteArray log;
    if (!runGit({QStringLiteral("log"),
            QStringLiteral("--max-count=%1").arg(m_maxEntries),
            QStringLiteral("--date=iso-strict"),
            QStringLiteral("--format=%H%x1f%aI%x1f%B%x1e")}, &log, error))
        return false;

    QVector<Entry> newestFirst;
    const QList<QByteArray> records = log.split('\x1e');
    const QRegularExpression revisionPattern(QStringLiteral("^[0-9a-f]{40}$"));
    for (QByteArray record : records) {
        record = record.trimmed();
        if (record.isEmpty())
            continue;
        const QList<QByteArray> fields = record.split('\x1f');
        if (fields.size() < 3)
            continue;
        Entry entry;
        entry.id = QString::fromLatin1(fields.at(0).trimmed());
        if (!revisionPattern.match(entry.id).hasMatch())
            continue;
        entry.timestamp = QDateTime::fromString(QString::fromLatin1(fields.at(1).trimmed()),
            Qt::ISODate);
        const QString message = QString::fromUtf8(fields.at(2));
        const QStringList lines = message.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
            Qt::SkipEmptyParts);
        QString kind;
        for (const QString& line : lines) {
            if (line.startsWith(QStringLiteral("History-Action: ")))
                entry.action = DecodeMetadata(line.mid(16).trimmed());
            else if (line.startsWith(QStringLiteral("History-Key: ")))
                entry.key = DecodeMetadata(line.mid(13).trimmed());
            else if (line.startsWith(QStringLiteral("History-Kind: ")))
                kind = line.mid(14).trimmed();
        }
        if (entry.action.isEmpty())
            entry.action = QStringLiteral("settings revision");
        if (entry.key.isEmpty())
            entry.key = QStringLiteral("(all settings)");
        entry.isSnapshot = kind != QStringLiteral("change");
        if (entry.timestamp.isValid())
            newestFirst.append(entry);
    }
    std::reverse(newestFirst.begin(), newestFirst.end());
    m_entries = newestFirst;
    return true;
}

QVector<CLocalSettingsHistory::Entry> CLocalSettingsHistory::entries() const
{
    QMutexLocker locker(&m_mutex);
    return m_entries;
}

bool CLocalSettingsHistory::readSnapshot(const QString& id, QVariantMap* values,
    QString* error) const
{
    if (!QRegularExpression(QStringLiteral("^[0-9a-f]{40}$"),
            QRegularExpression::CaseInsensitiveOption).match(id).hasMatch()) {
        if (error)
            *error = QStringLiteral("The selected Git revision identifier is invalid");
        return false;
    }
    QByteArray bytes;
    if (!runGit({QStringLiteral("show"), QStringLiteral("--no-ext-diff"),
            QStringLiteral("--no-textconv"), id + QLatin1Char(':') +
            QLatin1String(kSnapshotFileName)}, &bytes, error))
        return false;
    return SnapshotFromBytes(bytes, values, error);
}

bool CLocalSettingsHistory::restore(const QString& id, CSettings* settings,
    QString* error)
{
    QVariantMap target;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_available) {
            if (error)
                *error = m_lastError;
            return false;
        }
        const auto found = std::find_if(m_entries.cbegin(), m_entries.cend(),
            [&id](const Entry& candidate) { return candidate.id == id; });
        if (found == m_entries.cend()) {
            if (error)
                *error = QStringLiteral("The selected Git revision is unavailable");
            return false;
        }
        QString failure;
        if (!readSnapshot(id, &target, &failure)) {
            m_lastError = failure;
            if (error)
                *error = failure;
            return false;
        }
    }

    QString checkpointId;
    if (!checkpoint(settings, &checkpointId, error,
            QStringLiteral("settings restore checkpoint")))
        return false;
    if (!settings || !settings->ApplySnapshot(target)) {
        if (error)
            *error = QStringLiteral("The selected Git revision could not be applied");
        return false;
    }

    QString followUpError;
    if (!commitCurrentSnapshot(settings, QStringLiteral("settings restored"),
            QStringLiteral("(all settings)"), true, nullptr, &followUpError)) {
        if (error) {
            *error = QStringLiteral("Settings were restored, but the restored state could not be committed: %1")
                .arg(followUpError);
        }
    }
    return true;
}

bool CLocalSettingsHistory::exportBundle(const QString& path, QString* error) const
{
    if (path.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Choose a destination for the Git history bundle");
        return false;
    }
    QMutexLocker locker(&m_mutex);
    if (!m_available) {
        if (error)
            *error = m_lastError;
        return false;
    }
    if (!runGit({QStringLiteral("bundle"), QStringLiteral("create"),
            QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath()),
            QStringLiteral("--all")}, nullptr, error, 30000))
        return false;
    if (!QFileInfo::exists(path) || QFileInfo(path).size() <= 0) {
        if (error)
            *error = QStringLiteral("Git did not create a usable history bundle");
        return false;
    }
    return true;
}

bool CLocalSettingsHistory::isAvailable() const
{
    QMutexLocker locker(&m_mutex);
    return m_available;
}

QString CLocalSettingsHistory::lastError() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}
