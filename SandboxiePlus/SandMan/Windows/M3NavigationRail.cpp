#include "stdafx.h"
#include "M3NavigationRail.h"
#include "../../MiscHelpers/Common/M3Tokens.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QStyleOption>
#include <QButtonGroup>
#include <QEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QIcon fallbackDestinationIcon(QWidget* widget, const QString& id)
{
    QStyle* style = widget ? widget->style() : (qApp ? qApp->style() : nullptr);
    if (!style)
        return QIcon();
    if (id == QStringLiteral("boxes")) return style->standardIcon(QStyle::SP_DirIcon);
    if (id == QStringLiteral("recovery")) return style->standardIcon(QStyle::SP_DialogOpenButton);
    if (id == QStringLiteral("trace")) return style->standardIcon(QStyle::SP_FileDialogDetailedView);
    if (id == QStringLiteral("snapshots")) return style->standardIcon(QStyle::SP_DriveHDIcon);
    if (id == QStringLiteral("docs")) return style->standardIcon(QStyle::SP_FileIcon);
    if (id == QStringLiteral("sync")) return style->standardIcon(QStyle::SP_BrowserReload);
    if (id == QStringLiteral("skills")) return style->standardIcon(QStyle::SP_ComputerIcon);
    if (id == QStringLiteral("memory")) return style->standardIcon(QStyle::SP_DirHomeIcon);
    if (id == QStringLiteral("ops")) return style->standardIcon(QStyle::SP_MessageBoxInformation);
    if (id == QStringLiteral("status")) return style->standardIcon(QStyle::SP_DialogApplyButton);
    return style->standardIcon(QStyle::SP_FileDialogContentsView);
}

class RailDestinationButton final : public QToolButton
{
public:
    explicit RailDestinationButton(bool memoryOwned, QWidget* parent = nullptr)
        : QToolButton(parent), m_memoryOwned(memoryOwned)
    {
        setFixedSize(64, 56);
        setCheckable(true);
        setAutoRaise(true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const bool dark = qApp && qApp->property("m3Dark").toBool();
        const M3Tokens::Colors roles = M3Tokens::colors(dark);
        const QRectF indicator((width() - 56) / 2.0, 0.0, 56.0, 32.0);

        if (isChecked()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_memoryOwned ? roles.memoryContainer : roles.secondaryContainer);
            painter.drawRoundedRect(indicator, 16.0, 16.0);
        } else if (underMouse() || isDown()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(isDown() ? roles.stateLayerStrong : roles.stateLayer);
            painter.drawRoundedRect(indicator, 16.0, 16.0);
        }
        if (hasFocus()) {
            QPen pen(m_memoryOwned ? roles.memory : roles.primary, 3.0);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(indicator.adjusted(1.5, 1.5, -1.5, -1.5), 14.5, 14.5);
        }

        const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        const QIcon::State state = isChecked() ? QIcon::On : QIcon::Off;
        icon().paint(&painter, QRect((width() - 24) / 2, 4, 24, 24), Qt::AlignCenter, mode, state);

        QFont labelFont = font();
        labelFont.setPointSizeF(qMax(8.0, labelFont.pointSizeF() - 1.0));
        labelFont.setWeight(isChecked() ? QFont::Medium : QFont::Normal);
        painter.setFont(labelFont);
        QColor textColor = roles.onSurfaceVariant;
        if (isChecked())
            textColor = m_memoryOwned ? roles.memory : roles.primary;
        if (!isEnabled())
            textColor.setAlphaF(0.38);
        painter.setPen(textColor);
        const QFontMetrics metrics(labelFont);
        const QString label = metrics.elidedText(text(), Qt::ElideRight, width() - 4);
        painter.drawText(QRect(2, 35, width() - 4, 18), Qt::AlignHCenter | Qt::AlignTop, label);
    }

private:
    bool m_memoryOwned;
};

}

CM3NavigationRail::CM3NavigationRail(QWidget* parent)
    : QFrame(parent),
      m_scroller(new QScrollArea(this)),
      m_content(new QWidget(m_scroller)),
      m_layout(new QVBoxLayout(m_content)),
      m_group(new QButtonGroup(this))
{
    setObjectName(QStringLiteral("m3NavigationRail"));
    setFixedWidth(80);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setAttribute(Qt::WA_StyledBackground, true);
    setAccessibleName(tr("Primary navigation"));

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    m_scroller->setObjectName(QStringLiteral("m3NavigationRailScroller"));
    m_scroller->setWidget(m_content);
    m_scroller->setWidgetResizable(true);
    m_scroller->setFrameShape(QFrame::NoFrame);
    m_scroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroller->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroller->setFocusPolicy(Qt::NoFocus);
    m_content->setObjectName(QStringLiteral("m3NavigationRailContent"));
    m_content->setMinimumWidth(72);
    outerLayout->addWidget(m_scroller, 1);

    m_layout->setContentsMargins(4, 12, 4, 12);
    m_layout->setSpacing(4);
    m_group->setExclusive(true);

    setStyleSheet(QStringLiteral(
        "QFrame#m3NavigationRail, QScrollArea#m3NavigationRailScroller,"
        " QWidget#m3NavigationRailContent { background: palette(window); border: 0; }"
        "QScrollArea#m3NavigationRailScroller QScrollBar:vertical { width: 6px; margin: 2px 0; }"
        "QLabel#m3RailGroup { color: palette(mid); font-size: 8pt; font-weight: 600;"
        " padding: 12px 0 4px 0; }"));

    setDestinations(contractDestinations());
}

