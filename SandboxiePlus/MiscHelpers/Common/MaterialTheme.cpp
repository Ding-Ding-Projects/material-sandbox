#include "MaterialTheme.h"
#include "M3Tokens.h"

#include <QEvent>
#include <QPalette>
#include <QStyle>
#include <QWidget>
#include <QtMath>

namespace {

class M3FocusStateFilter : public QObject
{
public:
    explicit M3FocusStateFilter(QObject* parent)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        QWidget* widget = qobject_cast<QWidget*>(watched);
        if (!widget)
            return QObject::eventFilter(watched, event);

        const bool focusIn = event->type() == QEvent::FocusIn;
        const bool focusOut = event->type() == QEvent::FocusOut;
        if (!focusIn && !focusOut)
            return QObject::eventFilter(watched, event);

        const bool active = focusIn;
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
    QPalette palette;
    palette.setColor(QPalette::Window, r.surface);
    palette.setColor(QPalette::WindowText, r.onSurface);
    palette.setColor(QPalette::Base, r.surface);
    palette.setColor(QPalette::AlternateBase, r.surfaceContainer);
    palette.setColor(QPalette::Text, r.onSurface);
    palette.setColor(QPalette::Button, r.surfaceContainer);
    palette.setColor(QPalette::ButtonText, r.onSurface);
    palette.setColor(QPalette::Highlight, r.primary);
    palette.setColor(QPalette::HighlightedText, r.onPrimary);
    palette.setColor(QPalette::Link, r.primary);
    palette.setColor(QPalette::PlaceholderText, r.onSurfaceVariant);
    palette.setColor(QPalette::Mid, r.outlineVariant);
    palette.setColor(QPalette::ToolTipBase, r.inverseSurface);
    palette.setColor(QPalette::ToolTipText, r.inverseOnSurface);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, r.onSurfaceVariant);
    palette.setColor(QPalette::Disabled, QPalette::Text, r.onSurfaceVariant);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, r.onSurfaceVariant);
    return palette;
}

