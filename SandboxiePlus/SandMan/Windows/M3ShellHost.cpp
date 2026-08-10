#include "stdafx.h"
#include "M3ShellHost.h"
#include "M3Menu.h"
#include "M3PageNavigationHost.h"
#include "M3SearchField.h"
#include "SnackBar.h"

#include <QHash>
#include <QKeyEvent>
#include <QSet>
#include <QSizePolicy>
#include <QVariant>
#include <functional>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QShortcut>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QWindow>

namespace {

QPoint globalMousePoint(const QMouseEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

QString visibleActionText(QString text)
{
    text.replace(QStringLiteral("&&"), QString(QChar(0x0001)));
    text.remove(QLatin1Char('&'));
    text.replace(QChar(0x0001), QLatin1Char('&'));
    const int tab = text.indexOf(QLatin1Char('\t'));
    if (tab >= 0)
        text.truncate(tab);
    return text.trimmed();
}


QChar actionMnemonic(const QString& text)
{
    for (int index = 0; index + 1 < text.size(); ++index) {
        if (text.at(index) != QLatin1Char('&'))
            continue;
        if (text.at(index + 1) == QLatin1Char('&')) {
            ++index;
            continue;
        }
        return text.at(index + 1);
    }
    return QChar();
}

QAction* findActionByKeywords(const QList<QAction*>& actions, const QStringList& keywords)
{
    QAction* best = nullptr;
    int bestScore = 0;
    for (QAction* action : actions) {
        if (!action || action->isSeparator())
            continue;
        const QString text = visibleActionText(action->text()).toLower();
        const QString objectName = action->objectName().toLower();
        int score = 0;
        for (const QString& rawKeyword : keywords) {
            const QString keyword = rawKeyword.toLower();
            QString compact = keyword;
            compact.remove(QLatin1Char(' '));
            if (text == keyword) score += 20;
            else if (text.contains(keyword)) score += 8;
            if (!objectName.isEmpty() && objectName.contains(compact)) score += 12;
        }
        if (score > bestScore) {
            bestScore = score;
            best = action;
        }
    }
    return bestScore > 0 ? best : nullptr;
}


QTabWidget* primaryPageTabs(QDialog* dialog)
{
    if (!dialog)
        return nullptr;
    QTabWidget* best = nullptr;
    int bestScore = -1;
    const QList<QTabWidget*> candidates = dialog->findChildren<QTabWidget*>();
    for (QTabWidget* tabs : candidates) {
        if (!tabs)
            continue;
        const QString name = tabs->objectName().toLower();
        int score = tabs->count() * 10;
        if (name == QStringLiteral("tabs") || name == QStringLiteral("tabwidget")
            || name.contains(QStringLiteral("setting")) || name.contains(QStringLiteral("option")))
            score += 100;
        if (tabs->parentWidget() == dialog)
            score += 20;
        if (score > bestScore) {
            bestScore = score;
            best = tabs;
        }
    }
    return best;
}

QList<QAction*> flattenActions(const QList<QAction*>& roots)
{
    QList<QAction*> result;
    QSet<QAction*> seen;
    std::function<void(const QList<QAction*>&)> append = [&](const QList<QAction*>& actions) {
        for (QAction* action : actions) {
            if (!action || seen.contains(action))
                continue;
            seen.insert(action);
            if (!action->isSeparator() && !visibleActionText(action->text()).isEmpty())
                result.append(action);
            if (action->menu())
                append(action->menu()->actions());
        }
    };
    append(roots);
    return result;
}

class CommandResults final : public QFrame
{
public:
    CommandResults(QWidget* host, CM3SearchField* search, const QList<QAction*>& actions)
        : QFrame(host), m_host(host), m_search(search), m_list(new QListWidget(this))
    {
        for (QAction* action : actions)
            m_actions.append(QPointer<QAction>(action));
        setObjectName(QStringLiteral("m3CommandResults"));
        setAttribute(Qt::WA_StyledBackground, true);
        hide();
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(0);
        m_list->setFrameShape(QFrame::NoFrame);
        m_list->setUniformItemSizes(true);
        m_list->setAccessibleName(tr("Command search results"));
        layout->addWidget(m_list);
        setStyleSheet(QStringLiteral(
            "QFrame#m3CommandResults { background: palette(base); border: 1px solid palette(mid); border-radius: 12px; }"
            "QListWidget { background: transparent; border: 0; }"
            "QListWidget::item { min-height: 48px; border-radius: 8px; padding: 0 12px; }"
            "QListWidget::item:selected { background: palette(highlight); color: palette(highlighted-text); }"));

        QObject::connect(m_list, &QListWidget::itemActivated, [this](QListWidgetItem* item) {
            trigger(item);
        });
        m_search->lineEdit()->installEventFilter(this);
        if (m_host)
            m_host->installEventFilter(this);
    }

    void filter(const QString& query, const QRegularExpression& expression, bool valid)
    {
        m_list->clear();
        m_itemActions.clear();
        if (query.trimmed().isEmpty() || !valid) {
            hide();
            return;
        }
        int count = 0;
        for (const QPointer<QAction>& guardedAction : m_actions) {
            QAction* action = guardedAction.data();
            if (!action || !action->isVisible())
                continue;
            const QString label = visibleActionText(action->text());
            if (!expression.match(label).hasMatch() && !expression.match(action->toolTip()).hasMatch())
                continue;
            auto* item = new QListWidgetItem(action->icon(), label, m_list);
            m_itemActions.insert(item, guardedAction);
            item->setToolTip(action->toolTip());
            if (!action->isEnabled())
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            if (++count >= 12)
                break;
        }
        if (m_list->count() == 0) {
            auto* item = new QListWidgetItem(tr("No command matches that search."), m_list);
            item->setFlags(Qt::NoItemFlags);
        } else {
            m_list->setCurrentRow(0);
        }
        const int rows = qMin(m_list->count(), 8);
        setFixedHeight(16 + rows * 48);
        positionResults();
        show();
        raise();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_host && (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
            if (isVisible())
                positionResults();
            return false;
        }
        if (watched == m_search->lineEdit() && isVisible() && event->type() == QEvent::KeyPress) {
            auto* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Down) {
                m_list->setFocus();
                if (m_list->currentRow() < 0) m_list->setCurrentRow(0);
                return true;
            }
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                trigger(m_list->currentItem());
                return true;
            }
            if (key->key() == Qt::Key_Escape) {
                hide();
                return false;
            }
        }
        return QFrame::eventFilter(watched, event);
    }

private:
    void positionResults()
    {
        if (!m_host)
            return;
        const int available = qMax(240, m_host->width() - 16);
        setFixedWidth(qMin(420, available));
        const QPoint belowGlobal = m_search->mapToGlobal(
            QPoint(m_search->width() - width(), m_search->height() + 4));
        QPoint below = m_host->mapFromGlobal(belowGlobal);
        below.setX(qBound(0, below.x(), qMax(0, m_host->width() - width())));
        below.setY(qBound(0, below.y(), qMax(0, m_host->height() - height())));
        move(below);
    }

