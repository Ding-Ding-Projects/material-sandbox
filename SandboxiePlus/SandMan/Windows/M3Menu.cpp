#include "stdafx.h"
#include "M3Menu.h"
#include "M3SearchField.h"

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QScreen>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QPointer>
#include <QWidgetAction>

namespace {
constexpr int kMenuWidth = 304;
constexpr int kMenuRowHeight = 48;
constexpr int kMenuSearchHeight = 40;

QString nativeMenuLabel(QString text)
{
    text.replace(QStringLiteral("&&"), QString(QChar(0x0001)));
    text.remove(QLatin1Char('&'));
    text.replace(QChar(0x0001), QStringLiteral("&"));
    const int shortcut = text.indexOf(QLatin1Char('\t'));
    if (shortcut >= 0)
        text.truncate(shortcut);
    return text.trimmed();
}

class NativeMenuSearchInstaller final : public QObject
{
public:
    explicit NativeMenuSearchInstaller(QApplication* application)
        : QObject(application), m_application(application)
    {
        application->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        QMenu* menu = qobject_cast<QMenu*>(watched);
        if (!menu)
            return QObject::eventFilter(watched, event);
        const bool hasSearch = menu->findChild<CM3SearchField*>(
            QStringLiteral("m3NativeMenuSearch")) != nullptr;
        if ((event->type() == QEvent::Polish || event->type() == QEvent::Show)
            && (!menu->property("m3NativeSearchInstalled").toBool() || !hasSearch)) {
            menu->setProperty("m3NativeSearchInstalled", false);
            install(menu);
        }
        if (event->type() == QEvent::Show) {
            if (auto* field = menu->findChild<CM3SearchField*>(QStringLiteral("m3NativeMenuSearch")))
                QTimer::singleShot(0, field, &CM3SearchField::focusEditor);
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void install(QMenu* menu)
    {
        if (!menu || menu->property("m3NativeSearchInstalled").toBool())
            return;
        menu->setProperty("m3NativeSearchInstalled", true);

        auto* wrapper = new QWidget(menu);
        wrapper->setObjectName(QStringLiteral("m3NativeMenuSearchWrapper"));
        auto* wrapperLayout = new QVBoxLayout(wrapper);
        wrapperLayout->setContentsMargins(8, 4, 8, 4);
        auto* search = new CM3SearchField(wrapper);
        search->setObjectName(QStringLiteral("m3NativeMenuSearch"));
        search->setHeightVariant(CM3SearchField::Menu);
        search->setPlaceholderText(menu->title().isEmpty()
            ? tr("Search actions")
            : tr("Search %1 actions").arg(nativeMenuLabel(menu->title()).toLower()));
        wrapperLayout->addWidget(search);

        auto* searchAction = new QWidgetAction(menu);
        searchAction->setProperty("m3MenuInfrastructure", true);
        searchAction->setDefaultWidget(wrapper);

        auto* emptyLabel = new QLabel(tr("Nothing in this menu matches that search."), menu);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setWordWrap(true);
        emptyLabel->setMinimumHeight(72);
        auto* emptyAction = new QWidgetAction(menu);
        emptyAction->setProperty("m3MenuInfrastructure", true);
        emptyAction->setDefaultWidget(emptyLabel);
        emptyAction->setVisible(false);

        QAction* first = menu->actions().isEmpty() ? nullptr : menu->actions().first();
        menu->insertAction(first, searchAction);
        menu->insertAction(first, emptyAction);

        const auto filter = [menu, searchAction, emptyAction](QString query, bool,
                                                              QRegularExpression expression,
                                                              QString, bool valid, QString) {
            const bool active = !query.isEmpty();
            int visibleRows = 0;
            for (QAction* action : menu->actions()) {
                if (!action || action == searchAction || action == emptyAction
                    || action->property("m3MenuInfrastructure").toBool())
                    continue;
                const bool originallyVisible = action->property("m3OriginalVisible").isValid()
                    ? action->property("m3OriginalVisible").toBool()
                    : action->isVisible();
                bool show = originallyVisible;
                if (action->isSeparator())
                    show = originallyVisible && !active;
                else if (active)
                    show = originallyVisible && valid
                        && expression.match(nativeMenuLabel(action->text())).hasMatch();
                action->setVisible(show);
                if (show && !action->isSeparator())
                    ++visibleRows;
            }
            emptyAction->setVisible(active && visibleRows == 0);
            menu->adjustSize();
        };

        connect(search, &CM3SearchField::searchChanged, menu, filter);
        connect(search, &CM3SearchField::escapePressed, menu, &QMenu::close);
        connect(menu, &QMenu::aboutToShow, menu, [menu, searchAction, emptyAction, search]() {
            const bool resumeSearch = menu->property("m3ResumeMenuSearch").toBool();
            menu->setProperty("m3ResumeMenuSearch", false);
            if (!resumeSearch) {
                for (QAction* action : menu->actions()) {
                    if (!action || action == searchAction || action == emptyAction
                        || action->property("m3MenuInfrastructure").toBool())
                        continue;
                    action->setProperty("m3OriginalVisible", action->isVisible());
                }
                search->setQuery(QString());
            }
            QTimer::singleShot(0, search, &CM3SearchField::focusEditor);
        });
        connect(menu, &QMenu::aboutToHide, menu, [menu, searchAction, emptyAction, search]() {
            if (menu->property("m3ChildDialogActive").toBool())
                return;
            search->setQuery(QString());
            for (QAction* action : menu->actions()) {
                if (!action || action == searchAction || action == emptyAction
                    || action->property("m3MenuInfrastructure").toBool())
                    continue;
                if (action->property("m3OriginalVisible").isValid()) {
                    action->setVisible(action->property("m3OriginalVisible").toBool());
                    action->setProperty("m3OriginalVisible", QVariant());
                }
            }
            emptyAction->setVisible(false);
        });
    }

    QApplication* m_application;
};
}

CM3Menu::CM3Menu(QMenu* sourceMenu, QWidget* parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint),
      m_sourceMenu(sourceMenu),
      m_actions(sourceMenu ? sourceMenu->actions() : QList<QAction*>()),
      m_search(nullptr),
      m_rowsLayout(nullptr),
      m_emptyLabel(nullptr),
      m_filterActive(false)
{
    initialize(sourceMenu ? sourceMenu->title() : QString());
}

CM3Menu::CM3Menu(const QList<QAction*>& actions, const QString& title, QWidget* parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint),
      m_actions(actions),
      m_search(nullptr),
      m_rowsLayout(nullptr),
      m_emptyLabel(nullptr),
      m_filterActive(false)
{
    initialize(title);
}

CM3Menu* CM3Menu::popup(QMenu* sourceMenu, QWidget* invoker, const QPoint& globalPosition)
{
    if (!sourceMenu)
        return nullptr;
    auto* menu = new CM3Menu(sourceMenu, invoker ? invoker->window() : nullptr);
    menu->setAttribute(Qt::WA_DeleteOnClose, true);
    menu->openAt(globalPosition, invoker);
    return menu;
}

CM3Menu* CM3Menu::popup(const QList<QAction*>& actions, const QString& title,
                        QWidget* invoker, const QPoint& globalPosition)
{
    auto* menu = new CM3Menu(actions, title, invoker ? invoker->window() : nullptr);
    menu->setAttribute(Qt::WA_DeleteOnClose, true);
    menu->openAt(globalPosition, invoker);
    return menu;
}

void CM3Menu::installGlobal(QApplication* application)
{
    if (!application || application->property("m3NativeMenuSearchInstaller").toBool())
        return;
    application->setProperty("m3NativeMenuSearchInstaller", true);
    new NativeMenuSearchInstaller(application);
}

void CM3Menu::initialize(const QString& title)
{
    setObjectName(QStringLiteral("m3Menu"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);
    setFixedWidth(kMenuWidth);
    setAccessibleName(title.isEmpty() ? tr("Actions menu") : title);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(4);

    m_search = new CM3SearchField(this);
    m_search->setHeightVariant(CM3SearchField::Menu);
    m_search->setPlaceholderText(title.isEmpty() ? tr("Search actions")
                                                  : tr("Search %1 actions").arg(displayText(title).toLower()));
    m_search->setAccessibleName(tr("Search this menu"));
    root->addWidget(m_search);

    auto* scroller = new QScrollArea(this);
    scroller->setObjectName(QStringLiteral("m3MenuScroller"));
    scroller->setWidgetResizable(true);
    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroller->setFocusPolicy(Qt::NoFocus);
    auto* rows = new QWidget(scroller);
    rows->setObjectName(QStringLiteral("m3MenuRows"));
    m_rowsLayout = new QVBoxLayout(rows);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(0);
    m_emptyLabel = new QLabel(tr("Nothing in this menu matches that search."), rows);
    m_emptyLabel->setObjectName(QStringLiteral("m3MenuEmpty"));
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setMinimumHeight(72);
    m_emptyLabel->setVisible(false);
    scroller->setWidget(rows);
    root->addWidget(scroller, 1);

    setStyleSheet(QStringLiteral(
        "QFrame#m3Menu { background: palette(window); border: 1px solid palette(mid); border-radius: 12px; }"
        "QScrollArea#m3MenuScroller, QWidget#m3MenuRows { background: transparent; border: 0; }"
        "QToolButton#m3MenuRow { min-height: 48px; max-height: 48px; border-radius: 0;"
        " padding: 0 12px; text-align: left; }"
        "QToolButton#m3MenuRow:hover, QToolButton#m3MenuRow:focus { background: palette(alternate-base); }"
        "QFrame#m3MenuSeparator { min-height: 1px; max-height: 1px; background: palette(mid); margin: 8px 0; }"
        "QLabel#m3MenuEmpty { color: palette(mid); padding: 12px; }"));

    connect(m_search, &CM3SearchField::searchChanged, this, &CM3Menu::applyFilter);
    connect(m_search, &CM3SearchField::escapePressed, this, &CM3Menu::close);
    for (QAction* action : m_actions) {
        if (action)
            connect(action, &QAction::changed, this, &CM3Menu::rebuildRows, Qt::UniqueConnection);
    }
    installEventFilter(this);
    m_search->lineEdit()->installEventFilter(this);
    rebuildRows();
}

void CM3Menu::openAt(const QPoint& globalPosition, QWidget* invoker)
{
    m_invoker = invoker;
    adjustSize();
    const int rowCount = qMax(1, visibleButtons().size());
    const int wantedHeight = kMenuSearchHeight + 24 + qMin(rowCount, 9) * kMenuRowHeight;
    resize(kMenuWidth, qMax(120, wantedHeight));

    QScreen* screen = QGuiApplication::screenAt(globalPosition);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QRect available = screen ? screen->availableGeometry() : QRect(globalPosition, size());
    QPoint target = globalPosition;
    target.setX(qBound(available.left(), target.x(), available.right() - width()));
    target.setY(qBound(available.top(), target.y(), available.bottom() - height()));
    move(target);
    show();
    raise();
    activateWindow();
    m_search->focusEditor();
}

QList<QAction*> CM3Menu::actions() const { return m_actions; }

bool CM3Menu::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate && isVisible()
        && !property("m3ChildDialogActive").toBool())
        close();
    return QFrame::event(event);
}

bool CM3Menu::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return QFrame::eventFilter(watched, event);

