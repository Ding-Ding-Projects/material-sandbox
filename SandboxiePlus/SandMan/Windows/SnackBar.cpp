#include "stdafx.h"
#include "SnackBar.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

CSnackBar::CSnackBar(QWidget* host)
    : QFrame(host),
      m_host(host),
      m_label(new QLabel(this)),
      m_actionButton(new QPushButton(this)),
      m_dismissButton(new QPushButton(QString(QChar(0x00D7)), this)),
      m_timer(new QTimer(this))
{
    setObjectName(QStringLiteral("m3SnackBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(tr("Notification"));
    setMinimumHeight(48);
    setMaximumWidth(720);
    hide();

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 4, 4, 4);
    layout->setSpacing(8);
    m_label->setWordWrap(true);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_label, 1);
    m_actionButton->setProperty("m3", QStringLiteral("text"));
    m_actionButton->setVisible(false);
    m_dismissButton->setProperty("m3", QStringLiteral("text"));
    m_dismissButton->setToolTip(tr("Dismiss"));
    m_dismissButton->setAccessibleName(m_dismissButton->toolTip());
    m_dismissButton->setFixedSize(40, 40);
    layout->addWidget(m_actionButton);
    layout->addWidget(m_dismissButton);

    setStyleSheet(QStringLiteral(
        "QFrame#m3SnackBar { background: palette(tooltip-base); color: palette(tooltip-text);"
        " border-radius: 4px; }"
        "QFrame#m3SnackBar QLabel { color: palette(tooltip-text); }"
        "QFrame#m3SnackBar QPushButton { color: palette(link); border: 0; background: transparent; }"));

    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &CSnackBar::dismissCurrent);
    connect(m_actionButton, &QPushButton::clicked, this, &CSnackBar::invokeAction);
    connect(m_dismissButton, &QPushButton::clicked, this, &CSnackBar::dismissCurrent);
    if (m_host)
        m_host->installEventFilter(this);
}

void CSnackBar::showMessage(const QString& text, const QString& actionText,
                            const std::function<void()>& action, int timeoutMs)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty())
        return;
    if (isVisible() && m_current.text == normalized) {
        m_timer->start(qMax(1000, timeoutMs));
        return;
    }
    if (!m_queue.isEmpty() && m_queue.back().text == normalized)
        return;
    while (m_queue.size() >= 20)
        m_queue.dequeue();
    m_queue.enqueue({ normalized, actionText, action, qMax(1000, timeoutMs) });
    if (!isVisible())
        showNext();
}

void CSnackBar::clear()
{
    m_timer->stop();
    m_queue.clear();
    hide();
}

bool CSnackBar::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_host && (event->type() == QEvent::Resize || event->type() == QEvent::Move))
        positionOnHost();
    return QFrame::eventFilter(watched, event);
}

void CSnackBar::showNext()
{
    if (m_queue.isEmpty()) {
        hide();
        return;
    }
    m_current = m_queue.dequeue();
    m_label->setText(m_current.text);
    m_actionButton->setText(m_current.actionText);
    m_actionButton->setAccessibleName(m_current.actionText);
    m_actionButton->setVisible(!m_current.actionText.isEmpty() && static_cast<bool>(m_current.action));
    adjustSize();
    positionOnHost();
    show();
    raise();
    m_timer->start(m_current.timeoutMs);
}

void CSnackBar::invokeAction()
{
    const std::function<void()> action = m_current.action;
    dismissCurrent();
    if (action)
        action();
}

void CSnackBar::dismissCurrent()
{
    m_timer->stop();
    const QString text = m_current.text;
    hide();
    emit dismissed(text);
    QTimer::singleShot(0, this, &CSnackBar::showNext);
}

void CSnackBar::positionOnHost()
{
    if (!m_host)
        return;
    const int horizontalMargin = 24;
    const int bottomMargin = 24;
    const int availableWidth = qMax(40, m_host->width() - horizontalMargin * 2);
    const int width = qMin(maximumWidth(), availableWidth);
    resize(width, sizeHint().height());
    const int x = qMax(0, (m_host->width() - width) / 2);
    const int y = qMax(0, m_host->height() - height() - bottomMargin);
    move(x, y);
}
