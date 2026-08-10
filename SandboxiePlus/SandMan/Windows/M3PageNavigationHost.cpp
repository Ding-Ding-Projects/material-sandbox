#include "stdafx.h"
#include "M3PageNavigationHost.h"
#include "M3SearchField.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

CM3PageNavigationHost::CM3PageNavigationHost(QWidget* parent)
    : QWidget(parent),
      m_search(new CM3SearchField(this)),
      m_pageList(new QListWidget(this)),
      m_stack(new QStackedWidget(this)),
      m_rightLayout(nullptr),
      m_provenance(new QLabel(this))
{
    setObjectName(QStringLiteral("m3PageNavigationHost"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* left = new QWidget(this);
    left->setObjectName(QStringLiteral("m3PageNavigationLeft"));
    left->setFixedWidth(280);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(16, 24, 16, 24);
    leftLayout->setSpacing(12);
    m_search->setHeightVariant(CM3SearchField::Control);
    m_search->setPlaceholderText(tr("Search settings"));
    leftLayout->addWidget(m_search);
    m_pageList->setObjectName(QStringLiteral("m3PageNavigationList"));
    m_pageList->setFrameShape(QFrame::NoFrame);
    m_pageList->setSpacing(4);
    m_pageList->setUniformItemSizes(true);
    m_pageList->setAccessibleName(tr("Settings pages"));
    leftLayout->addWidget(m_pageList, 1);

    auto* right = new QWidget(this);
    right->setObjectName(QStringLiteral("m3PageNavigationRight"));
    right->setProperty("m3Pane", true);
    m_rightLayout = new QVBoxLayout(right);
    m_rightLayout->setContentsMargins(24, 24, 24, 24);
    m_rightLayout->setSpacing(12);
    m_provenance->setObjectName(QStringLiteral("m3SettingsProvenance"));
    m_provenance->setWordWrap(true);
    m_provenance->setVisible(false);
    m_rightLayout->addWidget(m_provenance);
    m_rightLayout->addWidget(m_stack, 1);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(left);
    root->addWidget(right, 1);

    setStyleSheet(QStringLiteral(
        "QWidget#m3PageNavigationHost { background: palette(window); }"
        "QWidget#m3PageNavigationLeft { background: palette(window); }"
        "QWidget#m3PageNavigationRight { background: palette(base); border-top-left-radius: 16px; }"
        "QListWidget#m3PageNavigationList { background: transparent; border: 0; }"
        "QListWidget#m3PageNavigationList::item { min-height: 56px; border-radius: 28px; padding: 0 16px; }"
        "QListWidget#m3PageNavigationList::item:selected { background: palette(highlight); color: palette(highlighted-text); }"
        "QLabel#m3SettingsProvenance { color: palette(mid); padding: 0 4px 8px 4px; }"
        "QWidget[m3SearchMatch='true'] { background: palette(alternate-base); border-radius: 4px; }"));

    connect(m_pageList, &QListWidget::currentRowChanged,
            this, &CM3PageNavigationHost::selectPage);
    connect(m_search, &CM3SearchField::searchChanged,
            this, &CM3PageNavigationHost::filterPages);
}

CM3PageNavigationHost* CM3PageNavigationHost::adapt(QDialog* dialog,
                                                     QTabWidget* tabs,
                                                     const QString& searchPlaceholder)
{
    if (!dialog || !tabs || tabs->property("m3TwoPaneAdapted").toBool())
        return nullptr;
    QLayout* parentLayout = tabs->parentWidget() ? tabs->parentWidget()->layout() : nullptr;
    if (!parentLayout)
        return nullptr;

    auto* host = new CM3PageNavigationHost(tabs->parentWidget());
    if (!searchPlaceholder.isEmpty())
        host->m_search->setPlaceholderText(searchPlaceholder);

    QLayoutItem* replaced = parentLayout->replaceWidget(tabs, host, Qt::FindDirectChildrenOnly);
    delete replaced;
    host->bindExistingTabs(tabs);
    tabs->setProperty("m3TwoPaneAdapted", true);
    return host;
}

void CM3PageNavigationHost::bindExistingTabs(QTabWidget* tabs)
{
    if (!tabs || m_adaptedTabs)
        return;
    m_adaptedTabs = tabs;
    const int originalIndex = tabs->currentIndex();

    QLayoutItem* replaced = m_rightLayout->replaceWidget(m_stack, tabs);
    delete replaced;
    m_stack->hide();
    tabs->setParent(m_rightLayout->parentWidget());
    tabs->show();
    tabs->setDocumentMode(true);
    tabs->setProperty("m3HostedTabs", true);
    if (tabs->tabBar())
        tabs->tabBar()->hide();
    tabs->setStyleSheet(tabs->styleSheet() + QStringLiteral(
        "QTabWidget[m3HostedTabs='true']::pane { border: 0; background: transparent; }"));

    m_titles.clear();
    m_pageList->clear();
    for (int i = 0; i < tabs->count(); ++i)
        addNavigationItem(tabs->tabText(i), tabs->tabIcon(i), i);

    connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (m_pageList->currentRow() != index)
            m_pageList->setCurrentRow(index);
        emit currentPageChanged(index);
    });
    connect(tabs->tabBar(), &QTabBar::tabMoved, this, [this](int, int) {
        if (!m_adaptedTabs)
            return;
        const int current = m_adaptedTabs->currentIndex();
        m_titles.clear();
        m_pageList->clear();
        for (int i = 0; i < m_adaptedTabs->count(); ++i)
            addNavigationItem(m_adaptedTabs->tabText(i), m_adaptedTabs->tabIcon(i), i);
        setCurrentIndex(current);
    });
    setCurrentIndex(qBound(0, originalIndex, qMax(0, tabs->count() - 1)));
}

