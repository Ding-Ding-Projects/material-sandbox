#include "stdafx.h"
#include "TestProxyDialog.h"
#include "M3DialogHost.h"
#include <QtConcurrent>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

constexpr auto TestProgressMax = 90;

CTestProxyDialog::CTestProxyDialog(const QString& IP, const QString& Port, COptionsWindow::EAuthMode AuthMode, const QString& Username, const QString& Password, QWidget* parent)

	: QDialog(parent)
{
	m_ProxyIP = IP;
	m_ProxyPort = Port;
	m_ProxyUsername = Username;
	m_ProxyPass = Password;
	m_AuthMode = AuthMode;

	m_TestShouldCancel = 0;
	m_Watcher = new QFutureWatcher<bool>(this);

	setWindowTitle(tr("Sandboxie-Plus - Test Proxy"));
	setFixedSize(680, 420);

	ui.stackedWidget = new QStackedWidget(this);
	QWidget* testPage = new QWidget(ui.stackedWidget);
	QWidget* settingsPage = new QWidget(ui.stackedWidget);
	ui.stackedWidget->addWidget(testPage);
	ui.stackedWidget->addWidget(settingsPage);

	// Test page: compact summary card, progress, transcript and actions.
	auto* testRoot = new QVBoxLayout(testPage);
	auto* summary = new QHBoxLayout();
	auto* details = new QFormLayout();
	details->setHorizontalSpacing(25);
	ui.labelAddressOut = new QLabel(this);
	ui.labelAddressOut->setTextInteractionFlags(Qt::TextSelectableByMouse);
	details->addRow(tr("Address:"), ui.labelAddressOut);
	details->addRow(tr("Protocol:"), new QLabel(tr("SOCKS 5"), this));
	ui.labelAuthOut = new QLabel(this);
	details->addRow(tr("Authentication:"), ui.labelAuthOut);
	ui.labelUsername = new QLabel(tr("Login:"), this);
	ui.labelUsernameOut = new QLabel(this);
	details->addRow(ui.labelUsername, ui.labelUsernameOut);
	summary->addLayout(details, 3);
	auto* summaryActions = new QVBoxLayout();
	ui.btnTestCustomize = new QPushButton(tr("Test Settings…"), this);
	ui.btnTestCustomize->setMinimumHeight(40);
	ui.labelTestResults = new QLabel(tr("Testing…"), this);
	ui.labelTestResults->setAlignment(Qt::AlignCenter);
	summaryActions->addWidget(ui.btnTestCustomize);
	summaryActions->addWidget(ui.labelTestResults);
	summary->addLayout(summaryActions, 2);
	testRoot->addLayout(summary);
	ui.progressBar = new QProgressBar(this);
	ui.progressBar->setTextVisible(false);
	testRoot->addWidget(ui.progressBar);
	ui.textBrowser = new QTextBrowser(this);
	ui.textBrowser->setFont(QFont(QStringLiteral("Consolas"), 9));
	testRoot->addWidget(ui.textBrowser, 1);
	ui.buttonBoxTest = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Retry, Qt::Horizontal, this);
	testRoot->addWidget(ui.buttonBoxTest);

	// Settings page: grouped test switches and bounded numeric inputs.
	auto* settingsRoot = new QVBoxLayout(settingsPage);
	auto* timeoutRow = new QHBoxLayout();
	ui.lineEditTimeout = new QLineEdit(this);
	ui.lineEditTimeout->setAccessibleName(tr("Timeout in seconds"));
	ui.lineEditTimeout->setMaximumWidth(90);
	timeoutRow->addWidget(new QLabel(tr("Timeout (secs):"), this));
	timeoutRow->addWidget(ui.lineEditTimeout);
	timeoutRow->addStretch();
	settingsRoot->addLayout(timeoutRow);
	QGroupBox* test1 = new QGroupBox(tr("Test 1: Connection to the Proxy Server"), this);
	auto* test1Layout = new QHBoxLayout(test1);
	ui.checkBoxTest1 = new QCheckBox(tr("Enable this test"), test1);
	test1Layout->addWidget(ui.checkBoxTest1);
	settingsRoot->addWidget(test1);
	QGroupBox* test2 = new QGroupBox(tr("Test 2: Connection through the Proxy Server"), this);
	auto* test2Layout = new QVBoxLayout(test2);
	ui.checkBoxTest2 = new QCheckBox(tr("Enable this test"), test2);
	test2Layout->addWidget(ui.checkBoxTest2);
	auto* hostRow = new QHBoxLayout();
	ui.labelHost = new QLabel(tr("Target host:"), this);
	ui.lineEditHost = new QLineEdit(this);
	ui.lineEditHost->setAccessibleName(tr("Target host"));
	ui.labelPort = new QLabel(tr("Port:"), this);
	ui.lineEditPort = new QLineEdit(this);
	ui.lineEditPort->setAccessibleName(tr("Target port"));
	ui.lineEditPort->setMaximumWidth(90);
	hostRow->addWidget(ui.labelHost);
	hostRow->addWidget(ui.lineEditHost, 1);
	hostRow->addWidget(ui.labelPort);
	hostRow->addWidget(ui.lineEditPort);
	test2Layout->addLayout(hostRow);
	ui.checkBoxTest2Load = new QCheckBox(tr("Load a default web page from the host. (There must be a web server running on the host)"), test2);
	test2Layout->addWidget(ui.checkBoxTest2Load);
	settingsRoot->addWidget(test2);
	QGroupBox* test3 = new QGroupBox(tr("Test 3: Proxy Server latency"), this);
	auto* test3Layout = new QVBoxLayout(test3);
	ui.checkBoxTest3 = new QCheckBox(tr("Enable this test"), test3);
	test3Layout->addWidget(ui.checkBoxTest3);
	auto* pingRow = new QHBoxLayout();
	pingRow->addWidget(new QLabel(tr("Ping count:"), this));
	ui.spinBoxPingCount = new QSpinBox(this);
	ui.spinBoxPingCount->setRange(1, 10);
	ui.spinBoxPingCount->setAccessibleName(tr("Ping count"));
	pingRow->addWidget(ui.spinBoxPingCount);
	pingRow->addStretch();
	test3Layout->addLayout(pingRow);
	auto* hint = new QLabel(tr("Increase ping count to improve the accuracy of the average latency calculation. More pings help to ensure that the average is representative of typical network conditions."), this);
	hint->setWordWrap(true);
	test3Layout->addWidget(hint);
	settingsRoot->addWidget(test3, 1);
	ui.buttonBoxSettings = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok | QDialogButtonBox::RestoreDefaults, Qt::Horizontal, this);
	settingsRoot->addWidget(ui.buttonBoxSettings);
	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(16, 16, 16, 16);
	root->addWidget(ui.stackedWidget);
	M3DialogHost::Install(this);

	RestoreDefaults();
	ui.stackedWidget->setCurrentIndex(0);

	ui.labelAddressOut->setText(IP + ":" + Port);
	ui.labelAuthOut->setText(COptionsWindow::GetAuthModeStr(AuthMode).toUpper());
	if (AuthMode == COptionsWindow::EAuthMode::eAuthEnabled)
	{
		ui.labelUsernameOut->setText(Username);
	}
	else
	{
		ui.labelUsernameOut->setText(tr("N/A"));
		ui.labelUsernameOut->hide();
		ui.labelUsername->hide();
	}

	ui.labelTestResults->setText(tr("Testing..."));
	ui.progressBar->setValue(0);
	ui.progressBar->setMinimum(0);
	ui.progressBar->setMaximum(TestProgressMax);
	ui.buttonBoxTest->button(QDialogButtonBox::Retry)->setFocus();

	connect(ui.buttonBoxTest->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,&CTestProxyDialog::accept);
	connect(ui.buttonBoxTest->button(QDialogButtonBox::Retry), &QPushButton::clicked, this, &CTestProxyDialog::OnRetry);
	connect(ui.btnTestCustomize, &QPushButton::clicked, this, &CTestProxyDialog::OnTestCustomize);
	connect(ui.buttonBoxSettings->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CTestProxyDialog::OnTestSettingsSave);
	connect(ui.buttonBoxSettings->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &CTestProxyDialog::OnTestSettingsCancel);
	connect(ui.buttonBoxSettings->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &CTestProxyDialog::OnTestSettingsRestoreDefaults);
	connect(this, &CTestProxyDialog::emitTestMessage, this, [this](const QString& message) { ui.textBrowser->append(message); });	
	connect(this, &CTestProxyDialog::emitUpdateProgress, this, [this](int value) { ui.progressBar->setValue(value); });                                
	connect(m_Watcher, &QFutureWatcher<bool>::finished, this, &CTestProxyDialog::OnTestFinished);
	connect(ui.checkBoxTest2, &QCheckBox::clicked, this, [this]() { Test2EnableParams(ui.checkBoxTest2->isChecked()); });
	connect(ui.checkBoxTest1, &QCheckBox::clicked, this, [this]() {
		if (!ui.checkBoxTest1->isChecked())
		{
			ui.checkBoxTest1->setChecked(true);
			QMessageBox::warning(this, tr("Sandboxie-Plus - Test Proxy"), tr("This test cannot be disabled."));
		}
	});
}

void CTestProxyDialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	ui.textBrowser->clear();
	OnRetry();
}

void CTestProxyDialog::RunTest1(bool& failed, int& progress, int segment)
{
	const constexpr int pollInterval = 1000;

	QThread::msleep(500);
	QString time = QTime::currentTime().toString("hh:mm:ss");
	emit emitTestMessage(tr("[%1] Starting Test 1: Connection to the Proxy Server").arg(time));
	emit emitTestMessage(tr("[%1] IP Address: %2").arg(time).arg(m_ProxyIP));

	QScopedPointer<QTcpSocket> socket(new QTcpSocket(this));

	bool success = false;
	for (int elapsed = 0; !success && elapsed < m_TestTimeout; elapsed += pollInterval)
	{
		socket->connectToHost(m_ProxyIP, m_ProxyPort.toInt());
		success = socket->waitForConnected(pollInterval);
		if (m_TestShouldCancel.loadAcquire()) return;
	}

	time = QTime::currentTime().toString("hh:mm:ss");
	if (success)
	{
		emit emitUpdateProgress(progress += segment);
		emit emitTestMessage(tr("[%1] Connection established.").arg(time));
		emit emitTestMessage(tr("[%1] Test passed.").arg(time));
	}
	else
	{
		failed = true;
		emit emitTestMessage(tr("[%1] Connection to proxy server failed: %2.").arg(time).arg(socket->errorString()));
		emit emitTestMessage(tr("[%1] Test failed.").arg(time));
	}
}

