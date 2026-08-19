#pragma once

#include <QObject>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QHash>
#include <QColor>
#include <QString>
#include <QStringList>

#include "../mischelpers_global.h"

class CSettings;
class QAbstractItemView;
class QShortcut;
class QStackedLayout;
class QTabWidget;
class QMenu;
class QWidget;

class MISCHELPERS_EXPORT CTabStateManager : public QObject
{
    Q_OBJECT
public:
    CTabStateManager(QTabWidget* tabs, CSettings* settings, const QString& key, QObject* parent = nullptr);
    CTabStateManager(QTabWidget* tabs,
                     QAbstractItemView* visibleNavigation,
                     QWidget* shortcutHost,
                     CSettings* settings,
                     const QString& key,
                     QObject* parent = nullptr);
    CTabStateManager(QStackedLayout* pages,
                     QAbstractItemView* visibleNavigation,
                     QWidget* shortcutHost,
                     CSettings* settings,
                     const QString& key,
                     QObject* parent = nullptr);
    ~CTabStateManager() override;

private slots:
    void showContextMenu(const QPoint& position);
    void showTabSearch(const QPoint& position);
    void showGroupPicker(const QString& tabName, const QPoint& position);

private:
    struct GroupMetadata
    {
        QColor color;
        int order = 0;
        bool collapsed = false;
    };

    enum class SearchScope { CurrentStrip, CurrentGroup, GroupNames, MasterTabs };
    void showScopedTabSearch(SearchScope scope, const QPoint& position, const QString& groupName = QString());
    bool eventFilter(QObject* watched, QEvent* event) override;
    QString tabKey(int index) const;
    QString tabKey(QWidget* page) const;
    bool assignStablePageKeys();
    void initialize();
    void trackTransient(QWidget* transient);
    QWidget* interactionWidget() const;
    QWidget* ownerWidget() const;
    int pageCount() const;
    QWidget* pageAt(int index) const;
    QString pageText(int index) const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    int itemAt(const QPoint& position) const;
    QRect itemRect(int index) const;
    QPoint mapToGlobal(const QPoint& position) const;
    void load();
    void save() const;
    void restoreOrder();
    void showManageGroups(const QPoint& position);
    void ensureGroupMetadata();
    void applyGroupPresentation();
    void restoreGroupedOrder();
    QStringList orderedGroupNames() const;
    QColor defaultGroupColor(const QString& groupName) const;
    bool validGroupName(const QString& value, QString* error = nullptr) const;
    bool groupNameInUse(const QString& value, const QString& except = QString()) const;
    QString groupForTab(const QString& tabName) const;
    void setGroupForTab(const QString& tabName, const QString& groupName);

    QPointer<QTabWidget> m_tabs;
    QPointer<QStackedLayout> m_pages;
    QPointer<QAbstractItemView> m_navigation;
    QPointer<QWidget> m_shortcutHost;
    QList<QPointer<QShortcut>> m_shortcuts;
    QList<QPointer<QWidget>> m_transients;
    bool m_restoring = false;
    CSettings* m_settings;
    QString m_key;
    QSet<QString> m_pinned;
    QSet<QString> m_appearanceOverrides;
    QHash<QString, QString> m_groups;
    QHash<QString, GroupMetadata> m_groupMetadata;
    QStringList m_groupOrder;
    QString m_active;
};
