#include "stdafx.h"
#include "DocumentationBrowser.h"

#include <QClipboard>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QString markdownToHtml(const QString& markdown)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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

    QTextBrowser* changelog = new QTextBrowser(m_tabs);
    QFile changelogFile(QStringLiteral(":/Docs/changelog.md"));
    if (changelogFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString markdown = QString::fromUtf8(changelogFile.readAll());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        changelog->setMarkdown(markdown);
#else
        changelog->setHtml(markdownToHtml(markdown));
#endif
    }
    m_tabs->addTab(changelog, tr("Changelog"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_tabs);
    setLayout(layout);

    connect(m_search, &QLineEdit::textChanged, this, &CDocumentationBrowser::filterArticles);
    connect(m_articleList, &QListWidget::currentRowChanged, this, &CDocumentationBrowser::showArticle);
    loadArticles();
}

void CDocumentationBrowser::loadArticles()
{
    const QStringList paths = { QStringLiteral(":/Docs/material-design.md"), QStringLiteral(":/Docs/contributor-build.md") };
    for (const QString& path : paths) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        Article article;
        article.slug = QFileInfo(path).baseName();
        article.body = QString::fromUtf8(file.readAll());
        const QStringList lines = article.body.split('\n');
        article.title = lines.value(0).remove('#').trimmed();
        m_articles.append(article);
        m_articleList->addItem(article.title);
    }
    if (!m_articles.isEmpty())
        m_articleList->setCurrentRow(0);
    m_status->setText(tr("%1 bundled articles · offline and searchable").arg(m_articles.size()));
}

void CDocumentationBrowser::filterArticles()
{
    const QString query = m_search->text();
    QRegularExpression expression = m_searchExpression;
    if (expression.pattern().isEmpty())
        expression = QRegularExpression(QRegularExpression::escape(query), QRegularExpression::CaseInsensitiveOption);
    for (int i = 0; i < m_articles.size(); ++i)
        m_articleList->item(i)->setHidden(!query.isEmpty() && !expression.match(m_articles.at(i).title + '\n' + m_articles.at(i).body).hasMatch());
    m_status->setText(query.isEmpty() ? tr("%1 bundled articles · offline and searchable").arg(m_articles.size()) : tr("Search applied to article titles and body text"));
}

void CDocumentationBrowser::showArticle(int row)
{
    if (row >= 0 && row < m_articles.size())
        renderArticle(m_articles.at(row));
}

void CDocumentationBrowser::renderArticle(const Article& article)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_view->setMarkdown(article.body);
#else
    m_view->setHtml(markdownToHtml(article.body));
#endif
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
    form->addRow(tr("Pattern"), pattern);
    form->addRow(tr("Flags"), flags);
    form->addRow(tr("Sample text"), sample);
    form->addRow(tr("Validation"), feedback);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, &dialog, [&]() {
        QRegularExpression::PatternOptions options = flags->text().contains('i') ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption;
        m_searchExpression = QRegularExpression(pattern->text().left(512), options);
        feedback->setText(m_searchExpression.isValid() ? tr("Valid · %1").arg(m_searchExpression.match(sample->text()).hasMatch() ? tr("sample matches") : tr("no sample match")) : m_searchExpression.errorString());
        if (m_searchExpression.isValid()) { m_search->setText(pattern->text()); filterArticles(); }
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