    void trigger(QListWidgetItem* item)
    {
        QAction* action = m_itemActions.value(item).data();
        if (action && action->isEnabled()) {
            hide();
            m_search->setQuery(QString());
            action->trigger();
        }
    }

    QWidget* m_host;
    CM3SearchField* m_search;
    QList<QPointer<QAction>> m_actions;
    QHash<QListWidgetItem*, QPointer<QAction>> m_itemActions;
    QListWidget* m_list;
};

class DialogHeader final : public QFrame
{
public:
    DialogHeader(QDialog* dialog, const QString& title, QWidget* parent = nullptr)
        : QFrame(parent), m_dialog(dialog), m_dragging(false)
    {
        setObjectName(QStringLiteral("m3DialogHeader"));
        setMinimumHeight(56);
        setAttribute(Qt::WA_StyledBackground, true);
        setMouseTracking(true);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 4, 4, 4);
        layout->setSpacing(8);
        auto* label = new QLabel(title, this);
        label->setObjectName(QStringLiteral("m3DialogTitle"));
        label->setProperty("m3Title", QStringLiteral("headline-small"));
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(label, 1);

        auto* close = new QToolButton(this);
        close->setObjectName(QStringLiteral("m3DialogCloseButton"));
        close->setText(QStringLiteral("×"));
        close->setFixedSize(40, 40);
        close->setToolTip(tr("Close"));
        close->setAccessibleName(close->toolTip());
        close->setAutoRaise(true);
        layout->addWidget(close);
        QObject::connect(close, &QToolButton::clicked, dialog, &QDialog::reject);

