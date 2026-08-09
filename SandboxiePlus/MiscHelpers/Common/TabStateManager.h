#pragma once

#include <QObject>
#include <QSet>
#include <QHash>
#include <QString>

#include "../mischelpers_global.h"

class CSettings;
class QTabWidget;
class QMenu;

class MISCHELPERS_EXPORT CTabStateManager : public QObject
{
    Q_OBJECT
public:
    CTabStateManager(QTabWidget* tabs, CSettings* settings, const QString& key, QObject* parent = nullptr);

private slots:
    void showContextMenu(const QPoint& position);
    void showTabSearch(const QPoint& position);
    void showGroupPicker(const QString& tabName, const QPoint& position);

private:
    enum class SearchScope { CurrentStrip, CurrentGroup, GroupNames, MasterTabs };
    void showScopedTabSearch(SearchScope scope, const QPoint& position, const QString& groupName = QString());
    bool eventFilter(QObject* watched, QEvent* event) override;
    QString tabKey(int index) const;
    QString tabKey(QWidget* page) const;
    void load();
    void save() const;
    void restoreOrder();

    QTabWidget* m_tabs;
    CSettings* m_settings;
    QString m_key;
    QSet<QString> m_pinned;
    QSet<QString> m_appearanceOverrides;
    QHash<QString, QString> m_groups;
    QString m_active;
};
