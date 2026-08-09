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
        const QByteArray line = file.readLine(64 * 1024 + 1);
        if (line.size() > 64 * 1024)
            continue;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(line, &error);
        if (error.error == QJsonParseError::NoError && document.isObject()) {
            const Entry entry = fromJson(document.object());
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
    for (const Entry& entry : m_entries)
        file.write(QJsonDocument(toJson(entry)).toJson(QJsonDocument::Compact) + '\n');
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
    if (entry.hadBefore)
        settings->SetValue(entry.key, entry.before);
    else
        settings->DelValue(entry.key);
    return true;
}
