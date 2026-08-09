#include "stdafx.h"
#include "TabStateManager.h"

#include "Settings.h"
#include <QAction>
#include <QDialog>
#include <QFontComboBox>
#include <QFormLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QWidgetAction>

CTabStateManager::CTabStateManager(QTabWidget* tabs, CSettings* settings, const QString& key, QObject* parent)
    : QObject(parent), m_tabs(tabs), m_settings(settings), m_key(key)
{
    if (!m_tabs)
        return;
    QTabBar* bar = m_tabs->tabBar();
    bar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(bar, &QTabBar::customContextMenuRequested, this, &CTabStateManager::showContextMenu);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) { save(); });
    connect(bar, &QTabBar::tabMoved, this, [this](int, int) { save(); });
    bar->installEventFilter(this);
    load();
    restoreOrder();
}

QString CTabStateManager::tabKey(int index) const
{
    return m_tabs && index >= 0 && index < m_tabs->count() ? tabKey(m_tabs->widget(index)) : QString();
}

QString CTabStateManager::tabKey(QWidget* page) const
{
    if (!page)
        return QString();
    const QString stable = page->objectName().trimmed();
    return stable.isEmpty() ? page->windowTitle().trimmed() : stable;
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
    if (error.error != QJsonParseError::NoError || root.value(QStringLiteral("schema")).toInt() != 1)
        return;
    for (const QJsonValue& value : root.value(QStringLiteral("pinned")).toArray())
        m_pinned.insert(value.toString());
    const QJsonObject groups = root.value(QStringLiteral("groups")).toObject();
    for (auto it = groups.begin(); it != groups.end(); ++it)
        m_groups.insert(it.key(), it.value().toString());
}

void CTabStateManager::save() const
{
    if (!m_settings || !m_tabs)
        return;
    QJsonObject root;
    root.insert(QStringLiteral("schema"), 1);
    QJsonArray order;
    for (int i = 0; i < m_tabs->count(); ++i)
        order.append(tabKey(i));
    root.insert(QStringLiteral("order"), order);
    QJsonArray pinned;
    for (const QString& name : m_pinned)
        pinned.append(name);
    root.insert(QStringLiteral("pinned"), pinned);
    QJsonObject groups;
    for (auto it = m_groups.cbegin(); it != m_groups.cend(); ++it)
        groups.insert(it.key(), it.value());
    root.insert(QStringLiteral("groups"), groups);
    m_settings->SetBlob(m_key, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void CTabStateManager::restoreOrder()
{
    if (!m_tabs || !m_settings)
        return;
    const QJsonDocument document = QJsonDocument::fromJson(m_settings->GetBlob(m_key));
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schema")).toInt() != 1)
        return;
    const QJsonArray order = root.value(QStringLiteral("order")).toArray();
    QTabBar* bar = m_tabs->tabBar();
    for (int target = 0; target < order.size(); ++target) {
        const QString wanted = order.at(target).toString();
        int current = -1;
        for (int i = target; i < m_tabs->count(); ++i) if (tabKey(i) == wanted) { current = i; break; }
        if (current >= 0 && current != target)
            bar->moveTab(current, target);
    }
}

void CTabStateManager::showContextMenu(const QPoint& position)
{
    if (!m_tabs)
        return;
    const int index = m_tabs->tabBar()->tabAt(position);
    if (index < 0)
        return;
    const QString name = tabKey(index);
    QMenu menu(m_tabs);
    QWidgetAction* searchAction = new QWidgetAction(&menu);
    QLineEdit* filter = new QLineEdit(&menu);
    filter->setPlaceholderText(tr("Filter tab actions"));
    filter->setAccessibleName(tr("Tab action search"));
    searchAction->setDefaultWidget(filter);
    menu.addAction(searchAction);
    QAction* pin = menu.addAction(m_pinned.contains(name) ? tr("Unpin tab") : tr("Pin tab"));
    QAction* group = menu.addAction(tr("Move… into group…"));
    QAction* clearGroup = menu.addAction(tr("Remove from group"));
    QAction* edit = menu.addAction(tr("Edit tab appearance…"));
    connect(filter, &QLineEdit::textChanged, &menu, [filter, pin, group, clearGroup, edit](const QString& query) {
        for (QAction* action : { pin, group, clearGroup, edit })
            action->setVisible(query.isEmpty() || action->text().contains(query, Qt::CaseInsensitive));
    });
    connect(pin, &QAction::triggered, &menu, [this, name]() {
        if (m_pinned.contains(name)) m_pinned.remove(name); else m_pinned.insert(name);
        save();
    });
    connect(group, &QAction::triggered, &menu, [this, name]() {
        bool ok = false;
        const QString value = QInputDialog::getText(m_tabs, tr("Move tab into group"), tr("Group name"), QLineEdit::Normal, m_groups.value(name), &ok).trimmed();
        if (ok && !value.isEmpty()) { m_groups.insert(name, value); save(); }
    });
    connect(clearGroup, &QAction::triggered, &menu, [this, name]() { m_groups.remove(name); save(); });
    connect(edit, &QAction::triggered, &menu, [this, name]() {
        QWidget* page = nullptr;
        for (int i = 0; i < m_tabs->count(); ++i)
            if (tabKey(i) == name) { page = m_tabs->widget(i); break; }
        if (!page)
            return;
        QDialog* editor = new QDialog(m_tabs, Qt::Tool | Qt::WindowStaysOnTopHint);
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->setWindowTitle(tr("Edit tab appearance"));
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
        QPushButton* apply = new QPushButton(tr("Apply"), editor);
        form->addRow(QString(), apply);
        connect(apply, &QPushButton::clicked, editor, [page, font, size, editor]() {
            QFont value = font->currentFont();
            value.setPointSize(size->value());
            page->setFont(value);
            editor->close();
        });
        const QPoint anchor = m_tabs->tabBar()->mapToGlobal(m_tabs->tabBar()->tabRect(m_tabs->indexOf(page)).bottomLeft());
        editor->move(anchor);
        editor->show();
    });
    menu.exec(m_tabs->tabBar()->mapToGlobal(position));
}

bool CTabStateManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_tabs->tabBar() && event->type() == QEvent::MouseButtonRelease)
        save();
    return QObject::eventFilter(watched, event);
}
