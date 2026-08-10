#pragma once

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QPoint>
#include <QString>
#include <QVariantMap>

class QTabBar;

class CM3TabStrip final : public QFrame
{
    Q_OBJECT

public:
    struct Tab {
        QString id;
        QString title;
        QIcon icon;
        bool closable = true;
        bool pinned = false;
        QString group;
        QVariantMap metadata;
    };

    explicit CM3TabStrip(QWidget* parent = nullptr);

    void setTabs(const QList<Tab>& tabs, const QString& activeId = QString());
    QList<Tab> tabs() const;
    QString currentId() const;
    void setCurrentId(const QString& id, bool emitSignal = false);
    void updateTab(const Tab& tab);
    void addTab(const Tab& tab, int index = -1);
    void removeTab(const QString& id);

signals:
    void currentTabChanged(QString id);
    void closeTabRequested(QString id);
    void reopenLastClosedRequested();
    void searchTabsRequested();
    void tabMoved(QString id, int from, int to);
    void contextMenuRequested(QString id, QPoint globalPosition);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onCurrentChanged(int index);
    void onCloseRequested(int index);
    void onMoved(int from, int to);
    void onContextMenu(const QPoint& point);

private:
    int indexOf(const QString& id) const;
    Tab tabAt(int index) const;
    void applyTabVisual(int index, const Tab& tab);

    QTabBar* m_tabBar;
};