        setStyleSheet(QStringLiteral(
            "QFrame#m3DialogHeader { background: palette(window); border: 0; }"
            "QLabel#m3DialogTitle { color: palette(window-text); font-size: 20px; font-weight: 500; }"
            "QToolButton#m3DialogCloseButton { border: 0; border-radius: 20px; font-size: 18px; }"
            "QToolButton#m3DialogCloseButton:hover { background: palette(alternate-base); }"));
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && !qobject_cast<QAbstractButton*>(childAt(event->pos()))) {
            m_dragging = true;
            m_dragOffset = globalMousePoint(event) - m_dialog->frameGeometry().topLeft();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            if (m_dialog->windowHandle() && m_dialog->windowHandle()->startSystemMove()) {
                m_dragging = false;
                event->accept();
                return;
            }
#endif
        }
        QFrame::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            m_dialog->move(globalMousePoint(event) - m_dragOffset);
            event->accept();
            return;
        }
        QFrame::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        m_dragging = false;
        QFrame::mouseReleaseEvent(event);
    }

private:
    QDialog* m_dialog;
    bool m_dragging;
    QPoint m_dragOffset;
};

class AppBar final : public QFrame
{
public:
    AppBar(QMainWindow* window, QMenuBar* sourceMenu)
        : QFrame(window), m_window(window), m_dragging(false)
    {
        setObjectName(QStringLiteral("m3TitleBar"));
        setProperty("m3TopAppBar", true);
        setFixedHeight(64);
        setMouseTracking(true);
        setAccessibleName(tr("Application bar"));

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 0, 8, 0);
        layout->setSpacing(4);

        m_appIcon = new QLabel(this);
        m_appIcon->setObjectName(QStringLiteral("m3AppIcon"));
        m_appIcon->setFixedSize(40, 40);
        m_appIcon->setAlignment(Qt::AlignCenter);
        const QIcon icon = window->windowIcon();
        if (!icon.isNull())
            m_appIcon->setPixmap(icon.pixmap(24, 24));
        m_appIcon->setAccessibleName(tr("Application icon"));
        layout->addWidget(m_appIcon);

        m_title = new QLabel(window->windowTitle(), this);
        m_title->setObjectName(QStringLiteral("m3TitleBarLabel"));
        m_title->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        m_title->setTextInteractionFlags(Qt::NoTextInteraction);
        layout->addWidget(m_title);

        if (sourceMenu) {
            for (QAction* menuAction : sourceMenu->actions()) {
                if (!menuAction || !menuAction->menu())
                    continue;
                auto* button = new QToolButton(this);
                button->setObjectName(QStringLiteral("m3MenuTrigger"));
                button->setText(visibleActionText(menuAction->text()));
                button->setToolButtonStyle(Qt::ToolButtonTextOnly);
                button->setAutoRaise(true);
                button->setMinimumSize(40, 40);
                button->setAccessibleName(button->text());
                const QPointer<QAction> guardedMenuAction(menuAction);
                const auto showMenu = [button, guardedMenuAction]() {
                    if (!guardedMenuAction || !guardedMenuAction->menu())
                        return;
                    CM3Menu::popup(guardedMenuAction->menu(), button,
                                   button->mapToGlobal(QPoint(0, button->height() + 4)));
                };
                QObject::connect(button, &QToolButton::clicked, showMenu);
                const QChar mnemonic = actionMnemonic(menuAction->text());
                if (!mnemonic.isNull()) {
                    auto* shortcut = new QShortcut(
                        QKeySequence(QStringLiteral("Alt+%1").arg(mnemonic.toUpper())), this);
                    shortcut->setContext(Qt::WindowShortcut);
                    QObject::connect(shortcut, &QShortcut::activated, showMenu);
                }
                m_menuButtons.append(button);
                m_menuActions.append(menuAction);
                layout->addWidget(button);
            }
            sourceMenu->hide();
            sourceMenu->setNativeMenuBar(false);
        }

        m_compactMenu = new QToolButton(this);
        m_compactMenu->setText(QStringLiteral("☰"));
        m_compactMenu->setFixedSize(40, 40);
        m_compactMenu->setAccessibleName(tr("Application menus"));
        m_compactMenu->setToolTip(m_compactMenu->accessibleName());
        m_compactMenu->setAutoRaise(true);
        m_compactMenu->setVisible(false);
        QObject::connect(m_compactMenu, &QToolButton::clicked, [this]() {
            CM3Menu::popup(m_menuActions, tr("Application menus"), m_compactMenu,
                           m_compactMenu->mapToGlobal(QPoint(0, m_compactMenu->height() + 4)));
        });
        layout->addWidget(m_compactMenu);
        layout->addStretch(1);

