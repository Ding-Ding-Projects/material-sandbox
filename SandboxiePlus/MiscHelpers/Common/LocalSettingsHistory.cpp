#include "stdafx.h"
#include "LocalSettingsHistory.h"

#include "Settings.h"
#include <algorithm>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

namespace {
constexpr int kMaxDeltaLineBytes = 64 * 1024;
constexpr int kMaxSnapshotBytes = 1024 * 1024;
constexpr int kMaxSnapshotKeys = 10000;
constexpr int kSnapshotSchemaVersion = 1;

QString VariantToBase64(bool present, const QVariant& value)
{
    if (!present)
        return QString();
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << value;
    return QString::fromLatin1(bytes.toBase64());
}

QVariant VariantFromBase64(bool present, const QString& encoded)
{
    if (!present || encoded.isEmpty())
        return QVariant();
    const QByteArray bytes = QByteArray::fromBase64(encoded.toLatin1());
    QDataStream stream(bytes);
    QVariant value;
    stream >> value;
    return stream.status() == QDataStream::Ok ? value : QVariant();
}

bool SnapshotToBase64(const QVariantMap& values, QString* encoded, QString* error)
{
    if (values.size() > kMaxSnapshotKeys) {
        if (error) *error = QStringLiteral("The settings checkpoint has too many keys");
        return false;
    }
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << values;
    if (stream.status() != QDataStream::Ok || bytes.size() > kMaxSnapshotBytes) {
        if (error) *error = QStringLiteral("The settings checkpoint exceeds the local size limit");
        return false;
    }
    if (encoded) *encoded = QString::fromLatin1(bytes.toBase64());
    return true;
}

bool SnapshotFromBase64(const QString& encoded, QVariantMap* values)
{
    if (encoded.isEmpty() || encoded.size() > (kMaxSnapshotBytes * 4 / 3 + 8))
        return false;
    const QByteArray bytes = QByteArray::fromBase64(encoded.toLatin1());
    if (bytes.isEmpty() || bytes.size() > kMaxSnapshotBytes)
        return false;
    QDataStream stream(bytes);
    stream.setVersion(QDataStream::Qt_5_15);
    QVariantMap decoded;
    stream >> decoded;
    if (stream.status() != QDataStream::Ok || decoded.size() > kMaxSnapshotKeys)
        return false;
    for (auto it = decoded.cbegin(); it != decoded.cend(); ++it) {
        if (it.key().isEmpty() || it.key().size() > 4096 ||
            it.key().startsWith(QStringLiteral("History/")))
            return false;
    }
    if (values) *values = decoded;
    return true;
}
}

CLocalSettingsHistory::CLocalSettingsHistory(const QString& filePath, int maxEntries)
    : m_filePath(filePath), m_maxEntries(qBound(50, maxEntries, 5000))
{
    load();
}

void CLocalSettingsHistory::load()
{
    QMutexLocker locker(&m_mutex);
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine(kMaxSnapshotBytes + 1);
        if (line.size() > kMaxSnapshotBytes)
            continue;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(line, &error);
        if (error.error == QJsonParseError::NoError && document.isObject()) {
            const QJsonObject object = document.object();
            if (!object.value(QStringLiteral("isSnapshot")).toBool() && line.size() > kMaxDeltaLineBytes)
                continue;
            const Entry entry = fromJson(object);
            if (!entry.id.isEmpty() && entry.timestamp.isValid() && !entry.key.isEmpty())
                m_entries.append(entry);
        }
    }
    if (m_entries.size() > m_maxEntries)
        m_entries = m_entries.mid(m_entries.size() - m_maxEntries);
}

QJsonObject CLocalSettingsHistory::toJson(const Entry& entry)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), entry.id);
    object.insert(QStringLiteral("timestamp"), entry.timestamp.toUTC().toString(Qt::ISODateWithMs));
    object.insert(QStringLiteral("key"), entry.key);
    object.insert(QStringLiteral("action"), entry.action);
    object.insert(QStringLiteral("isSnapshot"), entry.isSnapshot);
    if (entry.isSnapshot) {
        object.insert(QStringLiteral("snapshotSchema"), kSnapshotSchemaVersion);
        QString encoded;
        SnapshotToBase64(entry.snapshot, &encoded, nullptr);
        object.insert(QStringLiteral("snapshotData"), encoded);
        return object;
    }
    object.insert(QStringLiteral("hadBefore"), entry.hadBefore);
    object.insert(QStringLiteral("beforeData"), VariantToBase64(entry.hadBefore, entry.before));
    object.insert(QStringLiteral("hadAfter"), entry.hadAfter);
    object.insert(QStringLiteral("afterData"), VariantToBase64(entry.hadAfter, entry.after));
    return object;
}