QString buildStyleSheet(const M3Tokens::Colors& r, const M3Tokens::Density& density)
{
    QString sheet = QString::fromUtf8(R"QSS(
        QWidget { color: %(onSurface)s; font-family: "Roboto", "Segoe UI"; font-size: 10pt; }
        QMainWindow, QDialog, QWizard { background: %(surface)s; }

        QMenuBar { background: %(surface)s; padding: 4px 8px; }
        QMenuBar::item { padding: 10px 12px; border-radius: 20px; }
        QMenuBar::item:selected { background: %(stateLayer)s; }
        QMenu { background: %(surfaceCont)s; border: 0; border-radius: 12px; padding: 8px 0; }
        QMenu::item { min-height: 48px; padding: 0 24px 0 12px; border-radius: 0; }
        QMenu::item:selected { background: %(stateLayer)s; color: %(onSurface)s; }
        QMenu::separator { height: 1px; background: %(outlineVar)s; margin: 8px 0; }
        QMenu::icon { padding-left: 12px; }

        QPushButton { min-height: %(control)dpx; padding: 0 24px; border-radius: %(controlR)dpx;
                      border: 1px solid %(outline)s; background: transparent; color: %(onSurface)s;
                      font-weight: 500; }
        QPushButton:hover { background: %(stateLayer)s; }
        QPushButton:pressed { background: %(statePressed)s; }
        QPushButton:disabled { color: %(onSurfaceVar)s; border-color: %(outlineVar)s; }
        QPushButton[m3="filled"] { background: %(primary)s; color: %(onPrimary)s; border: 0; }
        QPushButton[m3="tonal"] { background: %(secondaryCont)s; color: %(onSecondaryCont)s; border: 0; }
        QPushButton[m3="danger"] { background: %(errorCont)s; color: %(onErrorCont)s; border: 0; }
        QPushButton[m3="text"] { border: 0; color: %(primary)s; padding: 0 12px; }
        QToolButton { min-width: %(control)dpx; min-height: %(control)dpx;
                      border: 0; border-radius: %(controlR)dpx; }
        QToolButton:hover { background: %(stateLayer)s; }
        QToolButton:pressed { background: %(statePressed)s; }

        QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            min-height: %(control)dpx; background: %(surface)s; border: 1px solid %(outline)s;
            border-radius: 4px; padding: 0 12px; selection-background-color: %(primary)s;
            selection-color: %(onPrimary)s; }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus,
        QSpinBox:focus, QDoubleSpinBox:focus { border: 2px solid %(primary)s; padding: 0 11px; }
        QLineEdit[m3="search"] { min-height: 56px; border: 0; border-radius: 28px;
                                 background: %(surfaceCont)s; padding: 0 16px; }

        QTabWidget::pane { border: 0; border-top: 1px solid %(outlineVar)s; background: %(surface)s; }
        QTabBar::tab { background: transparent; color: %(onSurfaceVar)s; min-height: 48px;
                       padding: 0 24px; border: 0; font-weight: 500; }
        QTabBar::tab:selected { color: %(primary)s; border-bottom: 3px solid %(primary)s; }
        QTabBar::tab:hover { background: %(stateLayer)s; }

        QTreeView, QListView, QTableView {
            background: %(surfaceCont)s; border: 0; border-radius: 12px;
            selection-background-color: %(secondaryCont)s; selection-color: %(onSecondaryCont)s;
            alternate-background-color: %(surfaceHigh)s; }
        QTreeView::item, QListView::item, QTableView::item { min-height: %(row)dpx; }
        QTreeView::item:hover, QListView::item:hover, QTableView::item:hover { background: %(stateLayer)s; }
        QHeaderView::section { background: %(surfaceCont)s; color: %(onSurfaceVar)s; border: 0;
                               border-bottom: 1px solid %(outlineVar)s; padding: 14px 16px;
                               font-size: 9pt; font-weight: 500; }

        QGroupBox { border: 0; border-radius: 12px; background: %(surfaceCont)s;
                    margin-top: 16px; padding: 16px; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 4px;
                           color: %(onSurface)s; font-weight: 500; }
        QToolBar { background: %(surface)s; border: 0; spacing: 8px; padding: 8px; }
        QStatusBar { background: %(surfaceCont)s; min-height: 40px; color: %(onSurfaceVar)s; }
        QStatusBar::item { border: 0; }

        QCheckBox, QRadioButton { spacing: 12px; min-height: %(control)dpx; }
        QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid %(outline)s;
                               border-radius: 2px; }
        QCheckBox::indicator:checked { background: %(primary)s; border-color: %(primary)s; }
        QSlider::groove:horizontal { height: 4px; background: %(surfaceHighest)s; border-radius: 2px; }
        QSlider::handle:horizontal { width: 20px; height: 20px; margin: -8px 0;
                                     border-radius: 10px; background: %(primary)s; }
        QProgressBar { border: 0; border-radius: 2px; background: %(surfaceHighest)s;
                       height: 4px; text-align: center; }
        QProgressBar::chunk { background: %(primary)s; border-radius: 2px; }

        QScrollBar:vertical { background: transparent; width: 12px; margin: 4px; }
        QScrollBar::handle:vertical { background: %(outline)s; min-height: 40px; border-radius: 4px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal { background: transparent; height: 12px; margin: 4px; }
        QScrollBar::handle:horizontal { background: %(outline)s; min-width: 40px; border-radius: 4px; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

        *[m3Focus="true"] { border: 3px solid %(primary)s; }
    )QSS");

    sheet.replace(QStringLiteral("%(surface)s"), cssColor(r.surface));
    sheet.replace(QStringLiteral("%(surfaceCont)s"), cssColor(r.surfaceContainer));
    sheet.replace(QStringLiteral("%(surfaceHigh)s"), cssColor(r.surfaceContainerHigh));
    sheet.replace(QStringLiteral("%(surfaceHighest)s"), cssColor(r.surfaceContainerHighest));
    sheet.replace(QStringLiteral("%(onSurface)s"), cssColor(r.onSurface));
    sheet.replace(QStringLiteral("%(onSurfaceVar)s"), cssColor(r.onSurfaceVariant));
    sheet.replace(QStringLiteral("%(outline)s"), cssColor(r.outline));
    sheet.replace(QStringLiteral("%(outlineVar)s"), cssColor(r.outlineVariant));
    sheet.replace(QStringLiteral("%(primary)s"), cssColor(r.primary));
    sheet.replace(QStringLiteral("%(onPrimary)s"), cssColor(r.onPrimary));
    sheet.replace(QStringLiteral("%(secondaryCont)s"), cssColor(r.secondaryContainer));
    sheet.replace(QStringLiteral("%(onSecondaryCont)s"), cssColor(r.onSecondaryContainer));
    sheet.replace(QStringLiteral("%(errorCont)s"), cssColor(r.errorContainer));
    sheet.replace(QStringLiteral("%(onErrorCont)s"), cssColor(r.onErrorContainer));
    sheet.replace(QStringLiteral("%(stateLayer)s"), cssColor(r.stateLayer));
    sheet.replace(QStringLiteral("%(statePressed)s"), cssColor(r.stateLayerStrong));
    sheet.replace(QStringLiteral("%(row)d"), QString::number(density.rowHeight));
    sheet.replace(QStringLiteral("%(control)d"), QString::number(density.controlHeight));
    sheet.replace(QStringLiteral("%(controlR)d"), QString::number(density.controlHeight / 2));
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
    if (!app->property("m3FocusFilterInstalled").toBool()) {
        app->installEventFilter(new M3FocusStateFilter(app));
        app->setProperty("m3FocusFilterInstalled", true);
    }
    app->setPalette(buildPalette(roles));
    app->setStyleSheet(buildStyleSheet(roles, metrics));
}

} // namespace MaterialTheme