        m_search = new CM3SearchField(this);
        m_search->setObjectName(QStringLiteral("m3GlobalSearch"));
        m_search->setHeightVariant(CM3SearchField::Control);
        m_search->setPlaceholderText(tr("Search actions"));
        m_search->setMinimumWidth(160);
        m_search->setMaximumWidth(420);
        layout->addWidget(m_search);

        const QList<QAction*> commandActions = sourceMenu
            ? flattenActions(sourceMenu->actions())
            : flattenActions(window->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly));
        m_results = new CommandResults(window, m_search, commandActions);
        QObject::connect(m_search, &CM3SearchField::searchChanged,
                         [this](QString query, bool, QRegularExpression expression,
                                QString, bool valid, QString) {
            if (m_results)
                m_results->filter(query, expression, valid);
        });
        QObject::connect(m_search, &CM3SearchField::escapePressed,
                         m_results.data(), &QWidget::hide);

        addExistingActionButton(layout, commandActions,
                                {QStringLiteral("language"), QStringLiteral("translation")},
                                tr("Language"), QStringLiteral("文"));
        addExistingActionButton(layout, commandActions,
                                {QStringLiteral("theme"), QStringLiteral("dark mode"), QStringLiteral("appearance")},
                                tr("Theme"), QStringLiteral("◐"));
        addExistingActionButton(layout, commandActions,
                                {QStringLiteral("notification")},
                                tr("Notifications"), QStringLiteral("!"));
        addToolbarOverflow(layout);
        addWindowButton(layout, QStringLiteral("m3MinimizeButton"), tr("Minimize"), QStringLiteral("—"),
                        [window]() { window->showMinimized(); });
        m_maximize = addWindowButton(layout, QStringLiteral("m3MaximizeButton"), tr("Maximize"),
                                     QStringLiteral("□"), [window]() {
            window->isMaximized() ? window->showNormal() : window->showMaximized();
        });
        addWindowButton(layout, QStringLiteral("m3CloseButton"), tr("Close"), QStringLiteral("×"),
                        [window]() { window->close(); }, true);

        QObject::connect(window, &QWidget::windowTitleChanged, m_title, &QLabel::setText);
        window->installEventFilter(this);
        setStyleSheet(QStringLiteral(
            "QFrame#m3TitleBar { background: palette(window); border: 0; }"
            "QLabel#m3TitleBarLabel { font-size: 16px; font-weight: 500; padding: 0 8px 0 0; }"
            "QToolButton#m3MenuTrigger { min-height: 40px; padding: 0 12px; border: 0; border-radius: 20px; }"
            "QToolButton#m3MenuTrigger:hover { background: palette(alternate-base); }"
            "QToolButton[m3WindowButton='true'] { min-width: 40px; min-height: 40px; border: 0; border-radius: 20px; font-size: 18px; }"
            "QToolButton[m3CloseButton='true']:hover { background: #B3261E; color: white; }"));
    }

    ~AppBar() override
    {
        // CommandResults is a window-level overlay so it is not clipped by the
        // 64 px app bar. Tie its lifetime to this action snapshot explicitly.
        delete m_results.data();
        m_results.clear();
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        const int available = width();
        const bool compactMenus = available < 900;
        const bool showOptional = available >= 760;
        for (QToolButton* button : m_menuButtons)
            button->setVisible(!compactMenus);
        if (m_compactMenu)
            m_compactMenu->setVisible(compactMenus && !m_menuActions.isEmpty());
        for (QToolButton* button : m_optionalButtons)
            button->setVisible(showOptional);
        if (m_title)
            m_title->setVisible(available >= 700);
        if (m_appIcon)
            m_appIcon->setVisible(available >= 520);
        if (m_search) {
            m_search->setMinimumWidth(available < 620 ? 96 : 160);
            m_search->setMaximumWidth(available < 540 ? 140 : available < 700 ? 220 : 420);
        }
        QFrame::resizeEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && !qobject_cast<QAbstractButton*>(childAt(event->pos()))) {
            m_window->isMaximized() ? m_window->showNormal() : m_window->showMaximized();
            event->accept();
            return;
        }
        QFrame::mouseDoubleClickEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && !qobject_cast<QAbstractButton*>(childAt(event->pos()))) {
            m_dragging = true;
            m_dragOffset = globalMousePoint(event) - m_window->frameGeometry().topLeft();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            if (m_window->windowHandle() && m_window->windowHandle()->startSystemMove()) {
                m_dragging = false;
                event->accept();
                return;
            }
#endif
        }
        QFrame::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton) && !m_window->isMaximized()) {
            m_window->move(globalMousePoint(event) - m_dragOffset);
            event->accept();
            return;
        }
        QFrame::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        m_dragging = false;
        QFrame::mouseReleaseEvent(event);
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_window && event->type() == QEvent::WindowStateChange && m_maximize) {
            m_maximize->setAccessibleName(m_window->isMaximized() ? tr("Restore") : tr("Maximize"));
            m_maximize->setToolTip(m_maximize->accessibleName());
        }
        return QFrame::eventFilter(watched, event);
    }

