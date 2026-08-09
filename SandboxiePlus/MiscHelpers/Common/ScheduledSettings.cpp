#include "ScheduledSettings.h"
#include "UserPresentationSettings.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUuid>
#include <QColor>

namespace {
constexpr int kSchemaVersion = 1;
constexpr const char* kStorageKey = "UIConfig/ScheduledRules";
const QStringList kKeys = {QStringLiteral("language"), QStringLiteral("theme"), QStringLiteral("density"), QStringLiteral("accent")};

QJsonObject toJson(const ScheduledSettings::Rule& rule)
{
	QJsonObject object;
	object.insert(QStringLiteral("id"), rule.id);
	object.insert(QStringLiteral("label"), rule.label);
	object.insert(QStringLiteral("enabled"), rule.enabled);
	object.insert(QStringLiteral("priority"), rule.priority);
	if (rule.startDate.isValid()) object.insert(QStringLiteral("startDate"), rule.startDate.toString(Qt::ISODate));
	if (rule.endDate.isValid()) object.insert(QStringLiteral("endDate"), rule.endDate.toString(Qt::ISODate));
	object.insert(QStringLiteral("startTime"), rule.startTime.toString(QStringLiteral("HH:mm:ss")));
	object.insert(QStringLiteral("endTime"), rule.endTime.toString(QStringLiteral("HH:mm:ss")));
	QJsonArray days;
	for (int day : rule.weekdays) days.append(day);
	object.insert(QStringLiteral("weekdays"), days);
	QJsonObject values;
	for (auto it = rule.values.cbegin(); it != rule.values.cend(); ++it)
		if (kKeys.contains(it.key())) values.insert(it.key(), it.value());
	object.insert(QStringLiteral("values"), values);
	return object;
}

ScheduledSettings::Rule fromJson(const QJsonObject& object)
{
	ScheduledSettings::Rule rule;
	rule.id = object.value(QStringLiteral("id")).toString();
	if (rule.id.isEmpty()) rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	rule.label = object.value(QStringLiteral("label")).toString();
	rule.enabled = object.value(QStringLiteral("enabled")).toBool(true);
	rule.priority = qBound(-1000, object.value(QStringLiteral("priority")).toInt(), 1000);
	rule.startDate = QDate::fromString(object.value(QStringLiteral("startDate")).toString(), Qt::ISODate);
	rule.endDate = QDate::fromString(object.value(QStringLiteral("endDate")).toString(), Qt::ISODate);
	const QTime start = QTime::fromString(object.value(QStringLiteral("startTime")).toString(), QStringLiteral("HH:mm:ss"));
	const QTime end = QTime::fromString(object.value(QStringLiteral("endTime")).toString(), QStringLiteral("HH:mm:ss"));
	if (start.isValid()) rule.startTime = start;
	if (end.isValid()) rule.endTime = end;
	for (const QJsonValue& value : object.value(QStringLiteral("weekdays")).toArray()) {
		const int day = value.toInt();
		if (day >= 1 && day <= 7 && !rule.weekdays.contains(day)) rule.weekdays.append(day);
	}
	const QJsonObject values = object.value(QStringLiteral("values")).toObject();
	for (const QString& key : kKeys)
		if (values.contains(key)) rule.values.insert(key, values.value(key).toString());
	return rule;
}
}

