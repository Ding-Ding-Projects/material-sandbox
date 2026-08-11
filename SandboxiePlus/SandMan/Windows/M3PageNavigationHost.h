#pragma once

#include <QHash>
#include <QIcon>
#include <QPointer>
#include <QRegularExpression>
#include <QStringList>
#include <QWidget>

class CM3SearchField;
class CSettings;
class CTabStateManager;
class QDialog;
class QLabel;
class QListWidget;
class QStackedLayout;
class QStackedWidget;
class QTabWidget;
class QTreeWidget;
class QVBoxLayout;

class CM3PageNavigationHost final : public QWidget
{
    Q_OBJECT

public:
    explicit CM3PageNavigationHost(QWidget* parent = nullptr);

    // Recompose an existing QTabWidget without removing/reparenting its pages.
    // This preserves object names, indexes, signals, and generated-form
    // behavior while changing only presentation.
    static CM3PageNavigationHost* adapt(QDialog* dialog,
                                        QTabWidget* tabs,
                                        const QString& searchPlaceholder = QString());

    // Settings and Options finish rebuilding their page containers after the
    // shared shell is installed. Rebind the host to that final container so a
    // queued delete of the Designer tab widget cannot strand the navigation.
    void rebind(QTabWidget* tabs);
    void rebind(QWidget* container, QStackedLayout* pages, QTreeWidget* titles);
    void rebind(QTabWidget* tabs, CSettings* settings, const QString& stateKey);
    void rebind(QWidget* container,
                QStackedLayout* pages,
                QTreeWidget* titles,
                CSettings* settings,
                const QString& stateKey);
    void releaseStateManager();

    void addPage(QWidget* page, const QString& title, const QIcon& icon = QIcon());
    int pageCount() const;
    int currentIndex() const;
    QWidget* currentPage() const;
    void setCurrentIndex(int index);
    void setProvenanceText(const QString& text);
    CM3SearchField* searchField() const;
    QListWidget* navigationList() const;

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
    enum class PageSource { InternalStack, TabWidget, ExternalStack };

    void disconnectPageContainer();
    void hostContainer(QWidget* container);
    void rebuildTabNavigation();
    void rebuildStackNavigation(QTreeWidget* titles);
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
    QPointer<QWidget> m_adaptedContainer;
    QPointer<QTabWidget> m_adaptedTabs;
    QPointer<QStackedLayout> m_adaptedStack;
    QPointer<CTabStateManager> m_stateManager;
    PageSource m_pageSource = PageSource::InternalStack;
    QStringList m_titles;
};