void CTestProxyDialog::RunTest2(bool& failed, int& progress, int segment, bool loadPage)
{
	const constexpr int pollInterval = 1000;

	QThread::msleep(500);
	QString time = QTime::currentTime().toString("hh:mm:ss");
	emit emitTestMessage(tr("[%1] Starting Test 2: Connection through the Proxy Server").arg(time));

	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::Socks5Proxy);
	proxy.setHostName(m_ProxyIP);
	proxy.setPort(m_ProxyPort.toInt());
	if (m_AuthMode == COptionsWindow::EAuthMode::eAuthEnabled) {
		proxy.setUser(m_ProxyUsername);
		proxy.setPassword(m_ProxyPass);
	}

	QScopedPointer<QTcpSocket> socket(new QTcpSocket(this));
	socket->setProxy(proxy);

	bool success = false;
	for (int elapsed = 0; !success && elapsed < m_TestTimeout; elapsed += pollInterval)
	{
		socket->connectToHost(m_TestHost, m_TestPort);
		success = socket->waitForConnected(pollInterval);
		if (m_TestShouldCancel.loadAcquire()) return;
	}

	time = QTime::currentTime().toString("hh:mm:ss");
	if (success)
	{
		segment /= loadPage + 1;
		emit emitUpdateProgress(progress += segment);
		emit emitTestMessage(tr("[%1] Authentication was successful.").arg(time));
		emit emitTestMessage(tr("[%1] Connection to %2 established through the proxy server.").arg(time).arg(m_TestHost + ":" + QString::number(m_TestPort)));

		if (loadPage)
		{
			emit emitTestMessage(tr("[%1] Loading a web page to test the proxy server.").arg(time));
			RunTest2LoadPage(proxy, failed);
			if (m_TestShouldCancel.loadAcquire()) return;
			emit emitUpdateProgress(progress += segment);
		}

		emit emitTestMessage(tr("[%1] %2.").arg(time).arg(!failed ? "Test passed" : "Test failed"));
	}
	else
	{
		failed = true;
		emit emitTestMessage(tr("[%1] Connection through proxy server failed: %2.").arg(time).arg(socket->errorString()));
		emit emitTestMessage(tr("[%1] Test failed.").arg(time));
	}
}

void CTestProxyDialog::RunTest2LoadPage(const QNetworkProxy& proxy, bool& failed)
{
	const constexpr int pollInterval = 100;

	QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager(this));
	manager->setProxy(proxy);

	QEventLoop loop;
	QNetworkRequest request(QUrl("http://" + m_TestHost + ":" + QString::number(m_TestPort)));
	QScopedPointer<QNetworkReply> reply(manager->get(request));

	QTimer timer;
	timer.setInterval(pollInterval);
	int elapsed = 0;

	connect(&timer, &QTimer::timeout, this, [&]() {
		elapsed += pollInterval;
		if (elapsed >= m_TestTimeout && !reply->isFinished()) reply->abort();
		if (m_TestShouldCancel.loadAcquire())
		{
			timer.stop();
			loop.quit();
		}
	});

	connect(reply.data(), &QNetworkReply::finished, this, [&]() {
		QString time = QTime::currentTime().toString("hh:mm:ss");
		if (reply->error() == QNetworkReply::NoError)
		{
			emit emitTestMessage(tr("[%1] Web page loaded successfully.").arg(time));
		}
		else
		{
			failed = true;
			QString error = reply->error() == QNetworkReply::OperationCanceledError ? tr("Timeout") : reply->errorString();
			emit emitTestMessage(tr("[%1] Failed to load web page: %2.").arg(time).arg(error));
		}
		timer.stop();
		loop.quit();
	});

	timer.start();
	loop.exec();
}

