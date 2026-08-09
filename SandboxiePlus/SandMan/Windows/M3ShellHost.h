#pragma once

#include <QMainWindow>

class QMenuBar;

// Material 3 application chrome boundary.
//
// The existing SandMan views remain functional children of the window, but
// native/platform title-bar chrome is no longer part of the product surface.
// Keeping this boundary in one small host lets each legacy view migrate
// independently without re-introducing a second title-bar implementation.
namespace M3ShellHost {

/// Install the frameless M3 title bar and menu surface on a QMainWindow.
/// The operation is idempotent and safe to call during UI rebuilds.
void Install(QMainWindow* window, QMenuBar* menuBar);

}
