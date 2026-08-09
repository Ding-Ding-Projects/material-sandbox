#pragma once

#include <QApplication>

namespace MaterialTheme {

// Applies the shared Material 3 Expressive baseline used by the desktop shell.
// The palette is intentionally kept in one place so dialogs and future surfaces
// can adopt the same roles without inventing another theme.
void Apply(QApplication* app, bool dark);

}
