#include "stdafx.h"
#include "M3TabStrip.h"

#include <QSignalBlocker>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <QStyle>
#include <QTabBar>
#include <QToolButton>

namespace {
constexpr int kTabStripHeight = 48;
constexpr int kIdRole = Qt::UserRole;
constexpr int kTabRole = Qt::UserRole + 1;
}

CM3TabStrip::CM3TabStrip(QWidget* parent)
    : QFrame(parent), m_tabBar(new QTabBar(this))
{
    setObjectName(QStringLiteral("m3TabStrip"));
    setFixedHeight(kTabStripHeight);
    setAttribute(Qt::WA_StyledBackground, true);
    setAccessibleName(tr("Workspace tabs"));

    m_tabBar->setObjectName(QStringLiteral("m3WorkspaceTabs"));
    m_tabBar->setDocumentMode(true);
    m_tabBar->setMovable(true);
    m_tabBar->setTabsClosable(false);
    m_tabBar->setExpanding(false);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tabBar->installEventFilter(this);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tabBar, 1);

    setStyleSheet(QStringLiteral(
        "QFrame#m3TabStrip { background: palette(window); border-bottom: 1px solid palette(mid); }"
        "QTabBar#m3WorkspaceTabs::tab { min-height: 48px; padding: 0 20px; border: 0;"
        " border-bottom: 3px solid transparent; }"
        "QTabBar#m3WorkspaceTabs::tab:selected { color: palette(highlight); border-bottom-color: palette(highlight); }"
        "QTabBar#m3WorkspaceTabs::tab:hover { background: palette(alternate-base); }"));

    connect(m_tabBar, &QTabBar::currentChanged, this, &CM3TabStrip::onCurrentChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &CM3TabStrip::onCloseRequested);
    connect(m_tabBar, &QTabBar::tabMoved, this, &CM3TabStrip::onMoved);
    connect(m_tabBar, &QTabBar::customContextMenuRequested, this, &CM3TabStrip::onContextMenu);

    auto* next = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this);
    connect(next, &QShortcut::activated, this, [this] {
        if (m_tabBar->count() > 0)
            m_tabBar->setCurrentIndex((m_tabBar->currentIndex() + 1) % m_tabBar->count());
    });
    auto* previous = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), this);
    connect(previous, &QShortcut::activated, this, [this] {
        if (m_tabBar->count() > 0)
            m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count());
    });
    auto* close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    connect(close, &QShortcut::activated, this, [this] { onCloseRequested(m_tabBar->currentIndex()); });
    auto* reopen = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), this);
    connect(reopen, &QShortcut::activated, this, &CM3TabStrip::reopenLastClosedRequested);
    auto* search = new QShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T), this);
    connect(search, &QShortcut::activated, this, &CM3TabStrip::searchTabsRequested);
}

void CM3TabStrip::setTabs(const QList<Tab>& tabs, const QString& activeId)
{
    const QSignalBlocker blocker(m_tabBar);
    while (m_tabBar->count() > 0)
        m_tabBar->removeTab(m_tabBar->count() - 1);
    for (const Tab& tab : tabs)
        addTab(tab);
    int active = indexOf(activeId);
    if (active < 0 && m_tabBar->count() > 0)
        active = 0;
    m_tabBar->setCurrentIndex(active);
}

QList<CM3TabStrip::Tab> CM3TabStrip::tabs() const
{
    QList<Tab> result;
    for (int i = 0; i < m_tabBar->count(); ++i)
        result.append(tabAt(i));
    return result;
}

QString CM3TabStrip::currentId() const
{
    const int index = m_tabBar->currentIndex();
    return index >= 0 ? m_tabBar->tabData(index).toMap().value(QStringLiteral("id")).toString() : QString();
}

void CM3TabStrip::setCurrentId(const QString& id, bool emitSignal)
{
    const int index = indexOf(id);
    if (index < 0)
        return;
    if (emitSignal)
        m_tabBar->setCurrentIndex(index);
    else {
        const QSignalBlocker blocker(m_tabBar);
        m_tabBar->setCurrentIndex(index);
    }
}

void CM3TabStrip::updateTab(const Tab& tab)
{
    const int index = indexOf(tab.id);
    if (index < 0)
        return;
    applyTabVisual(index, tab);
}

