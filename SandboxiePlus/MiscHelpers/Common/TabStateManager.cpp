#include "stdafx.h"
#include "TabStateManager.h"

#include "Settings.h"
#include <QAction>
#include <QBrush>
#include <QCheckBox>
#include <QComboBox>
#include <QColorDialog>
#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QFont>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QPushButton>
#include <QScrollArea>
#include <QSharedPointer>
#include <QShortcut>
#include <QSpinBox>
#include <QStackedLayout>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidgetAction>

#include <functional>
#include <utility>

CTabStateManager::CTabStateManager(QTabWidget* tabs,
                                   CSettings* settings,
                                   const QString& key,
                                   QObject* parent)
    : CTabStateManager(tabs, nullptr, tabs, settings, key, parent)
{
}

CTabStateManager::CTabStateManager(QTabWidget* tabs,
                                   QAbstractItemView* visibleNavigation,
                                   QWidget* shortcutHost,
                                   CSettings* settings,
                                   const QString& key,
                                   QObject* parent)
    : QObject(parent),
      m_tabs(tabs),
      m_navigation(visibleNavigation),
      m_shortcutHost(shortcutHost),
      m_settings(settings),
      m_key(key)
{
    initialize();
}

CTabStateManager::CTabStateManager(QStackedLayout* pages,
                                   QAbstractItemView* visibleNavigation,
                                   QWidget* shortcutHost,
                                   CSettings* settings,
                                   const QString& key,
                                   QObject* parent)
    : QObject(parent),
      m_pages(pages),
      m_navigation(visibleNavigation),
      m_shortcutHost(shortcutHost),
      m_settings(settings),
      m_key(key)
{
    initialize();
}

CTabStateManager::~CTabStateManager()
{
    if (QWidget* interaction = interactionWidget())
        interaction->removeEventFilter(this);
    for (const QPointer<QShortcut>& shortcut : std::as_const(m_shortcuts))
        delete shortcut.data();
    for (const QPointer<QWidget>& transient : std::as_const(m_transients))
        delete transient.data();
}

void CTabStateManager::initialize()
{
    QWidget* interaction = interactionWidget();
    QWidget* shortcutHost = ownerWidget();
    if (!interaction || !shortcutHost || pageCount() == 0)
        return;

    if (!assignStablePageKeys())
        return;
    interaction->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(interaction, &QWidget::customContextMenuRequested,
            this, &CTabStateManager::showContextMenu);
    auto saveActive = [this](int index) {
        m_active = tabKey(index);
        applyGroupPresentation();
        save();
    };
    if (m_tabs) {
        connect(m_tabs, &QTabWidget::currentChanged, this, saveActive);
        connect(m_tabs->tabBar(), &QTabBar::tabMoved, this, [this](int, int) { applyGroupPresentation(); save(); });
    }
    else if (m_pages) {
        connect(m_pages, &QStackedLayout::currentChanged, this, saveActive);
    }
    QObject* source = m_tabs ? static_cast<QObject*>(m_tabs.data())
                             : static_cast<QObject*>(m_pages.data());
    connect(source, &QObject::destroyed, this, [this]() { deleteLater(); });
    interaction->installEventFilter(this);
    auto anchor = [this]() {
        const int index = currentIndex();
        return index >= 0 ? itemRect(index).center() : QPoint(1, 1);
    };
    auto addSearchShortcut = [this, anchor, shortcutHost](const QKeySequence& sequence, SearchScope scope) {
        QShortcut* shortcut = new QShortcut(sequence, shortcutHost);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        m_shortcuts.append(shortcut);
        connect(shortcut, &QShortcut::activated, this, [this, anchor, scope]() {
            showScopedTabSearch(scope, anchor());
        });
    };
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), SearchScope::CurrentStrip);
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G), SearchScope::CurrentGroup);
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), SearchScope::GroupNames);
    addSearchShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), SearchScope::MasterTabs);
    load();
    restoreOrder();
    restoreGroupedOrder();
    applyGroupPresentation();
    save();
}

void CTabStateManager::trackTransient(QWidget* transient)
{
    if (transient)
        m_transients.append(transient);
}

QWidget* CTabStateManager::interactionWidget() const
{
    if (m_navigation)
        return m_navigation;
    return m_tabs ? m_tabs->tabBar() : nullptr;
}

QWidget* CTabStateManager::ownerWidget() const
{
    if (m_shortcutHost)
        return m_shortcutHost;
    if (m_navigation)
        return m_navigation;
    return m_tabs;
}

int CTabStateManager::pageCount() const
{
    if (m_tabs)
        return m_tabs->count();
    return m_pages ? m_pages->count() : 0;
}

QWidget* CTabStateManager::pageAt(int index) const
{
    if (index < 0 || index >= pageCount())
        return nullptr;
    if (m_tabs)
        return m_tabs->widget(index);
    return m_pages ? m_pages->widget(index) : nullptr;
}

QString CTabStateManager::pageText(int index) const
{
    if (auto* list = qobject_cast<QListWidget*>(m_navigation.data())) {
        if (index >= 0 && index < list->count())
            return list->item(index)->text();
    }
    if (auto* tree = qobject_cast<QTreeWidget*>(m_navigation.data())) {
        for (QTreeWidgetItemIterator iterator(tree); *iterator; ++iterator) {
            QTreeWidgetItem* item = *iterator;
            if (item->data(0, Qt::UserRole).toInt() != index)
                continue;
            return item->parent()
                ? QStringLiteral("%1 · %2").arg(item->parent()->text(0), item->text(0))
                : item->text(0);
        }
    }
    if (m_tabs && index >= 0 && index < m_tabs->count())
        return m_tabs->tabText(index);
    QWidget* page = pageAt(index);
    if (!page)
        return QString();
    const QString accessibleName = page->accessibleName().trimmed();
    return accessibleName.isEmpty() ? tabKey(page) : accessibleName;
}

int CTabStateManager::currentIndex() const
{
    if (m_tabs)
        return m_tabs->currentIndex();
    return m_pages ? m_pages->currentIndex() : -1;
}

void CTabStateManager::setCurrentIndex(int index)
{
    if (m_tabs)
        m_tabs->setCurrentIndex(index);
    else if (m_pages)
        m_pages->setCurrentIndex(index);
}

int CTabStateManager::itemAt(const QPoint& position) const
{
    if (auto* list = qobject_cast<QListWidget*>(m_navigation.data()))
        return list->indexAt(position).row();
    if (auto* tree = qobject_cast<QTreeWidget*>(m_navigation.data())) {
        QTreeWidgetItem* item = tree->itemAt(position);
        return item ? item->data(0, Qt::UserRole).toInt() : -1;
    }
    return m_tabs ? m_tabs->tabBar()->tabAt(position) : -1;
}

QRect CTabStateManager::itemRect(int index) const
{
    if (auto* list = qobject_cast<QListWidget*>(m_navigation.data())) {
        if (index >= 0 && index < list->count())
            return list->visualItemRect(list->item(index));
    }
    if (auto* tree = qobject_cast<QTreeWidget*>(m_navigation.data())) {
        for (QTreeWidgetItemIterator iterator(tree); *iterator; ++iterator) {
            QTreeWidgetItem* item = *iterator;
            if (item->data(0, Qt::UserRole).toInt() == index)
                return tree->visualItemRect(item);
        }
    }
    return m_tabs && index >= 0 && index < m_tabs->count()
        ? m_tabs->tabBar()->tabRect(index)
        : QRect(QPoint(0, 0), QSize(1, 1));
}

