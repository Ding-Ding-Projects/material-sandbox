#pragma once

class QDialog;
class QApplication;

namespace M3DialogHost {

// Wrap an existing dialog layout in one reusable Material 3 title surface.
// The operation is idempotent so dialog rebuilds cannot stack chrome.
void Install(QDialog* dialog);
void InstallForApplication(QApplication* application);

}