namespace ScheduledSettings {

bool Rule::matches(const QDateTime& local) const
{
	if (!enabled || !local.isValid()) return false;
	QDateTime cursor = local.toLocalTime();
	QDate date = cursor.date();
	QTime time = cursor.time();
	const bool crossesMidnight = startTime > endTime;
	if (crossesMidnight && time < startTime) date = date.addDays(-1);
	if (startDate.isValid() && date < startDate) return false;
	if (endDate.isValid() && date > endDate) return false;
	if (!weekdays.isEmpty() && !weekdays.contains(date.dayOfWeek())) return false;
	if (crossesMidnight)
		return time >= startTime || time <= endTime;
	return time >= startTime && time <= endTime;
}

QList<Rule> load(CSettings* settings)
{
	QList<Rule> result;
	if (!settings) return result;
	const QByteArray raw = settings->GetString(QString::fromLatin1(kStorageKey)).toUtf8();
	if (raw.isEmpty()) return result;
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(raw, &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject()) return result;
	const QJsonObject root = document.object();
	if (root.value(QStringLiteral("schemaVersion")).toInt() != kSchemaVersion) return result;
	for (const QJsonValue& value : root.value(QStringLiteral("rules")).toArray()) {
		if (value.isObject()) {
			const Rule rule = fromJson(value.toObject());
			if (validate(rule).isEmpty()) result.append(rule);
		}
	}
	return result;
}

bool save(CSettings* settings, const QList<Rule>& rules, QString* error)
{
	if (!settings) {
		if (error) *error = QObject::tr("Settings are not available.");
		return false;
	}
	QJsonArray array;
	for (const Rule& rule : rules) {
		const QStringList issues = validate(rule);
		if (!issues.isEmpty()) {
			if (error) *error = issues.join(QStringLiteral("\n"));
			return false;
		}
		array.append(toJson(rule));
	}
	QJsonObject root;
	root.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
	root.insert(QStringLiteral("rules"), array);
	return settings->SetValue(QString::fromLatin1(kStorageKey), QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

QStringList validate(const Rule& rule)
{
	QStringList issues;
	if (rule.id.trimmed().isEmpty()) issues << QObject::tr("A rule needs a stable identifier.");
	if (rule.label.trimmed().isEmpty()) issues << QObject::tr("A rule needs a label.");
	if (rule.priority < -1000 || rule.priority > 1000) issues << QObject::tr("Priority must be between -1000 and 1000.");
	if (!rule.startDate.isValid() && rule.startDate != QDate()) issues << QObject::tr("The start date is invalid.");
	if (!rule.endDate.isValid() && rule.endDate != QDate()) issues << QObject::tr("The end date is invalid.");
	if (rule.startDate.isValid() && rule.endDate.isValid() && rule.endDate < rule.startDate) issues << QObject::tr("The end date cannot precede the start date.");
	if (!rule.startTime.isValid() || !rule.endTime.isValid()) issues << QObject::tr("Start and end times must be valid.");
	for (int day : rule.weekdays) if (day < 1 || day > 7) issues << QObject::tr("Weekdays must use values 1 through 7.");
	for (auto it = rule.values.cbegin(); it != rule.values.cend(); ++it) {
		if (!kKeys.contains(it.key())) issues << QObject::tr("The setting '%1' is not allowed in a schedule.").arg(it.key());
		else if (it.key() == QStringLiteral("language") && !QStringList{QStringLiteral("english"), QStringLiteral("cantonese"), QStringLiteral("bilingual")}.contains(it.value()))
			issues << QObject::tr("Language must be English, Cantonese, or Bilingual.");
		else if (it.key() == QStringLiteral("theme") && !QStringList{QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("system")}.contains(it.value()))
			issues << QObject::tr("Theme must be Light, Dark, or System.");
		else if (it.key() == QStringLiteral("density") && !QStringList{QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("2")}.contains(it.value()))
			issues << QObject::tr("Density must be Compact, Comfortable, or Spacious.");
		else if (it.key() == QStringLiteral("accent") && !QColor(it.value()).isValid())
			issues << QObject::tr("Accent seed must be a valid color.");
	}
	return issues;
}

Rule effectiveRule(CSettings* settings, const QDateTime& local)
{
	Rule best;
	bool found = false;
	for (const Rule& candidate : load(settings)) {
		if (!candidate.matches(local)) continue;
		if (!found || candidate.priority > best.priority || (candidate.priority == best.priority && candidate.id < best.id)) {
			best = candidate;
			found = true;
		}
	}
	return found ? best : Rule();
}

void apply(CSettings* settings, const QDateTime& local)
{
	if (!settings) return;
	const Rule rule = effectiveRule(settings, local);
	if (rule.id.isEmpty()) return;
	const QString language = rule.values.value(QStringLiteral("language"));
	if (!UserPresentationSettings::schoolModeEnabled(settings) && !language.isEmpty()) {
		const QString current = settings->GetString(QStringLiteral("Options/LanguageMode"), QStringLiteral("english"));
		if (current != language) {
			if (language == QStringLiteral("cantonese")) UserPresentationSettings::setLanguageMode(settings, UserPresentationSettings::LanguageMode::Cantonese);
			else if (language == QStringLiteral("bilingual")) UserPresentationSettings::setLanguageMode(settings, UserPresentationSettings::LanguageMode::Bilingual);
			else if (language == QStringLiteral("english")) UserPresentationSettings::setLanguageMode(settings, UserPresentationSettings::LanguageMode::English);
		}
	}
	if (rule.values.contains(QStringLiteral("density"))) {
		bool ok = false;
		const int density = rule.values.value(QStringLiteral("density")).toInt(&ok);
		if (ok && density >= 0 && density <= 2 && settings->GetInt(CSettings::SStrRef("UIConfig/Density"), 1) != density)
			settings->SetValue(QStringLiteral("UIConfig/Density"), density);
	}
	const QString accent = rule.values.value(QStringLiteral("accent"));
	if (QColor(accent).isValid() && settings->GetString(QStringLiteral("UIConfig/AccentSeed")) != accent)
		settings->SetValue(QStringLiteral("UIConfig/AccentSeed"), accent);
	if (rule.values.contains(QStringLiteral("theme"))) {
		const QString theme = rule.values.value(QStringLiteral("theme"));
		int desired = 2;
		if (theme == QStringLiteral("light")) desired = 0;
		else if (theme == QStringLiteral("dark")) desired = 1;
		if (settings->GetInt(CSettings::SStrRef("Options/UseDarkTheme"), 2) != desired)
			settings->SetValue(QStringLiteral("Options/UseDarkTheme"), desired);
	}
}

QString valueLabel(const QString& key)
{
	if (key == QStringLiteral("language")) return QObject::tr("Language mode");
	if (key == QStringLiteral("theme")) return QObject::tr("Theme");
	if (key == QStringLiteral("density")) return QObject::tr("Density");
	if (key == QStringLiteral("accent")) return QObject::tr("Accent seed");
	return key;
}

QStringList valueKeys() { return kKeys; }

} // namespace ScheduledSettings
