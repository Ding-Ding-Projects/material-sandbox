#include "LocalMemoryRepository.h"

#include <QCoreApplication>
#include <QChar>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winternl.h>
#endif

namespace {

constexpr qint64 kMaximumTextBytes = 1024 * 1024;
constexpr int kMaximumEntries = 2000;
constexpr int kIdentityReadBytes = 4096;
const QString kInstructionMarker = QStringLiteral("# codingmachineedge shared agent instructions");
const QString kSkillMarker = QStringLiteral("name: agent-global-memory");

#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
std::function<void()> g_beforeCandidateOpenHook;
std::function<void()> g_afterDirectoryOpenHook;
std::function<void()> g_afterOpenHook;
std::function<void(int)> g_enumerationHook;
#endif

bool isWithinRoot(const QString& root, const QString& candidate)
{
#ifdef Q_OS_WIN
    // This is a diagnostic rename check only. Handle-relative traversal is the authority.
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    const QString cleanRoot = QDir::cleanPath(QDir::fromNativeSeparators(root));
    const QString cleanCandidate = QDir::cleanPath(QDir::fromNativeSeparators(candidate));
    if (cleanRoot.compare(cleanCandidate, sensitivity) == 0)
        return true;
    const QString prefix = cleanRoot.endsWith(QLatin1Char('/'))
        ? cleanRoot : cleanRoot + QLatin1Char('/');
    return cleanCandidate.startsWith(prefix, sensitivity);
}

bool isSafeComponent(const QString& component)
{
    if (component.isEmpty() || component == QStringLiteral(".") || component == QStringLiteral("..")
        || component.contains(QLatin1Char('/')) || component.contains(QLatin1Char('\\'))
        || component.contains(QLatin1Char(':')) || component.contains(QChar::Null)
        || component.endsWith(QLatin1Char('.')) || component.endsWith(QLatin1Char(' ')))
        return false;

    for (int index = 0; index < component.size(); ++index) {
        const QChar character = component.at(index);
        if (character.isHighSurrogate()) {
            if (index + 1 >= component.size() || !component.at(index + 1).isLowSurrogate())
                return false;
            ++index;
        } else if (character.isLowSurrogate() || character.unicode() < 0x20) {
            return false;
        }
    }
#ifdef Q_OS_WIN
    const QString base = component.section(QLatin1Char('.'), 0, 0).toUpper();
    if (base == QStringLiteral("CON") || base == QStringLiteral("PRN")
        || base == QStringLiteral("AUX") || base == QStringLiteral("NUL"))
        return false;
    if ((base.startsWith(QStringLiteral("COM")) || base.startsWith(QStringLiteral("LPT")))
        && base.size() == 4 && base.at(3) >= QLatin1Char('1') && base.at(3) <= QLatin1Char('9'))
        return false;
#endif
    return true;
}

QString normalizedRelative(const QString& value, bool* valid)
{
    const QString input = QDir::fromNativeSeparators(value.trimmed());
    const QStringList rawComponents = input.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    QString relative = QDir::cleanPath(input);
    bool ok = !relative.isEmpty()
        && relative != QStringLiteral(".")
        && !QDir::isAbsolutePath(relative)
        && relative != QStringLiteral("..")
        && !relative.startsWith(QStringLiteral("../"))
        && !relative.startsWith(QLatin1Char('/'))
        && !relative.contains(QLatin1Char(':'))
        && !relative.contains(QChar::Null)
        && !rawComponents.isEmpty();
    if (ok) {
        for (const QString& component : rawComponents) {
            if (!isSafeComponent(component)) {
                ok = false;
                break;
            }
        }
    }
    if (valid)
        *valid = ok;
    return relative;
}

bool isStrictUtf8(const QByteArray& bytes)
{
    const auto* data = reinterpret_cast<const unsigned char*>(bytes.constData());
    const int size = bytes.size();
    int index = 0;
    while (index < size) {
        const unsigned char first = data[index++];
        if (first <= 0x7f)
            continue;
        int continuation = 0;
        unsigned int codePoint = 0;
        unsigned int minimum = 0;
        if (first >= 0xc2 && first <= 0xdf) {
            continuation = 1; codePoint = first & 0x1f; minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            continuation = 2; codePoint = first & 0x0f; minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            continuation = 3; codePoint = first & 0x07; minimum = 0x10000;
        } else {
            return false;
        }
        if (index + continuation > size)
            return false;
        for (int offset = 0; offset < continuation; ++offset) {
            const unsigned char next = data[index++];
            if ((next & 0xc0) != 0x80)
                return false;
            codePoint = (codePoint << 6) | (next & 0x3f);
        }
        if (codePoint < minimum || codePoint > 0x10ffff
            || (codePoint >= 0xd800 && codePoint <= 0xdfff))
            return false;
    }
    return true;
}

bool hasLocalAbsoluteSyntax(const QString& value)
{
    const QString native = QDir::toNativeSeparators(value.trimmed());
#ifdef Q_OS_WIN
    if (native.startsWith(QStringLiteral("\\\\"))
        || native.startsWith(QStringLiteral("//"))
        || native.startsWith(QStringLiteral("\\\\?\\"))
        || native.startsWith(QStringLiteral("\\\\.\\")))
        return false;
    return native.size() >= 3
        && native.at(0).isLetter()
        && native.at(1) == QLatin1Char(':')
        && (native.at(2) == QLatin1Char('\\') || native.at(2) == QLatin1Char('/'));
#else
    return QDir::isAbsolutePath(value);
#endif
}

QDateTime dateTimeFromFileTime(qint64 ticks)
{
#ifdef Q_OS_WIN
    if (ticks <= 0)
        return QDateTime();
    constexpr qint64 kWindowsToUnixEpoch100ns = 116444736000000000LL;
    return QDateTime::fromMSecsSinceEpoch((ticks - kWindowsToUnixEpoch100ns) / 10000, Qt::UTC).toLocalTime();
#else
    Q_UNUSED(ticks);
    return QDateTime();
#endif
}

#ifdef Q_OS_WIN

#ifndef OBJ_DONT_REPARSE
#define OBJ_DONT_REPARSE 0x00001000L
#endif

class ScopedHandle
{
public:
    ScopedHandle() = default;
    explicit ScopedHandle(HANDLE value) : m_value(value) {}
    ~ScopedHandle() { reset(); }
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
    ScopedHandle(ScopedHandle&& other) noexcept : m_value(other.release()) {}
    ScopedHandle& operator=(ScopedHandle&& other) noexcept
    {
        if (this != &other)
            reset(other.release());
        return *this;
    }
    HANDLE get() const { return m_value; }
    bool valid() const { return m_value && m_value != INVALID_HANDLE_VALUE; }
    HANDLE release()
    {
        HANDLE value = m_value;
        m_value = INVALID_HANDLE_VALUE;
        return value;
    }
    void reset(HANDLE value = INVALID_HANDLE_VALUE)
    {
        if (valid())
            CloseHandle(m_value);
        m_value = value;
    }
private:
    HANDLE m_value = INVALID_HANDLE_VALUE;
};

struct HandleMetadata {
    QString displayPath;
    bool directory = false;
    qint64 size = 0;
    QDateTime modified;
};

struct RootSnapshot {
    ScopedHandle handle;
    QString canonicalPath;
    quint64 volumeSerial = 0;
    QByteArray fileId;
};

QString pathFromHandle(HANDLE handle)
{
    const DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
    const DWORD required = GetFinalPathNameByHandleW(handle, nullptr, 0, flags);
    if (!required)
        return QString();
    std::wstring buffer(static_cast<size_t>(required) + 1, L'\0');
    const DWORD written = GetFinalPathNameByHandleW(handle, &buffer[0],
                                                     static_cast<DWORD>(buffer.size()), flags);
    if (!written || written >= buffer.size())
        return QString();
    QString path = QString::fromWCharArray(buffer.data(), static_cast<int>(written));
    if (path.startsWith(QStringLiteral("\\\\?\\UNC\\"), Qt::CaseInsensitive))
        path = QStringLiteral("//") + path.mid(8);
    else if (path.startsWith(QStringLiteral("\\\\?\\")))
        path.remove(0, 4);
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}

QByteArray fileIdForHandle(HANDLE handle, quint64* volumeSerial)
{
    BY_HANDLE_FILE_INFORMATION information{};
    if (!GetFileInformationByHandle(handle, &information))
        return QByteArray();
    if (volumeSerial)
        *volumeSerial = information.dwVolumeSerialNumber;
    QByteArray id(sizeof(information.nFileIndexHigh) + sizeof(information.nFileIndexLow), Qt::Uninitialized);
    memcpy(id.data(), &information.nFileIndexHigh, sizeof(information.nFileIndexHigh));
    memcpy(id.data() + sizeof(information.nFileIndexHigh), &information.nFileIndexLow,
           sizeof(information.nFileIndexLow));
    return id;
}

CLocalMemoryRepository::Error metadataForHandle(HANDLE handle, HandleMetadata* metadata,
                                                 bool rejectHardLinks)
{
    FILE_ATTRIBUTE_TAG_INFO tag{};
    if (!GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &tag, sizeof(tag)))
        return CLocalMemoryRepository::AccessDenied;
    if ((tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
        return CLocalMemoryRepository::ReparsePoint;

    FILE_STANDARD_INFO standard{};
    if (!GetFileInformationByHandleEx(handle, FileStandardInfo, &standard, sizeof(standard)))
        return CLocalMemoryRepository::AccessDenied;
    const bool directory = standard.Directory != FALSE;
    if (standard.DeletePending)
        return CLocalMemoryRepository::EscapedRoot;
    if (rejectHardLinks && !directory && standard.NumberOfLinks > 1)
        return CLocalMemoryRepository::EscapedRoot;

    FILE_BASIC_INFO basic{};
    if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &basic, sizeof(basic)))
        return CLocalMemoryRepository::AccessDenied;