void CM3PageNavigationHost::addNavigationItem(const QString& title, const QIcon& icon, int index)
{
    auto* item = new QListWidgetItem(icon, title, m_pageList);
    item->setData(Qt::UserRole, index);
    item->setToolTip(title);
    item->setSizeHint(QSize(0, 56));
    m_titles.append(title);
}

void CM3PageNavigationHost::addPage(QWidget* page, const QString& title, const QIcon& icon)
{
    if (!page || m_adaptedTabs)
        return;
    page->setParent(m_stack);
    const int index = m_stack->addWidget(page);
    addNavigationItem(title, icon, index);
}

int CM3PageNavigationHost::pageCount() const
{
    return m_adaptedTabs ? m_adaptedTabs->count() : m_stack->count();
}

int CM3PageNavigationHost::currentIndex() const
{
    return m_adaptedTabs ? m_adaptedTabs->currentIndex() : m_stack->currentIndex();
}

void CM3PageNavigationHost::setCurrentIndex(int index)
{
    if (index < 0 || index >= pageCount())
        return;
    if (m_adaptedTabs)
        m_adaptedTabs->setCurrentIndex(index);
    else
        m_stack->setCurrentIndex(index);
    m_pageList->setCurrentRow(index);
}

void CM3PageNavigationHost::setProvenanceText(const QString& text)
{
    m_provenance->setText(text);
    m_provenance->setVisible(!text.trimmed().isEmpty());
}

CM3SearchField* CM3PageNavigationHost::searchField() const { return m_search; }

void CM3PageNavigationHost::selectPage(int row)
{
    if (row < 0 || row >= pageCount())
        return;
    if (m_adaptedTabs) {
        m_adaptedTabs->setCurrentIndex(row);
        return; // QTabWidget::currentChanged forwards the notification.
    }
    m_stack->setCurrentIndex(row);
    emit currentPageChanged(row);
}

void CM3PageNavigationHost::filterPages(QString query, bool, QRegularExpression expression,
                                        QString, bool valid, QString)
{
    const bool active = !query.isEmpty() && valid;
    int firstMatch = -1;
    for (int i = 0; i < pageCount(); ++i) {
        QWidget* page = pageAt(i);
        const int matches = active ? countMatches(page, expression) : 0;
        QListWidgetItem* item = m_pageList->item(i);
        if (item) {
            item->setText(active ? tr("%1  (%2)").arg(m_titles.value(i)).arg(matches)
                                 : m_titles.value(i));
            item->setHidden(active && matches == 0);
        }
        markMatches(page, expression, active);
        if (matches > 0 && firstMatch < 0)
            firstMatch = i;
    }
    QListWidgetItem* currentItem = m_pageList->item(currentIndex());
    if (active && firstMatch >= 0 && currentItem && currentItem->isHidden())
        setCurrentIndex(firstMatch);
    if (!active && pageCount() > 0 && m_pageList->currentRow() < 0)
        setCurrentIndex(0);
}

QWidget* CM3PageNavigationHost::pageAt(int index) const
{
    if (index < 0 || index >= pageCount())
        return nullptr;
    return m_adaptedTabs ? m_adaptedTabs->widget(index) : m_stack->widget(index);
}

QString CM3PageNavigationHost::searchableText(QWidget* page) const
{
    QStringList values;
    const QList<QWidget*> widgets = page->findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        if (auto* label = qobject_cast<QLabel*>(widget)) values << label->text();
        else if (auto* button = qobject_cast<QAbstractButton*>(widget)) values << button->text();
        else if (auto* group = qobject_cast<QGroupBox*>(widget)) values << group->title();
        else if (auto* line = qobject_cast<QLineEdit*>(widget)) values << line->placeholderText() << line->accessibleName();
        else if (auto* combo = qobject_cast<QComboBox*>(widget)) {
            values << combo->accessibleName();
            for (int i = 0; i < combo->count(); ++i) values << combo->itemText(i);
        } else values << widget->accessibleName();
    }
    return values.join(QLatin1Char('\n'));
}

int CM3PageNavigationHost::countMatches(QWidget* page, const QRegularExpression& expression) const
{
    if (!page || !expression.isValid())
        return 0;
    int count = 0;
    QRegularExpressionMatchIterator iterator = expression.globalMatch(searchableText(page));
    while (iterator.hasNext() && count < 999) {
        iterator.next();
        ++count;
    }
    return count;
}

void CM3PageNavigationHost::markMatches(QWidget* page, const QRegularExpression& expression, bool active)
{
    if (!page)
        return;
    const QList<QWidget*> widgets = page->findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        QString value;
        if (auto* label = qobject_cast<QLabel*>(widget)) value = label->text();
        else if (auto* button = qobject_cast<QAbstractButton*>(widget)) value = button->text();
        else if (auto* group = qobject_cast<QGroupBox*>(widget)) value = group->title();
        else value = widget->accessibleName();
        const bool match = active && !value.isEmpty() && expression.match(value).hasMatch();
        if (widget->property("m3SearchMatch").toBool() != match) {
            widget->setProperty("m3SearchMatch", match);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        }
    }
}
