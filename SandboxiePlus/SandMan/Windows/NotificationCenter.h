#pragma once

#include <QRegularExpression>
#include <QDateTime>
#include <QString>
#include <QWidget>

class CSettings;
class QLineEdit;
class QListWidget;
class QPushButton;
class QPlainTextEdit;
class QLabel;

class CNotificationCenter : public QWidget
{
    Q_OBJECT
public:
    enum Severity { Info = 0, Success, Warning, Error };
    explicit CNotificationCenter(CSettings* settings, QWidget* parent = nullptr);
    void post(Severity severity, const QString& title, const QString& body, const QString& link = QString());
    int count() const;

public slots:
    void dismissSelected();
    void dismissAll();
    void clearHistory();
    void exportJson();
    void exportMarkdown();

signals:
    void activated(const QString& link);

private slots:
    void applyFilter();
    void openRegexBuilder();

private:
    void load();
    void save() const;
    void addItem(Severity severity, const QString& title, const QString& body, const QString& link, const QDateTime& timestamp);

    CSettings* m_settings;
    QString m_key;
    QLineEdit* m_filter;
    QListWidget* m_list;
    QPushButton* m_regexButton;
    QPushButton* m_dismissSelected;
    QPushButton* m_dismissAll;
    QString m_regexFlags;
    QRegularExpression m_regex;
    bool m_regexEnabled = false;
};