    if (metadata) {
        metadata->displayPath = pathFromHandle(handle);
        metadata->directory = directory;
        metadata->size = directory ? 0 : standard.EndOfFile.QuadPart;
        metadata->modified = dateTimeFromFileTime(basic.LastWriteTime.QuadPart);
    }
    return CLocalMemoryRepository::NoError;
}

ScopedHandle openChild(HANDLE parent, const QString& component, bool directory,
                       bool readContents, CLocalMemoryRepository::Error* error)
{
    if (error)
        *error = CLocalMemoryRepository::NoError;
    if (!isSafeComponent(component)) {
        if (error) *error = CLocalMemoryRepository::InvalidPath;
        return ScopedHandle();
    }

    std::wstring wide = component.toStdWString();
    if (wide.size() > ((std::numeric_limits<USHORT>::max)() / sizeof(wchar_t))) {
        if (error) *error = CLocalMemoryRepository::InvalidPath;
        return ScopedHandle();
    }
    UNICODE_STRING name{};
    name.Buffer = &wide[0];
    name.Length = static_cast<USHORT>(wide.size() * sizeof(wchar_t));
    name.MaximumLength = name.Length;
    OBJECT_ATTRIBUTES attributes{};
    attributes.Length = sizeof(attributes);
    attributes.RootDirectory = parent;
    attributes.ObjectName = &name;
    attributes.Attributes = OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE;

    IO_STATUS_BLOCK io{};
    HANDLE child = INVALID_HANDLE_VALUE;
    ACCESS_MASK access = FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    if (directory)
        access |= FILE_LIST_DIRECTORY;
    else if (readContents)
        access |= FILE_READ_DATA;
    ULONG options = FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT;
    if (directory)
        options |= FILE_DIRECTORY_FILE;
    else if (readContents)
        options |= FILE_NON_DIRECTORY_FILE;

    const NTSTATUS status = NtCreateFile(&child, access, &attributes, &io, nullptr,
                                         FILE_ATTRIBUTE_NORMAL,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                         FILE_OPEN, options, nullptr, 0);
    if (status < 0 || child == INVALID_HANDLE_VALUE) {
        if (error) {
            const ULONG code = static_cast<ULONG>(status);
            if (code == 0xC000050BUL)
                *error = CLocalMemoryRepository::ReparsePoint; // STATUS_REPARSE_POINT_ENCOUNTERED
            else if (code == 0xC0000034UL || code == 0xC000003AUL)
                *error = CLocalMemoryRepository::NotFound; // name/path not found
            else
                *error = CLocalMemoryRepository::AccessDenied;
        }
        return ScopedHandle();
    }

    ScopedHandle result(child);
    const CLocalMemoryRepository::Error metadataError = metadataForHandle(result.get(), nullptr, false);
    if (metadataError != CLocalMemoryRepository::NoError) {
        if (error) *error = metadataError;
        return ScopedHandle();
    }
    return result;
}

