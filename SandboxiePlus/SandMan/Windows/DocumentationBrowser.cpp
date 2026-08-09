#include "stdafx.h"
#include "DocumentationBrowser.h"
#include "M3DialogHost.h"

#include <QAccessible>
#include <QClipboard>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextStream>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {
constexpr int kMaximumSearchPatternLength = 512;
constexpr int kMaximumSearchSampleLength = 4096;
constexpr int kMaximumSearchableDocumentLength = 512 * 1024;

QRegularExpression boundedSearchExpression(const QString& pattern, QRegularExpression::PatternOptions options)
{
    if (pattern.isEmpty())
        return QRegularExpression();
    const QString boundedPattern = QStringLiteral("(*LIMIT_MATCH=100000)(*LIMIT_DEPTH=1000)(?:%1)")
        .arg(pattern.left(kMaximumSearchPatternLength));
    return QRegularExpression(boundedPattern, options);
}

QString boundedSearchText(const QString& text)
{
    return text.left(kMaximumSearchableDocumentLength);
}

QString markdownToHtml(const QString& markdown)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return markdown;
#else
    QString html = markdown.toHtmlEscaped();
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");
    html.replace(QRegularExpression("\\[([^]]+)\\]\\(([^)]+)\\)"), "<a href=\\\"\\2\\\">\\1</a>");
    return QStringLiteral("<p>") + html.replace("\n\n", "</p><p>").replace("\n", "<br>") + QStringLiteral("</p>");
#endif
}
}

