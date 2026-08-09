#include "stdafx.h"
#include "M3DialogHost.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPoint GlobalPoint(const QMouseEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

class M3DialogTitle final : public QWidget
{
public:
    explicit M3DialogTitle(QDialog* dialog) : QWidget(dialog), m_dialog(dialog)
    {
        setObjectName(QStringLiteral("m3DialogTitle"));
        setMinimumHeight(48);
        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(16, 6, 8, 6);
        auto* title = new QLabel(dialog->windowTitle(), this);
        title->setObjectName(QStringLiteral("m3DialogTitleLabel"));
        row->addWidget(title, 1);
        auto* close = new QPushButton(QStringLiteral("×"), this);
        close->setObjectName(QStringLiteral("m3DialogClose"));
        close->setAccessibleName(QObject::tr("Close dialog"));
        close->setMinimumSize(40, 36);
        connect(close, &QPushButton::clicked, dialog, &QDialog::reject);
        row->addWidget(close);
        setStyleSheet(QStringLiteral(
            "#m3DialogTitle { background: palette(window); border-bottom: 1px solid palette(mid); }"
            "#m3DialogTitleLabel { color: palette(window-text); font-weight: 600; }"
            "#m3DialogClose { border: 0; border-radius: 18px; color: palette(window-text); }"
            "#m3DialogClose:hover { background: palette(alternate-base); }"
            "#m3DialogClose:focus-visible { outline: 2px solid palette(highlight); }"));
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragOffset = GlobalPoint(event) - m_dialog->frameGeometry().topLeft();
            m_dragging = true;
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            m_dialog->move(GlobalPoint(event) - m_dragOffset);
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        m_dragging = false;
        QWidget::mouseReleaseEvent(event);
    }

private:
    QDialog* m_dialog;
    QPoint m_dragOffset;
    bool m_dragging = false;
};

}

namespace M3DialogHost {

void Install(QDialog* dialog)
{
    if (!dialog || dialog->property("m3DialogInstalled").toBool())
        return;
    dialog->setProperty("m3DialogInstalled", true);
    dialog->setWindowFlag(Qt::FramelessWindowHint, true);

    QLayout* oldLayout = dialog->layout();
    if (oldLayout)
        oldLayout->setParent(nullptr);

    auto* shell = new QWidget(dialog);
    shell->setObjectName(QStringLiteral("m3DialogShell"));
    auto* shellLayout = new QVBoxLayout(shell);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);
    shellLayout->addWidget(new M3DialogTitle(dialog));
    if (oldLayout)
        shellLayout->addLayout(oldLayout);

    auto* rootLayout = new QVBoxLayout(dialog);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(shell);
}

}
