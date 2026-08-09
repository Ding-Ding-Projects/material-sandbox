#pragma once

#include <QDialog>
#include <QHash>
#include <QRegularExpression>
#include <QVector>

class QListWidget;
class QLineEdit;
class QTextBrowser;
class QLabel;
class QTabWidget;
class QDateEdit;
class QDate;
class QUrl;

class CDocumentationBrowser : public QDialog
{
    Q_OBJECT
public:
    explicit CDocumentationBrowser(QWidget* parent = nullptr);
    bool openArticle(const QString& slug);

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
    void handleDocumentationLink(const QUrl& url);

private:
    struct Article { QString slug; QString sourcePath; QString title; QString body; bool supplemental = false; };
    struct ChangelogEntry { QString title; QString body; QDate date; QString sha; };
    void loadArticles();
    void renderArticle(const Article& article);
    void renderChangelog();
    void setArticleStatus(const QString& text);
    bool changelogDateRangeValid(QDate* from, QDate* to);

    QTabWidget* m_tabs;
    QListWidget* m_articleList;
    QLineEdit* m_search;
    QTextBrowser* m_view;
    QLabel* m_status;
    QVector<Article> m_articles;
    QHash<QString, int> m_articleBySourcePath;
    QHash<QString, int> m_articleBySlug;
    int m_featureArticleCount = 0;
    int m_supplementalArticleCount = 0;
    int m_currentArticleIndex = -1;
    QRegularExpression m_searchExpression;
    QRegularExpression m_changelogExpression;
    QVector<ChangelogEntry> m_changelogEntries;
    QLineEdit* m_changelogSearch = nullptr;
    QLineEdit* m_changelogFrom = nullptr;
    QLineEdit* m_changelogTo = nullptr;
    QLabel* m_changelogDateError = nullptr;
    QLabel* m_changelogStatus = nullptr;
    QTextBrowser* m_changelogView = nullptr;
};