    auto* key = static_cast<QKeyEvent*>(event);
    switch (key->key()) {
    case Qt::Key_Escape:
        close();
        return true;
    case Qt::Key_Down:
        focusRelative(1);
        return true;
    case Qt::Key_Up:
        focusRelative(-1);
        return true;
    case Qt::Key_Home:
        focusBoundary(true);
        return true;
    case Qt::Key_End:
        focusBoundary(false);
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space: {
        auto* button = qobject_cast<QToolButton*>(QApplication::focusWidget());
        if (button && button->property("m3MenuAction").isValid()) {
            activateSender();
            return true;
        }
        break;
    }
    case Qt::Key_Right: {
        auto* button = qobject_cast<QToolButton*>(QApplication::focusWidget());
        if (button) {
            const quintptr raw = button->property("m3MenuAction").value<quintptr>();
            QAction* action = reinterpret_cast<QAction*>(raw);
            if (action && action->menu()) {
                openSubmenu(action, button);
                return true;
            }
        }
        break;
    }
    default:
        break;
    }
    return QFrame::eventFilter(watched, event);
}

void CM3Menu::hideEvent(QHideEvent* event)
{
    if (m_childMenu)
        m_childMenu->close();
    emit aboutToHide();
    QPointer<QWidget> invoker = m_invoker;
    if (invoker)
        QTimer::singleShot(0, invoker, [invoker] { if (invoker) invoker->setFocus(Qt::OtherFocusReason); });
    QFrame::hideEvent(event);
}

