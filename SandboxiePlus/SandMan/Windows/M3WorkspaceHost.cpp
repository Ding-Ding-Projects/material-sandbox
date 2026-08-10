#include "stdafx.h"
#include "M3WorkspaceHost.h"
#include "M3Menu.h"
#include "M3NavigationRail.h"
#include "M3TabStrip.h"
#include "../Views/MemoryInventoryView.h"
#include "../Views/MemorySyncView.h"
#include "../Views/OperationsView.h"
#include "../Views/SkillsView.h"
#include "../Views/StatusHubView.h"

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QString actionText(QString text)
{
    text.replace(QStringLiteral("&&"), QString(QChar(0x0001)));
    text.remove(QLatin1Char('&'));
    text.replace(QChar(0x0001), QLatin1Char('&'));
    const int tab = text.indexOf(QLatin1Char('\t'));
    if (tab >= 0) text.truncate(tab);
    return text.trimmed();
}
}

CM3WorkspaceHost::CM3WorkspaceHost(QMainWindow* window, QWidget* boxesView, QWidget* parent)
    : QWidget(parent), m_window(window), m_tabs(new CM3TabStrip(this)),
      m_rail(new CM3NavigationRail(this)), m_stack(new QStackedWidget(this))
{
    setObjectName(QStringLiteral("m3WorkspaceHost"));
    setAttribute(Qt::WA_StyledBackground, true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_tabs);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    bodyLayout->addWidget(m_rail);
    m_stack->setObjectName(QStringLiteral("m3DestinationStack"));
    m_stack->setProperty("m3Pane", true);
    bodyLayout->addWidget(m_stack, 1);
    root->addWidget(body, 1);

    setStyleSheet(QStringLiteral(
        "QWidget#m3WorkspaceHost { background: palette(window); }"
        "QStackedWidget#m3DestinationStack { background: palette(base); border-top-left-radius: 16px; }"));

    connect(m_rail, &CM3NavigationRail::destinationActivated,
            this, &CM3WorkspaceHost::activateDestination);
    connect(m_tabs, &CM3TabStrip::currentTabChanged,
            this, &CM3WorkspaceHost::activateDestination);
    connect(m_tabs, &CM3TabStrip::closeTabRequested, this, &CM3WorkspaceHost::closeTab);
    connect(m_tabs, &CM3TabStrip::reopenLastClosedRequested, this, &CM3WorkspaceHost::reopenLastClosed);
    connect(m_tabs, &CM3TabStrip::contextMenuRequested, this, &CM3WorkspaceHost::showTabContextMenu);
    connect(m_tabs, &CM3TabStrip::searchTabsRequested, this, &CM3WorkspaceHost::showTabSearch);

    registerContractPages(boxesView);
    activateDestination(QStringLiteral("boxes"));
}

CM3WorkspaceHost* CM3WorkspaceHost::install(QMainWindow* window)
{
    if (!window)
        return nullptr;

    QWidget* current = window->centralWidget();
    if (auto* activeHost = qobject_cast<CM3WorkspaceHost*>(current))
        return activeHost;

    // RebuildUI can leave the previous host queued for deletion while a new Boxes
    // central widget is already active. Never return that stale host.
    const QList<CM3WorkspaceHost*> staleHosts =
        window->findChildren<CM3WorkspaceHost*>(QStringLiteral("m3WorkspaceHost"),
                                                Qt::FindDirectChildrenOnly);
    for (CM3WorkspaceHost* stale : staleHosts) {
        if (stale && stale != current)
            stale->deleteLater();
    }

    current = window->takeCentralWidget();
    auto* host = new CM3WorkspaceHost(window, current, window);
    window->setCentralWidget(host);
    return host;
}

void CM3WorkspaceHost::registerDestination(const QString& id, const QString& title, QWidget* page,
                                            bool closable, bool memoryOwned)
{
    if (id.isEmpty() || !page || m_destinations.contains(id))
        return;
    page->setParent(m_stack);
    page->setProperty("m3DestinationId", id);
    page->setProperty("memoryOwned", memoryOwned);
    m_stack->addWidget(page);
    DestinationRecord record;
    record.title = title;
    record.page = page;
    record.closable = closable;
    record.memoryOwned = memoryOwned;
    m_destinations.insert(id, record);
}