private:
    template<typename Fn>
    QToolButton* addWindowButton(QHBoxLayout* layout, const QString& objectName,
                                 const QString& name, const QString& glyph,
                                 Fn callback, bool close = false)
    {
        auto* button = new QToolButton(this);
        button->setObjectName(objectName);
        button->setText(glyph);
        button->setProperty("m3WindowButton", true);
        button->setProperty("m3CloseButton", close);
        button->setAccessibleName(name);
        button->setToolTip(name);
        button->setFixedSize(40, 40);
        QObject::connect(button, &QToolButton::clicked, callback);
        layout->addWidget(button);
        return button;
    }

    void addExistingActionButton(QHBoxLayout* layout, const QList<QAction*>& actions,
                                 const QStringList& keywords, const QString& accessibleName,
                                 const QString& fallbackGlyph)
    {
        QAction* action = findActionByKeywords(actions, keywords);
        if (!action)
            return;
        const QPointer<QAction> guardedAction(action);
        auto* button = new QToolButton(this);
        button->setFixedSize(40, 40);
        button->setAutoRaise(true);
        button->setAccessibleName(accessibleName);
        button->setToolTip(accessibleName);
        if (!action->icon().isNull()) {
            button->setIcon(action->icon());
            button->setIconSize(QSize(20, 20));
        } else {
            button->setText(fallbackGlyph);
        }
        button->setEnabled(action->isEnabled());
        QObject::connect(action, &QAction::changed, button, [guardedAction, button] {
            button->setEnabled(guardedAction && guardedAction->isEnabled());
            if (guardedAction && !guardedAction->icon().isNull())
                button->setIcon(guardedAction->icon());
        });
        QObject::connect(button, &QToolButton::clicked, this, [button, guardedAction] {
            if (!guardedAction)
                return;
            if (guardedAction->menu())
                CM3Menu::popup(guardedAction->menu(), button,
                               button->mapToGlobal(QPoint(0, button->height() + 4)));
            else
                guardedAction->trigger();
        });
        m_optionalButtons.append(button);
        layout->addWidget(button);
    }

    void addToolbarOverflow(QHBoxLayout* layout)
    {
        QList<QAction*> actions;
        QSet<QAction*> seen;
        const QList<QToolBar*> toolbars = m_window->findChildren<QToolBar*>(QString(), Qt::FindDirectChildrenOnly);
        for (QToolBar* toolbar : toolbars) {
            for (QAction* action : toolbar->actions()) {
                if (action && !seen.contains(action)) {
                    seen.insert(action);
                    actions.append(action);
                }
            }
            if (!toolbar->property("m3KeepVisible").toBool())
                toolbar->hide();
        }
        if (actions.isEmpty())
            return;
        auto* overflow = new QToolButton(this);
        overflow->setText(QStringLiteral("⋮"));
        overflow->setFixedSize(40, 40);
        overflow->setToolTip(tr("More actions"));
        overflow->setAccessibleName(overflow->toolTip());
        overflow->setAutoRaise(true);
        QObject::connect(overflow, &QToolButton::clicked, [overflow, actions]() {
            CM3Menu::popup(actions, QObject::tr("More actions"), overflow,
                           overflow->mapToGlobal(QPoint(overflow->width() - 304, overflow->height() + 4)));
        });
        layout->addWidget(overflow);
    }

    QMainWindow* m_window;
    QLabel* m_appIcon = nullptr;
    QLabel* m_title;
    CM3SearchField* m_search;
    QPointer<CommandResults> m_results;
    QToolButton* m_maximize = nullptr;
    QList<QToolButton*> m_menuButtons;
    QList<QToolButton*> m_optionalButtons;
    QList<QAction*> m_menuActions;
    QToolButton* m_compactMenu = nullptr;
    bool m_dragging;
    QPoint m_dragOffset;
};