CDocumentationBrowser::CDocumentationBrowser(QWidget* parent)
    : QDialog(parent), m_tabs(new QTabWidget(this)), m_articleList(new QListWidget(this)),
      m_search(new QLineEdit(this)), m_view(new QTextBrowser(this)), m_status(new QLabel(this))
{
    setWindowTitle(tr("Offline Documentation"));
    resize(900, 620);
    m_search->setPlaceholderText(tr("Search documentation (plain text by default)"));
    m_search->setAccessibleName(tr("Documentation search"));
    m_search->setMaxLength(kMaximumSearchPatternLength);
    m_articleList->setAccessibleName(tr("Bundled documentation articles"));
    m_articleList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setAccessibleName(tr("Offline documentation article"));
    m_status->setAccessibleName(tr("Documentation status"));
    m_view->setOpenExternalLinks(false);
    m_view->setOpenLinks(false);

    QWidget* docsPage = new QWidget(m_tabs);
    QVBoxLayout* docsLayout = new QVBoxLayout(docsPage);
    QHBoxLayout* searchRow = new QHBoxLayout();
    searchRow->addWidget(m_search, 1);
    QToolButton* regex = new QToolButton(docsPage);
    regex->setText(tr("Regex…"));
    regex->setToolTip(tr("Open the full regex builder for this documentation search"));
    connect(regex, &QToolButton::clicked, this, &CDocumentationBrowser::openRegexBuilder);
    searchRow->addWidget(regex);
    QPushButton* copy = new QPushButton(tr("Copy"), docsPage);
    QPushButton* exportButton = new QPushButton(tr("Export…"), docsPage);
    connect(copy, &QPushButton::clicked, this, &CDocumentationBrowser::copyCurrentArticle);
    connect(exportButton, &QPushButton::clicked, this, &CDocumentationBrowser::exportCurrentArticle);
    searchRow->addWidget(copy);
    searchRow->addWidget(exportButton);
    docsLayout->addLayout(searchRow);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, docsPage);
    splitter->addWidget(m_articleList);
    splitter->addWidget(m_view);
    splitter->setStretchFactor(1, 1);
    docsLayout->addWidget(splitter, 1);
    docsLayout->addWidget(m_status);
    m_tabs->addTab(docsPage, tr("Articles"));

    QWidget* changelogPage = new QWidget(m_tabs);
    QVBoxLayout* changelogLayout = new QVBoxLayout(changelogPage);
    QHBoxLayout* changelogSearchRow = new QHBoxLayout();
    m_changelogSearch = new QLineEdit(changelogPage);
    m_changelogSearch->setPlaceholderText(tr("Search changelog (plain text by default)"));
    m_changelogSearch->setAccessibleName(tr("Changelog search"));
    m_changelogSearch->setMaxLength(kMaximumSearchPatternLength);
    changelogSearchRow->addWidget(m_changelogSearch, 1);
    QToolButton* changelogRegex = new QToolButton(changelogPage);
    changelogRegex->setText(tr("Regex…"));
    changelogRegex->setToolTip(tr("Open the full regex builder for this changelog search"));
    connect(changelogRegex, &QToolButton::clicked, this, &CDocumentationBrowser::openChangelogRegexBuilder);
    changelogSearchRow->addWidget(changelogRegex);
    QPushButton* changelogCopy = new QPushButton(tr("Copy filtered"), changelogPage);
    QPushButton* changelogExport = new QPushButton(tr("Export filtered…"), changelogPage);
    connect(changelogCopy, &QPushButton::clicked, this, &CDocumentationBrowser::copyFilteredChangelog);
    connect(changelogExport, &QPushButton::clicked, this, &CDocumentationBrowser::exportFilteredChangelog);
    changelogSearchRow->addWidget(changelogCopy);
    changelogSearchRow->addWidget(changelogExport);
    changelogLayout->addLayout(changelogSearchRow);
    QHBoxLayout* dateRow = new QHBoxLayout();
    m_changelogFrom = new QLineEdit(changelogPage); m_changelogFrom->setPlaceholderText(tr("From YYYY-MM-DD"));
    m_changelogTo = new QLineEdit(changelogPage); m_changelogTo->setPlaceholderText(tr("To YYYY-MM-DD"));
    m_changelogFrom->setAccessibleName(tr("Changelog start date")); m_changelogTo->setAccessibleName(tr("Changelog end date"));
    m_changelogFrom->setMaxLength(10); m_changelogTo->setMaxLength(10);
    dateRow->addWidget(m_changelogFrom); dateRow->addWidget(m_changelogTo);
    m_changelogDateError = new QLabel(changelogPage); m_changelogDateError->setStyleSheet(QStringLiteral("color:#b3261e"));
    m_changelogDateError->setAccessibleName(tr("Changelog date validation"));
    dateRow->addWidget(m_changelogDateError, 1);
    changelogLayout->addLayout(dateRow);
    m_changelogView = new QTextBrowser(changelogPage);
    m_changelogView->setAccessibleName(tr("Filtered changelog entries"));
    m_changelogView->setOpenExternalLinks(false);
    m_changelogView->setOpenLinks(false);
    connect(m_changelogView, &QTextBrowser::anchorClicked, this, &CDocumentationBrowser::handleDocumentationLink);
    changelogLayout->addWidget(m_changelogView, 1);
    m_changelogStatus = new QLabel(changelogPage);
    m_changelogStatus->setAccessibleName(tr("Changelog status"));
    changelogLayout->addWidget(m_changelogStatus);
    QFile changelogFile(QStringLiteral(":/Docs/changelog.md"));
    if (changelogFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString markdown = QString::fromUtf8(changelogFile.readAll());
        const QStringList sections = markdown.split(QRegularExpression(QStringLiteral("(?=^## )")), Qt::SkipEmptyParts);
        for (const QString& section : sections) {
            const QStringList lines = section.split('\n');
            if (lines.isEmpty() || !lines.first().startsWith(QStringLiteral("## "))) continue;
            ChangelogEntry entry; entry.title = lines.first().mid(3).trimmed(); entry.body = section;
            QRegularExpression dateRe(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2})"));
            const QRegularExpressionMatch dm = dateRe.match(entry.title); if (dm.hasMatch()) entry.date = QDate::fromString(dm.captured(1), Qt::ISODate);
            QRegularExpression shaRe(QStringLiteral("/commit/([0-9a-f]{40})")); const QRegularExpressionMatch sm = shaRe.match(section); if (sm.hasMatch()) entry.sha = sm.captured(1);
            m_changelogEntries.append(entry);
        }
    }
    m_tabs->addTab(changelogPage, tr("Changelog"));
    connect(m_changelogSearch, &QLineEdit::textEdited, this, [this]() { m_changelogExpression = QRegularExpression(); });
    connect(m_changelogSearch, &QLineEdit::textChanged, this, &CDocumentationBrowser::filterChangelog);
    connect(m_changelogFrom, &QLineEdit::textChanged, this, &CDocumentationBrowser::filterChangelog);
    connect(m_changelogTo, &QLineEdit::textChanged, this, &CDocumentationBrowser::filterChangelog);
    renderChangelog();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_tabs);
    setLayout(layout);

    connect(m_search, &QLineEdit::textEdited, this, [this]() { m_searchExpression = QRegularExpression(); });
    connect(m_search, &QLineEdit::textChanged, this, &CDocumentationBrowser::filterArticles);
    connect(m_articleList, &QListWidget::currentRowChanged, this, &CDocumentationBrowser::showArticle);
    connect(m_view, &QTextBrowser::anchorClicked, this, &CDocumentationBrowser::handleDocumentationLink);
    loadArticles();
    M3DialogHost::Install(this);
}

