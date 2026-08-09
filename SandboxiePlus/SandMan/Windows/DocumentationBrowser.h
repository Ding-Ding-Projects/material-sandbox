#pragma once

#include <QDialog>
#include <QRegularExpression>
#include <QVector>

class QListWidget;
class QLineEdit;
class QTextBrowser;
class QLabel;
class QTabWidget;
class QDateEdit;
class QDate;

class CDocumentationBrowser : public QDialog
{
    Q_OBJECT
public:
    explicit CDocumentationBrowser(QWidget* parent = nullptr);

private slots:
    void filterArticles();
    void showArticle(int row);
    void openRegexBuilder();
    void copyCurrentArticle();
    void exportCurrentArticle();
    void filterChangelog();
    void openChangelogRegexBuilder();
    void copyFilteredChangelog();
    void exportFilteredChangelog();

private:
    struct Article { QString slug; QString title; QString body; };
    struct ChangelogEntry { QString title; QString body; QDate date; QString sha; };
    void loadArticles();
    void renderArticle(const Article& article);
    void renderChangelog();
    bool changelogDateRangeValid(QDate* from, QDate* to);

    QTabWidget* m_tabs;
    QListWidget* m_articleList;
    QLineEdit* m_search;
    QTextBrowser* m_view;
    QLabel* m_status;
    QVector<Article> m_articles;
    QRegularExpression m_searchExpression;
    QRegularExpression m_changelogExpression;
    QVector<ChangelogEntry> m_changelogEntries;
    QLineEdit* m_changelogSearch = nullptr;
    QLineEdit* m_changelogFrom = nullptr;
    QLineEdit* m_changelogTo = nullptr;
    QLabel* m_changelogDateError = nullptr;
    QTextBrowser* m_changelogView = nullptr;
};