bool readHandle(HANDLE handle, qint64 maximumBytes, QByteArray* bytes,
                CLocalMemoryRepository::Error* error)
{
    if (error)
        *error = CLocalMemoryRepository::NoError;
    FILE_STANDARD_INFO standard{};
    if (!GetFileInformationByHandleEx(handle, FileStandardInfo, &standard, sizeof(standard))) {
        if (error) *error = CLocalMemoryRepository::AccessDenied;
        return false;
    }
    if (standard.Directory) {
        if (error) *error = CLocalMemoryRepository::InvalidPath;
        return false;
    }
    if (standard.EndOfFile.QuadPart < 0 || standard.EndOfFile.QuadPart > maximumBytes) {
        if (error) *error = CLocalMemoryRepository::TooLarge;
        return false;
    }
    const DWORD allocation = static_cast<DWORD>(maximumBytes + 1);
    QByteArray data(static_cast<int>(allocation), Qt::Uninitialized);
    DWORD total = 0;
    while (total < allocation) {
        DWORD received = 0;
        if (!ReadFile(handle, data.data() + total, allocation - total, &received, nullptr)) {
            if (error) *error = CLocalMemoryRepository::ReadFailed;
            return false;
        }
        if (!received)
            break;
        total += received;
    }
    data.resize(static_cast<int>(total));
    if (data.size() > maximumBytes) {
        if (error) *error = CLocalMemoryRepository::TooLarge;
        return false;
    }
    if (bytes)
        *bytes = data;
    return true;
}

bool rootIsLocalDrive(const QString& absolutePath)
{
    if (!hasLocalAbsoluteSyntax(absolutePath))
        return false;
    const QString native = QDir::toNativeSeparators(absolutePath);
    const QString driveRoot = native.left(3);
    const UINT kind = GetDriveTypeW(reinterpret_cast<LPCWSTR>(driveRoot.utf16()));
    return kind == DRIVE_FIXED || kind == DRIVE_REMOVABLE || kind == DRIVE_RAMDISK;
}

RootSnapshot openRootSnapshot(const QString& absoluteRoot, CLocalMemoryRepository::Error* error)
{
    RootSnapshot snapshot;
    if (error)
        *error = CLocalMemoryRepository::NoError;
    if (!rootIsLocalDrive(absoluteRoot)) {
        if (error) *error = CLocalMemoryRepository::RemoteRoot;
        return snapshot;
    }

    const QString native = QDir::toNativeSeparators(absoluteRoot);
    const QString driveRoot = native.left(3);
    ScopedHandle current(CreateFileW(reinterpret_cast<LPCWSTR>(driveRoot.utf16()),
                                     FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr, OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                     nullptr));
    if (!current.valid()) {
        if (error) *error = CLocalMemoryRepository::RootNotFound;
        return snapshot;
    }
    if (metadataForHandle(current.get(), nullptr, false) != CLocalMemoryRepository::NoError) {
        if (error) *error = CLocalMemoryRepository::ReparsePoint;
        return snapshot;
    }

    const QString relativeRoot = QDir::fromNativeSeparators(native.mid(3));
    const QStringList components = relativeRoot.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString& component : components) {
        CLocalMemoryRepository::Error componentError = CLocalMemoryRepository::NoError;
        ScopedHandle next = openChild(current.get(), component, true, false, &componentError);
        if (!next.valid()) {
            if (error) {
                *error = (componentError == CLocalMemoryRepository::NotFound
                          || componentError == CLocalMemoryRepository::NoError)
                    ? CLocalMemoryRepository::RootNotFound : componentError;
            }
            return snapshot;
        }
        current = std::move(next);
    }

    snapshot.canonicalPath = pathFromHandle(current.get());
    snapshot.fileId = fileIdForHandle(current.get(), &snapshot.volumeSerial);
    if (snapshot.canonicalPath.isEmpty() || snapshot.fileId.isEmpty()
        || !rootIsLocalDrive(snapshot.canonicalPath)) {
        if (error) *error = CLocalMemoryRepository::RemoteRoot;
        return RootSnapshot();
    }
    snapshot.handle = std::move(current);
    return snapshot;
}