QWidget* CM3WorkspaceHost::destinationPage(const QString& id) const
{
    return m_destinations.value(id).page;
}

QString CM3WorkspaceHost::currentDestination() const
{
    return m_tabs->currentId();
}

void CM3WorkspaceHost::registerContractPages(QWidget* boxesView)
{
    if (!boxesView) {
        boxesView = new QLabel(tr("The existing Boxes view was unavailable during shell installation."), this);
        qobject_cast<QLabel*>(boxesView)->setAlignment(Qt::AlignCenter);
    }
    registerDestination(QStringLiteral("boxes"), tr("Boxes"), boxesView, false, false);

    registerDestination(QStringLiteral("recovery"), tr("Recovery"),
        makeActionAdapter(QStringLiteral("recovery"), tr("Recovery"),
                          tr("The existing Recovery workflow is preserved. Open it through its live application action."),
                          {QStringLiteral("recovery"), QStringLiteral("recover files")}), true, false);
    registerDestination(QStringLiteral("trace"), tr("Trace"),
        makeActionAdapter(QStringLiteral("trace"), tr("Trace"),
                          tr("The existing Trace model and window remain authoritative."),
                          {QStringLiteral("trace"), QStringLiteral("monitor")}), true, false);
    registerDestination(QStringLiteral("snapshots"), tr("Snapshots"),
        makeActionAdapter(QStringLiteral("snapshots"), tr("Snapshots"),
                          tr("The existing snapshot tree and destructive-action gates are preserved."),
                          {QStringLiteral("snapshot")}), true, false);
    registerDestination(QStringLiteral("docs"), tr("Docs"),
        makeActionAdapter(QStringLiteral("docs"), tr("Docs"),
                          tr("Open the existing offline Documentation Browser."),
                          {QStringLiteral("documentation"), QStringLiteral("docs")}), true, false);

    const QString memoryRoot = CLocalMemoryRepository::discoverDefaultRoot();
    registerDestination(QStringLiteral("sync"), tr("Sync"), new CMemorySyncView(memoryRoot, this), true, true);
    registerDestination(QStringLiteral("skills"), tr("Skills"), new CSkillsView(memoryRoot, this), true, true);
    registerDestination(QStringLiteral("memory"), tr("Memory"), new CMemoryInventoryView(memoryRoot, this), true, true);
    registerDestination(QStringLiteral("ops"), tr("Ops"), new COperationsView(memoryRoot, this), true, true);
    registerDestination(QStringLiteral("status"), tr("Status"), new CStatusHubView(memoryRoot, this), true, true);

    registerDestination(QStringLiteral("settings"), tr("Settings"),
        makeActionAdapter(QStringLiteral("settings"), tr("Settings"),
                          tr("Open the existing Settings dialog in its Material two-pane host."),
                          {QStringLiteral("global settings"), QStringLiteral("settings")}),
        true, false);
}

QWidget* CM3WorkspaceHost::makeActionAdapter(const QString& id, const QString& title,
                                              const QString& description,
                                              const QStringList& actionKeywords)
{
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("m3ActionDestination_%1").arg(id));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);
    auto* heading = new QLabel(title, page);
    heading->setProperty("m3Title", QStringLiteral("display-small"));
    auto* copy = new QLabel(description, page);
    copy->setWordWrap(true);
    copy->setProperty("m3Text", QStringLiteral("secondary"));
    auto* open = new QPushButton(tr("Open %1").arg(title), page);
    open->setProperty("m3", QStringLiteral("filled"));
    QAction* action = findExistingAction(actionKeywords);
    if (action) {
        open->setEnabled(action->isEnabled());
        connect(action, &QAction::changed, open, [action, open] { open->setEnabled(action->isEnabled()); });
        connect(open, &QPushButton::clicked, action, &QAction::trigger);
    } else {
        open->setEnabled(false);
        open->setToolTip(tr("No matching existing application action was found."));
    }
    layout->addWidget(heading);
    layout->addWidget(copy);
    layout->addWidget(open, 0, Qt::AlignLeft);
    layout->addStretch(1);
    return page;
}

