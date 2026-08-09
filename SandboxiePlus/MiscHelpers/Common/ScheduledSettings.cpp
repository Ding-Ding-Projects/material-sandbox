#include "ScheduledSettings.h"
#include "UserPresentationSettings.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUuid>
#include <QColor>
#include <QCoreApplication>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QUrl>
#include <QRegularExpression>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#endif

namespace {
constexpr int kSchemaVersion = 1;
constexpr const char* kStorageKey = "UIConfig/ScheduledRules";
const QStringList kKeys = {QStringLiteral("language"), QStringLiteral("theme"), QStringLiteral("density"), QStringLiteral("accent")};
constexpr int kExternalResponseLimit = 64 * 1024;
constexpr int kExternalTimeoutMs = 10000;

struct ExternalState
{
	bool active = false;
	bool inFlight = false;
	QDateTime nextRefresh;
	QString status = QStringLiteral("external-source-not-activated");
};

QHash<QString, ExternalState> s_externalStates;

bool hasValidCredentialRef(const QString& ref)
{
	// The reference is an identifier, never a free-form secret. Keeping its
	// namespace fixed prevents a schedule from probing arbitrary credentials.
	return QRegularExpression(QStringLiteral("^os-vault://scheduled-settings/[A-Za-z0-9_-]{1,128}$")).match(ref).hasMatch();
}

bool validExternalUrl(const QString& text, const QString& kind)
{
	const QUrl url(text);
	if (!url.isValid() || url.scheme() != QStringLiteral("https") || url.host().isEmpty()
		|| !url.userName().isEmpty() || !url.password().isEmpty() || !url.query().isEmpty()
		|| !url.fragment().isEmpty() || (url.port() != -1 && url.port() != 443) || text.size() > 2048)
		return false;
	// Home Assistant sources are an origin, not an arbitrary API endpoint.
	return kind != QStringLiteral("home-assistant") || url.path().isEmpty() || url.path() == QStringLiteral("/");
}

QByteArray readVaultCredential(const QString& ref)
{
#ifdef Q_OS_WIN
	if (!hasValidCredentialRef(ref)) return {};
	PCREDENTIALW credential = nullptr;
	if (!CredReadW(reinterpret_cast<LPCWSTR>(ref.utf16()), CRED_TYPE_GENERIC, 0, &credential) || !credential)
		return {};
	QByteArray secret;
	if (credential->CredentialBlob && credential->CredentialBlobSize > 0 && credential->CredentialBlobSize <= 4096)
		secret = QByteArray(reinterpret_cast<const char*>(credential->CredentialBlob), credential->CredentialBlobSize);
	CredFree(credential);
	// Header injection and non-text blob values are never valid bearer tokens.
	if (secret.isEmpty() || secret.contains('\0') || secret.contains('\r') || secret.contains('\n')) {
		secret.fill('\0');
		return {};
	}
	return secret;
#else
	Q_UNUSED(ref);
	return {};
#endif
}

bool credentialAvailable(const QString& ref)
{
	QByteArray secret = readVaultCredential(ref);
	const bool available = !secret.isEmpty();
	secret.fill('\0');
	return available;
}

QString stateKey(const ScheduledSettings::Rule& rule)
{
	return rule.id + QLatin1Char('|') + rule.source.credentialRef;
}

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
	QJsonObject source;
	source.insert(QStringLiteral("kind"), rule.source.kind);
	if (!rule.source.url.isEmpty()) source.insert(QStringLiteral("url"), rule.source.url);
	if (!rule.source.entityId.isEmpty()) source.insert(QStringLiteral("entityId"), rule.source.entityId);
	if (!rule.source.credentialRef.isEmpty()) source.insert(QStringLiteral("credentialRef"), rule.source.credentialRef);
	source.insert(QStringLiteral("refreshSeconds"), rule.source.refreshSeconds);
	object.insert(QStringLiteral("source"), source);
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
	const QJsonObject source = object.value(QStringLiteral("source")).toObject();
	if (!source.isEmpty()) {
		rule.source.kind = source.value(QStringLiteral("kind")).toString(QStringLiteral("local"));
		rule.source.url = source.value(QStringLiteral("url")).toString();
		rule.source.entityId = source.value(QStringLiteral("entityId")).toString();
		rule.source.credentialRef = source.value(QStringLiteral("credentialRef")).toString();
		rule.source.refreshSeconds = source.value(QStringLiteral("refreshSeconds")).toInt(300);
	}
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
	const Source& source = rule.source;
	if (source.kind != QStringLiteral("local") && source.kind != QStringLiteral("https-api") && source.kind != QStringLiteral("home-assistant"))
		issues << QObject::tr("Source must be local, HTTPS API, or Home Assistant.");
	if (source.refreshSeconds < 15 || source.refreshSeconds > 86400)
		issues << QObject::tr("Source refresh must be between 15 seconds and 24 hours.");
	if (source.kind == QStringLiteral("local")) {
		if (!source.url.isEmpty() || !source.entityId.isEmpty() || !source.credentialRef.isEmpty())
			issues << QObject::tr("Local sources cannot include network or credential metadata.");
	} else {
		if (!validExternalUrl(source.url, source.kind))
			issues << QObject::tr("External sources require a bounded HTTPS URL without credentials, query, fragment, redirects, or a non-standard port.");
		if (!hasValidCredentialRef(source.credentialRef))
			issues << QObject::tr("External sources require an opaque os-vault://scheduled-settings/ credential reference; tokens are not accepted.");
		if (source.kind == QStringLiteral("https-api") && !source.entityId.isEmpty())
			issues << QObject::tr("HTTPS API sources cannot include a Home Assistant entity.");
		if (source.kind == QStringLiteral("home-assistant") && !QRegularExpression(QStringLiteral("^(binary_sensor|input_boolean)\\.[a-z0-9_]+$")).match(source.entityId).hasMatch())
			issues << QObject::tr("Home Assistant entity must be binary_sensor.* or input_boolean.*.");
	}
	return issues;
}

