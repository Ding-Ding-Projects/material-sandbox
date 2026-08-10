#pragma once

#include <QFrame>
#include <QList>
#include <QPoint>
#include <QString>
#include <QPointer>
#include <QRegularExpression>
#include <QVector>

class QAction;
class QApplication;
class CM3SearchField;
class QLabel;
class QMenu;
class QToolButton;
class QVBoxLayout;

class CM3Menu final : public QFrame
{
    Q_OBJECT

public:
    explicit CM3Menu(QMenu* sourceMenu, QWidget* parent = nullptr);
    explicit CM3Menu(const QList<QAction*>& actions, const QString& title = QString(), QWidget* parent = nullptr);

    static CM3Menu* popup(QMenu* sourceMenu, QWidget* invoker, const QPoint& globalPosition);
    static CM3Menu* popup(const QList<QAction*>& actions,
                          const QString& title,
                          QWidget* invoker,
                          const QPoint& globalPosition);

    // Adds the same bounded search/regex treatment to every native QMenu.
    // This preserves synchronous QMenu::exec() return values and existing
    // context-menu action handling while meeting the universal-search contract.
    static void installGlobal(QApplication* application);

    void openAt(const QPoint& globalPosition, QWidget* invoker = nullptr);
    QList<QAction*> actions() const;

signals:
    void aboutToHide();
    void actionTriggered(QAction* action);

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void applyFilter(QString query,
                     bool regexMode,
                     QRegularExpression expression,
                     QString flags,
                     bool valid,
                     QString error);
    void activateSender();
    void rebuildRows();

private:
    struct Row {
        QAction* action = nullptr;
        QWidget* widget = nullptr;
        QToolButton* button = nullptr;
        bool separator = false;
    };

    void initialize(const QString& title);
    void clearRows();
    void updateEmptyState();
    void focusRelative(int delta);
    void focusBoundary(bool first);
    QList<QToolButton*> visibleButtons() const;
    void activateAction(QAction* action, QToolButton* button);
    void openSubmenu(QAction* action, QToolButton* button);
    static QString displayText(const QString& actionText);

    QPointer<QMenu> m_sourceMenu;
    QList<QAction*> m_actions;
    QPointer<QWidget> m_invoker;
    CM3SearchField* m_search;
    QVBoxLayout* m_rowsLayout;
    QLabel* m_emptyLabel;
    QVector<Row> m_rows;
    QPointer<CM3Menu> m_childMenu;
    bool m_filterActive;
};
