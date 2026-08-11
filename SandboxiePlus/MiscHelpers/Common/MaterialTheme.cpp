#include "MaterialTheme.h"
#include "M3Tokens.h"

#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QStyle>
#include <QWidget>
#include <QtMath>

namespace {

class M3FocusStateFilter final : public QObject
{
public:
    explicit M3FocusStateFilter(QObject* parent) : QObject(parent) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        QWidget* widget = qobject_cast<QWidget*>(watched);
        if (!widget)
            return QObject::eventFilter(watched, event);

        if (event->type() != QEvent::FocusIn && event->type() != QEvent::FocusOut)
            return QObject::eventFilter(watched, event);

        const bool active = event->type() == QEvent::FocusIn;
        if (widget->property("m3Focus").toBool() != active) {
            widget->setProperty("m3Focus", active);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        }
        return QObject::eventFilter(watched, event);
    }
};

QString cssColor(const QColor& color)
{
    if (color.alpha() == 255)
        return color.name(QColor::HexRgb);
    return QStringLiteral("rgba(%1,%2,%3,%4%)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(qRound(color.alphaF() * 100.0));
}

QPalette buildPalette(const M3Tokens::Colors& r)
{
    QPalette p;
    p.setColor(QPalette::Window, r.surface);
    p.setColor(QPalette::WindowText, r.onSurface);
    p.setColor(QPalette::Base, r.surface);
    p.setColor(QPalette::AlternateBase, r.surfaceContainer);
    p.setColor(QPalette::Text, r.onSurface);
    p.setColor(QPalette::Button, r.surfaceContainer);
    p.setColor(QPalette::ButtonText, r.onSurface);
    p.setColor(QPalette::Highlight, r.primary);
    p.setColor(QPalette::HighlightedText, r.onPrimary);
    p.setColor(QPalette::Link, r.primary);
    p.setColor(QPalette::PlaceholderText, r.onSurfaceVariant);
    p.setColor(QPalette::Mid, r.outlineVariant);
    p.setColor(QPalette::ToolTipBase, r.inverseSurface);
    p.setColor(QPalette::ToolTipText, r.inverseOnSurface);
    p.setColor(QPalette::Disabled, QPalette::WindowText, r.onSurfaceVariant);
    p.setColor(QPalette::Disabled, QPalette::Text, r.onSurfaceVariant);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, r.onSurfaceVariant);
    return p;
}

QString buildStyleSheet(const M3Tokens::Colors& r, const M3Tokens::Density& density)
{
    QString sheet = QString::fromUtf8(R"QSS(
QWidget {
    color: %(onSurface)s;
    font-family: "Roboto", "Segoe UI";
    font-size: 10pt;
}
QMainWindow, QDialog, QWizard { background: %(surface)s; }

QMenuBar { background: %(surface)s; padding: 4px 8px; }
QMenuBar::item { padding: 10px 12px; border-radius: 20px; }
QMenuBar::item:selected { background: %(stateLayer)s; }
QMenu { background: %(surfaceCont)s; border: 0; border-radius: 12px; padding: 8px 0; }
QMenu::item { min-height: 48px; padding: 0 24px 0 12px; }
QMenu::item:selected { background: %(stateLayer)s; color: %(onSurface)s; }
QMenu::separator { height: 1px; background: %(outlineVar)s; margin: 8px 0; }

QPushButton {
    min-height: %(control)dpx;
    padding: 0 24px;
    border-radius: %(controlR)dpx;
    border: 1px solid %(outline)s;
    background: transparent;
    color: %(onSurface)s;
    font-weight: 500;
}
QPushButton:hover { background: %(stateLayer)s; }
QPushButton:pressed { background: %(statePressed)s; }
QPushButton:disabled { color: %(onSurfaceVar)s; border-color: %(outlineVar)s; }
QPushButton[m3="filled"] { background: %(primary)s; color: %(onPrimary)s; border: 0; }
QPushButton[m3="tonal"] { background: %(secondaryCont)s; color: %(onSecondaryCont)s; border: 0; }
QPushButton[m3="danger"] { background: %(errorCont)s; color: %(onErrorCont)s; border: 0; }
QPushButton[m3="text"] { border: 0; color: %(primary)s; padding: 0 12px; }

QToolButton {
    min-width: %(control)dpx;
    min-height: %(control)dpx;
    border: 0;
    border-radius: %(controlR)dpx;
    padding: 0 8px;
}
QToolButton:hover { background: %(stateLayer)s; }
QToolButton:pressed { background: %(statePressed)s; }
QToolButton:checked { background: %(secondaryCont)s; color: %(onSecondaryCont)s; }

QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    min-height: %(control)dpx;
    background: %(surface)s;
    border: 1px solid %(outline)s;
    border-radius: 4px;
    padding: 0 12px;
    selection-background-color: %(primary)s;
    selection-color: %(onPrimary)s;
}
QPlainTextEdit, QTextEdit { padding: 8px 12px; }
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus,
QSpinBox:focus, QDoubleSpinBox:focus { border: 2px solid %(primary)s; }
QLineEdit[m3="search"] {
    min-height: 56px;
    border: 0;
    border-radius: 28px;
    background: %(surfaceCont)s;
    padding: 0 16px;
}
QWidget[m3SearchSurface="true"] QLineEdit#m3SearchInput {
    min-width: 0;
    border: 0;
    border-radius: 0;
    background: transparent;
    padding: 0 8px;
}
QWidget[m3SearchSurface="true"] QLineEdit#m3SearchInput:focus,
QWidget[m3SearchSurface="true"] QLineEdit#m3SearchInput[m3Focus="true"] {
    border: 0;
    background: transparent;
}
QWidget[m3SearchSurface="true"] QToolButton#m3RegexBuilderButton[m3Invalid="true"] {
    background: %(errorCont)s;
    color: %(onErrorCont)s;
}
QWidget[m3SearchSurface="true"] QToolButton#m3RegexBuilderButton[m3Invalid="true"][m3Focus="true"] {
    border: 3px solid %(onErrorCont)s;
}