QString sourceStatus(const Source& source)
{
	if (source.kind == QStringLiteral("local")) return QStringLiteral("local");
	if (!validExternalUrl(source.url, source.kind) || !hasValidCredentialRef(source.credentialRef))
		return QStringLiteral("invalid-external-source");
	for (auto it = s_externalStates.cbegin(); it != s_externalStates.cend(); ++it) {
		if (!it.key().endsWith(QLatin1Char('|') + source.credentialRef)) continue;
		if (it->inFlight) return QStringLiteral("external-source-refreshing");
		if (it->status == QStringLiteral("external-source-active") || it->status == QStringLiteral("external-source-off") || it->status == QStringLiteral("external-source-failed"))
			return it->status;
	}
	if (!credentialAvailable(source.credentialRef)) return QStringLiteral("vault-credential-missing");
	return QStringLiteral("external-source-ready");
}

QString sourceStatusDescription(const Source& source)
{
	if (source.kind == QStringLiteral("local"))
		return QObject::tr("Local source · active");
	const QString status = sourceStatus(source);
	if (status == QStringLiteral("external-source-refreshing")) return QObject::tr("External source · refreshing safely");
	if (status == QStringLiteral("external-source-active")) return QObject::tr("External source · active (last bounded refresh returned on)");
	if (status == QStringLiteral("external-source-off")) return QObject::tr("External source · inactive (last bounded refresh returned off)");
	if (status == QStringLiteral("external-source-failed")) return QObject::tr("External source · not active (last refresh failed safely)");
	if (status == QStringLiteral("external-source-ready")) return QObject::tr("External source · ready to activate (Windows Credential Manager credential found)");
	if (status == QStringLiteral("vault-credential-missing")) return QObject::tr("External source · not active (Windows Credential Manager credential is missing)");
	return QObject::tr("External source · not active (invalid HTTPS or credential reference)");
}