void CDocumentationBrowser::loadArticles()
{
    QFile manifestFile(QStringLiteral(":/Docs/articles/index.json"));
    if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setArticleStatus(tr("Offline documentation inventory is unavailable."));
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setArticleStatus(tr("Offline documentation inventory is invalid: %1").arg(parseError.errorString()));
        return;
    }

    const QJsonObject manifest = document.object();
    if (manifest.value(QStringLiteral("version")).toInt() != 1) {
        setArticleStatus(tr("Offline documentation inventory uses an unsupported version."));
        return;
    }

    QVector<Article> articles;
    QHash<QString, int> articlesBySourcePath;
    QHash<QString, int> articlesBySlug;
    const QRegularExpression slugPattern(QStringLiteral("^[a-z0-9]+(?:-[a-z0-9]+)*$"));
    QString inventoryError;

    const auto appendInventory = [&](const QJsonArray& inventory, const QString& resourceGroup, bool supplemental) {
        for (const QJsonValue& value : inventory) {
            const QJsonObject record = value.toObject();
            const QString slug = record.value(QStringLiteral("slug")).toString();
            const QString relativePath = record.value(QStringLiteral("path")).toString();
            const QString title = record.value(QStringLiteral("title")).toString().trimmed();
            const QString sourcePath = QDir::cleanPath(QStringLiteral("articles/") + relativePath);
            if (!slugPattern.match(slug).hasMatch() || relativePath.isEmpty() || title.isEmpty() || QDir::isAbsolutePath(relativePath)
                || sourcePath == QStringLiteral("..") || sourcePath.startsWith(QStringLiteral("../"))
                || articlesBySlug.contains(slug) || articlesBySourcePath.contains(sourcePath)) {
                inventoryError = tr("Offline documentation inventory contains an unsafe or duplicate record.");
                return false;
            }

            const QString resourcePath = QStringLiteral(":/Docs/%1/%2.md").arg(resourceGroup, slug);
            QFile articleFile(resourcePath);
            if (!articleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                inventoryError = tr("Bundled documentation article is missing: %1").arg(slug);
                return false;
            }

            Article article;
            article.slug = slug;
            article.sourcePath = sourcePath;
            article.body = QString::fromUtf8(articleFile.readAll());
            article.supplemental = supplemental;
            const QRegularExpressionMatch titleMatch = QRegularExpression(
                QStringLiteral("^#\\s+(.+)$"), QRegularExpression::MultilineOption).match(article.body);
            if (!titleMatch.hasMatch() || titleMatch.captured(1).trimmed() != title) {
                inventoryError = tr("Bundled documentation title does not match the inventory: %1").arg(slug);
                return false;
            }
            article.title = title;

            const int index = articles.size();
            articlesBySlug.insert(slug, index);
            articlesBySourcePath.insert(sourcePath, index);
            articles.append(article);
        }
        return true;
    };

    const QJsonArray featureInventory = manifest.value(QStringLiteral("articles")).toArray();
    const QJsonArray supplementalInventory = manifest.value(QStringLiteral("supplemental")).toArray();
    if (featureInventory.isEmpty()
        || !appendInventory(featureInventory, QStringLiteral("articles"), false)
        || !appendInventory(supplementalInventory, QStringLiteral("supplemental"), true)) {
        setArticleStatus(inventoryError.isEmpty() ? tr("Offline documentation inventory has no feature articles.") : inventoryError);
        return;
    }

    m_articles = articles;
    m_articleBySourcePath = articlesBySourcePath;
    m_articleBySlug = articlesBySlug;
    m_featureArticleCount = featureInventory.size();
    m_supplementalArticleCount = supplementalInventory.size();
    for (const Article& article : m_articles)
        m_articleList->addItem(article.supplemental ? tr("%1 (supplemental)").arg(article.title) : article.title);
    m_articleList->setCurrentRow(0);
    setArticleStatus(tr("%1 feature articles + %2 supplemental documents · offline and searchable")
        .arg(m_featureArticleCount).arg(m_supplementalArticleCount));
}