void CM3Menu::applyFilter(QString query, bool, QRegularExpression expression,
                          QString, bool valid, QString)
{
    m_filterActive = !query.isEmpty();
    for (Row& row : m_rows) {
        if (!row.widget)
            continue;
        if (row.separator) {
            row.widget->setVisible(!m_filterActive);
            continue;
        }
        const QString label = displayText(row.action ? row.action->text() : QString());
        const bool match = !m_filterActive || (valid && expression.match(label).hasMatch());
        row.widget->setVisible(match && row.action && row.action->isVisible());
    }
    updateEmptyState();
}

void CM3Menu::activateSender()
{
    auto* button = qobject_cast<QToolButton*>(sender());
    if (!button)
        button = qobject_cast<QToolButton*>(QApplication::focusWidget());
    if (!button)
        return;
    const quintptr raw = button->property("m3MenuAction").value<quintptr>();
    QAction* action = reinterpret_cast<QAction*>(raw);
    activateAction(action, button);
}

void CM3Menu::rebuildRows()
{
    clearRows();
    for (QAction* action : m_actions) {
        if (!action)
            continue;
        if (action->isSeparator()) {
            auto* separator = new QFrame(this);
            separator->setObjectName(QStringLiteral("m3MenuSeparator"));
            separator->setVisible(action->isVisible() && !m_filterActive);
            m_rowsLayout->addWidget(separator);
            m_rows.push_back({ action, separator, nullptr, true });
            continue;
        }

        auto* button = new QToolButton(this);
        button->setObjectName(QStringLiteral("m3MenuRow"));
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setText(displayText(action->text()));
        button->setIcon(action->icon());
        button->setEnabled(action->isEnabled());
        button->setVisible(action->isVisible());
        button->setCheckable(action->isCheckable());
        button->setChecked(action->isChecked());
        button->setAccessibleName(button->text());
        button->setToolTip(action->toolTip().isEmpty() ? button->text() : action->toolTip());
        button->setProperty("m3MenuAction", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(action)));
        if (action->menu())
            button->setText(button->text() + QStringLiteral("    ›"));
        else if (!action->shortcut().isEmpty())
            button->setText(button->text() + QStringLiteral("    ") + action->shortcut().toString(QKeySequence::NativeText));
        button->installEventFilter(this);
        connect(button, &QToolButton::clicked, this, &CM3Menu::activateSender);
        m_rowsLayout->addWidget(button);
        m_rows.push_back({ action, button, button, false });
    }
    m_rowsLayout->addWidget(m_emptyLabel);
    m_rowsLayout->addStretch(1);
    updateEmptyState();
}

