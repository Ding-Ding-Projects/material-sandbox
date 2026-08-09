#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QTime>
#include <QString>
#include <QStringList>

#include "Settings.h"

// A deliberately local, offline scheduler for appearance/presentation values.
// The schema is versioned so future settings can migrate without guessing.
namespace ScheduledSettings {

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

	bool matches(const QDateTime& local) const;
};

QList<Rule> load(CSettings* settings);
bool save(CSettings* settings, const QList<Rule>& rules, QString* error = nullptr);
QStringList validate(const Rule& rule);

// Returns the highest-priority matching rule. Ties are deterministic by id.
Rule effectiveRule(CSettings* settings, const QDateTime& local = QDateTime::currentDateTime());

// Applies only the allowlisted local values. School mode remains the final
// presentation gate: a schedule never re-enables Cantonese or funny controls.
void apply(CSettings* settings, const QDateTime& local = QDateTime::currentDateTime());

QString valueLabel(const QString& key);
QStringList valueKeys();

} // namespace ScheduledSettings