void CDocumentationBrowser::filterArticles()
{
    const QString query = m_search->text().left(kMaximumSearchPatternLength);
    QRegularExpression expression = m_searchExpression;
    if (expression.pattern().isEmpty() && !query.isEmpty())
        expression = boundedSearchExpression(QRegularExpression::escape(query), QRegularExpression::CaseInsensitiveOption);

    int visibleCount = 0;
    int firstVisible = -1;
    for (int i = 0; i < m_articles.size(); ++i) {
        const bool visible = query.isEmpty() || expression.match(boundedSearchText(
            m_articles.at(i).title + '\n' + m_articles.at(i).body)).hasMatch();
        m_articleList->item(i)->setHidden(!visible);
        if (visible) {
            ++visibleCount;
            if (firstVisible < 0)
                firstVisible = i;
        }
    }

    if (visibleCount == 0) {
        m_currentArticleIndex = -1;
        m_articleList->setCurrentRow(-1);
        m_view->setHtml(tr("<p>No documentation articles match the active search.</p>"));
    } else if (m_articleList->currentRow() < 0 || m_articleList->item(m_articleList->currentRow())->isHidden()) {
        m_articleList->setCurrentRow(firstVisible);
    }
    setArticleStatus(tr("%1 of %2 bundled documents match").arg(visibleCount).arg(m_articles.size()));
}

void CDocumentationBrowser::showArticle(int row)
{
    if (row >= 0 && row < m_articles.size()) {
        m_currentArticleIndex = row;
        renderArticle(m_articles.at(row));
    }
}

void CDocumentationBrowser::renderArticle(const Article& article)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    m_view->document()->setMarkdown(article.body,
        QTextDocument::MarkdownFeatures{QTextDocument::MarkdownDialectGitHub, QTextDocument::MarkdownNoHTML});
#else
    m_view->setHtml(markdownToHtml(article.body));
#endif
}

bool CDocumentationBrowser::openArticle(const QString& slug)
{
    const auto target = m_articleBySlug.constFind(slug);
    if (target == m_articleBySlug.constEnd()) {
        setArticleStatus(tr("The requested documentation article is not bundled: %1").arg(slug));
        return false;
    }
    if (m_articleList->item(target.value())->isHidden())
        m_search->clear();
    m_tabs->setCurrentIndex(0);
    const bool alreadySelected = m_currentArticleIndex == target.value();
    m_articleList->setCurrentRow(target.value());
    if (alreadySelected)
        renderArticle(m_articles.at(target.value()));
    m_view->setFocus(Qt::OtherFocusReason);
    return true;
}

void CDocumentationBrowser::setArticleStatus(const QString& text)
{
    m_status->setText(text);
    QAccessibleEvent event(m_status, QAccessible::ValueChanged);
    QAccessible::updateAccessibility(&event);
}

