#include "stdafx.h"
#include "NotificationCenter.h"

#include "../../MiscHelpers/Common/Settings.h"
#include <QDateTime>
#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QLocale>
#include <QListWidget>
#include <QPushButton>
#include <QSaveFile>
#include <QVBoxLayout>
#include <QTextStream>

namespace {

QString escapeNotificationMarkdown(QString value)
{
    value.replace(QChar('\\'), QStringLiteral("\\\\"));
    value.replace(QChar('\n'), QChar(' '));
    value.replace(QChar('*'), QStringLiteral("\\*"));
    value.replace(QChar('['), QStringLiteral("\\["));
    return value;
}

}

CNotificationCenter::CNotificationCenter(CSettings* settings, QWidget* parent)
    : QWidget(parent), m_settings(settings), m_key(QStringLiteral("UIConfig/NotificationHistory"))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* filterLayout = new QHBoxLayout();
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Search notifications"));
    m_filter->setAccessibleName(tr("Notification search"));
    filterLayout->addWidget(m_filter, 1);
    m_regexButton = new QPushButton(tr("Regex…"), this);
    m_regexButton->setToolTip(tr("Enable a bounded regular-expression search for this notification history."));
    filterLayout->addWidget(m_regexButton);
    layout->addLayout(filterLayout);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_list->setAccessibleName(tr("Notification history"));
    layout->addWidget(m_list, 1);

    QHBoxLayout* actions = new QHBoxLayout();
    m_dismissSelected = new QPushButton(tr("Dismiss selected"), this);
    m_dismissAll = new QPushButton(tr("Dismiss all"), this);
    QPushButton* clear = new QPushButton(tr("Clear history"), this);
    QPushButton* exportJsonButton = new QPushButton(tr("Export JSON"), this);
    QPushButton* exportMarkdownButton = new QPushButton(tr("Export Markdown"), this);
    actions->addWidget(m_dismissSelected);
    actions->addWidget(m_dismissAll);
    actions->addWidget(clear);
    actions->addWidget(exportJsonButton);
    actions->addWidget(exportMarkdownButton);
    actions->addStretch(1);
    layout->addLayout(actions);

    connect(m_filter, &QLineEdit::textChanged, this, &CNotificationCenter::applyFilter);
    connect(m_regexButton, &QPushButton::clicked, this, &CNotificationCenter::openRegexBuilder);
    connect(m_dismissSelected, &QPushButton::clicked, this, &CNotificationCenter::dismissSelected);
    connect(m_dismissAll, &QPushButton::clicked, this, &CNotificationCenter::dismissAll);
    connect(clear, &QPushButton::clicked, this, &CNotificationCenter::clearHistory);
    connect(exportJsonButton, &QPushButton::clicked, this, &CNotificationCenter::exportJson);
    connect(exportMarkdownButton, &QPushButton::clicked, this, &CNotificationCenter::exportMarkdown);
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item && item->data(Qt::UserRole + 1).toString().startsWith(QStringLiteral("http")))
            emit activated(item->data(Qt::UserRole + 1).toString());
    });
    load();
}

int CNotificationCenter::count() const
{
    return m_list ? m_list->count() : 0;
}

void CNotificationCenter::addItem(Severity severity, const QString& title, const QString& body,
    const QString& link, const QDateTime& timestamp)
{
    QListWidgetItem* item = new QListWidgetItem(QStringLiteral("%1 · %2\n%3")
        .arg(QLocale().toString(timestamp.toLocalTime(), QLocale::ShortFormat), title, body), m_list);
    item->setData(Qt::UserRole, static_cast<int>(severity));
    item->setData(Qt::UserRole + 1, link);
    item->setData(Qt::UserRole + 2, title);
    item->setData(Qt::UserRole + 3, body);
    item->setData(Qt::UserRole + 4, timestamp.toUTC().toString(Qt::ISODate));
    item->setToolTip(link.isEmpty() ? body : tr("Double-click to open: %1").arg(link));
}

