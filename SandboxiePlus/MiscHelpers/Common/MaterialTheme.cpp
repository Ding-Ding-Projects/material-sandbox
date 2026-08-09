#include "MaterialTheme.h"

#include <QPalette>
#include <QStyleFactory>

namespace {

QPalette BuildPalette(bool dark)
{
    const QColor surface = dark ? QColor("#141218") : QColor("#FFFBFE");
    const QColor surfaceContainer = dark ? QColor("#211F26") : QColor("#F3EDF7");
    const QColor onSurface = dark ? QColor("#E6E0E9") : QColor("#1D1B20");
    const QColor primary = dark ? QColor("#D0BCFF") : QColor("#6750A4");
    const QColor onPrimary = dark ? QColor("#381E72") : QColor("#FFFFFF");

    QPalette palette;
    palette.setColor(QPalette::Window, surface);
    palette.setColor(QPalette::WindowText, onSurface);
    palette.setColor(QPalette::Base, surface);
    palette.setColor(QPalette::AlternateBase, surfaceContainer);
    palette.setColor(QPalette::Text, onSurface);
    palette.setColor(QPalette::Button, surfaceContainer);
    palette.setColor(QPalette::ButtonText, onSurface);
    palette.setColor(QPalette::Highlight, primary);
    palette.setColor(QPalette::HighlightedText, onPrimary);
    palette.setColor(QPalette::Link, primary);
    palette.setColor(QPalette::PlaceholderText, dark ? QColor("#938F99") : QColor("#79747E"));
    palette.setColor(QPalette::ToolTipBase, surfaceContainer);
    palette.setColor(QPalette::ToolTipText, onSurface);
    return palette;
}

QString BuildStyleSheet(bool dark)
{
    const QString surface = dark ? QStringLiteral("#141218") : QStringLiteral("#FFFBFE");
    const QString surfaceContainer = dark ? QStringLiteral("#211F26") : QStringLiteral("#F3EDF7");
    const QString surfaceHigh = dark ? QStringLiteral("#2B2930") : QStringLiteral("#ECE6F0");
    const QString onSurface = dark ? QStringLiteral("#E6E0E9") : QStringLiteral("#1D1B20");
    const QString outline = dark ? QStringLiteral("#938F99") : QStringLiteral("#79747E");
    const QString primary = dark ? QStringLiteral("#D0BCFF") : QStringLiteral("#6750A4");
    const QString onPrimary = dark ? QStringLiteral("#381E72") : QStringLiteral("#FFFFFF");

    return QString::fromUtf8(R"(
        QWidget { color: %1; font-size: 10pt; }
        QMainWindow, QDialog, QWizard { background: %2; }
        QMenuBar { background: %2; padding: 4px 8px; }
        QMenuBar::item { padding: 8px 12px; border-radius: 8px; }
        QMenuBar::item:selected, QMenu::item:selected { background: %3; color: %4; }
        QMenu { background: %3; border: 1px solid %5; border-radius: 12px; padding: 8px; }
        QMenu::item { padding: 8px 28px 8px 12px; border-radius: 8px; }
        QPushButton { background: %3; color: %1; border: 1px solid %5; border-radius: 20px; padding: 8px 18px; min-height: 20px; }
        QPushButton:hover { background: %6; }
        QPushButton:pressed { background: %7; }
        QPushButton:default { background: %8; color: %9; border: 0; }
        QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: %2; border: 1px solid %5; border-radius: 8px; padding: 7px 10px; selection-background-color: %8; }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 2px solid %8; padding: 6px 9px; }
        QTabWidget::pane { border: 1px solid %5; border-radius: 12px; background: %2; top: -1px; }
        QTabBar::tab { background: %3; border-radius: 10px; padding: 10px 16px; margin: 3px; }
        QTabBar::tab:selected { background: %8; color: %9; }
        QTreeView, QListView, QTableView { background: %2; alternate-background-color: %3; border: 1px solid %5; border-radius: 12px; selection-background-color: %8; selection-color: %9; }
        QHeaderView::section { background: %3; border: 0; padding: 8px; }
        QGroupBox { border: 1px solid %5; border-radius: 12px; margin-top: 12px; padding: 12px; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }
        QStatusBar { background: %3; }
        QToolBar { background: %2; border: 0; spacing: 6px; padding: 6px; }
        QToolButton { border-radius: 18px; padding: 6px; }
        QToolButton:hover { background: %3; }
        QScrollBar:vertical { background: %2; width: 12px; margin: 2px; }
        QScrollBar::handle:vertical { background: %5; min-height: 36px; border-radius: 6px; }
        QCheckBox, QRadioButton { spacing: 8px; }
    )").arg(onSurface, surface, surfaceContainer, onSurface, outline, surfaceHigh, surfaceContainer, primary, onPrimary);
}

}

namespace MaterialTheme {

void Apply(QApplication* app, bool dark)
{
    if (!app)
        return;

    app->setStyle(QStyleFactory::create("Fusion"));
    app->setPalette(BuildPalette(dark));
    app->setStyleSheet(BuildStyleSheet(dark));
}

}