QTabWidget::pane { border: 0; border-top: 1px solid %(outlineVar)s; background: %(surface)s; }
QTabBar::tab {
    background: transparent;
    color: %(onSurfaceVar)s;
    min-height: 48px;
    padding: 0 24px;
    border: 0;
    font-weight: 500;
}
QTabBar::tab:selected { color: %(primary)s; border-bottom: 3px solid %(primary)s; }
QTabBar::tab:hover { background: %(stateLayer)s; }

QTreeView, QListView, QTableView {
    background: %(surfaceCont)s;
    border: 0;
    border-radius: 12px;
    selection-background-color: %(secondaryCont)s;
    selection-color: %(onSecondaryCont)s;
    alternate-background-color: %(surfaceHigh)s;
}
QTreeView::item, QListView::item, QTableView::item { min-height: %(row)dpx; }
QTreeView::item:hover, QListView::item:hover, QTableView::item:hover { background: %(stateLayer)s; }
QHeaderView::section {
    background: %(surfaceCont)s;
    color: %(onSurfaceVar)s;
    border: 0;
    border-bottom: 1px solid %(outlineVar)s;
    padding: 14px 16px;
    font-size: 9pt;
    font-weight: 500;
}

QGroupBox {
    border: 0;
    border-radius: 12px;
    background: %(surfaceCont)s;
    margin-top: 16px;
    padding: 16px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 4px;
    color: %(onSurface)s;
    font-weight: 500;
}
QFrame[m3Card="true"] { background: %(surfaceCont)s; border-radius: 12px; }
QFrame[m3Pane="true"] { background: %(surfaceLow)s; border-top-left-radius: 16px; }
QLabel[m3Title="display-small"] { font-size: 24pt; font-weight: 400; }
QLabel[m3Title="medium"] { font-size: 12pt; font-weight: 500; }
QLabel[m3Text="secondary"], QLabel[m3Label="small"] { color: %(onSurfaceVar)s; }
QWidget[m3Monospace="true"], QPlainTextEdit[m3Monospace="true"] { font-family: "Roboto Mono", "Cascadia Mono", monospace; }
QLabel[m3Banner="warning"] { background: %(warningCont)s; color: %(onWarningCont)s; border-radius: 12px; padding: 12px 16px; }
QLabel[m3Banner="error"] { background: %(errorCont)s; color: %(onErrorCont)s; border-radius: 12px; padding: 12px 16px; }
QToolButton#m3RailDestination[memoryOwned="true"]:checked, QToolButton[memoryOwned="true"]:checked { background: %(memoryCont)s; color: %(onMemoryCont)s; }
QWidget[memoryOwned="true"] QPushButton[m3="filled"] { background: %(memory)s; color: %(onMemory)s; }
QToolBar { background: %(surface)s; border: 0; spacing: 8px; padding: 8px; }
QStatusBar { background: %(surfaceCont)s; min-height: 40px; color: %(onSurfaceVar)s; }
QStatusBar::item { border: 0; }
QStatusBar QWidget[m3StatusChip="true"] { min-height: 28px; border-radius: 14px; padding: 0 10px; background: %(surfaceHigh)s; }

