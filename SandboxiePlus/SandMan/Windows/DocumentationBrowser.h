#pragma once

#include <QDialog>
#include <QRegularExpression>
#include <QVector>

class QListWidget;
class QLineEdit;
class QTextBrowser;
class QLabel;
class QTabWidget;

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

private:
    struct Article { QString slug; QString title; QString body; };
    void loadArticles();
    void renderArticle(const Article& article);

    QTabWidget* m_tabs;
    QListWidget* m_articleList;
    QLineEdit* m_search;
    QTextBrowser* m_view;
    QLabel* m_status;
    QVector<Article> m_articles;
    QRegularExpression m_searchExpression;
};