void CNotificationCenter::post(Severity severity, const QString& title, const QString& body, const QString& link)
{
    addItem(severity, title, body, link, QDateTime::currentDateTime());
    while (m_list->count() > 100)
        delete m_list->takeItem(0);
    save();
    m_list->scrollToBottom();
}

void CNotificationCenter::dismissSelected()
{
    for (QListWidgetItem* item : m_list->selectedItems())
        delete m_list->takeItem(m_list->row(item));
    save();
}

void CNotificationCenter::dismissAll()
{
    m_list->clear();
    save();
}

void CNotificationCenter::clearHistory()
{
    dismissAll();
}

void CNotificationCenter::exportJson()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Export notification history"), QString(), tr("JSON files (*.json)"));
    if (path.isEmpty()) return;
    QJsonArray array;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        if (item->isHidden()) continue;
        QJsonObject object;
        object.insert(QStringLiteral("severity"), item->data(Qt::UserRole).toInt());
        object.insert(QStringLiteral("title"), item->data(Qt::UserRole + 2).toString());
        object.insert(QStringLiteral("body"), item->data(Qt::UserRole + 3).toString());
        object.insert(QStringLiteral("link"), item->data(Qt::UserRole + 1).toString());
        object.insert(QStringLiteral("timestamp"), item->data(Qt::UserRole + 4).toString());
        array.append(object);
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    const QByteArray payload = QJsonDocument(array).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size())
        return;
    file.commit();
}

void CNotificationCenter::exportMarkdown()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Export notification history"), QString(), tr("Markdown files (*.md)"));
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream stream(&file);
    QString search = m_filter->text();
    search.replace(QChar('`'), QChar('\''));
    stream << QStringLiteral("# Notification history\n\nSearch: `") << search << QStringLiteral("`\n\n");
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        if (item->isHidden()) continue;
        stream << QStringLiteral("- **") << escapeNotificationMarkdown(item->data(Qt::UserRole + 2).toString())
            << QStringLiteral("** — ") << escapeNotificationMarkdown(item->data(Qt::UserRole + 3).toString())
            << QStringLiteral(" (") << item->data(Qt::UserRole + 4).toString() << QStringLiteral(")\n");
    }
    stream.flush();
    file.commit();
}

void CNotificationCenter::applyFilter()
{
    const QString query = m_filter->text();
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        const bool match = query.isEmpty() || (m_regexEnabled ? m_regex.match(item->text()).hasMatch() : item->text().contains(query, Qt::CaseInsensitive));
        item->setHidden(!match);
    }
}