QCheckBox, QRadioButton { spacing: 12px; min-height: %(control)dpx; }
QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid %(outline)s; border-radius: 2px; }
QCheckBox::indicator:checked { background: %(primary)s; border-color: %(primary)s; }
QRadioButton::indicator { width: 18px; height: 18px; border: 2px solid %(outline)s; border-radius: 9px; }
QRadioButton::indicator:checked { background: %(primary)s; border: 5px solid %(surface)s; }
QSlider::groove:horizontal { height: 4px; background: %(surfaceHighest)s; border-radius: 2px; }
QSlider::handle:horizontal { width: 20px; height: 20px; margin: -8px 0; border-radius: 10px; background: %(primary)s; }
QProgressBar { border: 0; border-radius: 2px; background: %(surfaceHighest)s; height: 4px; text-align: center; }
QProgressBar::chunk { background: %(primary)s; border-radius: 2px; }

QScrollBar:vertical { background: transparent; width: 12px; margin: 4px; }
QScrollBar::handle:vertical { background: %(outline)s; min-height: 40px; border-radius: 4px; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 4px; }
QScrollBar::handle:horizontal { background: %(outline)s; min-width: 40px; border-radius: 4px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }

QPushButton[m3Focus="true"], QToolButton[m3Focus="true"],
QLineEdit[m3Focus="true"], QComboBox[m3Focus="true"],
QTreeView[m3Focus="true"], QListView[m3Focus="true"], QTableView[m3Focus="true"] {
    border: 3px solid %(primary)s;
}
)QSS");

    const QPair<QString, QString> substitutions[] = {
        { QStringLiteral("%(surface)s"), cssColor(r.surface) },
        { QStringLiteral("%(surfaceLow)s"), cssColor(r.surfaceContainerLow) },
        { QStringLiteral("%(surfaceCont)s"), cssColor(r.surfaceContainer) },
        { QStringLiteral("%(surfaceHigh)s"), cssColor(r.surfaceContainerHigh) },
        { QStringLiteral("%(surfaceHighest)s"), cssColor(r.surfaceContainerHighest) },
        { QStringLiteral("%(onSurface)s"), cssColor(r.onSurface) },
        { QStringLiteral("%(onSurfaceVar)s"), cssColor(r.onSurfaceVariant) },
        { QStringLiteral("%(outline)s"), cssColor(r.outline) },
        { QStringLiteral("%(outlineVar)s"), cssColor(r.outlineVariant) },
        { QStringLiteral("%(secondaryCont)s"), cssColor(r.secondaryContainer) },
        { QStringLiteral("%(onSecondaryCont)s"), cssColor(r.onSecondaryContainer) },
        { QStringLiteral("%(errorCont)s"), cssColor(r.errorContainer) },
        { QStringLiteral("%(onErrorCont)s"), cssColor(r.onErrorContainer) },
        { QStringLiteral("%(warningCont)s"), cssColor(r.warningContainer) },
        { QStringLiteral("%(onWarningCont)s"), cssColor(r.onWarningContainer) },
        { QStringLiteral("%(memory)s"), cssColor(r.memory) },
        { QStringLiteral("%(onMemory)s"), cssColor(r.onMemory) },
        { QStringLiteral("%(memoryCont)s"), cssColor(r.memoryContainer) },
        { QStringLiteral("%(onMemoryCont)s"), cssColor(r.onMemoryContainer) },
        { QStringLiteral("%(primary)s"), cssColor(r.primary) },
        { QStringLiteral("%(onPrimary)s"), cssColor(r.onPrimary) },
        { QStringLiteral("%(stateLayer)s"), cssColor(r.stateLayer) },
        { QStringLiteral("%(statePressed)s"), cssColor(r.stateLayerStrong) },
        { QStringLiteral("%(row)d"), QString::number(density.rowHeight) },
        { QStringLiteral("%(control)d"), QString::number(density.controlHeight) },
        { QStringLiteral("%(controlR)d"), QString::number(density.controlHeight / 2) }
    };
    for (const auto& substitution : substitutions)
        sheet.replace(substitution.first, substitution.second);
    return sheet;
}

} // namespace

namespace MaterialTheme {

void Apply(QApplication* app, bool dark, const QColor& accentSeed, int density)
{
    if (!app)
        return;

    const M3Tokens::Colors roles = M3Tokens::colors(dark, accentSeed);
    const M3Tokens::Density metrics = M3Tokens::densityFromSettingIndex(density);
    app->setProperty("m3Dark", dark);
    app->setProperty("m3Density", density);
    app->setPalette(buildPalette(roles));
    app->setStyleSheet(buildStyleSheet(roles, metrics));

    if (!app->property("m3FocusFilterInstalled").toBool()) {
        app->installEventFilter(new M3FocusStateFilter(app));
        app->setProperty("m3FocusFilterInstalled", true);
    }
}

} // namespace MaterialTheme