CLocalSettingsHistory::Entry CLocalSettingsHistory::fromJson(const QJsonObject& object)
{
    Entry entry;
    entry.id = object.value(QStringLiteral("id")).toString();
    entry.timestamp = QDateTime::fromString(object.value(QStringLiteral("timestamp")).toString(), Qt::ISODateWithMs);
    entry.key = object.value(QStringLiteral("key")).toString();
    entry.action = object.value(QStringLiteral("action")).toString();
    entry.isSnapshot = object.value(QStringLiteral("isSnapshot")).toBool();
    if (entry.isSnapshot) {
        if (object.value(QStringLiteral("snapshotSchema")).toInt() != kSnapshotSchemaVersion) {
            entry.id.clear();
            return entry;
        }
        if (!SnapshotFromBase64(object.value(QStringLiteral("snapshotData")).toString(), &entry.snapshot))
            entry.id.clear();
        return entry;
    }
    entry.hadBefore = object.value(QStringLiteral("hadBefore")).toBool();
    entry.before = VariantFromBase64(entry.hadBefore, object.value(QStringLiteral("beforeData")).toString());
    entry.hadAfter = object.value(QStringLiteral("hadAfter")).toBool();
    entry.after = VariantFromBase64(entry.hadAfter, object.value(QStringLiteral("afterData")).toString());
    return entry;
}

bool CLocalSettingsHistory::write() const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    for (const Entry& entry : m_entries) {
        const QByteArray line = QJsonDocument(toJson(entry)).toJson(QJsonDocument::Compact) + '\n';
        if (line.size() > kMaxSnapshotBytes)
            return false;
        file.write(line);
    }
    return file.commit();
}

void CLocalSettingsHistory::record(const QString& key, bool hadBefore, const QVariant& before,
    bool hadAfter, const QVariant& after, const QString& action)
{
    if (key.isEmpty() || key.startsWith(QStringLiteral("History/")) ||
        (hadBefore == hadAfter && (!hadBefore || before == after)))
        return;
    QMutexLocker locker(&m_mutex);
    Entry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.timestamp = QDateTime::currentDateTimeUtc();
    entry.key = key;
    entry.action = action;
    entry.hadBefore = hadBefore;
    entry.before = before;
    entry.hadAfter = hadAfter;
    entry.after = after;
    m_entries.append(entry);
    if (m_entries.size() > m_maxEntries)
        m_entries.remove(0, m_entries.size() - m_maxEntries);
    write();
}

bool CLocalSettingsHistory::checkpoint(CSettings* settings, QString* id, QString* error,
    const QString& action)
{
    if (!settings) {
        if (error) *error = QStringLiteral("Settings store is unavailable");
        return false;
    }
    const QVariantMap values = settings->SnapshotValues();
    QString encoded;
    if (!SnapshotToBase64(values, &encoded, error))
        return false;

    Entry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.timestamp = QDateTime::currentDateTimeUtc();
    entry.key = QStringLiteral("(all settings)");
    entry.action = action;
    entry.isSnapshot = true;
    entry.snapshot = values;

    QMutexLocker locker(&m_mutex);
    const QVector<Entry> previous = m_entries;
    m_entries.append(entry);
    if (m_entries.size() > m_maxEntries)
        m_entries.remove(0, m_entries.size() - m_maxEntries);
    if (!write()) {
        m_entries = previous;
        if (error) *error = QStringLiteral("The settings checkpoint could not be written");
        return false;
    }
    if (id) *id = entry.id;
    Q_UNUSED(encoded);
    return true;
}

QVector<CLocalSettingsHistory::Entry> CLocalSettingsHistory::entries() const
{
    QMutexLocker locker(&m_mutex);
    return m_entries;
}

bool CLocalSettingsHistory::restore(const QString& id, CSettings* settings, QString* error) const
{
    if (!settings) {
        if (error) *error = QStringLiteral("Settings store is unavailable");
        return false;
    }
    Entry entry;
    {
        QMutexLocker locker(&m_mutex);
        auto it = std::find_if(m_entries.cbegin(), m_entries.cend(), [&id](const Entry& candidate) { return candidate.id == id; });
        if (it == m_entries.cend()) {
            if (error) *error = QStringLiteral("The selected revision is unavailable");
            return false;
        }
        entry = *it;
    }
    // Capture the live state first so restoring any revision is itself
    // reversible, including a full-state apply that would otherwise avoid
    // emitting one noisy delta record per key.
    if (!const_cast<CLocalSettingsHistory*>(this)->checkpoint(settings, nullptr, error,
            QStringLiteral("settings restore checkpoint")))
        return false;
    if (entry.isSnapshot) {
        if (!settings->ApplySnapshot(entry.snapshot)) {
            if (error) *error = QStringLiteral("The settings checkpoint could not be applied");
            return false;
        }
        return true;
    }
    if (entry.hadBefore)
        settings->SetValue(entry.key, entry.before);
    else
        settings->DelValue(entry.key);
    return true;
}