void CDocumentationBrowser::handleDocumentationLink(const QUrl& url)
{
    QTextBrowser* sourceView = qobject_cast<QTextBrowser*>(sender());
    const auto setLinkStatus = [this, sourceView](const QString& text) {
        if (sourceView == m_changelogView && m_changelogStatus) {
            m_changelogStatus->setText(text);
            QAccessibleEvent event(m_changelogStatus, QAccessible::ValueChanged);
            QAccessible::updateAccessibility(&event);
        } else {
            setArticleStatus(text);
        }
    };
    if (url.isRelative() || url.scheme().isEmpty()) {
        if (url.path().isEmpty() && !url.fragment().isEmpty()) {
            if (sourceView)
                sourceView->scrollToAnchor(url.fragment());
            return;
        }

        if (sourceView != m_view || m_currentArticleIndex < 0 || m_currentArticleIndex >= m_articles.size()) {
            setLinkStatus(tr("This local documentation link is not available in the current view."));
            return;
        }

        const QString currentDirectory = QFileInfo(m_articles.at(m_currentArticleIndex).sourcePath).path();
        const QString targetPath = QDir::cleanPath(QDir(currentDirectory).filePath(url.path()));
        if (targetPath == QStringLiteral("..") || targetPath.startsWith(QStringLiteral("../"))) {
            setLinkStatus(tr("Blocked a documentation link outside the bundled inventory."));
            return;
        }
        const auto target = m_articleBySourcePath.constFind(targetPath);
        if (target == m_articleBySourcePath.constEnd()) {
            setLinkStatus(tr("This local documentation destination is not bundled: %1").arg(targetPath));
            return;
        }

        if (m_articleList->item(target.value())->isHidden())
            m_search->clear();
        m_tabs->setCurrentIndex(0);
        const bool alreadySelected = m_currentArticleIndex == target.value();
        m_articleList->setCurrentRow(target.value());
        if (alreadySelected)
            renderArticle(m_articles.at(target.value()));
        if (!url.fragment().isEmpty())
            m_view->scrollToAnchor(url.fragment());
        return;
    }

    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("https") && url.isValid() && !url.host().isEmpty() && url.userInfo().isEmpty()) {
        if (!QDesktopServices::openUrl(url))
            setLinkStatus(tr("The external documentation link could not be opened."));
        return;
    }

    setLinkStatus(tr("Blocked an unsupported documentation link."));
}

void CDocumentationBrowser::openRegexBuilder()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Documentation regex builder"));
    QFormLayout* form = new QFormLayout(&dialog);
    QLineEdit* pattern = new QLineEdit(m_search->text(), &dialog);
    QLineEdit* flags = new QLineEdit(QStringLiteral("i"), &dialog);
    QLineEdit* sample = new QLineEdit(tr("Material theme"), &dialog);
    QLabel* feedback = new QLabel(&dialog);
    pattern->setMaxLength(kMaximumSearchPatternLength);
    flags->setMaxLength(8);
    sample->setMaxLength(kMaximumSearchSampleLength);
    form->addRow(tr("Pattern"), pattern);
    form->addRow(tr("Flags"), flags);
    form->addRow(tr("Sample text"), sample);
    form->addRow(tr("Validation"), feedback);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, &dialog, [&]() {
        QRegularExpression::PatternOptions options = flags->text().contains('i') ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption;
        const QString candidatePattern = pattern->text().left(kMaximumSearchPatternLength);
        const QRegularExpression candidate = boundedSearchExpression(candidatePattern, options);
        if (!candidatePattern.isEmpty() && !candidate.isValid()) {
            feedback->setText(candidate.errorString());
            return;
        }
        m_searchExpression = candidate;
        feedback->setText(tr("Valid · %1").arg(candidatePattern.isEmpty()
            || candidate.match(sample->text().left(kMaximumSearchSampleLength)).hasMatch() ? tr("sample matches") : tr("no sample match")));
        m_search->setText(candidatePattern);
        filterArticles();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}

void CDocumentationBrowser::copyCurrentArticle()
{
    if (m_articleList->currentRow() >= 0)
        QApplication::clipboard()->setText(m_articles.at(m_articleList->currentRow()).body);
}