void CTestProxyDialog::RunTest3(bool& failed, int& progress, int segment)
{
	const constexpr int pollInterval = 100;
	const int totalTimeout = m_TestTimeout * m_TestPingCount;

	QThread::msleep(500);
	QString time = QTime::currentTime().toString("hh:mm:ss");
	emit emitTestMessage(tr("[%1] Starting Test 3: Proxy Server latency").arg(time));

	bool finished = false;
	QScopedPointer<QProcess> pingProc(new QProcess(this));
	QString program = "ping";
	QStringList args = { "-n", QString::number(m_TestPingCount), "-w", QString::number(m_TestTimeout), m_ProxyIP };
	connect(pingProc.data(), 
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 
        this, 
        [&finished](int, QProcess::ExitStatus) { finished = true; });
	pingProc->start(program, args);

	for (int elapsed = 0; !finished && elapsed < totalTimeout; elapsed += pollInterval)
	{
		finished = pingProc->waitForFinished(pollInterval);
		if (m_TestShouldCancel.loadAcquire())
		{
			pingProc->kill();
			return;
		}
	}

	QString pingOutput = pingProc->readAllStandardOutput();
	time = QTime::currentTime().toString("hh:mm:ss");
	if (pingProc->exitStatus() == QProcess::NormalExit && pingProc->exitCode() == 0)
	{
		QRegularExpression re(" = (\\d+)ms");
		QRegularExpressionMatchIterator it = re.globalMatch(pingOutput);

		QVector<int> values;
		while (it.hasNext()) {
			QRegularExpressionMatch m = it.next();
			values.append(m.captured(1).toInt());
		}

		if (!values.isEmpty())
		{
			int elapsed = values.last(); // last (3rd) is average
			emit emitUpdateProgress(progress += segment);
			emit emitTestMessage(tr("[%1] Latency through proxy server: %2ms.").arg(time).arg(elapsed));
			emit emitTestMessage(tr("[%1] Test passed.").arg(time));
		}
		else
		{
			//failed = true;
			emit emitTestMessage(tr("[%1] Failed to get proxy server latency: Request timeout.").arg(time));
			emit emitTestMessage(tr("[%1] Test failed.").arg(time));
		}
	}
	else
	{
		//failed = true;
		emit emitTestMessage(tr("[%1] Failed to get proxy server latency.").arg(time));
		emit emitTestMessage(tr("[%1] Test failed.").arg(time));
	}
}

void CTestProxyDialog::Test2EnableParams(bool enable)
{
	ui.checkBoxTest2Load->setEnabled(enable);
	ui.lineEditHost->setEnabled(enable);
	ui.lineEditPort->setEnabled(enable);
	ui.labelHost->setEnabled(enable);
	ui.labelPort->setEnabled(enable);
}

void CTestProxyDialog::TestProxy()
{
	QFuture<bool> future = QtConcurrent::run([this]() {
		bool test1 = ui.checkBoxTest1->isChecked();
		bool test2 = ui.checkBoxTest2->isChecked();
		bool test2LoadPage = ui.checkBoxTest2Load->isChecked();
		bool test3 = ui.checkBoxTest3->isChecked();
		bool failed = false;
		int segment = TestProgressMax / (test1 + test2 + test3);
		int progress = 0;

		if (test1 && !m_TestShouldCancel.loadAcquire())			   RunTest1(failed, progress, segment);
		if (test2 && !failed && !m_TestShouldCancel.loadAcquire()) RunTest2(failed, progress, segment, test2LoadPage);
		if (test3 && !failed && !m_TestShouldCancel.loadAcquire()) RunTest3(failed, progress, segment);

		emit emitTestMessage(tr("[%1] Test Finished.").arg(QTime::currentTime().toString("hh:mm:ss")));
		return !failed;
	});

	m_Watcher->setFuture(future);
}

