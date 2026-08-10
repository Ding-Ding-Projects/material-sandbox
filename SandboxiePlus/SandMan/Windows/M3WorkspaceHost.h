#pragma once

#include <QHash>
#include <QList>
#include <QPointer>
#include <QPoint>
#include <QStringList>
#include <QWidget>

class QAction;
class CM3NavigationRail;
class CM3TabStrip;
class QMainWindow;
class QStackedWidget;

class CM3WorkspaceHost final : public QWidget
{
    Q_OBJECT
public:
    explicit CM3WorkspaceHost(QMainWindow* window, QWidget* boxesView, QWidget* parent = nullptr);

    static CM3WorkspaceHost* install(QMainWindow* window);

    void registerDestination(const QString& id, const QString& title, QWidget* page,
                             bool closable = true, bool memoryOwned = false);
    QWidget* destinationPage(const QString& id) const;
    QString currentDestination() const;

signals:
    void destinationRequested(QString id);

public slots:
    void activateDestination(const QString& id);

private slots:
    void closeTab(const QString& id);
    void reopenLastClosed();
    void showTabContextMenu(const QString& id, const QPoint& globalPosition);
    void showTabSearch();

private:
    struct DestinationRecord {
        QString title;
        QPointer<QWidget> page;
        bool closable = true;
        bool memoryOwned = false;
    };

    void registerContractPages(QWidget* boxesView);
    QWidget* makeActionAdapter(const QString& id, const QString& title, const QString& description,
                               const QStringList& actionKeywords);
    QAction* findExistingAction(const QStringList& keywords) const;
    void ensureTab(const QString& id);
    void selectPage(const QString& id);

    QMainWindow* m_window;
    CM3TabStrip* m_tabs;
    CM3NavigationRail* m_rail;
    QStackedWidget* m_stack;
    QHash<QString, DestinationRecord> m_destinations;
    QList<QString> m_closedTabs;
};