void refreshExternalSources(CSettings* settings, bool force, const std::function<void()>& changed)
{
	if (!settings || !QCoreApplication::instance()) return;
	const QDateTime now = QDateTime::currentDateTimeUtc();
	for (const Rule& rule : load(settings)) {
		if (!rule.enabled || rule.source.kind == QStringLiteral("local") || !rule.matches(QDateTime::currentDateTime())) continue;
		const Source& source = rule.source;
		if (!validExternalUrl(source.url, source.kind) || !hasValidCredentialRef(source.credentialRef)) continue;
		const QString key = stateKey(rule);
		ExternalState& state = s_externalStates[key];
		if (state.inFlight || (!force && state.nextRefresh.isValid() && state.nextRefresh > now)) continue;
		QByteArray secret = readVaultCredential(source.credentialRef);
		if (secret.isEmpty()) {
			state.active = false;
			state.status = QStringLiteral("vault-credential-missing");
			state.nextRefresh = now.addSecs(source.refreshSeconds);
			if (changed) changed();
			continue;
		}
		QUrl requestUrl(source.url);
		if (source.kind == QStringLiteral("home-assistant")) {
			requestUrl.setPath(QStringLiteral("/api/states/") + QString::fromLatin1(QUrl::toPercentEncoding(source.entityId)));
		}
		QNetworkRequest request(requestUrl);
		request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
		request.setRawHeader("Authorization", QByteArray("Bearer ") + secret);
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
		secret.fill('\0');
		state.inFlight = true;
		state.active = false;
		state.status = QStringLiteral("external-source-refreshing");
		QNetworkAccessManager* manager = new QNetworkAccessManager(QCoreApplication::instance());
		QNetworkReply* reply = manager->get(request);
		QTimer* timeout = new QTimer(reply);
		timeout->setSingleShot(true);
		QObject::connect(timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
		timeout->start(kExternalTimeoutMs);
		QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError>&) { reply->abort(); });
		QObject::connect(reply, &QNetworkReply::finished, reply, [settings, rule, key, manager, reply, changed]() {
			ExternalState& finished = s_externalStates[key];
			finished.inFlight = false;
			finished.active = false;
			finished.nextRefresh = QDateTime::currentDateTimeUtc().addSecs(rule.source.refreshSeconds);
			const QByteArray body = reply->readAll();
			const bool redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid();
			const QByteArray contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray().toLower();
			if (reply->error() == QNetworkReply::NoError && !redirect && body.size() <= kExternalResponseLimit && contentType.startsWith("application/json")) {
				QJsonParseError error;
				const QJsonDocument document = QJsonDocument::fromJson(body, &error);
				if (error.error == QJsonParseError::NoError && document.isObject()) {
					const QJsonObject object = document.object();
					if (rule.source.kind == QStringLiteral("home-assistant")) {
						const QString value = object.value(QStringLiteral("state")).toString();
						finished.active = value == QStringLiteral("on");
						finished.status = finished.active ? QStringLiteral("external-source-active") : QStringLiteral("external-source-off");
					} else if (object.size() == 1 && object.value(QStringLiteral("active")).isBool()) {
						finished.active = object.value(QStringLiteral("active")).toBool();
						finished.status = finished.active ? QStringLiteral("external-source-active") : QStringLiteral("external-source-off");
					}
				}
			}
			if (!finished.active && finished.status == QStringLiteral("external-source-refreshing")) finished.status = QStringLiteral("external-source-failed");
			reply->deleteLater();
			manager->deleteLater();
			apply(settings);
			if (changed) changed();
		});
	}
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
	if (rule.source.kind != QStringLiteral("local")) {
		refreshExternalSources(settings);
		const auto state = s_externalStates.constFind(stateKey(rule));
		// A missing/off/failed source never applies a partial or stale override.
		if (state == s_externalStates.cend() || !state->active) return;
	}
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
