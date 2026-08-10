#pragma once

#include <QString>

class QDialog;
class QMainWindow;
class QMenuBar;
class QWidget;

namespace M3ShellHost
{
    // Installs the Material 3 app bar without replacing the existing QMainWindow,
    // QAction graph, central widget, status bar, or window-domain behavior.
    void Install(QMainWindow* window, QMenuBar* menuBar = nullptr);

    // Rebinds the app bar to the current menu/action graph after SandMan rebuilds
    // its UI. This is intentionally separate from Install so runtime UI refreshes
    // do not duplicate global installers, snackbars, or window flags.
    void Refresh(QMainWindow* window, QMenuBar* menuBar = nullptr);

    // Applies the shared Material dialog treatment. Settings and Box Options are
    // adapted to the two-pane contract in-place while retaining their live pages.
    void InstallDialog(QDialog* dialog, const QString& title = QString());

    QWidget* AppBar(QMainWindow* window);
}
