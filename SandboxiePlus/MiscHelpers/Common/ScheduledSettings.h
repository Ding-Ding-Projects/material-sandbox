#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QTime>
#include <QString>
#include <QStringList>

#include <functional>

#include "Settings.h"

// A deliberately local, offline scheduler for appearance/presentation values.
// The schema is versioned so future settings can migrate without guessing.
namespace ScheduledSettings {

struct Source
{
	// schema-v1 source metadata. External sources are validated and retained,
	// but are not fetched/applied until a credential-vault integration exists.
	QString kind = QStringLiteral("local"); // local, https-api, home-assistant
	QString url; // HTTPS endpoint for https-api/home-assistant
	QString entityId; // binary_sensor.* or input_boolean.* for Home Assistant
	QString credentialRef; // opaque OS credential-vault reference; never a token
	int refreshSeconds = 300;
};

struct Rule
{
	QString id;
	QString label;
	bool enabled = true;
	int priority = 0;
	QDate startDate;
	QDate endDate;
	QTime startTime = QTime(0, 0);
	QTime endTime = QTime(23, 59, 59);
	QList<int> weekdays; // Qt day numbers: 1=Monday ... 7=Sunday; empty means every day.
	QMap<QString, QString> values; // allowlisted presentation/appearance keys only
	Source source;

	bool matches(const QDateTime& local) const;
};

QList<Rule> load(CSettings* settings);
bool save(CSettings* settings, const QList<Rule>& rules, QString* error = nullptr);
QStringList validate(const Rule& rule);

// External sources use an opaque Windows Credential Manager reference. The
// schedule never contains, displays, or exports the credential value.
QString sourceStatus(const Source& source);
QString sourceStatusDescription(const Source& source);

// Starts only bounded, validated external refreshes. Results remain in memory:
// an "on" response gates that rule's already-local allowlisted values; every
// error, timeout, redirect, malformed response, or missing credential fails
// closed to the base settings. The callback contains no source or credential
// data and is suitable for refreshing a status-only UI.
void refreshExternalSources(CSettings* settings, bool force = false, const std::function<void()>& changed = {});

// Returns the highest-priority matching rule. Ties are deterministic by id.
Rule effectiveRule(CSettings* settings, const QDateTime& local = QDateTime::currentDateTime());

// Applies only the allowlisted local values. School mode remains the final
// presentation gate: a schedule never re-enables Cantonese or funny controls.
void apply(CSettings* settings, const QDateTime& local = QDateTime::currentDateTime());

QString valueLabel(const QString& key);
QStringList valueKeys();

} // namespace ScheduledSettings