class SecureRootSession
{
public:
    SecureRootSession(HANDLE retainedRoot, const QString& frozenCanonical,
                      quint64 frozenVolume, const QByteArray& frozenId)
    {
        HANDLE duplicate = INVALID_HANDLE_VALUE;
        if (!retainedRoot || retainedRoot == INVALID_HANDLE_VALUE
            || !DuplicateHandle(GetCurrentProcess(), retainedRoot, GetCurrentProcess(), &duplicate,
                                0, FALSE, DUPLICATE_SAME_ACCESS)) {
            m_error = CLocalMemoryRepository::AccessDenied;
            return;
        }
        m_root.handle.reset(duplicate);
        m_root.canonicalPath = pathFromHandle(m_root.handle.get());
        m_root.fileId = fileIdForHandle(m_root.handle.get(), &m_root.volumeSerial);
        if (m_root.canonicalPath.isEmpty() || m_root.fileId.isEmpty()) {
            m_error = CLocalMemoryRepository::AccessDenied;
            return;
        }
        if (m_root.canonicalPath.compare(frozenCanonical, Qt::CaseInsensitive) != 0
            || m_root.volumeSerial != frozenVolume || m_root.fileId != frozenId) {
            m_error = CLocalMemoryRepository::EscapedRoot;
            return;
        }
        m_error = validateIdentity();
    }

    CLocalMemoryRepository::Error error() const { return m_error; }
    const QString& canonicalRoot() const { return m_root.canonicalPath; }
    HANDLE rootHandle() const { return m_root.handle.get(); }

    ScopedHandle openRelative(const QString& relativePath, bool requireDirectory,
                              bool readContents, HandleMetadata* metadata,
                              CLocalMemoryRepository::Error* error,
                              bool rejectHardLinks = true,
                              bool invokeTestHook = true) const
    {
        if (error)
            *error = m_error;
        if (m_error != CLocalMemoryRepository::NoError)
            return ScopedHandle();
        bool valid = false;
        const QString relative = normalizedRelative(relativePath, &valid);
        if (!valid) {
            if (error) *error = CLocalMemoryRepository::InvalidPath;
            return ScopedHandle();
        }
        const QStringList components = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
        if (invokeTestHook && g_beforeCandidateOpenHook)
            g_beforeCandidateOpenHook();
#endif
        HANDLE parent = m_root.handle.get();
        ScopedHandle current;
        for (int index = 0; index < components.size(); ++index) {
            const bool last = index == components.size() - 1;
            const bool directory = !last || requireDirectory;
            CLocalMemoryRepository::Error componentError = CLocalMemoryRepository::NoError;
            ScopedHandle next = openChild(parent, components.at(index), directory,
                                          last && readContents, &componentError);
            if (!next.valid()) {
                if (error) *error = componentError;
                return ScopedHandle();
            }
            current = std::move(next);
            parent = current.get();
        }
        const CLocalMemoryRepository::Error metadataError =
            metadataForHandle(current.get(), metadata, rejectHardLinks);
        if (metadataError != CLocalMemoryRepository::NoError) {
            if (error) *error = metadataError;
            return ScopedHandle();
        }
        if (metadata && (metadata->displayPath.isEmpty()
                         || !isWithinRoot(m_root.canonicalPath, metadata->displayPath))) {
            if (error) *error = CLocalMemoryRepository::EscapedRoot;
            return ScopedHandle();
        }
        if (error)
            *error = CLocalMemoryRepository::NoError;
        return current;
    }

private:
    CLocalMemoryRepository::Error validateIdentity()
    {
        const CLocalMemoryRepository::Error instructionError =
            markerContains(QStringLiteral("memory/SHARED_INSTRUCTIONS.md"), kInstructionMarker);
        if (instructionError != CLocalMemoryRepository::NoError)
            return instructionError;
        return markerContains(QStringLiteral("skills/agent-global-memory/SKILL.md"), kSkillMarker);
    }

    CLocalMemoryRepository::Error markerContains(const QString& relativePath, const QString& marker)
    {
        HandleMetadata metadata;
        CLocalMemoryRepository::Error markerError = CLocalMemoryRepository::NoError;
        ScopedHandle handle = openRelative(relativePath, false, true, &metadata, &markerError,
                                           true, false);
        if (!handle.valid() || markerError != CLocalMemoryRepository::NoError) {
            return markerError == CLocalMemoryRepository::ReparsePoint
                ? markerError : CLocalMemoryRepository::InvalidRepository;
        }
        if (metadata.size <= 0 || metadata.size > kMaximumTextBytes)
            return CLocalMemoryRepository::InvalidRepository;
        QByteArray prefix(kIdentityReadBytes, Qt::Uninitialized);
        DWORD received = 0;
        if (!ReadFile(handle.get(), prefix.data(), static_cast<DWORD>(prefix.size()), &received, nullptr))
            return CLocalMemoryRepository::InvalidRepository;
        prefix.resize(static_cast<int>(received));
        if (!isStrictUtf8(prefix) || !QString::fromUtf8(prefix).contains(marker))
            return CLocalMemoryRepository::InvalidRepository;
        return CLocalMemoryRepository::NoError;
    }