QPoint CTabStateManager::mapToGlobal(const QPoint& position) const
{
    if (m_navigation)
        return m_navigation->viewport()->mapToGlobal(position);
    QWidget* interaction = interactionWidget();
    return interaction ? interaction->mapToGlobal(position) : position;
}

QString CTabStateManager::tabKey(int index) const
{
    return tabKey(pageAt(index));
}

QString CTabStateManager::tabKey(QWidget* page) const
{
    if (!page)
        return QString();
    const QString assigned = page->property("tabStateManagerKey").toString().trimmed();
    if (!assigned.isEmpty())
        return assigned;
    const QString stable = page->objectName().trimmed();
    return stable;
}

bool CTabStateManager::assignStablePageKeys()
{
    QSet<QString> used;
    for (int index = 0; index < pageCount(); ++index) {
        QWidget* page = pageAt(index);
        if (!page)
            continue;
        QString key = page->property("tabStateManagerKey").toString().trimmed();
        if (key.isEmpty())
            key = page->objectName().trimmed();
        // Designer-generated pages are not guaranteed to have an object name,
        // and duplicate names are common in nested option forms. Do not turn
        // off the entire state manager for those pages: derive a deterministic
        // per-position fallback and keep the explicit key when it is unique.
        if (key.isEmpty())
            key = QStringLiteral("page-%1").arg(index + 1);
        const QString base = key;
        int suffix = 2;
        while (used.contains(key))
            key = QStringLiteral("%1-%2").arg(base).arg(suffix++);
        page->setProperty("tabStateManagerKey", key);
        used.insert(key);
    }
    return used.size() == pageCount();
}

QString CTabStateManager::groupForTab(const QString& tabName) const
{
    return m_groups.value(tabName).trimmed();
}

void CTabStateManager::setGroupForTab(const QString& tabName, const QString& groupName)
{
    if (tabName.isEmpty())
        return;
    if (groupName.isEmpty())
        m_groups.remove(tabName);
    else
        m_groups.insert(tabName, groupName);
}

bool CTabStateManager::validGroupName(const QString& value, QString* error) const
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        if (error) *error = tr("Enter a group name.");
        return false;
    }
    if (trimmed.size() > 80) {
        if (error) *error = tr("Group names are limited to 80 UTF-16 characters.");
        return false;
    }
    for (const QChar character : trimmed) {
        if (character.category() == QChar::Other_Control) {
            if (error) *error = tr("Group names cannot contain control characters.");
            return false;
        }
    }
    return true;
}