void CM3TabStrip::addTab(const Tab& tab, int index)
{
    QVariantMap data = tab.metadata;
    data.insert(QStringLiteral("id"), tab.id);
    data.insert(QStringLiteral("title"), tab.title);
    data.insert(QStringLiteral("closable"), tab.closable);
    data.insert(QStringLiteral("pinned"), tab.pinned);
    data.insert(QStringLiteral("group"), tab.group);

    const int inserted = index < 0
        ? m_tabBar->addTab(tab.icon, tab.title)
        : m_tabBar->insertTab(index, tab.icon, tab.title);
    m_tabBar->setTabData(inserted, data);
    applyTabVisual(inserted, tab);
}

void CM3TabStrip::removeTab(const QString& id)
{
    const int index = indexOf(id);
    if (index >= 0)
        m_tabBar->removeTab(index);
}

bool CM3TabStrip::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_tabBar && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Menu || (key->key() == Qt::Key_F10 && (key->modifiers() & Qt::ShiftModifier))) {
            const int index = m_tabBar->currentIndex();
            if (index >= 0) {
                emit contextMenuRequested(tabAt(index).id,
                    m_tabBar->mapToGlobal(m_tabBar->tabRect(index).bottomLeft()));
                return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void CM3TabStrip::onCurrentChanged(int index)
{
    if (index >= 0)
        emit currentTabChanged(tabAt(index).id);
}

void CM3TabStrip::onCloseRequested(int index)
{
    if (index < 0)
        return;
    const Tab tab = tabAt(index);
    if (tab.closable && !tab.pinned)
        emit closeTabRequested(tab.id);
}

void CM3TabStrip::onMoved(int from, int to)
{
    if (to >= 0)
        emit tabMoved(tabAt(to).id, from, to);
}

void CM3TabStrip::onContextMenu(const QPoint& point)
{
    const int index = m_tabBar->tabAt(point);
    if (index >= 0)
        emit contextMenuRequested(tabAt(index).id, m_tabBar->mapToGlobal(point));
}

int CM3TabStrip::indexOf(const QString& id) const
{
    for (int i = 0; i < m_tabBar->count(); ++i) {
        if (tabAt(i).id == id)
            return i;
    }
    return -1;
}

CM3TabStrip::Tab CM3TabStrip::tabAt(int index) const
{
    Tab tab;
    if (index < 0 || index >= m_tabBar->count())
        return tab;
    const QVariantMap data = m_tabBar->tabData(index).toMap();
    tab.id = data.value(QStringLiteral("id")).toString();
    tab.title = data.value(QStringLiteral("title"), m_tabBar->tabText(index)).toString();
    tab.icon = m_tabBar->tabIcon(index);
    tab.closable = data.value(QStringLiteral("closable"), true).toBool();
    tab.pinned = data.value(QStringLiteral("pinned"), false).toBool();
    tab.group = data.value(QStringLiteral("group")).toString();
    tab.metadata = data;
    return tab;
}

void CM3TabStrip::applyTabVisual(int index, const Tab& tab)
{
    QVariantMap data = tab.metadata;
    data.insert(QStringLiteral("id"), tab.id);
    data.insert(QStringLiteral("title"), tab.title);
    data.insert(QStringLiteral("closable"), tab.closable);
    data.insert(QStringLiteral("pinned"), tab.pinned);
    data.insert(QStringLiteral("group"), tab.group);
    m_tabBar->setTabData(index, data);
    m_tabBar->setTabText(index, tab.pinned ? QString(QChar(0x2022)) + QLatin1Char(' ') + tab.title : tab.title);
    m_tabBar->setTabIcon(index, tab.icon);
    m_tabBar->setTabToolTip(index, tab.group.isEmpty() ? tab.title : tr("%1 — %2").arg(tab.group, tab.title));
    QWidget* existingButton = m_tabBar->tabButton(index, QTabBar::RightSide);
    m_tabBar->setTabButton(index, QTabBar::RightSide, nullptr);
    if (existingButton)
        existingButton->deleteLater();
    if (tab.closable && !tab.pinned) {
        auto* closeButton = new QToolButton(m_tabBar);
        closeButton->setObjectName(QStringLiteral("m3TabCloseButton"));
        closeButton->setAutoRaise(true);
        closeButton->setFixedSize(40, 40);
        closeButton->setIcon(m_tabBar->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
        closeButton->setIconSize(QSize(18, 18));
        closeButton->setToolTip(tr("Close %1").arg(tab.title));
        closeButton->setAccessibleName(closeButton->toolTip());
        const QString id = tab.id;
        connect(closeButton, &QToolButton::clicked, this, [this, id] {
            const int currentIndex = indexOf(id);
            if (currentIndex >= 0)
                onCloseRequested(currentIndex);
        });
        m_tabBar->setTabButton(index, QTabBar::RightSide, closeButton);
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    m_tabBar->setTabVisible(index, true);
#endif
}