    RootSnapshot m_root;
    CLocalMemoryRepository::Error m_error = CLocalMemoryRepository::NoError;
};

bool freezeRoot(const QString& root, QString* canonical, quint64* volume, QByteArray* id,
                quintptr* retainedHandle,
                CLocalMemoryRepository::Error* error)
{
    RootSnapshot snapshot = openRootSnapshot(root, error);
    if (!snapshot.handle.valid())
        return false;

    // Validate repository identity through the same anchored root handle before trusting it.
    const QString frozenPath = snapshot.canonicalPath;
    const quint64 frozenVolume = snapshot.volumeSerial;
    const QByteArray frozenId = snapshot.fileId;
    SecureRootSession session(snapshot.handle.get(), frozenPath, frozenVolume, frozenId);
    if (session.error() != CLocalMemoryRepository::NoError) {
        if (error) *error = session.error();
        return false;
    }
    if (canonical) *canonical = frozenPath;
    if (volume) *volume = frozenVolume;
    if (id) *id = frozenId;
    if (retainedHandle) {
        HANDLE duplicate = INVALID_HANDLE_VALUE;
        if (!DuplicateHandle(GetCurrentProcess(), snapshot.handle.get(), GetCurrentProcess(), &duplicate,
                             0, FALSE, DUPLICATE_SAME_ACCESS)) {
            if (error) *error = CLocalMemoryRepository::AccessDenied;
            return false;
        }
        *retainedHandle = reinterpret_cast<quintptr>(duplicate);
    }
    if (error) *error = CLocalMemoryRepository::NoError;
    return true;
}

#else

struct HandleMetadata {
    QString displayPath;
    bool directory = false;
    qint64 size = 0;
    QDateTime modified;
};

bool freezeRoot(const QString& root, QString* canonical, quint64* volume, QByteArray* id,
                quintptr* retainedHandle,
                CLocalMemoryRepository::Error* error)
{
    if (error) *error = CLocalMemoryRepository::NoError;
    if (!hasLocalAbsoluteSyntax(root)) {
        if (error) *error = CLocalMemoryRepository::RemoteRoot;
        return false;
    }
    const QFileInfo rootInfo(root);
    const QString rootCanonical = rootInfo.canonicalFilePath();
    if (!rootInfo.isDir() || rootInfo.isSymLink() || rootCanonical.isEmpty()) {
        if (error) *error = CLocalMemoryRepository::RootNotFound;
        return false;
    }
    const QFileInfo instructions(QDir(rootCanonical).filePath(QStringLiteral("memory/SHARED_INSTRUCTIONS.md")));
    const QFileInfo skill(QDir(rootCanonical).filePath(QStringLiteral("skills/agent-global-memory/SKILL.md")));
    QFile instructionsFile(instructions.filePath());
    QFile skillFile(skill.filePath());
    if (instructions.isSymLink() || skill.isSymLink()
        || !instructionsFile.open(QIODevice::ReadOnly)
        || !skillFile.open(QIODevice::ReadOnly)
        || !QString::fromUtf8(instructionsFile.read(kIdentityReadBytes)).contains(kInstructionMarker)
        || !QString::fromUtf8(skillFile.read(kIdentityReadBytes)).contains(kSkillMarker)) {
        if (error) *error = CLocalMemoryRepository::InvalidRepository;
        return false;
    }
    if (canonical) *canonical = rootCanonical;
    if (volume) *volume = 0;
    if (id) *id = rootCanonical.toUtf8();
    if (retainedHandle) *retainedHandle = 0;
    return true;
}

#endif

} // namespace

CLocalMemoryRepository::CLocalMemoryRepository(const QString& root)
{
    configureRoot(root.isEmpty() ? discoverDefaultRoot() : root);
}

CLocalMemoryRepository::~CLocalMemoryRepository()
{
#ifdef Q_OS_WIN
    if (m_rootHandle)
        CloseHandle(reinterpret_cast<HANDLE>(m_rootHandle));
#endif
}

void CLocalMemoryRepository::setRoot(const QString& root)
{
    configureRoot(root);
}

void CLocalMemoryRepository::configureRoot(const QString& root)
{
#ifdef Q_OS_WIN
    if (m_rootHandle)
        CloseHandle(reinterpret_cast<HANDLE>(m_rootHandle));
#endif
    m_rootHandle = 0;
    m_root.clear();
    m_canonicalRoot.clear();
    m_rootVolumeSerial = 0;
    m_rootFileId.clear();
    m_configurationError = Unconfigured;

    const QString trimmed = root.trimmed();
    if (trimmed.isEmpty())
        return;
    if (!hasLocalAbsoluteSyntax(trimmed)) {
        m_configurationError = RemoteRoot;
        return;
    }
    m_root = QDir::cleanPath(QDir::fromNativeSeparators(trimmed));
    Error validationError = NoError;
    if (!freezeRoot(m_root, &m_canonicalRoot, &m_rootVolumeSerial, &m_rootFileId,
                    &m_rootHandle, &validationError)) {
        m_canonicalRoot.clear();
        m_rootVolumeSerial = 0;
        m_rootFileId.clear();
        m_configurationError = validationError == NoError ? InvalidRepository : validationError;
        return;
    }
    m_configurationError = NoError;
}