void CTestProxyDialog::OnRetry()
{
	if (m_Watcher->isRunning())
	{
		ui.buttonBoxTest->button(QDialogButtonBox::Retry)->setEnabled(false);
		m_TestShouldCancel.storeRelease(1);
		ui.labelTestResults->setText(tr("Stopped"));
		return;
	}

	ui.progressBar->setValue(0);
	ui.btnTestCustomize->setEnabled(false);
	ui.buttonBoxTest->button(QDialogButtonBox::Retry)->setEnabled(true);
	ui.labelTestResults->setStyleSheet("");
	ui.labelTestResults->setText(tr("Testing..."));
	ui.buttonBoxTest->button(QDialogButtonBox::Retry)->setText(tr("Stop"));

	ui.textBrowser->append(
		tr("[%1] Testing started...\n"
			"\tProxy Server\n"
			"\tAddress:\t\t%2\n"
			"\tProtocol:\t\t%3\n"
			"\tAuthentication:\t%4%5")
		.arg(QTime::currentTime().toString("hh:mm:ss"))
		.arg(m_ProxyIP + ":" + m_ProxyPort)
		.arg("SOCKS 5")
		.arg(COptionsWindow::GetAuthModeStr(m_AuthMode).toUpper())
		.arg(m_AuthMode == COptionsWindow::eAuthEnabled ? QString("\n\tUsername:\t\t%1").arg(m_ProxyUsername) : QString()));

	TestProxy();
}

void CTestProxyDialog::OnTestFinished()
{
	bool success = m_Watcher->future().result();

	ui.progressBar->setValue(TestProgressMax);
	ui.btnTestCustomize->setEnabled(true);
	ui.buttonBoxTest->button(QDialogButtonBox::Retry)->setText(tr("Retry"));
	ui.buttonBoxTest->button(QDialogButtonBox::Retry)->setEnabled(true);

	if (success) {
		if (m_TestShouldCancel.loadAcquire())
			ui.labelTestResults->setText(tr("Stopped"));
		else
		{
			ui.labelTestResults->setStyleSheet("color: green;");
			ui.labelTestResults->setText(tr("Test Passed"));
		}
	}
	else
	{
		ui.labelTestResults->setStyleSheet("color: red;");
		ui.labelTestResults->setText(tr("Test Failed"));
	}
	ui.buttonBoxTest->button(QDialogButtonBox::Ok)->setFocus();

	m_TestShouldCancel.storeRelease(0);
}

void CTestProxyDialog::OnTestSettingsSave()
{
	bool ok;
	m_TestTimeout = ui.lineEditTimeout->text().toInt(&ok);
	if (!ok || m_TestTimeout < 1 || m_TestTimeout > 60)
	{
		QMessageBox::warning(this, tr("Sandboxie-Plus - Test Proxy"), tr("Invalid Timeout value. Please enter a value between 1 and 60."));
		return;
	}
	m_TestTimeout *= 1000; // Convert to ms
	m_TestPort = ui.lineEditPort->text().toInt(&ok);
	if (!ok || m_TestPort < 1 || m_TestPort > 65535)
	{
		QMessageBox::warning(this, tr("Sandboxie-Plus - Test Proxy"), tr("Invalid Port value. Please enter a value between 1 and 65535."));
		return;
	}
	m_TestHost = ui.lineEditHost->text();
	if (m_TestHost.isEmpty() || m_TestHost.contains(QRegularExpression("http[s]?://")))
	{
		QMessageBox::warning(this, tr("Sandboxie-Plus - Test Proxy"), tr("Invalid Host value. Please enter a valid host name excluding 'http[s]://'."));
		return;
	}
	m_TestPingCount = ui.spinBoxPingCount->value();
	if (m_TestPingCount < 1 || m_TestPingCount > 10)
	{
		QMessageBox::warning(this, tr("Sandboxie-Plus - Test Proxy"), tr("Invalid Ping Count value. Please enter a value between 1 and 10."));
		return;
	}

	ui.stackedWidget->setCurrentIndex(0);
}

void CTestProxyDialog::RestoreDefaults()
{
	m_TestTimeout = 5 * 1000; // 5000ms
	m_TestHost = QString("www.google.com");
	m_TestPort = 80;
	m_TestPingCount = 4;

	ui.lineEditTimeout->setText(QString::number(5));
	ui.checkBoxTest1->setChecked(true);
	ui.checkBoxTest2->setChecked(true);
	ui.checkBoxTest3->setChecked(true);
	ui.checkBoxTest2Load->setChecked(true);
	ui.lineEditHost->setText(m_TestHost);
	ui.lineEditPort->setText(QString::number(m_TestPort));
	ui.spinBoxPingCount->setValue(m_TestPingCount);

	Test2EnableParams(true);
}