QList<CM3NavigationRail::Destination> CM3NavigationRail::contractDestinations()
{
    return {
        { QStringLiteral("boxes"), tr("Boxes"), QIcon(), QStringLiteral("SANDBOX"), false, true },
        { QStringLiteral("recovery"), tr("Recovery"), QIcon(), QStringLiteral("SANDBOX"), false, true },
        { QStringLiteral("trace"), tr("Trace"), QIcon(), QStringLiteral("SANDBOX"), false, true },
        { QStringLiteral("snapshots"), tr("Snapshots"), QIcon(), QStringLiteral("SANDBOX"), false, true },
        { QStringLiteral("docs"), tr("Docs"), QIcon(), QStringLiteral("SANDBOX"), false, true },
        { QStringLiteral("sync"), tr("Sync"), QIcon(), QStringLiteral("MEMORY"), true, true },
        { QStringLiteral("skills"), tr("Skills"), QIcon(), QStringLiteral("MEMORY"), true, true },
        { QStringLiteral("memory"), tr("Memory"), QIcon(), QStringLiteral("MEMORY"), true, true },
        { QStringLiteral("ops"), tr("Ops"), QIcon(), QStringLiteral("MEMORY"), true, true },
        { QStringLiteral("status"), tr("Status"), QIcon(), QStringLiteral("MEMORY"), true, true },
        { QStringLiteral("settings"), tr("Settings"), QIcon(), QStringLiteral("SYSTEM"), false, true }
    };
}

void CM3NavigationRail::setDestinations(const QList<Destination>& destinations)
{
    clearRail();
    m_destinations = destinations;
    QString groupName;
    for (const Destination& destination : m_destinations) {
        if (destination.group != groupName) {
            groupName = destination.group;
            auto* groupLabel = new QLabel(groupName, m_content);
            groupLabel->setObjectName(QStringLiteral("m3RailGroup"));
            groupLabel->setAlignment(Qt::AlignHCenter);
            groupLabel->setAccessibleName(tr("%1 navigation group").arg(groupName));
            m_layout->addWidget(groupLabel);
        }

        auto* button = new RailDestinationButton(destination.memoryOwned, m_content);
        button->setObjectName(QStringLiteral("m3RailDestination"));
        button->setText(destination.label);
        button->setIcon(destination.icon.isNull()
            ? fallbackDestinationIcon(this, destination.id)
            : destination.icon);
        button->setIconSize(QSize(24, 24));
        button->setEnabled(destination.enabled);
        button->setAccessibleName(destination.label);
        button->setToolTip(destination.label);
        button->setProperty("destinationId", destination.id);
        button->setProperty("memoryOwned", destination.memoryOwned);
        button->installEventFilter(this);
        m_group->addButton(button);
        m_layout->addWidget(button, 0, Qt::AlignHCenter);
        m_buttons.insert(destination.id, button);
        connect(button, &QToolButton::clicked, this, &CM3NavigationRail::activateButton);
    }
    m_layout->addStretch(1);
    if (!m_destinations.isEmpty())
        setCurrentDestination(m_destinations.first().id, false);
}

QList<CM3NavigationRail::Destination> CM3NavigationRail::destinations() const { return m_destinations; }
QString CM3NavigationRail::currentDestination() const { return m_currentId; }

void CM3NavigationRail::setCurrentDestination(const QString& id, bool emitSignal)
{
    QToolButton* button = m_buttons.value(id, nullptr);
    if (!button || !button->isEnabled())
        return;
    m_currentId = id;
    button->setChecked(true);
    if (emitSignal)
        emit destinationActivated(id);
}

void CM3NavigationRail::setDestinationEnabled(const QString& id, bool enabled)
{
    if (QToolButton* button = m_buttons.value(id, nullptr))
        button->setEnabled(enabled);
}

bool CM3NavigationRail::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
        case Qt::Key_Down:
        case Qt::Key_Right:
            focusRelative(1);
            return true;
        case Qt::Key_Up:
        case Qt::Key_Left:
            focusRelative(-1);
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            if (auto* button = qobject_cast<QToolButton*>(watched)) {
                button->click();
                return true;
            }
            break;
        default:
            break;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void CM3NavigationRail::activateButton()
{
    auto* button = qobject_cast<QToolButton*>(sender());
    if (!button)
        return;
    const QString id = button->property("destinationId").toString();
    if (id.isEmpty())
        return;
    m_currentId = id;
    emit destinationActivated(id);
}

void CM3NavigationRail::clearRail()
{
    m_buttons.clear();
    const QList<QAbstractButton*> buttons = m_group->buttons();
    for (QAbstractButton* button : buttons)
        m_group->removeButton(button);
    while (QLayoutItem* item = m_layout->takeAt(0)) {
        if (item->widget())
            delete item->widget();
        delete item;
    }
}

QList<QToolButton*> CM3NavigationRail::navigableButtons() const
{
    QList<QToolButton*> result;
    for (const Destination& destination : m_destinations) {
        QToolButton* button = m_buttons.value(destination.id, nullptr);
        if (button && button->isVisible() && button->isEnabled())
            result.append(button);
    }
    return result;
}

void CM3NavigationRail::focusRelative(int delta)
{
    const QList<QToolButton*> buttons = navigableButtons();
    if (buttons.isEmpty())
        return;
    int index = buttons.indexOf(qobject_cast<QToolButton*>(focusWidget()));
    if (index < 0)
        index = buttons.indexOf(m_buttons.value(m_currentId, nullptr));
    if (index < 0)
        index = 0;
    index = (index + delta + buttons.size()) % buttons.size();
    QToolButton* target = buttons.at(index);
    target->setFocus(Qt::TabFocusReason);
    m_scroller->ensureWidgetVisible(target, 4, 4);
}