void styleStatusBar(QMainWindow* window)
{
    if (!window || !window->statusBar())
        return;
    QStatusBar* status = window->statusBar();
    status->setFixedHeight(40);
    status->setProperty("m3StatusBar", true);
    const QList<QWidget*> children = status->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children) {
        child->setProperty("m3StatusChip", true);
        child->setMinimumHeight(28);
    }
}

}

namespace M3ShellHost
{

void Install(QMainWindow* window, QMenuBar* menuBar)
{
    if (!window)
        return;

    if (!window->property("m3ShellInstalled").toBool()) {
        window->setProperty("m3ShellInstalled", true);
        CM3Menu::installGlobal(qApp);
        window->setAttribute(Qt::WA_StyledBackground, true);
        window->setWindowFlag(Qt::FramelessWindowHint, true);

        if (!window->findChild<CSnackBar*>(QStringLiteral("m3SnackBar"),
                                           Qt::FindDirectChildrenOnly)) {
            auto* snackBar = new CSnackBar(window);
            snackBar->setObjectName(QStringLiteral("m3SnackBar"));
        }
        window->setProperty("m3SnackBarInstalled", true);
    }

    Refresh(window, menuBar);
}

void Refresh(QMainWindow* window, QMenuBar* menuBar)
{
    if (!window)
        return;

    // Create the replacement before retiring the old app bar. The source QMenuBar
    // and its QAction graph remain owned by SandMan and are only hidden.
    auto* shellRoot = new QWidget(window);
    shellRoot->setObjectName(QStringLiteral("m3ShellRoot"));
    auto* shellLayout = new QVBoxLayout(shellRoot);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);
    auto* bar = new ::AppBar(window, menuBar);
    shellLayout->addWidget(bar);

    QPointer<QWidget> previous = window->menuWidget();
    window->setMenuWidget(shellRoot);
    if (previous && previous != shellRoot
        && previous->objectName() == QStringLiteral("m3ShellRoot")) {
        previous->hide();
        previous->deleteLater();
    }

    styleStatusBar(window);
    // SandMan adds/recreates permanent status widgets during CreateUI/RebuildUI.
    // Re-apply on the next event turn without replacing their behavior.
    QTimer::singleShot(0, window, [window] { styleStatusBar(window); });
}

void InstallDialog(QDialog* dialog, const QString& title)
{
    if (!dialog || dialog->property("m3DialogInstalled").toBool())
        return;
    dialog->setProperty("m3DialogInstalled", true);
    dialog->setProperty("m3Dialog", true);
    dialog->setAttribute(Qt::WA_StyledBackground, true);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::FramelessWindowHint);

    if (dialog->layout()) {
        dialog->layout()->setContentsMargins(24, 24, 24, 24);
        dialog->layout()->setSpacing(16);
        if (auto* boxLayout = qobject_cast<QBoxLayout*>(dialog->layout())) {
            const QString dialogTitle = title.trimmed().isEmpty() ? dialog->windowTitle() : title;
            if (!dialog->findChild<QWidget*>(QStringLiteral("m3DialogHeader"), Qt::FindDirectChildrenOnly))
                boxLayout->insertWidget(0, new DialogHeader(dialog, dialogTitle, dialog));
        }
    }
    dialog->setStyleSheet(dialog->styleSheet() + QStringLiteral(
        "QDialog[m3Dialog='true'] { background: palette(window); border-radius: 28px; }"));

    const QString identity = (dialog->objectName() + QLatin1Char(' ') + dialog->windowTitle()).toLower();
    if (identity.contains(QStringLiteral("setting")) || identity.contains(QStringLiteral("option"))) {
        QTabWidget* tabs = primaryPageTabs(dialog);
        if (tabs) {
            const QString placeholder = identity.contains(QStringLiteral("option"))
                ? QObject::tr("Search sandbox options") : QObject::tr("Search settings");
            CM3PageNavigationHost::adapt(dialog, tabs, placeholder);
        }
    }
}

QWidget* AppBar(QMainWindow* window)
{
    return window ? window->findChild<QWidget*>(QStringLiteral("m3TitleBar")) : nullptr;
}

}