QString CLocalMemoryRepository::root() const { return m_root; }
QString CLocalMemoryRepository::canonicalRoot() const { return m_canonicalRoot; }

bool CLocalMemoryRepository::isConfigured() const
{
    if (m_root.isEmpty() || m_canonicalRoot.isEmpty() || m_rootFileId.isEmpty())
        return false;
#ifdef Q_OS_WIN
    if (!m_rootHandle)
        return false;
    const SecureRootSession session(reinterpret_cast<HANDLE>(m_rootHandle), m_canonicalRoot,
                                    m_rootVolumeSerial, m_rootFileId);
    return session.error() == NoError;
#else
    QString canonical;
    quint64 volume = 0;
    QByteArray id;
    Error error = NoError;
    return freezeRoot(m_root, &canonical, &volume, &id, nullptr, &error)
        && canonical == m_canonicalRoot && id == m_rootFileId;
#endif
}

QString CLocalMemoryRepository::safeExistingPath(const QString& relativePath, Error* error) const
{
    if (error)
        *error = NoError;
    if (m_root.isEmpty()) {
        if (error) *error = m_configurationError;
        return QString();
    }
    if (m_canonicalRoot.isEmpty()) {
        if (error) *error = m_configurationError;
        return QString();
    }
#ifdef Q_OS_WIN
    const SecureRootSession session(reinterpret_cast<HANDLE>(m_rootHandle), m_canonicalRoot,
                                    m_rootVolumeSerial, m_rootFileId);
    if (session.error() != NoError) {
        if (error) *error = session.error();
        return QString();
    }
    HandleMetadata metadata;
    ScopedHandle handle = session.openRelative(relativePath, false, false, &metadata, error, true);
    return handle.valid() ? metadata.displayPath : QString();
#else
    bool valid = false;
    const QString relative = normalizedRelative(relativePath, &valid);
    if (!valid) {
        if (error) *error = InvalidPath;
        return QString();
    }
    const QFileInfo candidate(QDir(m_canonicalRoot).filePath(relative));
    const QString canonical = candidate.canonicalFilePath();
    if (!candidate.exists()) {
        if (error) *error = NotFound;
        return QString();
    }
    if (candidate.isSymLink()) {
        if (error) *error = ReparsePoint;
        return QString();
    }
    if (canonical.isEmpty() || !isWithinRoot(m_canonicalRoot, canonical)) {
        if (error) *error = EscapedRoot;
        return QString();
    }
    return canonical;
#endif
}

