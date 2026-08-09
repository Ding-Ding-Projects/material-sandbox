#include "stdafx.h"
#include "Settings.h"
#include "LocalSettingsHistory.h"
//#include "qzlib.h"
#include "Common.h"
#include <QStandardPaths>

bool TestWriteRight(const QString& Path)
{
	QFile TestFile(Path + "/~test-" + GetRand64Str() + ".tmp");
	if(!TestFile.open(QFile::WriteOnly))
		return false;
	TestFile.close();
	return TestFile.remove();
}

CSettings::CSettings(const QString& AppDir, const QString& AppName, const QString& GroupName, QMap<QString, SSetting> DefaultValues, QObject* qObject) : QObject(qObject)
{
	m_ConfigDir = AppDir;
	if (!(m_bPortable = QFile::exists(m_ConfigDir + "/" + AppName + ".ini")))
	{
		QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
		if (dirs.isEmpty())
			m_ConfigDir = QDir::homePath() + "/." + GroupName + "/" + AppName;
		//
		// if ini is present in the shared location it take precedence over an ini in a user location
		//
		else if(dirs.count() > 2 && QFile::exists(dirs[1] + "/" + GroupName + "/" + AppName + "/" + AppName + ".ini"))
			m_ConfigDir = dirs[1] + "/" + GroupName + "/" + AppName;
		else
			m_ConfigDir = dirs[0] + "/" + GroupName + "/" + AppName;
		QDir().mkpath(m_ConfigDir);
	}

	m_pConf = new QSettings(m_ConfigDir + "/" + AppName + ".ini", QSettings::IniFormat, this);
	m_History = new CLocalSettingsHistory(m_ConfigDir + "/history/settings-history.jsonl");

	m_pConf->sync();

	//m_DefaultValues = DefaultValues;
	//foreach (const QString& Key, m_DefaultValues.keys())
	//{
	//	const SSetting& Setting = m_DefaultValues[Key];
	//	if(!m_pConf->contains(Key) || !Setting.Check(m_pConf->value(Key)))
	//	{
	//		if(Setting.IsBlob())
	//			m_pConf->setValue(Key, Setting.Value.toByteArray().toBase64().replace("+","-").replace("/","_").replace("=",""));
	//		else
	//			m_pConf->setValue(Key, Setting.Value);
	//	}
	//}
}

CSettings::~CSettings()
{
	m_pConf->sync();
	delete m_History;
}

void CSettings::DelValue(const QString& key)
{
	bool hadBefore = false;
	QVariant before;
	{
		QMutexLocker Locker(&m_Mutex);
		hadBefore = m_pConf->contains(key);
		before = hadBefore ? m_pConf->value(key) : QVariant();
		m_pConf->remove(key);
		m_ValueCache.clear();
	}
	if (m_History)
		m_History->record(key, hadBefore, before, false, QVariant(), QStringLiteral("setting deleted"));
}

bool CSettings::SetValue(const QString &key, const QVariant &value)
{
	bool hadBefore = false;
	QVariant before;
	{
		QMutexLocker Locker(&m_Mutex);
		hadBefore = m_pConf->contains(key);
		before = hadBefore ? m_pConf->value(key) : QVariant();

//	if (!m_DefaultValues.isEmpty())
//	{
//		ASSERT(m_pConf->contains(key));
//#ifndef _DEBUG
//		if (!m_DefaultValues[key].Check(value))
//			return false;
//#endif
//	}

		m_pConf->setValue(key, value);
		m_ValueCache.clear();
	}
	if (m_History)
		m_History->record(key, hadBefore, before, true, value);
	return true;
}

QVariant CSettings::GetValue(const QString &key, const QVariant& preset)
{
	QMutexLocker Locker(&m_Mutex);

//	ASSERT(m_DefaultValues.isEmpty() || m_pConf->contains(key));	

	return m_pConf->value(key, preset);
}

QVariantMap CSettings::SnapshotValues() const
{
	QMutexLocker Locker(&m_Mutex);
	QVariantMap values;
	const QStringList keys = m_pConf->allKeys();
	for (const QString& key : keys) {
		// History is an implementation detail and must never recursively become
		// part of a checkpoint or expose its local records to an export.
		if (!key.startsWith(QStringLiteral("History/")))
			values.insert(key, m_pConf->value(key));
	}
	return values;
}

bool CSettings::ApplySnapshot(const QVariantMap& values)
{
	QMutexLocker Locker(&m_Mutex);
	const QStringList keys = m_pConf->allKeys();
	for (const QString& key : keys) {
		if (!key.startsWith(QStringLiteral("History/")) && !values.contains(key))
			m_pConf->remove(key);
	}
	for (auto it = values.cbegin(); it != values.cend(); ++it)
		m_pConf->setValue(it.key(), it.value());
	m_ValueCache.clear();
	m_pConf->sync();
	return m_pConf->status() == QSettings::NoError;
}

void CSettings::SetBlob(const QString& key, const QByteArray& value)
{
	QString str;
	//QByteArray data = Pack(value);
	//if(data.size() < value.size())
	//	str = ":PackedArray:" + data.toBase64().replace("+","-").replace("/","_").replace("=","");
	//else
		str = ":ByteArray:" + value.toBase64().replace("+","-").replace("/","_").replace("=","");
	SetValue(key, str);
}

QByteArray CSettings::GetBlob(const QString& key)
{
	QByteArray value;
	QByteArray str = GetValue(key).toByteArray();
	if(str.left(11) == ":ByteArray:")
		value = QByteArray::fromBase64(str.mid(11).replace("-","+").replace("_","/"));
	//else if(str.left(13) == ":PackedArray:")
	//	value = Unpack(QByteArray::fromBase64(str.mid(13).replace("-","+").replace("_","/")));
	return value;
}

QStringList CSettings::ListKeys(const QString& Root)
{
	QMutexLocker Locker(&m_Mutex); 
	QStringList Keys;
	foreach(const QString& Key, m_pConf->allKeys())
	{
		QStringList Path = Key.split("/");
		ASSERT(Path.count() == 2);
		if(Path[0] == Root)
			Keys.append(Path[1]);
	}
	return Keys;
}
