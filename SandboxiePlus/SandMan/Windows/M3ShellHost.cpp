#include "stdafx.h"
#include "M3ShellHost.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMouseEvent>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

static QPoint globalPoint(const QMouseEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

class M3TitleBar final : public QWidget
{
public:
    explicit M3TitleBar(QMainWindow* window)
        : QWidget(window), m_window(window), m_dragging(false)
    {
        setObjectName(QStringLiteral("m3TitleBar"));
        setMinimumHeight(40);
        setAttribute(Qt::WA_StyledBackground, true);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(16, 4, 8, 4);
        layout->setSpacing(8);

        m_title = new QLabel(window->windowTitle(), this);
        m_title->setObjectName(QStringLiteral("m3TitleBarLabel"));
        m_title->setTextInteractionFlags(Qt::NoTextInteraction);
        layout->addWidget(m_title, 1);

        auto addButton = [this, layout](const QString& text, const QString& name, auto handler) {
            auto* button = new QToolButton(this);
            button->setObjectName(name);
            button->setText(text);
            button->setToolTip(button->text());
            button->setAccessibleName(button->toolTip());
            button->setMinimumSize(40, 32);
            connect(button, &QToolButton::clicked, this, handler);
            layout->addWidget(button);
        };
        addButton(QStringLiteral("—"), QStringLiteral("m3MinimizeButton"), [this] { m_window->showMinimized(); });
        addButton(QStringLiteral("□"), QStringLiteral("m3MaximizeButton"), [this] {
            if (m_window->isMaximized()) m_window->showNormal(); else m_window->showMaximized();
        });
        addButton(QStringLiteral("×"), QStringLiteral("m3CloseButton"), [this] { m_window->close(); });

        setStyleSheet(QStringLiteral(
            "#m3TitleBar { background: palette(window); border-bottom: 1px solid palette(mid); }"
            "#m3TitleBarLabel { color: palette(window-text); font-weight: 600; }"
            "#m3TitleBar QToolButton { border: 0; border-radius: 16px; padding: 4px 10px; color: palette(window-text); }"
            "#m3TitleBar QToolButton:hover { background: palette(alternate-base); }"
            "#m3TitleBar QToolButton:focus-visible { outline: 2px solid palette(highlight); }"));
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = globalPoint(event) - m_window->frameGeometry().topLeft();
            event->accept();
        } else QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton) && !m_window->isMaximized()) {
            m_window->move(globalPoint(event) - m_dragOffset);
            event->accept();
        } else QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        m_dragging = false;
        QWidget::mouseReleaseEvent(event);
    }

private:
    QMainWindow* m_window;
    QLabel* m_title = nullptr;
    QPoint m_dragOffset;
    bool m_dragging;
};

class M3ShellRoot final : public QWidget
{
public:
    M3ShellRoot(QMainWindow* window, QMenuBar* menuBar)
        : QWidget(window)
    {
        setObjectName(QStringLiteral("m3ShellRoot"));
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(new M3TitleBar(window));
        if (menuBar) {
            menuBar->setObjectName(QStringLiteral("m3MenuBar"));
            layout->addWidget(menuBar);
        }
    }
};

}

namespace M3ShellHost {

void Install(QMainWindow* window, QMenuBar* menuBar)
{
    if (!window || window->property("m3ShellInstalled").toBool()) return;
    window->setProperty("m3ShellInstalled", true);
    window->setWindowFlags(window->windowFlags() | Qt::FramelessWindowHint);
    window->setMenuWidget(new M3ShellRoot(window, menuBar));
    QObject::connect(window, &QMainWindow::windowTitleChanged, window, [window] {
        if (auto* title = window->findChild<QLabel*>(QStringLiteral("m3TitleBarLabel"))) title->setText(window->windowTitle());
    });
}

}