CLocalMemoryRepository::TextResult CLocalMemoryRepository::readText(const QString& relativePath,
                                                                     qint64 maximumBytes) const
{
    TextResult result;
    if (maximumBytes < 1 || maximumBytes > kMaximumTextBytes) {
        result.error = InvalidLimit;
        result.message = errorText(result.error);
        return result;
    }
    if (m_root.isEmpty()) {
        result.error = m_configurationError;
        result.message = errorText(result.error);
        return result;
    }
    if (m_canonicalRoot.isEmpty()) {
        result.error = m_configurationError;
        result.message = errorText(result.error);
        return result;
    }
#ifdef Q_OS_WIN
    const SecureRootSession session(reinterpret_cast<HANDLE>(m_rootHandle), m_canonicalRoot,
                                    m_rootVolumeSerial, m_rootFileId);
    if (session.error() != NoError) {
        result.error = session.error();
        result.message = errorText(result.error);
        return result;
    }
    HandleMetadata metadata;
    ScopedHandle handle = session.openRelative(relativePath, false, true, &metadata, &result.error, true);
    if (!handle.valid()) {
        result.message = errorText(result.error);
        return result;
    }
    if (metadata.directory) {
        result.error = InvalidPath;
        result.message = errorText(result.error);
        return result;
    }
    result.absolutePath = metadata.displayPath;
    result.size = metadata.size;
    result.modified = metadata.modified;
#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
    if (g_afterOpenHook)
        g_afterOpenHook();
#endif
    QByteArray bytes;
    if (!readHandle(handle.get(), maximumBytes, &bytes, &result.error)) {
        result.message = errorText(result.error);
        return result;
    }
#else
    result.absolutePath = safeExistingPath(relativePath, &result.error);
    if (result.error != NoError) {
        result.message = errorText(result.error);
        return result;
    }
    const QFileInfo info(result.absolutePath);
    result.size = info.size();
    result.modified = info.lastModified();
    if (!info.isFile() || info.size() < 0) {
        result.error = InvalidPath;
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
#endif
    if (bytes.size() > maximumBytes) {
        result.error = TooLarge;
        result.message = errorText(result.error);
        return result;
    }
    if (!isStrictUtf8(bytes)) {
        result.error = ReadFailed;
        result.message = QObject::tr("The file is not valid UTF-8 text.");
        return result;
    }
    result.text = QString::fromUtf8(bytes);
    return result;
}

CLocalMemoryRepository::ListResult CLocalMemoryRepository::list(const QString& relativeDirectory,
                                                                 const QStringList& nameFilters,
                                                                 int maximumEntries) const
{
    ListResult result;
    if (maximumEntries <= 0 || maximumEntries > kMaximumEntries) {
        result.error = InvalidLimit;
        result.message = errorText(result.error);
        return result;
    }
    const int acceptedLimit = qMin(maximumEntries, kMaximumEntries);
    if (m_root.isEmpty()) {
        result.error = m_configurationError;
        result.message = errorText(result.error);
        return result;
    }
    if (m_canonicalRoot.isEmpty()) {
        result.error = m_configurationError;
        result.message = errorText(result.error);
        return result;
    }

    const QString requestedDirectory = relativeDirectory.trimmed();
    QString normalizedDirectory;
    if (!requestedDirectory.isEmpty()) {
        bool valid = false;
        normalizedDirectory = normalizedRelative(requestedDirectory, &valid);
        if (!valid) {
            result.error = InvalidPath;
            result.message = errorText(result.error);
            return result;
        }
    }

#ifdef Q_OS_WIN
    const SecureRootSession session(reinterpret_cast<HANDLE>(m_rootHandle), m_canonicalRoot,
                                    m_rootVolumeSerial, m_rootFileId);
    if (session.error() != NoError) {
        result.error = session.error();
        result.message = errorText(result.error);
        return result;
    }
    HandleMetadata directoryMetadata;
    ScopedHandle directory;
    if (normalizedDirectory.isEmpty()) {
        HANDLE duplicate = INVALID_HANDLE_VALUE;
        if (!DuplicateHandle(GetCurrentProcess(), session.rootHandle(), GetCurrentProcess(), &duplicate,
                             0, FALSE, DUPLICATE_SAME_ACCESS)) {
            result.error = AccessDenied;
            result.message = errorText(result.error);
            return result;
        }
        directory.reset(duplicate);
    } else {
        directory = session.openRelative(normalizedDirectory, true, false,
                                         &directoryMetadata, &result.error, false);
    }
    if (!directory.valid()) {
        if (result.error == NoError) result.error = AccessDenied;
        result.message = errorText(result.error);
        return result;
    }
#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
    if (g_afterDirectoryOpenHook)
        g_afterDirectoryOpenHook();
#endif

    QByteArray buffer(64 * 1024, Qt::Uninitialized);
    int scanned = 0;
    auto consumeBuffer = [&]() -> bool {
        const char* const bufferBegin = buffer.constData();
        const char* const bufferEnd = bufferBegin + buffer.size();
        constexpr size_t recordHeaderBytes = offsetof(FILE_ID_BOTH_DIR_INFO, FileName);
        auto* record = reinterpret_cast<FILE_ID_BOTH_DIR_INFO*>(buffer.data());
        for (;;) {
            const char* const recordBytes = reinterpret_cast<const char*>(record);
            const std::ptrdiff_t recordOffset = recordBytes - bufferBegin;
            const std::ptrdiff_t bytesRemaining = bufferEnd - recordBytes;
            if (recordOffset < 0 || bytesRemaining < 0
                || static_cast<size_t>(bytesRemaining) < recordHeaderBytes
                || (record->FileNameLength % sizeof(wchar_t)) != 0
                || static_cast<size_t>(record->FileNameLength)
                    > static_cast<size_t>(bytesRemaining) - recordHeaderBytes) {
                result.entries.clear();
                result.error = AccessDenied;
                result.message = errorText(result.error);
                return false;
            }
            const QString name = QString::fromWCharArray(record->FileName,
                static_cast<int>(record->FileNameLength / sizeof(wchar_t)));
            if (name != QStringLiteral(".") && name != QStringLiteral("..")) {
                ++scanned;
#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
                if (g_enumerationHook)
                    g_enumerationHook(scanned);
#endif
                if (scanned > kMaximumEntries) {
                    result.entries.clear();
                    result.error = TooManyEntries;
                    result.message = errorText(result.error);
                    return false;
                }
                if (nameFilters.isEmpty() || QDir::match(nameFilters, name)) {
                    if (result.entries.size() >= acceptedLimit) {
                        result.entries.clear();
                        result.error = TooManyEntries;
                        result.message = errorText(result.error);
                        return false;
                    }
                    const QString relative = normalizedDirectory.isEmpty()
                        ? name : normalizedDirectory + QLatin1Char('/') + name;
                    HandleMetadata metadata;
                    Error entryError = NoError;
                    ScopedHandle entry = openChild(directory.get(), name, false, false, &entryError);
                    if (entry.valid()) {
                        entryError = metadataForHandle(entry.get(), &metadata, true);
                        if (entryError == NoError
                            && (metadata.displayPath.isEmpty()
                                || !isWithinRoot(session.canonicalRoot(), metadata.displayPath)))
                            entryError = EscapedRoot;
                    }
                    if (!entry.valid()) {
                        result.entries.clear();
                        result.error = entryError;
                        result.message = errorText(result.error);
                        return false;
                    }
                    if (entryError != NoError) {
                        result.entries.clear();
                        result.error = entryError;
                        result.message = errorText(result.error);
                        return false;
                    }
                    Entry item;
                    item.name = name;
                    item.relativePath = relative;
                    item.absolutePath = metadata.displayPath;
                    item.directory = metadata.directory;
                    item.size = metadata.size;
                    item.modified = metadata.modified;
                    result.entries.append(item);
                }
            }
            if (!record->NextEntryOffset)
                break;
            if ((record->NextEntryOffset % sizeof(quint64)) != 0
                || record->NextEntryOffset < recordHeaderBytes + record->FileNameLength
                || record->NextEntryOffset > static_cast<ULONG>(bytesRemaining)) {
                result.entries.clear();
                result.error = AccessDenied;
                result.message = errorText(result.error);
                return false;
            }
            record = reinterpret_cast<FILE_ID_BOTH_DIR_INFO*>(
                reinterpret_cast<char*>(record) + record->NextEntryOffset);
        }
        return true;
    };

    bool firstQuery = true;
    for (;;) {
        const FILE_INFO_BY_HANDLE_CLASS queryClass = firstQuery
            ? FileIdBothDirectoryRestartInfo : FileIdBothDirectoryInfo;
        if (!GetFileInformationByHandleEx(directory.get(), queryClass,
                                          buffer.data(), static_cast<DWORD>(buffer.size()))) {
            const DWORD code = GetLastError();
            if (code == ERROR_NO_MORE_FILES)
                break;
            result.error = AccessDenied;
            result.message = errorText(result.error);
            return result;
        }
        firstQuery = false;
        if (!consumeBuffer())
            return result;
    }
#else
    const QString directoryPath = normalizedDirectory.isEmpty()
        ? m_canonicalRoot : safeExistingPath(normalizedDirectory, &result.error);
    if (result.error != NoError) {
        result.message = errorText(result.error);
        return result;
    }
    QDirIterator iterator(directoryPath, QDir::NoDotAndDotDot | QDir::AllEntries,
                          QDirIterator::NoIteratorFlags);
    int scanned = 0;
    while (iterator.hasNext()) {
        iterator.next();
        if (++scanned > kMaximumEntries) {
            result.entries.clear(); result.error = TooManyEntries;
            result.message = errorText(result.error); return result;
        }
        const QFileInfo info = iterator.fileInfo();
        if (!nameFilters.isEmpty() && !QDir::match(nameFilters, info.fileName()))
            continue;
        if (result.entries.size() >= acceptedLimit) {
            result.entries.clear(); result.error = TooManyEntries;
            result.message = errorText(result.error); return result;
        }
        const QString canonical = info.canonicalFilePath();
        if (info.isSymLink() || canonical.isEmpty() || !isWithinRoot(m_canonicalRoot, canonical)) {
            result.entries.clear(); result.error = EscapedRoot;
            result.message = errorText(result.error); return result;
        }
        Entry item;
        item.name = info.fileName(); item.absolutePath = canonical;
        item.relativePath = QDir(m_canonicalRoot).relativeFilePath(canonical);
        item.directory = info.isDir(); item.size = info.size(); item.modified = info.lastModified();
        result.entries.append(item);
    }
#endif

    std::sort(result.entries.begin(), result.entries.end(), [](const Entry& left, const Entry& right) {
        if (left.directory != right.directory)
            return left.directory;
        return QString::compare(left.name, right.name, Qt::CaseInsensitive) < 0;
    });
    return result;
}

QString CLocalMemoryRepository::discoverDefaultRoot()
{
    const QString environment = qEnvironmentVariable("SANDMAN_MEMORY_ROOT").trimmed();
    if (!environment.isEmpty() && hasLocalAbsoluteSyntax(environment)) {
        CLocalMemoryRepository repository(environment);
        if (repository.isConfigured())
            return repository.canonicalRoot();
    }

    QDir cursor(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 5; ++depth) {
        const QString direct = cursor.absoluteFilePath(QStringLiteral("agent-global-memory"));
        CLocalMemoryRepository directRepository(direct);
        if (directRepository.isConfigured())
            return directRepository.canonicalRoot();
        CLocalMemoryRepository cursorRepository(cursor.absolutePath());
        if (cursorRepository.isConfigured())
            return cursorRepository.canonicalRoot();
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
    case AccessDenied: return QObject::tr("The requested local file cannot be read securely.");
    case EscapedRoot: return QObject::tr("The requested path or file identity escaped the configured repository.");
    case TooLarge: return QObject::tr("The requested text file exceeds the 1 MiB display limit.");
    case TooManyEntries: return QObject::tr("The directory exceeds the 2,000-entry display limit.");
    case InvalidPath: return QObject::tr("The requested local path is invalid.");
    case ReadFailed: return QObject::tr("The requested local text could not be decoded.");
    case InvalidLimit: return QObject::tr("The requested read or listing limit is outside the supported bounds.");
    case RemoteRoot: return QObject::tr("The memory repository must be on a local drive; network and device paths are refused.");
    case ReparsePoint: return QObject::tr("The requested path contains a symbolic link, junction, or other reparse point.");
    case InvalidRepository: return QObject::tr("The configured directory is not a validated agent-global-memory repository.");
    }
    return QObject::tr("The local repository request failed.");
}

#ifdef LOCAL_MEMORY_REPOSITORY_TESTING
void CLocalMemoryRepository::setTestBeforeCandidateOpenHook(const std::function<void()>& hook)
{
    g_beforeCandidateOpenHook = hook;
}

void CLocalMemoryRepository::setTestAfterDirectoryOpenHook(const std::function<void()>& hook)
{
    g_afterDirectoryOpenHook = hook;
}

void CLocalMemoryRepository::setTestAfterOpenHook(const std::function<void()>& hook)
{
    g_afterOpenHook = hook;
}

void CLocalMemoryRepository::setTestEnumerationHook(const std::function<void(int)>& hook)
{
    g_enumerationHook = hook;
}
#endif
