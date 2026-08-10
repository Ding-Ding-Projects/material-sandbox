#pragma once

#include <QHash>
#include <QIcon>
#include <QPointer>
#include <QRegularExpression>
#include <QStringList>
#include <QWidget>

class CM3SearchField;
class QDialog;
class QLabel;
class QListWidget;
class QStackedWidget;
class QTabWidget;
class QVBoxLayout;

class CM3PageNavigationHost final : public QWidget
{
    Q_OBJECT

public:
    explicit CM3PageNavigationHost(QWidget* parent = nullptr);

    // Recompose an existing QTabWidget without removing/reparenting its pages.
    // This preserves CTabStateManager ownership, object names, indexes, signals,
    // and generated-form behavior while changing only presentation.
    static CM3PageNavigationHost* adapt(QDialog* dialog,
                                        QTabWidget* tabs,
                                        const QString& searchPlaceholder = QString());

    void addPage(QWidget* page, const QString& title, const QIcon& icon = QIcon());
    int pageCount() const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    void setProvenanceText(const QString& text);
    CM3SearchField* searchField() const;

signals:
    void currentPageChanged(int index);

private slots:
    void selectPage(int row);
    void filterPages(QString query,
                     bool regexMode,
                     QRegularExpression expression,
                     QString flags,
                     bool valid,
                     QString error);

private:
    void bindExistingTabs(QTabWidget* tabs);
    void addNavigationItem(const QString& title, const QIcon& icon, int index);
    QWidget* pageAt(int index) const;
    QString searchableText(QWidget* page) const;
    int countMatches(QWidget* page, const QRegularExpression& expression) const;
    void markMatches(QWidget* page, const QRegularExpression& expression, bool active);

    CM3SearchField* m_search;
    QListWidget* m_pageList;
    QStackedWidget* m_stack;
    QVBoxLayout* m_rightLayout;
    QLabel* m_provenance;
    QPointer<QTabWidget> m_adaptedTabs;
    QStringList m_titles;
};