QAction* CM3WorkspaceHost::findExistingAction(const QStringList& keywords) const
{
    QAction* best = nullptr;
    int bestScore = 0;
    const QList<QAction*> actions = m_window ? m_window->findChildren<QAction*>() : QList<QAction*>();
    for (QAction* action : actions) {
        if (!action || action->isSeparator() || action->menu())
            continue;
        const QString text = actionText(action->text()).toLower();
        const QString object = action->objectName().toLower();
        int score = 0;
        for (const QString& raw : keywords) {
            const QString keyword = raw.toLower();
            QString objectKeyword = keyword;
            objectKeyword.remove(QLatin1Char(' '));
            if (text == keyword) score += 20;
            else if (text.contains(keyword)) score += 8;
            if (object.contains(objectKeyword)) score += 12;
        }
        if (score > bestScore) {
            bestScore = score;
            best = action;
        }
    }
    return bestScore > 0 ? best : nullptr;
}

void CM3WorkspaceHost::ensureTab(const QString& id)
{
    if (!m_destinations.contains(id))
        return;
    for (const CM3TabStrip::Tab& tab : m_tabs->tabs()) {
        if (tab.id == id)
            return;
    }
    const DestinationRecord record = m_destinations.value(id);
    CM3TabStrip::Tab tab;
    tab.id = id;
    tab.title = record.title;
    tab.closable = record.closable;
    if (id == QStringLiteral("settings"))
        tab.group = QStringLiteral("SYSTEM");
    else
        tab.group = record.memoryOwned ? QStringLiteral("MEMORY") : QStringLiteral("SANDBOX");
    m_tabs->addTab(tab);
}

void CM3WorkspaceHost::selectPage(const QString& id)
{
    QWidget* page = destinationPage(id);
    if (page)
        m_stack->setCurrentWidget(page);
}

void CM3WorkspaceHost::activateDestination(const QString& id)
{
    if (!m_destinations.contains(id)) {
        emit destinationRequested(id);
        return;
    }
    ensureTab(id);
    selectPage(id);
    m_tabs->setCurrentId(id, false);
    m_rail->setCurrentDestination(id, false);
}

void CM3WorkspaceHost::closeTab(const QString& id)
{
    if (id == QStringLiteral("boxes") || !m_destinations.contains(id))
        return;
    m_tabs->removeTab(id);
    m_closedTabs.removeAll(id);
    m_closedTabs.prepend(id);
    while (m_closedTabs.size() > 20)
        m_closedTabs.removeLast();
    const QString next = m_tabs->currentId().isEmpty() ? QStringLiteral("boxes") : m_tabs->currentId();
    activateDestination(next);
}

void CM3WorkspaceHost::reopenLastClosed()
{
    if (m_closedTabs.isEmpty())
        return;
    activateDestination(m_closedTabs.takeFirst());
}

void CM3WorkspaceHost::showTabContextMenu(const QString& id, const QPoint& globalPosition)
{
    if (!m_destinations.contains(id))
        return;
    auto* source = new QMenu(this);
    QAction* activate = source->addAction(tr("Activate"));
    connect(activate, &QAction::triggered, this, [this, id] { activateDestination(id); });
    QAction* close = source->addAction(tr("Close tab"));
    close->setEnabled(id != QStringLiteral("boxes") && m_destinations.value(id).closable);
    connect(close, &QAction::triggered, this, [this, id] { closeTab(id); });
    CM3Menu* popup = CM3Menu::popup(source, this, globalPosition);
    if (popup)
        connect(popup, &CM3Menu::aboutToHide, source, &QObject::deleteLater);
    else
        source->deleteLater();
}

void CM3WorkspaceHost::showTabSearch()
{
    auto* source = new QMenu(this);
    for (const auto& destination : CM3NavigationRail::contractDestinations()) {
        QAction* action = source->addAction(destination.label);
        action->setData(destination.id);
        connect(action, &QAction::triggered, this, [this, action] {
            activateDestination(action->data().toString());
        });
    }
    const QPoint position = m_tabs->mapToGlobal(QPoint(qMax(0, m_tabs->width() - 320), m_tabs->height()));
    CM3Menu* popup = CM3Menu::popup(source, this, position);
    if (popup)
        connect(popup, &CM3Menu::aboutToHide, source, &QObject::deleteLater);
    else
        source->deleteLater();
}
