#include "stdafx.h"
#include "LocalMemoryRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

bool isWithinRoot(const QString& root, const QString& candidate)
{
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    if (root.compare(candidate, sensitivity) == 0)
        return true;
    const QString prefix = root.endsWith(QDir::separator()) ? root : root + QDir::separator();
    return candidate.startsWith(prefix, sensitivity);
}

QString normalizedRelative(const QString& value, bool* valid)
{
    QString relative = QDir::cleanPath(QDir::fromNativeSeparators(value.trimmed()));
    const bool ok = !relative.isEmpty()
        && relative != QStringLiteral(".")
        && !QDir::isAbsolutePath(relative)
        && relative != QStringLiteral("..")
        && !relative.startsWith(QStringLiteral("../"));
    if (valid)
        *valid = ok;
    return relative;
}

}

CLocalMemoryRepository::CLocalMemoryRepository(const QString& root)
    : m_root(root.isEmpty() ? discoverDefaultRoot() : QDir::cleanPath(root))
{
}

void CLocalMemoryRepository::setRoot(const QString& root)
{
    m_root = QDir::cleanPath(root.trimmed());
}

QString CLocalMemoryRepository::root() const { return m_root; }

QString CLocalMemoryRepository::canonicalRoot() const
{
    if (m_root.isEmpty())
        return QString();
    const QFileInfo info(m_root);
    return info.exists() && info.isDir() ? info.canonicalFilePath() : QString();
}

bool CLocalMemoryRepository::isConfigured() const
{
    return !canonicalRoot().isEmpty();
}

QString CLocalMemoryRepository::safeExistingPath(const QString& relativePath, Error* error) const
{
    if (error)
        *error = NoError;
    const QString rootPath = canonicalRoot();
    if (m_root.isEmpty()) {
        if (error) *error = Unconfigured;
        return QString();
    }
    if (rootPath.isEmpty()) {
        if (error) *error = RootNotFound;
        return QString();
    }
    bool valid = false;
    const QString relative = normalizedRelative(relativePath, &valid);
    if (!valid) {
        if (error) *error = InvalidPath;
        return QString();
    }
    const QFileInfo candidate(QDir(rootPath).absoluteFilePath(relative));
    if (!candidate.exists()) {
        if (error) *error = NotFound;
        return QString();
    }
    const QString canonical = candidate.canonicalFilePath();
    if (canonical.isEmpty()) {
        if (error) *error = AccessDenied;
        return QString();
    }
    if (!isWithinRoot(rootPath, canonical)) {
        if (error) *error = EscapedRoot;
        return QString();
    }
    return canonical;
}

CLocalMemoryRepository::TextResult CLocalMemoryRepository::readText(const QString& relativePath,
                                                                     qint64 maximumBytes) const
{
    TextResult result;
    result.absolutePath = safeExistingPath(relativePath, &result.error);
    if (result.error != NoError) {
        result.message = errorText(result.error);
        return result;
    }
    const QFileInfo info(result.absolutePath);
    if (!info.isFile()) {
        result.error = InvalidPath;
        result.message = errorText(result.error);
        return result;
    }
    if (info.size() > maximumBytes) {
        result.error = TooLarge;
        result.message = errorText(result.error);
        return result;
    }
    QFile file(result.absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = AccessDenied;
        result.message = file.errorString();
        return result;
    }
    const QByteArray bytes = file.read(maximumBytes + 1);
    if (bytes.size() > maximumBytes) {
        result.error = TooLarge;
        result.message = errorText(result.error);
        return result;
    }
    result.text = QString::fromUtf8(bytes);
    if (result.text.contains(QChar::ReplacementCharacter) && !bytes.isEmpty()) {
        result.error = ReadFailed;
        result.message = QObject::tr("The file is not valid UTF-8 text.");
        result.text.clear();
    }
    return result;
}

CLocalMemoryRepository::ListResult CLocalMemoryRepository::list(const QString& relativeDirectory,
                                                                 const QStringList& nameFilters,
                                                                 int maximumEntries) const
{
    ListResult result;
    QString directoryPath;
    if (relativeDirectory.trimmed().isEmpty()) {
        directoryPath = canonicalRoot();
        if (m_root.isEmpty()) result.error = Unconfigured;
        else if (directoryPath.isEmpty()) result.error = RootNotFound;
    } else {
        directoryPath = safeExistingPath(relativeDirectory, &result.error);
    }
    if (result.error != NoError) {
        result.message = errorText(result.error);
        return result;
    }
    const QFileInfo directoryInfo(directoryPath);
    if (!directoryInfo.isDir()) {
        result.error = InvalidPath;
        result.message = errorText(result.error);
        return result;
    }
    QDir directory(directoryPath);
    if (!nameFilters.isEmpty())
        directory.setNameFilters(nameFilters);
    const QFileInfoList infos = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries,
                                                         QDir::DirsFirst | QDir::Name);
    if (infos.size() > maximumEntries) {
        result.error = TooManyEntries;
        result.message = errorText(result.error);
        return result;
    }
    const QString rootPath = canonicalRoot();
    for (const QFileInfo& info : infos) {
        const QString canonical = info.canonicalFilePath();
        if (canonical.isEmpty() || !isWithinRoot(rootPath, canonical))
            continue;
        Entry entry;
        entry.name = info.fileName();
        entry.absolutePath = canonical;
        entry.relativePath = QDir(rootPath).relativeFilePath(canonical);
        entry.directory = info.isDir();
        entry.size = info.size();
        entry.modified = info.lastModified();
        result.entries.append(entry);
    }
    return result;
}

QString CLocalMemoryRepository::discoverDefaultRoot()
{
    const QString environment = qEnvironmentVariable("SANDMAN_MEMORY_ROOT").trimmed();
    if (!environment.isEmpty() && QFileInfo(environment).isDir())
        return QDir::cleanPath(environment);

    QDir cursor(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 5; ++depth) {
        const QString direct = cursor.absoluteFilePath(QStringLiteral("agent-global-memory"));
        if (QFileInfo(direct).isDir())
            return QDir::cleanPath(direct);
        if (QFileInfo(cursor.absoluteFilePath(QStringLiteral("memory/SHARED_INSTRUCTIONS.md"))).isFile()
            || QFileInfo(cursor.absoluteFilePath(QStringLiteral("SHARED_INSTRUCTIONS.md"))).isFile())
            return cursor.absolutePath();
        if (!cursor.cdUp())
            break;
    }
    return QString();
}

QString CLocalMemoryRepository::errorText(Error error)
{
    switch (error) {
    case NoError: return QString();
    case Unconfigured: return QObject::tr("No local memory repository is configured.");
    case RootNotFound: return QObject::tr("The configured local memory repository does not exist.");
    case NotFound: return QObject::tr("The requested file or directory was not found.");
    case AccessDenied: return QObject::tr("The requested local file cannot be read.");
    case EscapedRoot: return QObject::tr("The requested path resolves outside the configured repository.");
    case TooLarge: return QObject::tr("The requested text file exceeds the 1 MiB display limit.");
    case TooManyEntries: return QObject::tr("The directory exceeds the 2,000-entry display limit.");
    case InvalidPath: return QObject::tr("The requested local path is invalid.");
    case ReadFailed: return QObject::tr("The requested local text could not be decoded.");
    }
    return QObject::tr("The local repository request failed.");
}