void CNotificationCenter::openRegexBuilder()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Notification search regex"));
    QFormLayout form(&dialog);
    QLineEdit pattern(&dialog);
    pattern.setText(m_regexEnabled ? m_regex.pattern() : m_filter->text());
    pattern.setMaxLength(512);
    pattern.setAccessibleName(tr("Regular-expression pattern"));
    QLineEdit flags(&dialog);
    flags.setText(m_regexFlags);
    flags.setMaxLength(8);
    flags.setPlaceholderText(QStringLiteral("imxsU"));
    flags.setAccessibleName(tr("Regular-expression flags"));
    QPlainTextEdit sample(&dialog);
    sample.setPlainText(m_filter->text());
    sample.setMaximumHeight(72);
    sample.setAccessibleName(tr("Regular-expression sample text"));
    QLabel status(&dialog);
    status.setWordWrap(true);
    QLabel help(tr("Qt QRegularExpression; plain text remains the default."), &dialog);
    help.setWordWrap(true);
    form.addRow(tr("Pattern"), &pattern);
    form.addRow(tr("Flags"), &flags);
    form.addRow(tr("Sample text"), &sample);
    form.addRow(QString(), &help);
    form.addRow(tr("Validation"), &status);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(QString(), &buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto updateStatus = [&]() {
        QRegularExpression::PatternOptions options;
        const QString value = flags.text();
        bool validFlags = true;
        for (const QChar flag : value) {
            if (flag == QLatin1Char('i')) options |= QRegularExpression::CaseInsensitiveOption;
            else if (flag == QLatin1Char('m')) options |= QRegularExpression::MultilineOption;
            else if (flag == QLatin1Char('s')) options |= QRegularExpression::DotMatchesEverythingOption;
            else if (flag == QLatin1Char('U')) options |= QRegularExpression::InvertedGreedinessOption;
            else if (flag == QLatin1Char('x')) options |= QRegularExpression::ExtendedPatternSyntaxOption;
            else validFlags = false;
        }
        const QRegularExpression candidate(pattern.text(), options);
        if (!validFlags || !candidate.isValid()) {
            status.setText(tr("Invalid pattern or flags: %1").arg(!validFlags ? tr("use only i, m, s, x, U") : candidate.errorString()));
            return;
        }
        int matches = 0;
        QRegularExpressionMatchIterator iterator = candidate.globalMatch(sample.toPlainText());
        while (iterator.hasNext()) { iterator.next(); ++matches; }
        status.setText(tr("Valid Qt QRegularExpression. Sample contains at least %1 match(es).").arg(matches));
    };
    connect(&pattern, &QLineEdit::textChanged, &dialog, updateStatus);
    connect(&flags, &QLineEdit::textChanged, &dialog, updateStatus);
    connect(&sample, &QPlainTextEdit::textChanged, &dialog, updateStatus);
    updateStatus();
    if (dialog.exec() != QDialog::Accepted)
        return;
    QRegularExpression::PatternOptions options;
    bool validFlags = true;
    for (const QChar flag : flags.text()) {
        if (flag == QLatin1Char('i')) options |= QRegularExpression::CaseInsensitiveOption;
        else if (flag == QLatin1Char('m')) options |= QRegularExpression::MultilineOption;
        else if (flag == QLatin1Char('s')) options |= QRegularExpression::DotMatchesEverythingOption;
        else if (flag == QLatin1Char('U')) options |= QRegularExpression::InvertedGreedinessOption;
        else if (flag == QLatin1Char('x')) options |= QRegularExpression::ExtendedPatternSyntaxOption;
        else validFlags = false;
    }
    const QRegularExpression candidate(pattern.text(), options);
    if (!validFlags || !candidate.isValid()) {
        m_regexEnabled = false;
        m_filter->setToolTip(candidate.errorString());
        applyFilter();
        return;
    }
    m_regex = candidate;
    m_regexFlags = flags.text();
    m_regexEnabled = !pattern.text().isEmpty();
    m_filter->setText(pattern.text());
    m_filter->setToolTip(m_regexEnabled ? tr("Regex mode enabled") : tr("Plain-text mode enabled"));
    applyFilter();
}

void CNotificationCenter::load()
{
    if (!m_settings)
        return;
    const QJsonDocument document = QJsonDocument::fromJson(m_settings->GetBlob(m_key));
    for (const QJsonValue& value : document.array()) {
        const QJsonObject object = value.toObject();
        addItem(static_cast<Severity>(object.value(QStringLiteral("severity")).toInt()),
            object.value(QStringLiteral("title")).toString(), object.value(QStringLiteral("body")).toString(),
            object.value(QStringLiteral("link")).toString(), QDateTime::fromString(object.value(QStringLiteral("timestamp")).toString(), Qt::ISODate));
    }
}

void CNotificationCenter::save() const
{
    if (!m_settings)
        return;
    QJsonArray array;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        QJsonObject object;
        object.insert(QStringLiteral("severity"), item->data(Qt::UserRole).toInt());
        object.insert(QStringLiteral("title"), item->data(Qt::UserRole + 2).toString());
        object.insert(QStringLiteral("body"), item->data(Qt::UserRole + 3).toString());
        object.insert(QStringLiteral("link"), item->data(Qt::UserRole + 1).toString());
        object.insert(QStringLiteral("timestamp"), item->data(Qt::UserRole + 4).toString());
        array.append(object);
    }
    m_settings->SetBlob(m_key, QJsonDocument(array).toJson(QJsonDocument::Compact));
}