void CM3Menu::clearRows()
{
    m_rows.clear();
    while (QLayoutItem* item = m_rowsLayout->takeAt(0)) {
        if (item->widget() && item->widget() != m_emptyLabel) {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void CM3Menu::updateEmptyState()
{
    const bool anyVisible = !visibleButtons().isEmpty();
    m_emptyLabel->setVisible(!anyVisible);
}

void CM3Menu::focusRelative(int delta)
{
    const QList<QToolButton*> buttons = visibleButtons();
    if (buttons.isEmpty())
        return;
    int index = buttons.indexOf(qobject_cast<QToolButton*>(QApplication::focusWidget()));
    if (index < 0)
        index = delta > 0 ? -1 : 0;
    index = (index + delta + buttons.size()) % buttons.size();
    buttons.at(index)->setFocus(Qt::TabFocusReason);
}

void CM3Menu::focusBoundary(bool first)
{
    const QList<QToolButton*> buttons = visibleButtons();
    if (!buttons.isEmpty())
        (first ? buttons.first() : buttons.last())->setFocus(Qt::TabFocusReason);
}

QList<QToolButton*> CM3Menu::visibleButtons() const
{
    QList<QToolButton*> result;
    for (const Row& row : m_rows) {
        if (row.button && row.button->isVisible() && row.button->isEnabled())
            result.append(row.button);
    }
    return result;
}

void CM3Menu::activateAction(QAction* action, QToolButton* button)
{
    if (!action || !action->isEnabled())
        return;
    if (action->menu()) {
        openSubmenu(action, button);
        return;
    }
    action->trigger();
    emit actionTriggered(action);
    close();
}

void CM3Menu::openSubmenu(QAction* action, QToolButton* button)
{
    if (!action || !action->menu() || !button)
        return;
    if (m_childMenu)
        m_childMenu->close();
    const QPoint point = button->mapToGlobal(QPoint(button->width() - 4, 0));
    m_childMenu = CM3Menu::popup(action->menu(), button, point);
}

QString CM3Menu::displayText(const QString& actionText)
{
    QString text = actionText;
    text.replace(QStringLiteral("&&"), QString(QChar(0x0001)));
    text.remove(QLatin1Char('&'));
    text.replace(QChar(0x0001), QLatin1Char('&'));
    const int tab = text.indexOf(QLatin1Char('\t'));
    if (tab >= 0)
        text.truncate(tab);
    return text.trimmed();
}