bool CTabStateManager::groupNameInUse(const QString& value, const QString& except) const
{
    const QString candidate = value.trimmed();
    for (const QString& name : m_groupMetadata.keys()) {
        if (!except.isEmpty() && name.compare(except, Qt::CaseInsensitive) == 0)
            continue;
        if (name.compare(candidate, Qt::CaseInsensitive) == 0)
            return true;
    }
    for (const QString& name : m_groups.values()) {
        if (!except.isEmpty() && name.compare(except, Qt::CaseInsensitive) == 0)
            continue;
        if (name.compare(candidate, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QColor CTabStateManager::defaultGroupColor(const QString& groupName) const
{
    quint32 hash = 2166136261u;
    for (const QChar character : groupName) {
        hash ^= character.unicode();
        hash *= 16777619u;
    }
    return QColor::fromHsv(static_cast<int>(hash % 360u), 150, 225, 255);
}

QStringList CTabStateManager::orderedGroupNames() const
{
    QStringList names;
    auto appendUnique = [this, &names](const QString& candidate) {
        if (candidate.isEmpty())
            return;
        if (!validGroupName(candidate))
            return;
        for (const QString& existing : std::as_const(names))
            if (existing.compare(candidate, Qt::CaseInsensitive) == 0)
                return;
        names.append(candidate);
    };

    for (const QString& name : m_groupOrder)
        appendUnique(name);
    for (const QString& name : m_groupMetadata.keys())
        appendUnique(name);
    for (const QString& name : m_groups.values())
        appendUnique(name);

    QStringList remainder;
    for (const QString& name : names) {
        bool inOrder = false;
        for (const QString& ordered : m_groupOrder)
            if (ordered.compare(name, Qt::CaseInsensitive) == 0) { inOrder = true; break; }
        if (!inOrder)
            remainder.append(name);
    }
    remainder.sort(Qt::CaseInsensitive);
    QStringList result;
    for (const QString& name : m_groupOrder) {
        for (const QString& candidate : names) {
            if (candidate.compare(name, Qt::CaseInsensitive) == 0 && !result.contains(candidate)) {
                result.append(candidate);
                break;
            }
        }
    }
    for (const QString& name : remainder)
        if (!result.contains(name))
            result.append(name);
    return result;
}

void CTabStateManager::ensureGroupMetadata()
{
    const QStringList names = orderedGroupNames();
    QHash<QString, QString> canonical;
    for (const QString& name : names)
        canonical.insert(name.toLower(), name);

    for (auto it = m_groups.begin(); it != m_groups.end();) {
        const QString value = it.value().trimmed();
        const QString canonicalName = canonical.value(value.toLower());
        if (canonicalName.isEmpty())
            it = m_groups.erase(it);
        else {
            it.value() = canonicalName;
            ++it;
        }
    }

    QHash<QString, GroupMetadata> normalizedMetadata;
    for (const QString& name : names) {
        for (auto it = m_groupMetadata.cbegin(); it != m_groupMetadata.cend(); ++it) {
            if (it.key().compare(name, Qt::CaseInsensitive) == 0) {
                normalizedMetadata.insert(name, it.value());
                break;
            }
        }
    }
    m_groupMetadata = normalizedMetadata;

    m_groupOrder = orderedGroupNames();
    for (int index = 0; index < m_groupOrder.size(); ++index) {
        const QString name = m_groupOrder.at(index);
        auto metadata = m_groupMetadata.find(name);
        if (metadata == m_groupMetadata.end()) {
            GroupMetadata value;
            value.color = defaultGroupColor(name);
            value.order = index;
            value.collapsed = false;
            m_groupMetadata.insert(name, value);
        } else {
            metadata->order = index;
            if (!metadata->color.isValid())
                metadata->color = defaultGroupColor(name);
        }
    }
}

void CTabStateManager::applyGroupPresentation()
{
    ensureGroupMetadata();
    const int activeIndex = currentIndex();
    for (int index = 0; index < pageCount(); ++index) {
        QWidget* page = pageAt(index);
        const QString key = tabKey(index);
        const QString groupName = groupForTab(key);
        const GroupMetadata metadata = m_groupMetadata.value(groupName);
        const bool collapsed = !groupName.isEmpty() && metadata.collapsed;
        const bool visible = groupName.isEmpty() || m_pinned.contains(key) || index == activeIndex || !collapsed;
        const QString description = groupName.isEmpty()
            ? tr("Ungrouped tab")
            : tr("Tab group %1, %2").arg(groupName, collapsed ? tr("collapsed") : tr("expanded"));
        if (page)
            page->setAccessibleDescription(description);
        if (m_tabs) {
            m_tabs->setTabVisible(index, visible);
            m_tabs->tabBar()->setTabTextColor(index, groupName.isEmpty() ? QColor() : metadata.color);
            m_tabs->setTabToolTip(index, groupName.isEmpty() ? pageText(index) : tr("%1 — %2").arg(pageText(index), description));
        }
        if (auto* list = qobject_cast<QListWidget*>(m_navigation.data())) {
            if (index >= 0 && index < list->count()) {
                QListWidgetItem* item = list->item(index);
                item->setHidden(!visible);
                item->setForeground(groupName.isEmpty() ? QBrush() : QBrush(metadata.color));
                item->setToolTip(groupName.isEmpty() ? pageText(index) : tr("%1 — %2").arg(pageText(index), description));
            }
        }
        if (auto* tree = qobject_cast<QTreeWidget*>(m_navigation.data())) {
            for (QTreeWidgetItemIterator iterator(tree); *iterator; ++iterator) {
                QTreeWidgetItem* item = *iterator;
                if (item->data(0, Qt::UserRole).toInt() != index)
                    continue;
                item->setHidden(!visible);
                item->setForeground(0, groupName.isEmpty() ? QBrush() : QBrush(metadata.color));
                item->setToolTip(0, groupName.isEmpty() ? pageText(index) : tr("%1 — %2").arg(pageText(index), description));
                break;
            }
        }
    }
}

void CTabStateManager::restoreGroupedOrder()
{
    if (!m_tabs && !m_pages)
        return;
    QStringList desired;
    auto append = [&desired](const QString& key) { if (!key.isEmpty() && !desired.contains(key)) desired.append(key); };
    for (int index = 0; index < pageCount(); ++index)
        if (m_pinned.contains(tabKey(index))) append(tabKey(index));
    for (const QString& groupName : orderedGroupNames())
        for (int index = 0; index < pageCount(); ++index)
            if (!m_pinned.contains(tabKey(index)) && groupForTab(tabKey(index)).compare(groupName, Qt::CaseInsensitive) == 0)
                append(tabKey(index));
    for (int index = 0; index < pageCount(); ++index)
        if (!m_pinned.contains(tabKey(index)) && groupForTab(tabKey(index)).isEmpty()) append(tabKey(index));

    QScopedValueRollback<bool> restoring(m_restoring, true);
    if (m_tabs) {
        QTabBar* bar = m_tabs->tabBar();
        for (int target = 0; target < desired.size(); ++target) {
            int current = -1;
            for (int index = target; index < pageCount(); ++index)
                if (tabKey(index) == desired.at(target)) { current = index; break; }
            if (current >= 0 && current != target)
                bar->moveTab(current, target);
        }
    } else if (m_pages) {
        QList<QWidget*> pages;
        for (const QString& key : desired)
            for (int index = 0; index < pageCount(); ++index)
                if (tabKey(index) == key) { pages.append(pageAt(index)); break; }
        QHash<QString, QListWidgetItem*> navigationItems;
        if (auto* list = qobject_cast<QListWidget*>(m_navigation.data())) {
            for (int index = list->count() - 1; index >= 0; --index) {
                QListWidgetItem* item = list->takeItem(index);
                const int pageIndex = item->data(Qt::UserRole).toInt();
                if (pageIndex >= 0 && pageIndex < pageCount())
                    navigationItems.insert(tabKey(pageIndex), item);
                else
                    delete item;
            }
            list->clear();
        }
        for (int target = 0; target < pages.size(); ++target) {
            m_pages->removeWidget(pages.at(target));
            m_pages->insertWidget(target, pages.at(target));
        }
        if (auto* list = qobject_cast<QListWidget*>(m_navigation.data())) {
            for (const QString& key : desired) {
                QListWidgetItem* item = navigationItems.take(key);
                if (!item)
                    continue;
                item->setData(Qt::UserRole, list->count());
                list->addItem(item);
            }
        }
        if (!m_active.isEmpty()) {
            for (int index = 0; index < pageCount(); ++index)
                if (tabKey(index) == m_active) { setCurrentIndex(index); break; }
        }
    }
}

void CTabStateManager::load()
{
    if (!m_settings)
        return;
    const QByteArray data = m_settings->GetBlob(m_key);
    if (data.isEmpty())
        return;
    QJsonParseError error;
    const QJsonObject root = QJsonDocument::fromJson(data, &error).object();
    const int schema = root.value(QStringLiteral("schema")).toInt();
    if (error.error != QJsonParseError::NoError || (schema != 1 && schema != 2 && schema != 3))
        return;
    m_pinned.clear();
    m_groups.clear();
    m_groupMetadata.clear();
    m_groupOrder.clear();
    m_active.clear();
    for (const QJsonValue& value : root.value(QStringLiteral("pinned")).toArray())
        if (!value.toString().trimmed().isEmpty())
            m_pinned.insert(value.toString().trimmed());
    const QJsonObject groups = root.value(QStringLiteral("groups")).toObject();
    for (auto it = groups.begin(); it != groups.end(); ++it)
        if (!it.key().trimmed().isEmpty())
            m_groups.insert(it.key().trimmed(), it.value().toString().trimmed());
    if (schema >= 2)
        m_active = root.value(QStringLiteral("active")).toString().trimmed();
    if (schema >= 3) {
        for (const QJsonValue& value : root.value(QStringLiteral("groupOrder")).toArray()) {
            const QString name = value.toString().trimmed();
            if (!name.isEmpty())
                m_groupOrder.append(name);
        }
        const QJsonObject metadata = root.value(QStringLiteral("groupMetadata")).toObject();
        for (auto it = metadata.begin(); it != metadata.end(); ++it) {
            const QString name = it.key().trimmed();
            const QJsonObject value = it.value().toObject();
            if (name.isEmpty() || value.isEmpty())
                continue;
            GroupMetadata group;
            group.color = QColor(value.value(QStringLiteral("color")).toString());
            group.order = value.value(QStringLiteral("order")).toInt();
            group.collapsed = value.value(QStringLiteral("collapsed")).toBool();
            m_groupMetadata.insert(name, group);
        }
    }
    ensureGroupMetadata();

    // Per-tab appearance is a real element target: restore only the bounded
    // QFont attributes that Qt widgets can apply without inventing layout
    // semantics such as line-height or baseline offsets.
    const QJsonObject appearance = root.value(QStringLiteral("appearance")).toObject();
    for (auto it = appearance.begin(); it != appearance.end(); ++it) {
        m_appearanceOverrides.insert(it.key());
        QWidget* page = nullptr;
        for (int i = 0; i < pageCount(); ++i)
            if (tabKey(i) == it.key()) { page = pageAt(i); break; }
        if (!page || !it.value().isObject())
            continue;
        const QJsonObject value = it.value().toObject();
        QFont font = page->font();
        if (value.contains(QStringLiteral("family"))) font.setFamily(value.value(QStringLiteral("family")).toString());
        if (value.contains(QStringLiteral("pointSize"))) font.setPointSize(value.value(QStringLiteral("pointSize")).toInt());
        if (value.contains(QStringLiteral("weight"))) font.setWeight(static_cast<QFont::Weight>(value.value(QStringLiteral("weight")).toInt()));
        if (value.contains(QStringLiteral("style"))) font.setStyle(static_cast<QFont::Style>(value.value(QStringLiteral("style")).toInt()));
        if (value.contains(QStringLiteral("underline"))) font.setUnderline(value.value(QStringLiteral("underline")).toBool());
        if (value.contains(QStringLiteral("strikeOut"))) font.setStrikeOut(value.value(QStringLiteral("strikeOut")).toBool());
        if (value.contains(QStringLiteral("overline"))) font.setOverline(value.value(QStringLiteral("overline")).toBool());
        if (value.contains(QStringLiteral("capitalization"))) font.setCapitalization(static_cast<QFont::Capitalization>(value.value(QStringLiteral("capitalization")).toInt()));
        if (value.contains(QStringLiteral("letterSpacing"))) font.setLetterSpacing(QFont::AbsoluteSpacing, value.value(QStringLiteral("letterSpacing")).toDouble());
        if (value.contains(QStringLiteral("wordSpacing"))) font.setWordSpacing(value.value(QStringLiteral("wordSpacing")).toDouble());
        page->setFont(font);
    }
}

void CTabStateManager::save() const
{
    if (m_restoring || !m_settings || pageCount() == 0)
        return;
    QJsonObject root;
    root.insert(QStringLiteral("schema"), 3);
    QJsonArray order;
    for (int i = 0; i < pageCount(); ++i)
        order.append(tabKey(i));
    root.insert(QStringLiteral("order"), order);
    QJsonArray pinned;
    QStringList pinnedNames = m_pinned.values();
    pinnedNames.sort(Qt::CaseInsensitive);
    for (const QString& name : pinnedNames)
        pinned.append(name);
    root.insert(QStringLiteral("pinned"), pinned);
    QJsonObject groups;
    for (auto it = m_groups.cbegin(); it != m_groups.cend(); ++it)
        groups.insert(it.key(), it.value());
    root.insert(QStringLiteral("groups"), groups);
    QJsonArray groupOrder;
    const QStringList ordered = orderedGroupNames();
    for (const QString& name : ordered)
        groupOrder.append(name);
    root.insert(QStringLiteral("groupOrder"), groupOrder);
    QJsonObject metadata;
    for (const QString& name : ordered) {
        const GroupMetadata value = m_groupMetadata.value(name);
        QJsonObject entry;
        entry.insert(QStringLiteral("color"), value.color.name(QColor::HexArgb));
        entry.insert(QStringLiteral("order"), value.order);
        entry.insert(QStringLiteral("collapsed"), value.collapsed);
        metadata.insert(name, entry);
    }
    root.insert(QStringLiteral("groupMetadata"), metadata);
    root.insert(QStringLiteral("active"), m_active.isEmpty() ? tabKey(currentIndex()) : m_active);
    QJsonObject appearance;
    for (const QString& key : m_appearanceOverrides) {
        QWidget* page = nullptr;
        for (int i = 0; i < pageCount(); ++i)
            if (tabKey(i) == key) { page = pageAt(i); break; }
        if (!page)
            continue;
        const QFont font = page->font();
        QJsonObject value;
        value.insert(QStringLiteral("family"), font.family());
        value.insert(QStringLiteral("pointSize"), font.pointSize());
        value.insert(QStringLiteral("weight"), static_cast<int>(font.weight()));
        value.insert(QStringLiteral("style"), static_cast<int>(font.style()));
        value.insert(QStringLiteral("underline"), font.underline());
        value.insert(QStringLiteral("strikeOut"), font.strikeOut());
        value.insert(QStringLiteral("overline"), font.overline());
        value.insert(QStringLiteral("capitalization"), static_cast<int>(font.capitalization()));
        value.insert(QStringLiteral("letterSpacing"), font.letterSpacing());
        value.insert(QStringLiteral("wordSpacing"), font.wordSpacing());
        appearance.insert(key, value);
    }
    root.insert(QStringLiteral("appearance"), appearance);
    m_settings->SetBlob(m_key, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void CTabStateManager::restoreOrder()
{
    if ((!m_tabs && !m_pages) || !m_settings)
        return;
    const QJsonDocument document = QJsonDocument::fromJson(m_settings->GetBlob(m_key));
    const QJsonObject root = document.object();
    const int schema = root.value(QStringLiteral("schema")).toInt();
    if (schema != 1 && schema != 2 && schema != 3)
        return;
    QScopedValueRollback<bool> restoring(m_restoring, true);
    const QJsonArray order = root.value(QStringLiteral("order")).toArray();
    if (m_tabs) {
        QTabBar* bar = m_tabs->tabBar();
        for (int target = 0; target < order.size(); ++target) {
            const QString wanted = order.at(target).toString();
            int current = -1;
            for (int i = target; i < pageCount(); ++i) if (tabKey(i) == wanted) { current = i; break; }
            if (current >= 0 && current != target)
                bar->moveTab(current, target);
        }
    } else if (m_pages) {
        QList<QWidget*> pages;
        for (const QJsonValue& value : order) {
            const QString wanted = value.toString();
            for (int index = 0; index < pageCount(); ++index)
                if (tabKey(index) == wanted) { pages.append(pageAt(index)); break; }
        }
        for (int index = 0; index < pageCount(); ++index)
            if (!pages.contains(pageAt(index)))
                pages.append(pageAt(index));
        QHash<QString, QListWidgetItem*> navigationItems;
        if (auto* list = qobject_cast<QListWidget*>(m_navigation.data())) {
            for (int index = list->count() - 1; index >= 0; --index) {
                QListWidgetItem* item = list->takeItem(index);
                const int pageIndex = item->data(Qt::UserRole).toInt();
                if (pageIndex >= 0 && pageIndex < pageCount())
                    navigationItems.insert(tabKey(pageIndex), item);
                else
                    delete item;
            }
            list->clear();
        }
        for (int target = 0; target < pages.size(); ++target) {
            m_pages->removeWidget(pages.at(target));
            m_pages->insertWidget(target, pages.at(target));
        }
        if (auto* list = qobject_cast<QListWidget*>(m_navigation.data())) {
            for (int index = 0; index < pages.size(); ++index) {
                QListWidgetItem* item = navigationItems.take(tabKey(index));
                if (!item)
                    continue;
                item->setData(Qt::UserRole, index);
                list->addItem(item);
            }
        }
    }
    if (!m_active.isEmpty()) {
        for (int i = 0; i < pageCount(); ++i) {
            if (tabKey(i) == m_active) {
                setCurrentIndex(i);
                break;
            }
        }
    }
    ensureGroupMetadata();
}

void CTabStateManager::showContextMenu(const QPoint& position)
{
    QWidget* owner = ownerWidget();
    if (!owner || pageCount() == 0)
        return;
    const int index = itemAt(position);
    if (index < 0)
        return;
    const QString name = tabKey(index);
    QMenu menu;
    QWidgetAction* searchAction = new QWidgetAction(&menu);
    QLineEdit* filter = new QLineEdit(&menu);
    filter->setPlaceholderText(tr("Filter tab actions"));
    filter->setAccessibleName(tr("Tab action search"));
    searchAction->setDefaultWidget(filter);
    menu.addAction(searchAction);
    QAction* pin = menu.addAction(m_pinned.contains(name) ? tr("Unpin tab") : tr("Pin tab"));
    QAction* currentStripSearch = menu.addAction(tr("Search current tab strip…"));
    currentStripSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    QAction* currentGroupSearch = menu.addAction(tr("Search current tab group…"));
    currentGroupSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    currentGroupSearch->setEnabled(!groupForTab(name).isEmpty());
    QAction* groupNameSearch = menu.addAction(tr("Search tab groups…"));
    groupNameSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    QAction* searchTabs = menu.addAction(tr("Search all open tabs…"));
    searchTabs->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    QAction* group = menu.addAction(tr("Move… into group…"));
    QAction* manageGroups = menu.addAction(tr("Manage tab groups…"));
    QAction* clearGroup = menu.addAction(tr("Remove from group"));
    QAction* edit = menu.addAction(tr("Edit tab page typography…"));
    connect(filter, &QLineEdit::textChanged, &menu, [filter, pin, currentStripSearch, currentGroupSearch, groupNameSearch, searchTabs, group, manageGroups, clearGroup, edit](const QString& query) {
        for (QAction* action : { pin, currentStripSearch, currentGroupSearch, groupNameSearch, searchTabs, group, manageGroups, clearGroup, edit })
            action->setVisible(query.isEmpty() || action->text().contains(query, Qt::CaseInsensitive));
    });
    connect(currentStripSearch, &QAction::triggered, this, [this, position]() { showScopedTabSearch(SearchScope::CurrentStrip, position); });
    connect(currentGroupSearch, &QAction::triggered, this, [this, position, name]() { showScopedTabSearch(SearchScope::CurrentGroup, position, groupForTab(name)); });
    connect(groupNameSearch, &QAction::triggered, this, [this, position]() { showScopedTabSearch(SearchScope::GroupNames, position); });
    connect(searchTabs, &QAction::triggered, this, [this, position]() { showScopedTabSearch(SearchScope::MasterTabs, position); });
    connect(pin, &QAction::triggered, this, [this, name]() {
        if (m_pinned.contains(name)) m_pinned.remove(name); else m_pinned.insert(name);
        restoreGroupedOrder();
        applyGroupPresentation();
        save();
    });
    connect(group, &QAction::triggered, this, [this, name, position]() {
        showGroupPicker(name, position);
    });
    connect(manageGroups, &QAction::triggered, this, [this, position]() { showManageGroups(position); });
    connect(clearGroup, &QAction::triggered, this, [this, name]() { setGroupForTab(name, QString()); restoreGroupedOrder(); applyGroupPresentation(); save(); });
    connect(edit, &QAction::triggered, this, [this, name]() {
        QWidget* page = nullptr;
        for (int i = 0; i < pageCount(); ++i)
            if (tabKey(i) == name) { page = pageAt(i); break; }
        if (!page)
            return;
        QDialog* editor = new QDialog(ownerWidget(), Qt::Tool | Qt::WindowStaysOnTopHint);
        trackTransient(editor);
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->setWindowTitle(tr("Edit tab page typography"));
        QFormLayout* form = new QFormLayout(editor);
        QFontComboBox* font = new QFontComboBox(editor);
        font->setCurrentFont(page->font());
        font->setAccessibleName(tr("Tab font"));
        form->addRow(tr("Font"), font);
        QSpinBox* size = new QSpinBox(editor);
        size->setRange(8, 48);
        size->setValue(page->font().pointSize() > 0 ? page->font().pointSize() : 10);
        size->setAccessibleName(tr("Tab font size"));
        form->addRow(tr("Size"), size);
        QComboBox* weight = new QComboBox(editor);
        const QList<QPair<QString, int>> weights = {
            {tr("Thin"), QFont::Thin}, {tr("Extra light"), QFont::ExtraLight},
            {tr("Light"), QFont::Light}, {tr("Normal"), QFont::Normal},
            {tr("Medium"), QFont::Medium}, {tr("Demi bold"), QFont::DemiBold},
            {tr("Bold"), QFont::Bold}, {tr("Extra bold"), QFont::ExtraBold},
            {tr("Black"), QFont::Black}
        };
        for (const auto& option : weights)
            weight->addItem(option.first, option.second);
        const int weightIndex = weight->findData(page->font().weight());
        if (weightIndex >= 0)
            weight->setCurrentIndex(weightIndex);
        weight->setAccessibleName(tr("Tab font weight"));
        form->addRow(tr("Weight"), weight);
        QComboBox* style = new QComboBox(editor);
        style->addItem(tr("Normal"), QFont::StyleNormal);
        style->addItem(tr("Italic"), QFont::StyleItalic);
        style->addItem(tr("Oblique"), QFont::StyleOblique);
        const int styleIndex = style->findData(page->font().style());
        if (styleIndex >= 0)
            style->setCurrentIndex(styleIndex);
        style->setAccessibleName(tr("Tab font style"));
        form->addRow(tr("Style"), style);
        QCheckBox* underline = new QCheckBox(tr("Underline"), editor);
        underline->setChecked(page->font().underline());
        underline->setAccessibleName(tr("Tab underline"));
        QCheckBox* strikeOut = new QCheckBox(tr("Strikeout"), editor);
        strikeOut->setChecked(page->font().strikeOut());
        strikeOut->setAccessibleName(tr("Tab strikeout"));
        QCheckBox* overline = new QCheckBox(tr("Overline"), editor);
        overline->setChecked(page->font().overline());
        overline->setAccessibleName(tr("Tab overline"));
        QHBoxLayout* decorations = new QHBoxLayout();
        decorations->addWidget(underline);
        decorations->addWidget(strikeOut);
        decorations->addWidget(overline);
        decorations->addStretch();
        form->addRow(tr("Decorations"), decorations);
        QComboBox* capitalization = new QComboBox(editor);
        capitalization->addItem(tr("Mixed case"), QFont::MixedCase);
        capitalization->addItem(tr("Small caps"), QFont::SmallCaps);
        capitalization->addItem(tr("All uppercase"), QFont::AllUppercase);
        capitalization->addItem(tr("All lowercase"), QFont::AllLowercase);
        capitalization->addItem(tr("Capitalize words"), QFont::Capitalize);
        const int capitalizationIndex = capitalization->findData(page->font().capitalization());
        if (capitalizationIndex >= 0)
            capitalization->setCurrentIndex(capitalizationIndex);
        capitalization->setAccessibleName(tr("Tab capitalization"));
        form->addRow(tr("Capitalization"), capitalization);
        QDoubleSpinBox* letterSpacing = new QDoubleSpinBox(editor);
        QDoubleSpinBox* wordSpacing = new QDoubleSpinBox(editor);
        for (QDoubleSpinBox* spacing : {letterSpacing, wordSpacing}) {
            spacing->setRange(-20.0, 100.0);
            spacing->setDecimals(1);
            spacing->setSingleStep(0.5);
            spacing->setSuffix(tr(" px"));
        }
        letterSpacing->setValue(page->font().letterSpacing());
        wordSpacing->setValue(page->font().wordSpacing());
        letterSpacing->setAccessibleName(tr("Tab letter spacing"));
        wordSpacing->setAccessibleName(tr("Tab word spacing"));
        form->addRow(tr("Letter spacing"), letterSpacing);
        form->addRow(tr("Word spacing"), wordSpacing);
        QLabel* limitation = new QLabel(tr("This target applies to the tab page content, not the tab-bar label. It uses only QFont properties that Qt widgets can render consistently. Line-height, baseline offset, superscript, subscript, variable-font axes, underline variants, and text effects such as outline, shadow, and glow are not represented."), editor);
        limitation->setWordWrap(true);
        limitation->setProperty("secondary", true);
        form->addRow(tr("Not represented"), limitation);
        QPushButton* apply = new QPushButton(tr("Apply"), editor);
        form->addRow(QString(), apply);
        QPointer<QWidget> pageGuard(page);
        connect(apply, &QPushButton::clicked, editor, [this, pageGuard, font, size, weight, style, underline, strikeOut, overline, capitalization, letterSpacing, wordSpacing, editor, name]() {
            if (!pageGuard) {
                editor->close();
                return;
            }
            QFont value = font->currentFont();
            value.setPointSize(size->value());
            value.setWeight(static_cast<QFont::Weight>(weight->currentData().toInt()));
            value.setStyle(static_cast<QFont::Style>(style->currentData().toInt()));
            value.setUnderline(underline->isChecked());
            value.setStrikeOut(strikeOut->isChecked());
            value.setOverline(overline->isChecked());
            value.setCapitalization(static_cast<QFont::Capitalization>(capitalization->currentData().toInt()));
            value.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing->value());
            value.setWordSpacing(wordSpacing->value());
            pageGuard->setFont(value);
            m_appearanceOverrides.insert(name);
            save();
            editor->close();
        });
        int pageIndex = -1;
        for (int i = 0; i < pageCount(); ++i)
            if (pageAt(i) == page) { pageIndex = i; break; }
        const QPoint anchor = mapToGlobal(itemRect(pageIndex).bottomLeft());
        editor->move(anchor);
        editor->show();
    });
    QPointer<CTabStateManager> guard(this);
    menu.exec(mapToGlobal(position));
    if (!guard)
        return;
}

void CTabStateManager::showManageGroups(const QPoint& position)
{
    QWidget* owner = ownerWidget();
    if (!owner || pageCount() == 0)
        return;

    ensureGroupMetadata();
    QDialog* dialog = new QDialog(owner, Qt::Tool | Qt::WindowStaysOnTopHint);
    trackTransient(dialog);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::NonModal);
    dialog->setWindowTitle(tr("Manage tab groups"));
    dialog->setAccessibleName(tr("Manage tab groups"));
    dialog->setMinimumSize(560, 420);

    auto* root = new QVBoxLayout(dialog);
    auto* description = new QLabel(tr("Create and arrange groups without closing tabs. Pinned tabs stay first and visible; ungrouped tabs stay last."), dialog);
    description->setWordWrap(true);
    description->setAccessibleDescription(tr("Group order, colors, and collapsed state are saved with this tab strip."));
    root->addWidget(description);

    auto* createRow = new QHBoxLayout();
    auto* createName = new QLineEdit(dialog);
    createName->setMaxLength(80);
    createName->setPlaceholderText(tr("New group name"));
    createName->setAccessibleName(tr("New tab group name"));
    auto* create = new QPushButton(tr("Create group"), dialog);
    create->setAccessibleDescription(tr("Create an empty tab group with the entered name."));
    createRow->addWidget(createName, 1);
    createRow->addWidget(create);
    root->addLayout(createRow);

    auto* scroll = new QScrollArea(dialog);
    scroll->setWidgetResizable(true);
    scroll->setAccessibleName(tr("Tab group list"));
    auto* panel = new QWidget(scroll);
    auto* groupsLayout = new QVBoxLayout(panel);
    groupsLayout->setContentsMargins(4, 4, 4, 4);
    scroll->setWidget(panel);
    root->addWidget(scroll, 1);

    auto* close = new QPushButton(tr("Close"), dialog);
    close->setAccessibleDescription(tr("Close group management and keep the saved changes."));
    root->addWidget(close, 0, Qt::AlignRight);
    connect(close, &QPushButton::clicked, dialog, &QDialog::close);

    const auto rebuild = QSharedPointer<std::function<void()>>::create();
    *rebuild = [this, dialog, createName, groupsLayout, rebuild]() {
        while (QLayoutItem* item = groupsLayout->takeAt(0)) {
            if (QWidget* widget = item->widget())
                widget->deleteLater();
            delete item;
        }
        ensureGroupMetadata();
        const QStringList names = orderedGroupNames();
        if (names.isEmpty()) {
            auto* empty = new QLabel(tr("No tab groups yet. Create one above or use Move… into group… from a tab."), groupsLayout->parentWidget());
            empty->setWordWrap(true);
            empty->setAccessibleDescription(tr("The group list is empty."));
            groupsLayout->addWidget(empty);
        }
        for (const QString& originalName : names) {
            const GroupMetadata metadata = m_groupMetadata.value(originalName);
            int memberCount = 0;
            for (int index = 0; index < pageCount(); ++index)
                if (groupForTab(tabKey(index)).compare(originalName, Qt::CaseInsensitive) == 0)
                    ++memberCount;
            auto* row = new QGroupBox(tr("%1 (%2 members)").arg(originalName).arg(memberCount), groupsLayout->parentWidget());
            auto* layout = new QGridLayout(row);
            auto* name = new QLineEdit(originalName, row);
            name->setMaxLength(80);
            name->setAccessibleName(tr("Name for tab group %1").arg(originalName));
            auto* color = new QPushButton(tr("Color"), row);
            color->setAccessibleName(tr("Color for tab group %1").arg(originalName));
            color->setStyleSheet(QStringLiteral("QPushButton { background: %1; }").arg(metadata.color.name(QColor::HexArgb)));
            auto* collapsed = new QCheckBox(tr("Collapsed"), row);
            collapsed->setChecked(metadata.collapsed);
            collapsed->setAccessibleDescription(tr("Hide non-active members while keeping this preference saved."));
            auto* up = new QPushButton(tr("Move up"), row);
            auto* down = new QPushButton(tr("Move down"), row);
            auto* remove = new QPushButton(tr("Remove"), row);
            remove->setAccessibleDescription(tr("Remove the group and leave its tabs open and ungrouped."));
            layout->addWidget(new QLabel(tr("Name"), row), 0, 0);
            layout->addWidget(name, 0, 1, 1, 5);
            layout->addWidget(color, 1, 0);
            layout->addWidget(collapsed, 1, 1);
            layout->addWidget(up, 1, 2);
            layout->addWidget(down, 1, 3);
            layout->addWidget(remove, 1, 4);
            groupsLayout->addWidget(row);

            connect(name, &QLineEdit::editingFinished, dialog, [this, name, originalName, rebuild]() {
                const QString candidate = name->text().trimmed();
                QString error;
                if (!validGroupName(candidate, &error) || groupNameInUse(candidate, originalName)) {
                    name->setText(originalName);
                    return;
                }
                for (auto it = m_groups.begin(); it != m_groups.end(); ++it)
                    if (it.value().compare(originalName, Qt::CaseInsensitive) == 0)
                        it.value() = candidate;
                GroupMetadata metadata = m_groupMetadata.take(originalName);
                m_groupMetadata.insert(candidate, metadata);
                for (QString& group : m_groupOrder)
                    if (group.compare(originalName, Qt::CaseInsensitive) == 0)
                        group = candidate;
                ensureGroupMetadata();
                save();
                applyGroupPresentation();
                (*rebuild)();
            });
            connect(color, &QPushButton::clicked, dialog, [this, color, originalName, dialog]() {
                const QColor chosen = QColorDialog::getColor(m_groupMetadata.value(originalName).color, dialog, tr("Choose tab group color"), QColorDialog::ShowAlphaChannel);
                if (!chosen.isValid())
                    return;
                m_groupMetadata[originalName].color = chosen;
                color->setStyleSheet(QStringLiteral("QPushButton { background: %1; }").arg(chosen.name(QColor::HexArgb)));
                applyGroupPresentation();
                save();
            });
            connect(collapsed, &QCheckBox::toggled, dialog, [this, originalName](bool value) {
                m_groupMetadata[originalName].collapsed = value;
                applyGroupPresentation();
                save();
            });
            connect(up, &QPushButton::clicked, dialog, [this, originalName, rebuild]() {
                const int index = m_groupOrder.indexOf(originalName);
                if (index > 0) {
                    m_groupOrder.swapItemsAt(index, index - 1);
                    ensureGroupMetadata();
                    restoreGroupedOrder();
                    applyGroupPresentation();
                    save();
                    (*rebuild)();
                }
            });
            connect(down, &QPushButton::clicked, dialog, [this, originalName, rebuild]() {
                const int index = m_groupOrder.indexOf(originalName);
                if (index >= 0 && index + 1 < m_groupOrder.size()) {
                    m_groupOrder.swapItemsAt(index, index + 1);
                    ensureGroupMetadata();
                    restoreGroupedOrder();
                    applyGroupPresentation();
                    save();
                    (*rebuild)();
                }
            });
            connect(remove, &QPushButton::clicked, dialog, [this, originalName, dialog, rebuild]() {
                if (QMessageBox::question(dialog, tr("Remove tab group"), tr("Remove %1? Its tabs remain open and become ungrouped.").arg(originalName)) != QMessageBox::Yes)
                    return;
                for (auto it = m_groups.begin(); it != m_groups.end();) {
                    if (it.value().compare(originalName, Qt::CaseInsensitive) == 0)
                        it = m_groups.erase(it);
                    else
                        ++it;
                }
                m_groupMetadata.remove(originalName);
                m_groupOrder.removeAll(originalName);
                applyGroupPresentation();
                save();
                (*rebuild)();
            });
        }
        groupsLayout->addStretch(1);
        createName->setFocus();
    };
    connect(create, &QPushButton::clicked, dialog, [this, createName, rebuild]() {
        const QString candidate = createName->text().trimmed();
        QString error;
        if (!validGroupName(candidate, &error) || groupNameInUse(candidate))
            return;
        GroupMetadata metadata;
        metadata.color = defaultGroupColor(candidate);
        metadata.order = m_groupOrder.size();
        metadata.collapsed = false;
        m_groupMetadata.insert(candidate, metadata);
        m_groupOrder.append(candidate);
        ensureGroupMetadata();
        save();
        applyGroupPresentation();
        createName->clear();
        (*rebuild)();
    });
    (*rebuild)();
    dialog->move(mapToGlobal(position));
    dialog->show();
}

void CTabStateManager::showGroupPicker(const QString& tabName, const QPoint& position)
{
    QWidget* owner = ownerWidget();
    if (!owner || pageCount() == 0)
        return;

    QDialog* dialog = new QDialog(owner, Qt::Tool | Qt::WindowStaysOnTopHint);
    trackTransient(dialog);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Move tab into group"));
    dialog->setMinimumSize(420, 340);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* description = new QLabel(tr("Choose an existing group or create a new one. The group stays collapsed when it was collapsed before this move."), dialog);
    description->setWordWrap(true);
    layout->addWidget(description);

    QHBoxLayout* searchRow = new QHBoxLayout();
    QLineEdit* search = new QLineEdit(dialog);
    search->setPlaceholderText(tr("Search groups"));
    search->setAccessibleName(tr("Group search"));
    QCheckBox* regex = new QCheckBox(tr("Regex"), dialog);
    regex->setToolTip(tr("Use the regex builder for this group search"));
    regex->setAccessibleName(tr("Enable regular expression group search"));
    searchRow->addWidget(search, 1);
    searchRow->addWidget(regex);
    layout->addLayout(searchRow);

    QGroupBox* builder = new QGroupBox(tr("Regex builder"), dialog);
    QFormLayout* builderLayout = new QFormLayout(builder);
    QLineEdit* pattern = new QLineEdit(dialog);
    pattern->setPlaceholderText(tr("Raw pattern (for example: ^Work)"));
    pattern->setAccessibleName(tr("Group regex pattern"));
    pattern->setEnabled(false);
    QLabel* validation = new QLabel(tr("Plain-text search is active."), dialog);
    validation->setWordWrap(true);
    validation->setAccessibleName(tr("Group regex validation"));
    builderLayout->addRow(tr("Pattern"), pattern);
    builderLayout->addRow(tr("Validation"), validation);
    layout->addWidget(builder);

    QListWidget* groups = new QListWidget(dialog);
    groups->setAccessibleName(tr("Available tab groups"));
    groups->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(groups, 1);

    QHBoxLayout* actions = new QHBoxLayout();
    QPushButton* create = new QPushButton(tr("Create new group…"), dialog);
    create->setAccessibleName(tr("Create a new tab group"));
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Move"));
    actions->addWidget(create);
    actions->addStretch(1);
    actions->addWidget(buttons);
    layout->addLayout(actions);

    auto refresh = [this, search, regex, pattern, validation, groups]() {
        const QString needle = (regex->isChecked() ? pattern->text() : search->text()).left(4096);
        QRegularExpression expression;
        if (regex->isChecked()) {
            expression = QRegularExpression(needle);
            if (!expression.isValid()) {
                validation->setText(tr("Invalid pattern: %1").arg(expression.errorString()));
                groups->clear();
                return;
            }
            validation->setText(tr("Valid regex; results update as you type."));
        } else {
            validation->setText(tr("Plain-text search is active."));
        }
        const QStringList sorted = orderedGroupNames();
        groups->clear();
        for (const QString& name : sorted) {
            const bool match = needle.isEmpty() || (regex->isChecked() ? expression.match(name).hasMatch() : name.contains(needle, Qt::CaseInsensitive));
            if (!match)
                continue;
            int members = 0;
            for (int index = 0; index < pageCount(); ++index)
                if (groupForTab(tabKey(index)).compare(name, Qt::CaseInsensitive) == 0)
                    ++members;
            const GroupMetadata metadata = m_groupMetadata.value(name);
            const QString state = metadata.collapsed ? tr("collapsed") : tr("expanded");
            QListWidgetItem* item = new QListWidgetItem(tr("%1  · %2 tabs  · %3").arg(name).arg(members).arg(state), groups);
            item->setData(Qt::UserRole, name);
            item->setToolTip(tr("Group %1 contains %2 tab(s), color %3, and is %4").arg(name).arg(members).arg(metadata.color.name(QColor::HexArgb), state));
            item->setBackground(metadata.color);
        }
        if (groups->count() > 0)
            groups->setCurrentRow(0);
    };
    connect(search, &QLineEdit::textChanged, dialog, [pattern, regex, refresh](const QString& value) {
        if (!regex->isChecked()) {
            QSignalBlocker blocker(pattern);
            pattern->setText(value);
        }
        refresh();
    });
    connect(pattern, &QLineEdit::textChanged, dialog, [refresh](const QString&) { refresh(); });
    connect(regex, &QCheckBox::toggled, dialog, [search, pattern, refresh](bool enabled) {
        QSignalBlocker blocker(pattern);
        pattern->setText(search->text());
        pattern->setEnabled(enabled);
        refresh();
    });
    connect(create, &QPushButton::clicked, dialog, [this, tabName, dialog, search, refresh]() {
        const QString value = search->text().trimmed();
        QString error;
        if (!validGroupName(value, &error) || groupNameInUse(value)) {
            search->setFocus();
            return;
        }
        GroupMetadata metadata;
        metadata.color = defaultGroupColor(value);
        metadata.order = m_groupOrder.size();
        metadata.collapsed = false;
        m_groupMetadata.insert(value, metadata);
        m_groupOrder.append(value);
        setGroupForTab(tabName, value);
        ensureGroupMetadata();
        restoreGroupedOrder();
        applyGroupPresentation();
        save();
        dialog->accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, dialog, [this, tabName, groups, dialog]() {
        QListWidgetItem* item = groups->currentItem();
        const QString value = item ? item->data(Qt::UserRole).toString().trimmed() : QString();
        if (value.isEmpty())
            return;
        setGroupForTab(tabName, value);
        ensureGroupMetadata();
        restoreGroupedOrder();
        applyGroupPresentation();
        save();
        dialog->accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    search->setFocus();
    refresh();
    const int anchorIndex = itemAt(position);
    const QRect anchorRect = anchorIndex >= 0 ? itemRect(anchorIndex) : QRect(QPoint(0, 0), QSize(1, 1));
    dialog->move(mapToGlobal(anchorRect.bottomLeft()));
    dialog->show();
}

void CTabStateManager::showTabSearch(const QPoint& position)
{
    showScopedTabSearch(SearchScope::MasterTabs, position);
}

void CTabStateManager::showScopedTabSearch(SearchScope scope, const QPoint& position, const QString& groupName)
{
    QWidget* owner = ownerWidget();
    if (!owner || pageCount() == 0)
        return;

    QDialog* dialog = new QDialog(owner, Qt::Tool | Qt::WindowStaysOnTopHint);
    trackTransient(dialog);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    const bool groupNames = scope == SearchScope::GroupNames;
    const bool groupScoped = scope == SearchScope::CurrentGroup;
    const QString title = groupNames ? tr("Search tab groups") : groupScoped ? tr("Search current tab group") : scope == SearchScope::CurrentStrip ? tr("Search current tab strip") : tr("Search all open tabs");
    dialog->setWindowTitle(title);
    dialog->setMinimumSize(440, 360);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QHBoxLayout* searchRow = new QHBoxLayout();
    QLineEdit* query = new QLineEdit(dialog);
    query->setPlaceholderText(groupNames ? tr("Search group names") : groupScoped ? tr("Search tabs in group %1").arg(groupName) : scope == SearchScope::CurrentStrip ? tr("Search tabs in this strip") : tr("Search tab labels and groups"));
    query->setAccessibleName(title);
    QCheckBox* regex = new QCheckBox(tr("Regex"), dialog);
    regex->setToolTip(tr("Use the regex builder for this tab search"));
    regex->setAccessibleName(tr("Enable regular expression search"));
    QCheckBox* caseSensitive = new QCheckBox(tr("Case sensitive"), dialog);
    caseSensitive->setAccessibleName(tr("Case sensitive tab search"));
    searchRow->addWidget(query, 1);
    searchRow->addWidget(regex);
    searchRow->addWidget(caseSensitive);
    layout->addLayout(searchRow);

    QGroupBox* builder = new QGroupBox(tr("Regex builder"), dialog);
    QFormLayout* builderLayout = new QFormLayout(builder);
    QLineEdit* pattern = new QLineEdit(dialog);
    pattern->setPlaceholderText(tr("Raw pattern (for example: ^Settings)"));
    pattern->setAccessibleName(tr("Regex pattern"));
    pattern->setEnabled(false);
    QLineEdit* sample = new QLineEdit(dialog);
    sample->setPlaceholderText(tr("Sample text for capture preview"));
    sample->setAccessibleName(tr("Regex sample text"));
    QLabel* regexStatus = new QLabel(tr("Plain-text search is active."), dialog);
    regexStatus->setWordWrap(true);
    regexStatus->setAccessibleName(tr("Regex validation and capture preview"));
    builderLayout->addRow(tr("Pattern"), pattern);
    builderLayout->addRow(tr("Sample"), sample);
    builderLayout->addRow(tr("Validation"), regexStatus);
    layout->addWidget(builder);

    QListWidget* results = new QListWidget(dialog);
    results->setAccessibleName(groupNames ? tr("Tab group search results") : tr("Open tab search results"));
    results->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(results, 1);
    QLabel* count = new QLabel(dialog);
    count->setAccessibleName(tr("Matching tab count"));
    layout->addWidget(count);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    auto refresh = [this, query, regex, caseSensitive, pattern, sample, regexStatus, results, count, groupScoped, groupName, groupNames]() {
        const QString needle = query->text().left(4096);
        const bool useRegex = regex->isChecked();
        const Qt::CaseSensitivity sensitivity = caseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
        QRegularExpression expression;
        if (useRegex) {
            QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
            if (!caseSensitive->isChecked())
                options |= QRegularExpression::CaseInsensitiveOption;
            expression = QRegularExpression(pattern->text().left(4096), options);
            if (!expression.isValid()) {
                regexStatus->setText(tr("Invalid pattern: %1").arg(expression.errorString()));
                results->clear();
                count->setText(tr("0 matching tabs"));
                return;
            }
        }
        regexStatus->setText(useRegex ? tr("Valid pattern. Capture preview: %1").arg(expression.match(sample->text()).capturedTexts().join(QStringLiteral(" · "))) : tr("Plain-text search is active."));
        results->clear();
        int matches = 0;
        if (groupNames) {
            for (const QString& group : orderedGroupNames()) {
                if (!needle.isEmpty()) {
                    const bool matched = useRegex ? expression.match(group).hasMatch() : group.contains(needle, sensitivity);
                    if (!matched)
                        continue;
                }
                QListWidgetItem* item = new QListWidgetItem(group, results);
                int firstMember = -1;
                for (int index = 0; index < pageCount(); ++index)
                    if (groupForTab(tabKey(index)).compare(group, Qt::CaseInsensitive) == 0) { firstMember = index; break; }
                item->setData(Qt::UserRole, firstMember);
                item->setToolTip(tr("Tab group %1, %2").arg(group, m_groupMetadata.value(group).collapsed ? tr("collapsed") : tr("expanded")));
                item->setBackground(m_groupMetadata.value(group).color);
                ++matches;
            }
            count->setText(tr("%1 matching tab groups").arg(matches));
            if (results->count() > 0)
                results->setCurrentRow(0);
            return;
        }
        for (int i = 0; i < pageCount(); ++i) {
            const QString key = tabKey(i);
            const QString group = groupForTab(key);
            const QString label = pageText(i);
            if (groupScoped && group != groupName)
                continue;
            const QString haystack = label + QStringLiteral(" ") + key + QStringLiteral(" ") + group;
            bool matched = needle.isEmpty();
            if (!needle.isEmpty())
                matched = useRegex ? expression.match(haystack).hasMatch() : haystack.contains(needle, sensitivity);
            if (!matched)
                continue;
            QString display = label.isEmpty() ? key : label;
            if (!groupNames && !group.isEmpty())
                display += tr("  · group: %1").arg(group);
            if (!groupNames && m_pinned.contains(key))
                display += tr("  · pinned");
            QListWidgetItem* item = new QListWidgetItem(display, results);
            item->setData(Qt::UserRole, i);
            ++matches;
        }
        count->setText(tr("%1 matching tabs").arg(matches));
        if (results->count() > 0)
            results->setCurrentRow(0);
    };
    connect(query, &QLineEdit::textChanged, dialog, [pattern, regex, refresh](const QString& value) {
        if (!regex->isChecked()) {
            QSignalBlocker blocker(pattern);
            pattern->setText(value);
        }
        refresh();
    });
    connect(pattern, &QLineEdit::textChanged, dialog, [query, regex, refresh](const QString& value) {
        if (regex->isChecked()) {
            QSignalBlocker blocker(query);
            query->setText(value);
        }
        refresh();
    });
    connect(regex, &QCheckBox::toggled, dialog, [query, pattern, refresh](bool enabled) {
        QSignalBlocker blocker(pattern);
        pattern->setText(query->text());
        refresh();
        pattern->setEnabled(enabled);
    });
    connect(caseSensitive, &QCheckBox::toggled, dialog, [refresh](bool) { refresh(); });
    connect(sample, &QLineEdit::textChanged, dialog, [refresh](const QString&) { refresh(); });
    connect(results, &QListWidget::itemActivated, dialog, [this, dialog](QListWidgetItem* item) {
        const int index = item ? item->data(Qt::UserRole).toInt() : -1;
        if (index >= 0 && index < pageCount()) {
            setCurrentIndex(index);
            m_active = tabKey(index);
            save();
            dialog->close();
        }
    });
    query->setFocus();
    refresh();
    const int anchorIndex = itemAt(position);
    const QRect anchorRect = anchorIndex >= 0 ? itemRect(anchorIndex) : QRect(QPoint(0, 0), QSize(1, 1));
    const QPoint anchor = mapToGlobal(anchorRect.bottomLeft());
    dialog->move(anchor);
    dialog->show();
}

bool CTabStateManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == interactionWidget() && event->type() == QEvent::MouseButtonRelease)
        save();
    return QObject::eventFilter(watched, event);
}