void CDocumentationBrowser::exportCurrentArticle()
{
    if (m_articleList->currentRow() < 0)
        return;
    const QString path = QFileDialog::getSaveFileName(this, tr("Export article"), QString(), tr("Markdown (*.md)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        file.write(m_articles.at(m_articleList->currentRow()).body.toUtf8());
}

bool CDocumentationBrowser::changelogDateRangeValid(QDate* from, QDate* to)
{
    const QString fromText = m_changelogFrom->text().trimmed();
    const QString toText = m_changelogTo->text().trimmed();
    *from = fromText.isEmpty() ? QDate() : QDate::fromString(fromText, Qt::ISODate);
    *to = toText.isEmpty() ? QDate() : QDate::fromString(toText, Qt::ISODate);
    if ((!fromText.isEmpty() && !from->isValid()) || (!toText.isEmpty() && !to->isValid())) {
        m_changelogDateError->setText(tr("Use ISO dates like 2026-08-09")); return false;
    }
    if (from->isValid() && to->isValid() && *from > *to) {
        m_changelogDateError->setText(tr("Start date must not be after end date")); return false;
    }
    m_changelogDateError->clear(); return true;
}

void CDocumentationBrowser::filterChangelog()
{
    renderChangelog();
}

void CDocumentationBrowser::renderChangelog()
{
    if (!m_changelogView) return;
    QDate from, to; if (!changelogDateRangeValid(&from, &to)) {
        m_changelogView->setHtml(tr("<p>Correct the date range to view changelog entries.</p>"));
        if (m_changelogStatus) m_changelogStatus->setText(tr("Changelog date filter is invalid"));
        return;
    }
    const QString query = m_changelogSearch->text().left(kMaximumSearchPatternLength);
    QRegularExpression expression = m_changelogExpression;
    if (expression.pattern().isEmpty() && !query.isEmpty())
        expression = boundedSearchExpression(QRegularExpression::escape(query), QRegularExpression::CaseInsensitiveOption);
    QString html;
    QString markdown;
    int shown = 0;
    for (const ChangelogEntry& entry : m_changelogEntries) {
        if (from.isValid() && entry.date.isValid() && entry.date < from) continue;
        if (to.isValid() && entry.date.isValid() && entry.date > to) continue;
        if (!query.isEmpty() && !expression.match(boundedSearchText(entry.title + '\n' + entry.body)).hasMatch()) continue;
        QString rendered = markdownToHtml(entry.body);
        html += rendered + QStringLiteral("<hr>");
        markdown += entry.body + QStringLiteral("\n\n---\n\n");
        ++shown;
    }
    if (m_changelogStatus)
        m_changelogStatus->setText(tr("%1 of %2 changelog entries match").arg(shown).arg(m_changelogEntries.size()));
    if (shown == 0) html = tr("<p>No changelog entries match the active filters.</p>");
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (shown > 0) {
        m_changelogView->document()->setMarkdown(markdown,
            QTextDocument::MarkdownFeatures{QTextDocument::MarkdownDialectGitHub, QTextDocument::MarkdownNoHTML});
        return;
    }
#endif
    m_changelogView->setHtml(html);
}

void CDocumentationBrowser::openChangelogRegexBuilder()
{
    QDialog dialog(this); dialog.setWindowTitle(tr("Changelog regex builder"));
    QFormLayout* form = new QFormLayout(&dialog); QLineEdit* pattern = new QLineEdit(m_changelogSearch->text(), &dialog);
    QLineEdit* flags = new QLineEdit(QStringLiteral("i"), &dialog); QLineEdit* sample = new QLineEdit(tr("Material"), &dialog); QLabel* feedback = new QLabel(&dialog);
    pattern->setMaxLength(kMaximumSearchPatternLength); flags->setMaxLength(8); sample->setMaxLength(kMaximumSearchSampleLength);
    form->addRow(tr("Pattern"), pattern); form->addRow(tr("Flags"), flags); form->addRow(tr("Sample text"), sample); form->addRow(tr("Validation"), feedback);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, &dialog); form->addRow(buttons);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, &dialog, [&]() {
        QRegularExpression::PatternOptions options = flags->text().contains('i') ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption;
        const QString candidatePattern = pattern->text().left(kMaximumSearchPatternLength);
        const QRegularExpression candidate = boundedSearchExpression(candidatePattern, options);
        if (!candidatePattern.isEmpty() && !candidate.isValid()) { feedback->setText(candidate.errorString()); return; }
        m_changelogExpression = candidate;
        m_changelogSearch->setText(candidatePattern);
        feedback->setText(tr("Valid · %1").arg(candidatePattern.isEmpty()
            || candidate.match(sample->text().left(kMaximumSearchSampleLength)).hasMatch() ? tr("sample matches") : tr("no sample match")));
        renderChangelog();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject); dialog.exec();
}

void CDocumentationBrowser::copyFilteredChangelog()
{
    if (m_changelogView) QApplication::clipboard()->setText(m_changelogView->toPlainText());
}

void CDocumentationBrowser::exportFilteredChangelog()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Export filtered changelog"), QString(), tr("Markdown (*.md);;Plain text (*.txt)")); if (path.isEmpty()) return;
    QFile file(path); if (file.open(QIODevice::WriteOnly | QIODevice::Text)) file.write(m_changelogView->toPlainText().toUtf8());
}
