#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QStackedWidget>
#include "OptionsWindow.h"
#include <QPropertyAnimation>

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTextBrowser;

class CTestProxyDialog : public QDialog
{
	Q_OBJECT

public:
	CTestProxyDialog(const QString& IP, const QString& Port, COptionsWindow::EAuthMode AuthMode, const QString& Username = QString(), const QString& Password = QString(), QWidget* parent = Q_NULLPTR);
	~CTestProxyDialog() { ; }

protected:
	void showEvent(QShowEvent* event) override;
	void TestProxy();

signals:
	void emitTestMessage(const QString& message);
	void emitUpdateProgress(int value);

private slots:
	void OnRetry();
	void OnTestFinished();
	void OnTestCustomize() { ui.stackedWidget->setCurrentIndex(1); ui.buttonBoxSettings->button(QDialogButtonBox::Ok)->setFocus(); }
	void OnTestSettingsCancel() { RestoreDefaults();  ui.stackedWidget->setCurrentIndex(0); }
	void OnTestSettingsSave();
	void OnTestSettingsRestoreDefaults() { RestoreDefaults(); }
	void RestoreDefaults();

private:
	// Native Material 3 controls. Keeping the same names as the former generated
	// form lets the test pipeline stay focused on behaviour while the chrome is
	// rebuilt in code and can be themed consistently with the rest of SandMan.
	struct Controls {
		QStackedWidget* stackedWidget = nullptr;
		QPushButton* btnTestCustomize = nullptr;
		QLabel* labelTestResults = nullptr;
		QLabel* labelAddressOut = nullptr;
		QLabel* labelAuthOut = nullptr;
		QLabel* labelUsername = nullptr;
		QLabel* labelUsernameOut = nullptr;
		QProgressBar* progressBar = nullptr;
		QTextBrowser* textBrowser = nullptr;
		QDialogButtonBox* buttonBoxTest = nullptr;
		QDialogButtonBox* buttonBoxSettings = nullptr;
		QCheckBox* checkBoxTest1 = nullptr;
		QCheckBox* checkBoxTest2 = nullptr;
		QCheckBox* checkBoxTest2Load = nullptr;
		QCheckBox* checkBoxTest3 = nullptr;
		QLineEdit* lineEditTimeout = nullptr;
		QLineEdit* lineEditHost = nullptr;
		QLineEdit* lineEditPort = nullptr;
		QSpinBox* spinBoxPingCount = nullptr;
		QLabel* labelHost = nullptr;
		QLabel* labelPort = nullptr;
	} ui;

	QString m_ProxyIP;
	QString m_ProxyPort;
	QString m_ProxyUsername;
	QString m_ProxyPass;
	COptionsWindow::EAuthMode m_AuthMode;

	int m_TestTimeout;
	int m_TestPort;
	int m_TestPingCount;
	QString m_TestHost;

	QFutureWatcher<bool>* m_Watcher;
	QAtomicInt m_TestShouldCancel;

	void RunTest1(bool& failed, int& progress, int segment);
	void RunTest2(bool& failed, int& progress, int segment, bool loadPage);
	void RunTest2LoadPage(const QNetworkProxy& proxy, bool& failed);
	void RunTest3(bool& failed, int& progress, int segment);
	void Test2EnableParams(bool enable);
};
