#include "stdafx.h"
#include "OptionsWindow.h"
#include "M3DialogHost.h"
#include "EditorSettingsWindow.h"
#include "SandMan.h"
#include "SettingsWindow.h"
#include "../MiscHelpers/Common/Settings.h"
#include "../MiscHelpers/Common/TabStateManager.h"
#include "../MiscHelpers/Common/Common.h"
#include "../MiscHelpers/Common/ComboInputDialog.h"
#include "../MiscHelpers/Common/SettingsWidgets.h"
#include "Helpers/WinAdmin.h"
#include "../Wizards/TemplateWizard.h"
#include "Helpers/TabOrder.h"
#include "../MiscHelpers/Common/CodeEdit.h"
#include "Helpers/IniHighlighter.h"


class NoEditDelegate : public QStyledItemDelegate {
public:
	NoEditDelegate(QObject* parent = 0) : QStyledItemDelegate(parent) {}

	virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		return NULL;
	}
};

class QTreeWidgetHacker : public QTreeWidget
{
public:
	friend class ProgramsDelegate;
	//QModelIndex indexFromItem(const QTreeWidgetItem *item, int column = 0) const;
	//QTreeWidgetItem *itemFromIndex(const QModelIndex &index) const;
};


//////////////////////////////////////////////////////////////////////////
// ProgramsDelegate

class ProgramsDelegate : public QStyledItemDelegate {
public:
	ProgramsDelegate(COptionsWindow* pOptions, QTreeWidget* pTree, int Column, QObject* parent = 0) : QStyledItemDelegate(parent) {
		m_pOptions = pOptions; 
		m_pTree = pTree; 
		m_Column = (m_Group = (Column == -2)) ? -1 : Column;
	}

	virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		QTreeWidgetItem* pItem = ((QTreeWidgetHacker*)m_pTree)->itemFromIndex(index);
		if (!pItem->data(index.column(), Qt::UserRole).isValid())
			return NULL;

		if(m_Group && !pItem->parent()) // for groups use simple edit
			return QStyledItemDelegate::createEditor(parent, option, index);

		if (m_Column == -1 || pItem->data(m_Column, Qt::UserRole).toInt() == COptionsWindow::eProcess) {
			QComboBox* pBox = new QComboBox(parent);
			pBox->setEditable(true);
			foreach(const QString Group, m_pOptions->GetCurrentGroups()) {
				QString GroupName = Group.mid(1, Group.length() - 2);
				pBox->addItem(tr("Group: %1").arg(GroupName), Group);
			}
			foreach(const QString & Name, m_pOptions->GetPrograms())
				pBox->addItem(Name, Name);

			connect(pBox->lineEdit(), &QLineEdit::textEdited, [pBox](const QString& text){
				/*if (pBox->currentIndex() != -1) {
					int pos = pBox->lineEdit()->cursorPosition();
					pBox->setCurrentIndex(-1);
					pBox->setCurrentText(text);
					pBox->lineEdit()->setCursorPosition(pos);
				}*/
				pBox->setProperty("value", text);
			});
			connect(pBox->lineEdit(), &QLineEdit::returnPressed, [pBox](){
				/*if (pBox->currentIndex() != -1) {
					int pos = pBox->lineEdit()->cursorPosition();
					pBox->setCurrentIndex(-1);
					pBox->setCurrentText(text);
					pBox->lineEdit()->setCursorPosition(pos);
				}*/
				pBox->setProperty("value", pBox->lineEdit()->text());
			});

			connect(pBox, qOverload<int>(&QComboBox::currentIndexChanged), [pBox](int index){
				if (index != -1) {
					QString Program = pBox->itemData(index).toString();
					pBox->setProperty("value", Program);
					pBox->lineEdit()->setReadOnly(Program.left(1) == "<");
				}
			});

			return pBox;
		}
		else if (pItem->data(0, Qt::UserRole).toInt() == COptionsWindow::ePath)
			return QStyledItemDelegate::createEditor(parent, option, index);
		else
			return NULL;
	}

	virtual void setEditorData(QWidget* editor, const QModelIndex& index) const {
		QComboBox* pBox = qobject_cast<QComboBox*>(editor);
		if (pBox) {
			QTreeWidgetItem* pItem = ((QTreeWidgetHacker*)m_pTree)->itemFromIndex(index);
			QString Program = pItem->data(index.column(), Qt::UserRole).toString();

			pBox->setProperty("value", Program);
			pBox->lineEdit()->setReadOnly(Program.left(1) == "<");

			int Index = pBox->findData(Program);
			pBox->setCurrentIndex(Index);
			if (Index == -1)
				pBox->setCurrentText(Program);
		}
		else
			QStyledItemDelegate::setEditorData(editor, index);
	}

	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
		QTreeWidgetItem* pItem = ((QTreeWidgetHacker*)m_pTree)->itemFromIndex(index);

		QComboBox* pBox = qobject_cast<QComboBox*>(editor);
		if (pBox) {
			
			QString Value = pBox->property("value").toString();
			bool prev = m_pTree->blockSignals(true);
			pItem->setText(index.column(), pBox->currentText());
			m_pTree->blockSignals(prev);
			//QString Text = pBox->currentText();
			//QVariant Data = pBox->currentData();
			pItem->setData(index.column(), Qt::UserRole, Value);
		}

		QLineEdit* pEdit = qobject_cast<QLineEdit*>(editor);
		if (pEdit) {
			bool prev = m_pTree->blockSignals(true);
			pItem->setText(index.column(), pEdit->text());
			m_pTree->blockSignals(prev);
			QString Value = pEdit->text();
			if (m_Group) Value = "<" + Value + ">";
			pItem->setData(index.column(), Qt::UserRole, Value);
		}
	}

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QSize size = QStyledItemDelegate::sizeHint(option, index);
		if(size.height() < 20) size.setHeight(20); // ensure enough room for the combo box
		return size;
	}

protected:
	COptionsWindow* m_pOptions;
	QTreeWidget* m_pTree;
	int m_Column;
	bool m_Group;
};


//////////////////////////////////////////////////////////////////////////
// COptionsWindow

COptionsWindow::COptionsWindow(const QSharedPointer<CSbieIni>& pBox, const QString& Name, QWidget *parent)
	: CConfigDialog(parent)
{
	m_pBox = pBox;

	m_Template = pBox->GetName().left(9).compare("Template_", Qt::CaseInsensitive) == 0;
	bool ReadOnly = /*pBox->GetAPI()->IsConfigLocked() ||*/ (m_Template && pBox->GetName().mid(9, 6).compare("Local_", Qt::CaseInsensitive) != 0);
	
	m_HoldChange = false;
	m_SkipSaveOnToggle = false;

	m_ImageSize = 2ull*1024*1024*1024;

	QSharedPointer<CSandBoxPlus> pBoxPlus = m_pBox.objectCast<CSandBoxPlus>();
	if (!pBoxPlus.isNull())
		m_Programs = pBoxPlus->GetRecentPrograms();
	m_Programs.insert("program.exe");

	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::CustomizeWindowHint;
	//flags &= ~Qt::WindowContextHelpButtonHint;
	//flags &= ~Qt::WindowSystemMenuHint;
	//flags &= ~Qt::WindowMinMaxButtonsHint;
	flags |= Qt::WindowMinimizeButtonHint;
	//flags &= ~Qt::WindowCloseButtonHint;
	setWindowFlags(flags);

	this->setWindowFlag(Qt::WindowStaysOnTopHint, theGUI->IsAlwaysOnTop());

	ui.setupUi(this);
	new CTabStateManager(ui.tabs, theConf, QStringLiteral("OptionsWindow/Tabs"), this);
	this->setWindowTitle(tr("Sandboxie Plus - '%1' Options").arg(QString(Name).replace("_", " ")));
	// The options content remains behavior-rich and Designer-backed for now;
	// the shared host replaces its platform chrome while each tab is migrated.
	M3DialogHost::Install(this);

	// Replace the self-contained File Options child tab with native M3 controls
	// while preserving OptionsGeneral's generated pointers and handlers.
	if (ui.tabsGeneral && ui.tabFile) {
		const int fileIndex = ui.tabsGeneral->indexOf(ui.tabFile);
		if (fileIndex >= 0) {
			auto* nativeFile = new QWidget(ui.tabsGeneral);
			auto* fileLayout = new QVBoxLayout(nativeFile);
			ui.lblStructure = new QLabel(tr("Box Structure"), nativeFile);
			ui.lblStructure->setProperty("m3NativeSurface", true);
			ui.lblStructure->setStyleSheet("font-weight: 600;");
			fileLayout->addWidget(ui.lblStructure);
			ui.lblWhenEmpty = new QLabel(tr("The box structure can only be changed when the sandbox is empty"), nativeFile);
			ui.lblWhenEmpty->setWordWrap(true);
			fileLayout->addWidget(ui.lblWhenEmpty);
			ui.lblScheme = new QLabel(tr("Virtualization scheme"), nativeFile);
			ui.cmbVersion = new QComboBox(nativeFile);
			fileLayout->addWidget(ui.lblScheme);
			fileLayout->addWidget(ui.cmbVersion);
			ui.chkSeparateUserFolders = new QCheckBox(tr("Separate user folders"), nativeFile);
			ui.chkUseVolumeSerialNumbers = new QCheckBox(tr("Use volume serial numbers for drives, like: \\drive\\C~1234-ABCD"), nativeFile);
			ui.chkRamBox = new QCheckBox(tr("Store the sandbox content in a Ram Disk"), nativeFile);
			ui.chkEncrypt = new QCheckBox(tr("Encrypt sandbox content"), nativeFile);
			ui.btnPassword = new QToolButton(nativeFile);
			ui.btnPassword->setText(tr("Set Password"));
			fileLayout->addWidget(ui.chkSeparateUserFolders);
			fileLayout->addWidget(ui.chkUseVolumeSerialNumbers);
			fileLayout->addWidget(ui.chkRamBox);
			fileLayout->addWidget(ui.chkEncrypt);
			fileLayout->addWidget(ui.btnPassword);
			ui.lblImDisk = new QLabel(tr("<a href=\"addon://ImDisk\">Install ImDisk</a> driver to enable Ram Disk and Disk Image support."), nativeFile);
			ui.lblImDisk->setOpenExternalLinks(false);
			ui.lblImDisk->setWordWrap(true);
			fileLayout->addWidget(ui.lblImDisk);
			ui.lblCrypto = new QLabel(nativeFile);
			ui.lblCrypto->setWordWrap(true);
			fileLayout->addWidget(ui.lblCrypto);
			ui.lblDelete = new QLabel(tr("Box Delete options"), nativeFile);
			ui.lblDelete->setProperty("m3NativeSurface", true);
			ui.lblDelete->setStyleSheet("font-weight: 600;");
			fileLayout->addWidget(ui.lblDelete);
			ui.chkForceProtection = new QCheckBox(tr("Force protection on mount"), nativeFile);
			ui.chkAutoEmpty = new QCheckBox(tr("Auto delete content changes when last sandboxed process terminates"), nativeFile);
			ui.chkProtectBox = new QCheckBox(tr("Protect this sandbox from deletion or emptying"), nativeFile);
			ui.chkProtectBox->setTristate(true);
			fileLayout->addWidget(ui.chkForceProtection);
			fileLayout->addWidget(ui.chkAutoEmpty);
			fileLayout->addWidget(ui.chkProtectBox);
			ui.lblRawDisk = new QLabel(tr("Disk/File access"), nativeFile);
			ui.lblRawDisk->setProperty("m3NativeSurface", true);
			ui.lblRawDisk->setStyleSheet("font-weight: 600;");
			fileLayout->addWidget(ui.lblRawDisk);
			ui.chkRawDiskRead = new QCheckBox(tr("Allow elevated sandboxed applications to read the harddrive"), nativeFile);
			ui.chkRawDiskNotify = new QCheckBox(tr("Warn when an application opens a harddrive handle"), nativeFile);
			ui.chkAllowEfs = new QCheckBox(tr("Allow sandboxed processes to open files protected by EFS"), nativeFile);
			fileLayout->addWidget(ui.chkRawDiskRead);
			fileLayout->addWidget(ui.chkRawDiskNotify);
			fileLayout->addWidget(ui.chkAllowEfs);
			fileLayout->addStretch();
			ui.tabsGeneral->removeTab(fileIndex);
			ui.tabsGeneral->insertTab(fileIndex, nativeFile, tr("File Options"));
			ui.tabFile->deleteLater();
		}
	}

	// Replace the self-contained File Migration child tab with native M3
	// controls while preserving OptionsGeneral's copy-rule model and handlers.
	if (ui.tabsGeneral && ui.tabMigration) {
		const int migrationIndex = ui.tabsGeneral->indexOf(ui.tabMigration);
		if (migrationIndex >= 0) {
			auto* nativeMigration = new QWidget(ui.tabsGeneral);
			auto* migrationLayout = new QVBoxLayout(nativeMigration);
			ui.lblMigration = new QLabel(tr("File Migration"), nativeMigration);
			ui.lblMigration->setProperty("m3NativeSurface", true);
			ui.lblMigration->setStyleSheet("font-weight: 600;");
			migrationLayout->addWidget(ui.lblMigration);
			auto* migrationHint = new QLabel(tr("Sandboxie copies host files into the sandbox when applications modify them; configure limits and wildcard-specific behavior here."), nativeMigration);
			migrationHint->setWordWrap(true);
			migrationLayout->addWidget(migrationHint);
			auto* limitRow = new QHBoxLayout();
			ui.chkCopyLimit = new QCheckBox(tr("Copy file size limit:"), nativeMigration);
			ui.txtCopyLimit = new QLineEdit(nativeMigration);
			ui.txtCopyLimit->setMaximumWidth(100);
			ui.lblCopyLimit = new QLabel(tr("kilobytes"), nativeMigration);
			limitRow->addWidget(ui.chkCopyLimit);
			limitRow->addWidget(ui.txtCopyLimit);
			limitRow->addWidget(ui.lblCopyLimit);
			limitRow->addStretch();
			migrationLayout->addLayout(limitRow);
			ui.chkCopyPrompt = new QCheckBox(tr("Prompt user for large file migration"), nativeMigration);
			ui.chkNoCopyWarn = new QCheckBox(tr("Issue message 2102 when a file is too large"), nativeMigration);
			ui.chkDenyWrite = new QCheckBox(tr("When a file cannot be migrated, open it in read-only mode instead"), nativeMigration);
			migrationLayout->addWidget(ui.chkCopyPrompt);
			migrationLayout->addWidget(ui.chkNoCopyWarn);
			migrationLayout->addWidget(ui.chkDenyWrite);
			migrationLayout->addWidget(new QLabel(tr("Wildcard patterns can configure file-specific behavior:"), nativeMigration));
			ui.treeCopy = new QTreeWidget(nativeMigration);
			ui.treeCopy->setColumnCount(3);
			ui.treeCopy->setHeaderLabels(QStringList() << tr("Action") << tr("Program") << tr("Pattern"));
			ui.treeCopy->setSortingEnabled(true);
			migrationLayout->addWidget(ui.treeCopy, 1);
			auto* copyActions = new QHBoxLayout();
			ui.btnAddCopy = new QPushButton(tr("Add Pattern"), nativeMigration);
			ui.btnDelCopy = new QPushButton(tr("Remove Pattern"), nativeMigration);
			ui.chkShowCopyTmpl = new QCheckBox(tr("Show Templates"), nativeMigration);
			copyActions->addWidget(ui.btnAddCopy);
			copyActions->addWidget(ui.btnDelCopy);
			copyActions->addWidget(ui.chkShowCopyTmpl);
			copyActions->addStretch();
			migrationLayout->addLayout(copyActions);
			ui.chkNoCopyMsg = new QCheckBox(tr("Issue message 2113/2114/2115 when a file is not fully migrated"), nativeMigration);
			ui.chkNoCopyMsg->setToolTip(tr("2113: content discarded; 2114: write access denied; 2115: file opened read-only."));
			migrationLayout->addWidget(ui.chkNoCopyMsg);
			ui.tabsGeneral->removeTab(migrationIndex);
			ui.tabsGeneral->insertTab(migrationIndex, nativeMigration, tr("File Migration"));
			ui.tabMigration->deleteLater();
		}
	}

	// Replace the self-contained Restrictions child tab with native M3 controls
	// while preserving OptionsGeneral's existing handlers and persistence.
	if (ui.tabsGeneral && ui.tabRestrictions) {
		const int restrictionIndex = ui.tabsGeneral->indexOf(ui.tabRestrictions);
		if (restrictionIndex >= 0) {
			auto* nativeRestrictions = new QWidget(ui.tabsGeneral);
			auto* restrictionLayout = new QVBoxLayout(nativeRestrictions);
			ui.lblPrinting = new QLabel(tr("Printing restrictions"), nativeRestrictions);
			ui.lblPrinting->setProperty("m3NativeSurface", true);
			ui.lblPrinting->setStyleSheet("font-weight: 600;");
			restrictionLayout->addWidget(ui.lblPrinting);
			ui.chkBlockSpooler = new QCheckBox(tr("Block access to the printer spooler"), nativeRestrictions);
			ui.chkOpenSpooler = new QCheckBox(tr("Remove spooler restriction; printers can be installed outside the sandbox"), nativeRestrictions);
			ui.chkPrintToFile = new QCheckBox(tr("Allow the print spooler to print to files outside the sandbox"), nativeRestrictions);
			restrictionLayout->addWidget(ui.chkBlockSpooler);
			restrictionLayout->addWidget(ui.chkOpenSpooler);
			restrictionLayout->addWidget(ui.chkPrintToFile);
			ui.lblOther = new QLabel(tr("Other restrictions"), nativeRestrictions);
			ui.lblOther->setProperty("m3NativeSurface", true);
			ui.lblOther->setStyleSheet("font-weight: 600;");
			restrictionLayout->addWidget(ui.lblOther);
			ui.chkOpenProtectedStorage = new QCheckBox(tr("Open System Protected Storage"), nativeRestrictions);
			ui.chkOpenCredentials = new QCheckBox(tr("Open Windows Credentials Store (user mode)"), nativeRestrictions);
			ui.chkCloseClipBoard = new QCheckBox(tr("Block read access to the clipboard"), nativeRestrictions);
			ui.chkVmRead = new QCheckBox(tr("Allow reading memory of unsandboxed processes (not recommended)"), nativeRestrictions);
			ui.chkVmReadNotify = new QCheckBox(tr("Issue message 2111 when a process access is denied"), nativeRestrictions);
			ui.chkProtectPower = new QCheckBox(tr("Prevent sandboxed processes from interfering with power operations (Experimental)"), nativeRestrictions);
			ui.chkUserOperation = new QCheckBox(tr("Prevent interference with the user interface (Experimental)"), nativeRestrictions);
			ui.chkCoverBar = new QCheckBox(tr("Allow sandboxed windows to cover the taskbar"), nativeRestrictions);
			ui.chkBlockCapture = new QCheckBox(tr("Prevent sandboxed processes from capturing window images (Experimental, may cause UI glitches)"), nativeRestrictions);
			restrictionLayout->addWidget(ui.chkOpenProtectedStorage);
			restrictionLayout->addWidget(ui.chkOpenCredentials);
			restrictionLayout->addWidget(ui.chkCloseClipBoard);
			restrictionLayout->addWidget(ui.chkVmRead);
			restrictionLayout->addWidget(ui.chkVmReadNotify);
			restrictionLayout->addWidget(ui.chkProtectPower);
			restrictionLayout->addWidget(ui.chkUserOperation);
			restrictionLayout->addWidget(ui.chkCoverBar);
			restrictionLayout->addWidget(ui.chkBlockCapture);
			restrictionLayout->addStretch();
			ui.tabsGeneral->removeTab(restrictionIndex);
			ui.tabsGeneral->insertTab(restrictionIndex, nativeRestrictions, tr("Restrictions"));
			ui.tabRestrictions->deleteLater();
		}
	}

	// Replace the compact Isolation child tab with native M3 controls while
	// retaining OptionsAdvanced's access-isolation persistence and enablement.
	if (ui.tabsGeneral && ui.tabOtherRestrictions) {
		const int isolationIndex = ui.tabsGeneral->indexOf(ui.tabOtherRestrictions);
		if (isolationIndex >= 0) {
			auto* nativeIsolation = new QWidget(ui.tabsGeneral);
			auto* isolationLayout = new QVBoxLayout(nativeIsolation);
			ui.lblAccess = new QLabel(tr("Access Isolation"), nativeIsolation);
			ui.lblAccess->setProperty("m3NativeSurface", true);
			ui.lblAccess->setStyleSheet("font-weight: 600;");
			isolationLayout->addWidget(ui.lblAccess);
			auto* isolationHint = new QLabel(tr("The options below can be used safely when you do not grant admin rights."), nativeIsolation);
			isolationHint->setWordWrap(true);
			isolationLayout->addWidget(isolationHint);
			ui.chkOpenDevCMApi = new QCheckBox(tr("Allow sandboxed programs to manage Hardware/Devices"), nativeIsolation);
			ui.chkOpenSamEndpoint = new QCheckBox(tr("Open access to Windows Security Account Manager"), nativeIsolation);
			ui.chkOpenLsaEndpoint = new QCheckBox(tr("Open access to Windows Local Security Authority"), nativeIsolation);
			ui.chkOpenWpadEndpoint = new QCheckBox(tr("Open access to Proxy Configurations"), nativeIsolation);
			isolationLayout->addWidget(ui.chkOpenDevCMApi);
			isolationLayout->addWidget(ui.chkOpenSamEndpoint);
			isolationLayout->addWidget(ui.chkOpenLsaEndpoint);
			isolationLayout->addWidget(ui.chkOpenWpadEndpoint);
			isolationLayout->addStretch();
			ui.tabsGeneral->removeTab(isolationIndex);
			ui.tabsGeneral->insertTab(isolationIndex, nativeIsolation, tr("Isolation"));
			ui.tabOtherRestrictions->deleteLater();
		}
	}

	// Replace the Run Menu child tab with native M3 controls while retaining
	// OptionsGeneral's command model, menu population, and reorder handlers.
	if (ui.tabsGeneral && ui.tabRun) {
		const int runIndex = ui.tabsGeneral->indexOf(ui.tabRun);
		if (runIndex >= 0) {
			auto* nativeRun = new QWidget(ui.tabsGeneral);
			auto* runLayout = new QVBoxLayout(nativeRun);
			auto* runHint = new QLabel(tr("Configure custom entries for the sandbox run menu."), nativeRun);
			runHint->setWordWrap(true);
			runLayout->addWidget(runHint);
			ui.treeRun = new QTreeWidget(nativeRun);
			ui.treeRun->setColumnCount(2);
			ui.treeRun->setHeaderLabels(QStringList() << tr("Name") << tr("Command Line"));
			ui.treeRun->setSortingEnabled(true);
			runLayout->addWidget(ui.treeRun, 1);
			auto* runActions = new QHBoxLayout();
			ui.btnAddCmd = new QToolButton(nativeRun);
			ui.btnAddCmd->setText(tr("Add program"));
			ui.btnAddCmd->setPopupMode(QToolButton::MenuButtonPopup);
			ui.btnDelCmd = new QToolButton(nativeRun);
			ui.btnDelCmd->setText(tr("Remove"));
			ui.btnCmdUp = new QToolButton(nativeRun);
			ui.btnCmdUp->setText(tr("Move Up"));
			ui.btnCmdDown = new QToolButton(nativeRun);
			ui.btnCmdDown->setText(tr("Move Down"));
			runActions->addWidget(ui.btnAddCmd);
			runActions->addWidget(ui.btnDelCmd);
			runActions->addWidget(ui.btnCmdUp);
			runActions->addWidget(ui.btnCmdDown);
			runActions->addStretch();
			runLayout->addLayout(runActions);
			ui.tabsGeneral->removeTab(runIndex);
			ui.tabsGeneral->insertTab(runIndex, nativeRun, tr("Run Menu"));
			ui.tabRun->deleteLater();
		}
	}

	// Replace the Advanced Security child tab with native M3 controls while
	// preserving OptionsAdvanced's security persistence and dependency rules.
	if (ui.tabsSecurity && ui.tabPrivileges) {
		const int privilegeIndex = ui.tabsSecurity->indexOf(ui.tabPrivileges);
		if (privilegeIndex >= 0) {
			auto* nativePrivileges = new QWidget(ui.tabsSecurity);
			auto* privilegeLayout = new QVBoxLayout(nativePrivileges);
			ui.lblPrivilege = new QLabel(tr("Privilege isolation"), nativePrivileges);
			ui.lblPrivilege->setProperty("m3NativeSurface", true);
			ui.lblPrivilege->setStyleSheet("font-weight: 600;");
			privilegeLayout->addWidget(ui.lblPrivilege);
			ui.chkProtectSCM = new QCheckBox(tr("Allow only privileged processes to access the Service Control Manager"), nativePrivileges);
			ui.chkRestrictServices = new QCheckBox(tr("Do not start sandboxed services using a system token (recommended)"), nativePrivileges);
			ui.chkElevateRpcss = new QCheckBox(tr("Start the sandboxed RpcSs as a SYSTEM process (not recommended)"), nativePrivileges);
			ui.chkProtectSystem = new QCheckBox(tr("Protect sandboxed SYSTEM processes from unprivileged processes"), nativePrivileges);
			ui.chkDropPrivileges = new QCheckBox(tr("Drop critical privileges from processes running with a SYSTEM token"), nativePrivileges);
			ui.chkDropConHostIntegrity = new QCheckBox(tr("Drop ConHost.exe Process Integrity Level"), nativePrivileges);
			privilegeLayout->addWidget(ui.chkProtectSCM);
			privilegeLayout->addWidget(ui.chkRestrictServices);
			privilegeLayout->addWidget(ui.chkElevateRpcss);
			privilegeLayout->addWidget(ui.chkProtectSystem);
			privilegeLayout->addWidget(ui.chkDropPrivileges);
			privilegeLayout->addWidget(ui.chkDropConHostIntegrity);
			ui.lblToken = new QLabel(tr("Sandboxie token"), nativePrivileges);
			ui.lblToken->setProperty("m3NativeSurface", true);
			ui.lblToken->setStyleSheet("font-weight: 600;");
			privilegeLayout->addWidget(ui.lblToken);
			ui.chkSbieLogon = new QCheckBox(tr("Use a Sandboxie login instead of an anonymous token"), nativePrivileges);
			ui.chkSbieLogon->setToolTip(tr("A Sandboxie login identifies the box while retaining token isolation."));
			ui.chkCreateToken = new QCheckBox(tr("Create a new sandboxed token instead of stripping down the original token"), nativePrivileges);
			ui.chkCreateToken->setTristate(true);
			ui.chkCreateToken->setToolTip(tr("Checked: add the box group to the token. Partially checked: create the token without extra groups."));
			ui.chkNotUntrusted = new QCheckBox(tr("Use LOW integrity token instead of UNTRUSTED (reduces isolation)"), nativePrivileges);
			privilegeLayout->addWidget(ui.chkSbieLogon);
			privilegeLayout->addWidget(ui.chkCreateToken);
			privilegeLayout->addWidget(ui.chkNotUntrusted);
			privilegeLayout->addStretch();
			ui.tabsSecurity->removeTab(privilegeIndex);
			ui.tabsSecurity->insertTab(privilegeIndex, nativePrivileges, tr("Advanced Security"));
			ui.tabPrivileges->deleteLater();
		}
	}

	// Replace Security Isolation with native M3 controls while preserving
	// OptionsAdvanced's safety dependencies and compatibility semantics.
	if (ui.tabsSecurity && ui.tabIsolation) {
		const int isolationIndex = ui.tabsSecurity->indexOf(ui.tabIsolation);
		if (isolationIndex >= 0) {
			auto* nativeSecurityIsolation = new QWidget(ui.tabsSecurity);
			auto* securityLayout = new QVBoxLayout(nativeSecurityIsolation);
			auto* compatibilityHint = new QLabel(tr("Isolation features can affect compatibility. Disable them only when this sandbox is used for portability rather than security."), nativeSecurityIsolation);
			compatibilityHint->setWordWrap(true);
			securityLayout->addWidget(compatibilityHint);
			ui.lblIsolation = new QLabel(tr("Security Isolation & Filtering"), nativeSecurityIsolation);
			ui.lblIsolation->setProperty("m3NativeSurface", true);
			ui.lblIsolation->setStyleSheet("font-weight: 600;");
			securityLayout->addWidget(ui.lblIsolation);
			ui.chkNoSecurityIsolation = new QCheckBox(tr("Disable Security Isolation"), nativeSecurityIsolation);
			ui.chkNoSecurityIsolation->setToolTip(tr("Disabling token isolation reduces security and enables application-compartment behavior."));
			ui.chkNoSecurityFiltering = new QCheckBox(tr("Disable Security Filtering (not recommended)"), nativeSecurityIsolation);
			ui.chkNoSecurityFiltering->setToolTip(tr("Security filtering enforces filesystem, registry, and process-access restrictions."));
			securityLayout->addWidget(ui.chkNoSecurityIsolation);
			securityLayout->addWidget(ui.chkNoSecurityFiltering);
			ui.lblDesktop = new QLabel(tr("Desktop Isolation"), nativeSecurityIsolation);
			ui.lblDesktop->setProperty("m3NativeSurface", true);
			ui.lblDesktop->setStyleSheet("font-weight: 600;");
			securityLayout->addWidget(ui.lblDesktop);
			ui.chkSbieDesktop = new QCheckBox(tr("Run processes on an own sandboxed desktop"), nativeSecurityIsolation);
			ui.chkOpenWndStation = new QCheckBox(tr("Open Window Station (improves compatibility by reducing desktop isolation)"), nativeSecurityIsolation);
			securityLayout->addWidget(ui.chkSbieDesktop);
			securityLayout->addWidget(ui.chkOpenWndStation);
			securityLayout->addStretch();
			ui.tabsSecurity->removeTab(isolationIndex);
			ui.tabsSecurity->insertTab(isolationIndex, nativeSecurityIsolation, tr("Security Isolation"));
			ui.tabIsolation->deleteLater();
		}
	}

	// Replace Box Protection with native M3 controls while preserving host
	// process rules, template toggles, and existing protection persistence.
	if (ui.tabsSecurity && ui.tabPrivate) {
		const int protectionIndex = ui.tabsSecurity->indexOf(ui.tabPrivate);
		if (protectionIndex >= 0) {
			auto* nativeProtection = new QWidget(ui.tabsSecurity);
			auto* protectionLayout = new QVBoxLayout(nativeProtection);
			auto* protectionHint = new QLabel(tr("Confidential sandboxes protect processes and files from unauthorized host access."), nativeProtection);
			protectionHint->setWordWrap(true);
			protectionLayout->addWidget(protectionHint);
			ui.lblBoxProtection = new QLabel(tr("Box Protection"), nativeProtection);
			ui.lblBoxProtection->setProperty("m3NativeSurface", true);
			ui.lblBoxProtection->setStyleSheet("font-weight: 600;");
			protectionLayout->addWidget(ui.lblBoxProtection);
			ui.chkConfidential = new QCheckBox(tr("Protect processes within this box from host processes"), nativeProtection);
			ui.chkLessConfidential = new QCheckBox(tr("Allow useful Windows processes access to protected processes"), nativeProtection);
			ui.chkProtectWindow = new QCheckBox(tr("Prevent processes from capturing window images from sandboxed windows"), nativeProtection);
			ui.chkProtectAdminOnly = new QCheckBox(tr("Require SandMan to run as Administrator to access protected box files"), nativeProtection);
			ui.chkAdminOnly = new QCheckBox(tr("Only Administrator user accounts can make changes to this sandbox"), nativeProtection);
			ui.chkNotifyProtect = new QCheckBox(tr("Issue message 1318/1317 when a host process tries to access a sandboxed process or the box root"), nativeProtection);
			protectionLayout->addWidget(ui.chkConfidential);
			protectionLayout->addWidget(ui.chkLessConfidential);
			protectionLayout->addWidget(ui.chkProtectWindow);
			protectionLayout->addWidget(ui.chkProtectAdminOnly);
			protectionLayout->addWidget(ui.chkAdminOnly);
			protectionLayout->addWidget(ui.chkNotifyProtect);
			protectionLayout->addWidget(new QLabel(tr("Protected host processes"), nativeProtection));
			ui.treeHostProc = new QTreeWidget(nativeProtection);
			ui.treeHostProc->setColumnCount(3);
			ui.treeHostProc->setHeaderLabels(QStringList() << tr("Process") << tr("Action") << QString());
			ui.treeHostProc->setSortingEnabled(true);
			protectionLayout->addWidget(ui.treeHostProc, 1);
			auto* hostActions = new QHBoxLayout();
			ui.btnHostProcessAllow = new QPushButton(tr("Allow Process"), nativeProtection);
			ui.btnHostProcessDeny = new QPushButton(tr("Deny Process"), nativeProtection);
			ui.btnDelHostProcess = new QPushButton(tr("Remove"), nativeProtection);
			ui.chkShowHostProcTmpl = new QCheckBox(tr("Show Templates"), nativeProtection);
			hostActions->addWidget(ui.btnHostProcessAllow);
			hostActions->addWidget(ui.btnHostProcessDeny);
			hostActions->addWidget(ui.btnDelHostProcess);
			hostActions->addWidget(ui.chkShowHostProcTmpl);
			hostActions->addStretch();
			protectionLayout->addLayout(hostActions);
			ui.tabsSecurity->removeTab(protectionIndex);
			ui.tabsSecurity->insertTab(protectionIndex, nativeProtection, tr("Box Protection"));
			ui.tabPrivate->deleteLater();
		}
	}

	// Replace Job Object with native M3 controls while preserving limit
	// validation, dynamic enablement, and OptionsAdvanced persistence.
	if (ui.tabsSecurity && ui.tabJob) {
		const int jobIndex = ui.tabsSecurity->indexOf(ui.tabJob);
		if (jobIndex >= 0) {
			auto* nativeJob = new QWidget(ui.tabsSecurity);
			auto* jobLayout = new QVBoxLayout(nativeJob);
			ui.lblJob = new QLabel(tr("Other isolation"), nativeJob);
			ui.lblJob->setProperty("m3NativeSurface", true);
			ui.lblJob->setStyleSheet("font-weight: 600;");
			jobLayout->addWidget(ui.lblJob);
			ui.chkAddToJob = new QCheckBox(tr("Add sandboxed processes to job objects (recommended)"), nativeJob);
			ui.chkNestedJobs = new QCheckBox(tr("Allow use of nested job objects (works on Windows 8 and later)"), nativeJob);
			jobLayout->addWidget(ui.chkAddToJob);
			jobLayout->addWidget(ui.chkNestedJobs);
			ui.lblLimit = new QLabel(tr("Limit restrictions"), nativeJob);
			ui.lblLimit->setProperty("m3NativeSurface", true);
			ui.lblLimit->setStyleSheet("font-weight: 600;");
			jobLayout->addWidget(ui.lblLimit);
			auto addLimit = [&](QLineEdit*& edit, QLabel*& unit, const QString& label, const QString& placeholder) {
				auto* row = new QHBoxLayout();
				row->addWidget(new QLabel(label, nativeJob));
				edit = new QLineEdit(nativeJob);
				edit->setMaximumWidth(125);
				edit->setPlaceholderText(placeholder);
				unit = new QLabel(tr("bytes"), nativeJob);
				row->addWidget(edit);
				row->addWidget(unit);
				row->addStretch();
				jobLayout->addLayout(row);
			};
			addLimit(ui.txtSingleMemory, ui.lblSingleMemory, tr("Single Process Memory Limit:"), tr("unlimited"));
			addLimit(ui.txtTotalMemory, ui.lblTotalMemory, tr("Total Processes Memory Limit:"), tr("unlimited"));
			addLimit(ui.txtTotalNumber, ui.lblTotalNumber, tr("Total Processes Number Limit:"), tr("unlimited"));
			addLimit(ui.txtCpuRateLimit, ui.lblCpuRateLimit, tr("Total CPU Rate Limit (%):"), tr("unlimited"));
			ui.lblCpuRateLimit->setText(tr("%"));
			jobLayout->addStretch();
			ui.tabsSecurity->removeTab(jobIndex);
			ui.tabsSecurity->insertTab(jobIndex, nativeJob, tr("Job Object"));
			ui.tabJob->deleteLater();
		}
	}

	// Replace Program Groups with native M3 controls while preserving the
	// existing group tree model, delegates, and add/remove handlers.
	if (ui.tabs && ui.tabGroups) {
		const int groupIndex = ui.tabs->indexOf(ui.tabGroups);
		if (groupIndex >= 0) {
			auto* legacyGroups = ui.tabGroups;
			auto* nativeGroups = new QWidget(ui.tabs);
			auto* groupsLayout = new QVBoxLayout(nativeGroups);
			auto* groupsHint = new QLabel(tr("Group programs under a shared name for use by sandbox settings; box groups override template groups."), nativeGroups);
			groupsHint->setWordWrap(true);
			groupsLayout->addWidget(groupsHint);
			ui.treeGroups = new QTreeWidget(nativeGroups);
			ui.treeGroups->setColumnCount(1);
			ui.treeGroups->setHeaderLabels(QStringList() << tr("Name"));
			ui.treeGroups->setSortingEnabled(true);
			groupsLayout->addWidget(ui.treeGroups, 1);
			auto* groupActions = new QHBoxLayout();
			ui.btnAddGroup = new QPushButton(tr("Add Group"), nativeGroups);
			ui.btnAddProg = new QPushButton(tr("Add Program"), nativeGroups);
			ui.btnDelProg = new QPushButton(tr("Remove"), nativeGroups);
			ui.chkShowGroupTmpl = new QCheckBox(tr("Show Templates"), nativeGroups);
			groupActions->addWidget(ui.btnAddGroup);
			groupActions->addWidget(ui.btnAddProg);
			groupActions->addWidget(ui.btnDelProg);
			groupActions->addWidget(ui.chkShowGroupTmpl);
			groupActions->addStretch();
			groupsLayout->addLayout(groupActions);
			ui.tabs->removeTab(groupIndex);
			ui.tabs->insertTab(groupIndex, nativeGroups, tr("Program Groups"));
			ui.tabGroups = nativeGroups;
			legacyGroups->deleteLater();
		}
	}

	// Replace Force Programs with native M3 controls while preserving force
	// rules, browse menus, delegates, and disable-for-box behavior.
	if (ui.tabsForce && ui.tabForceProgs) {
		const int forceIndex = ui.tabsForce->indexOf(ui.tabForceProgs);
		if (forceIndex >= 0) {
			auto* legacyForce = ui.tabForceProgs;
			auto* nativeForce = new QWidget(ui.tabsForce);
			auto* forceLayout = new QVBoxLayout(nativeForce);
			auto* forceHint = new QLabel(tr("Programs started from these entries or locations are placed in this sandbox unless explicitly started elsewhere."), nativeForce);
			forceHint->setWordWrap(true);
			forceLayout->addWidget(forceHint);
			ui.treeForced = new QTreeWidget(nativeForce);
			ui.treeForced->setColumnCount(2);
			ui.treeForced->setHeaderLabels(QStringList() << tr("Type") << tr("Name"));
			ui.treeForced->setSortingEnabled(true);
			forceLayout->addWidget(ui.treeForced, 1);
			auto* forceActions = new QHBoxLayout();
			ui.btnForceProg = new QToolButton(nativeForce);
			ui.btnForceProg->setText(tr("Force Program"));
			ui.btnForceChild = new QToolButton(nativeForce);
			ui.btnForceChild->setText(tr("Force Children"));
			ui.btnForceDir = new QToolButton(nativeForce);
			ui.btnForceDir->setText(tr("Force Folder"));
			ui.btnDelForce = new QPushButton(tr("Remove"), nativeForce);
			ui.chkShowForceTmpl = new QCheckBox(tr("Show Templates"), nativeForce);
			forceActions->addWidget(ui.btnForceProg);
			forceActions->addWidget(ui.btnForceChild);
			forceActions->addWidget(ui.btnForceDir);
			forceActions->addWidget(ui.btnDelForce);
			forceActions->addWidget(ui.chkShowForceTmpl);
			forceActions->addStretch();
			forceLayout->addLayout(forceActions);
			ui.chkDisableForced = new QCheckBox(tr("Disable forced Process and Folder rules for this sandbox"), nativeForce);
			forceLayout->addWidget(ui.chkDisableForced);
			ui.tabsForce->removeTab(forceIndex);
			ui.tabsForce->insertTab(forceIndex, nativeForce, tr("Force Programs"));
			ui.tabForceProgs = nativeForce;
			ui.tabForceProgs->setProperty("m3NativeSurface", true);
			legacyForce->deleteLater();
		}
	}

	// Replace Breakout Programs with native M3 controls while preserving
	// browse menus, security advisory copy, delegates, and breakout handlers.
	if (ui.tabsForce && ui.tabBreakout) {
		const int breakoutIndex = ui.tabsForce->indexOf(ui.tabBreakout);
		if (breakoutIndex >= 0) {
			auto* legacyBreakout = ui.tabBreakout;
			auto* nativeBreakout = new QWidget(ui.tabsForce);
			auto* breakoutLayout = new QVBoxLayout(nativeBreakout);
			auto* breakoutHint = new QLabel(tr("Programs listed here may break out of this sandbox or be captured into another sandbox."), nativeBreakout);
			breakoutHint->setWordWrap(true);
			breakoutLayout->addWidget(breakoutHint);
			ui.treeBreakout = new QTreeWidget(nativeBreakout);
			ui.treeBreakout->setColumnCount(2);
			ui.treeBreakout->setHeaderLabels(QStringList() << tr("Type") << tr("Name"));
			ui.treeBreakout->setSortingEnabled(true);
			breakoutLayout->addWidget(ui.treeBreakout, 1);
			auto* breakoutActions = new QHBoxLayout();
			ui.btnBreakoutProg = new QToolButton(nativeBreakout);
			ui.btnBreakoutProg->setText(tr("Breakout Program"));
			ui.btnBreakoutDir = new QToolButton(nativeBreakout);
			ui.btnBreakoutDir->setText(tr("Breakout Folder"));
			ui.btnBreakoutDoc = new QToolButton(nativeBreakout);
			ui.btnBreakoutDoc->setText(tr("Breakout Document"));
			ui.btnDelBreakout = new QPushButton(tr("Remove"), nativeBreakout);
			ui.chkShowBreakoutTmpl = new QCheckBox(tr("Show Templates"), nativeBreakout);
			breakoutActions->addWidget(ui.btnBreakoutProg);
			breakoutActions->addWidget(ui.btnBreakoutDir);
			breakoutActions->addWidget(ui.btnBreakoutDoc);
			breakoutActions->addWidget(ui.btnDelBreakout);
			breakoutActions->addWidget(ui.chkShowBreakoutTmpl);
			breakoutActions->addStretch();
			breakoutLayout->addLayout(breakoutActions);
			ui.lblBreakOut = new QLabel(tr("<b>Security advisory:</b> Breakout rules combined with broad resource paths can compromise isolation; review the documentation before use."), nativeBreakout);
			ui.lblBreakOut->setWordWrap(true);
			ui.lblBreakOut->setTextFormat(Qt::RichText);
			ui.lblBreakOut->setOpenExternalLinks(false);
			breakoutLayout->addWidget(ui.lblBreakOut);
			ui.tabsForce->removeTab(breakoutIndex);
			ui.tabsForce->insertTab(breakoutIndex, nativeBreakout, tr("Breakout Programs"));
			ui.tabBreakout = nativeBreakout;
			legacyBreakout->deleteLater();
		}
	}

	// Replace Start Restrictions with native M3 controls while preserving
	// radio semantics, program-list delegates, and start-policy persistence.
	if (ui.tabs && ui.tabStart) {
		const int startIndex = ui.tabs->indexOf(ui.tabStart);
		if (startIndex >= 0) {
			auto* legacyStart = ui.tabStart;
			auto* nativeStart = new QWidget(ui.tabs);
			auto* startLayout = new QVBoxLayout(nativeStart);
			auto* startHint = new QLabel(tr("Choose which programs may start in this sandbox; installed programs cannot start when selection-only mode is active."), nativeStart);
			startHint->setWordWrap(true);
			startLayout->addWidget(startHint);
			ui.radStartAll = new QRadioButton(tr("Allow all programs to start in this sandbox."), nativeStart);
			ui.radStartExcept = new QRadioButton(tr("Prevent selected programs from starting in this sandbox."), nativeStart);
			ui.radStartSelected = new QRadioButton(tr("Allow only selected programs to start in this sandbox."), nativeStart);
			startLayout->addWidget(ui.radStartAll);
			startLayout->addWidget(ui.radStartExcept);
			startLayout->addWidget(ui.radStartSelected);
			ui.treeStart = new QTreeWidget(nativeStart);
			ui.treeStart->setColumnCount(1);
			ui.treeStart->setHeaderLabels(QStringList() << tr("Name"));
			ui.treeStart->setSortingEnabled(true);
			startLayout->addWidget(ui.treeStart, 1);
			auto* startActions = new QHBoxLayout();
			ui.btnAddStartProg = new QPushButton(tr("Add Program"), nativeStart);
			ui.btnDelStartProg = new QPushButton(tr("Remove"), nativeStart);
			ui.chkShowStartTmpl = new QCheckBox(tr("Show Templates"), nativeStart);
			ui.chkShowStartTmpl->setVisible(false);
			startActions->addWidget(ui.btnAddStartProg);
			startActions->addWidget(ui.btnDelStartProg);
			startActions->addWidget(ui.chkShowStartTmpl);
			startActions->addStretch();
			startLayout->addLayout(startActions);
			ui.chkStartBlockMsg = new QCheckBox(tr("Issue message 1308 when a program fails to start"), nativeStart);
			ui.chkAlertBeforeStart = new QCheckBox(tr("Display a warning before starting a process in this sandbox from an external source"), nativeStart);
			ui.chkAlertBeforeStart->setToolTip(tr("Helps prevent programs from running without the user's knowledge or consent."));
			startLayout->addWidget(ui.chkStartBlockMsg);
			startLayout->addWidget(ui.chkAlertBeforeStart);
			ui.tabs->removeTab(startIndex);
			ui.tabs->insertTab(startIndex, nativeStart, tr("Start Restrictions"));
			ui.tabStart = nativeStart;
			legacyStart->deleteLater();
		}
	}

	// Replace Resource Access > Files with native M3 controls while retaining
	// access-rule columns, add-menu semantics, templates, and deletion handlers.
	if (ui.tabsAccess && ui.tabFiles) {
		const int filesIndex = ui.tabsAccess->indexOf(ui.tabFiles);
		if (filesIndex >= 0) {
			auto* legacyFiles = ui.tabFiles;
			auto* nativeFiles = new QWidget(ui.tabsAccess);
			auto* filesLayout = new QVBoxLayout(nativeFiles);
			auto* filesHint = new QLabel(tr("Configure which processes can access files, folders, and pipes. Open access applies to programs outside the sandbox."), nativeFiles);
			filesHint->setWordWrap(true);
			filesLayout->addWidget(filesHint);
			ui.treeFiles = new QTreeWidget(nativeFiles);
			ui.treeFiles->setColumnCount(4);
			ui.treeFiles->setHeaderLabels(QStringList() << tr("Type") << tr("Program") << tr("Access") << tr("Path"));
			ui.treeFiles->setSortingEnabled(true);
			filesLayout->addWidget(ui.treeFiles, 1);
			auto* filesActions = new QHBoxLayout();
			ui.btnAddFile = new QToolButton(nativeFiles);
			ui.btnAddFile->setText(tr("Add File/Folder"));
			ui.btnDelFile = new QPushButton(tr("Remove"), nativeFiles);
			ui.chkShowFilesTmpl = new QCheckBox(tr("Show Templates"), nativeFiles);
			filesActions->addWidget(ui.btnAddFile);
			filesActions->addWidget(ui.btnDelFile);
			filesActions->addWidget(ui.chkShowFilesTmpl);
			filesActions->addStretch();
			filesLayout->addLayout(filesActions);
			ui.tabsAccess->removeTab(filesIndex);
			ui.tabsAccess->insertTab(filesIndex, nativeFiles, tr("Files"));
			ui.tabFiles = nativeFiles;
			legacyFiles->deleteLater();
		}
	}

	// Replace Resource Access > Registry with native M3 controls while
	// retaining access columns, add-menu behavior, templates, and deletion.
	if (ui.tabsAccess && ui.tabKeys) {
		const int keysIndex = ui.tabsAccess->indexOf(ui.tabKeys);
		if (keysIndex >= 0) {
			auto* legacyKeys = ui.tabKeys;
			auto* nativeKeys = new QWidget(ui.tabsAccess);
			auto* keysLayout = new QVBoxLayout(nativeKeys);
			auto* keysHint = new QLabel(tr("Configure which processes can access the Registry. Open access applies to programs outside the sandbox."), nativeKeys);
			keysHint->setWordWrap(true);
			keysLayout->addWidget(keysHint);
			ui.treeKeys = new QTreeWidget(nativeKeys);
			ui.treeKeys->setColumnCount(4);
			ui.treeKeys->setHeaderLabels(QStringList() << tr("Type") << tr("Program") << tr("Access") << tr("Path"));
			ui.treeKeys->setSortingEnabled(true);
			keysLayout->addWidget(ui.treeKeys, 1);
			auto* keysActions = new QHBoxLayout();
			ui.btnAddKey = new QToolButton(nativeKeys);
			ui.btnAddKey->setText(tr("Add Reg Key"));
			ui.btnDelKey = new QPushButton(tr("Remove"), nativeKeys);
			ui.chkShowKeysTmpl = new QCheckBox(tr("Show Templates"), nativeKeys);
			keysActions->addWidget(ui.btnAddKey);
			keysActions->addWidget(ui.btnDelKey);
			keysActions->addWidget(ui.chkShowKeysTmpl);
			keysActions->addStretch();
			keysLayout->addLayout(keysActions);
			ui.tabsAccess->removeTab(keysIndex);
			ui.tabsAccess->insertTab(keysIndex, nativeKeys, tr("Registry"));
			ui.tabKeys = nativeKeys;
			legacyKeys->deleteLater();
		}
	}

	// Replace Resource Access > IPC with native M3 controls while preserving
	// IPC path columns, add-menu semantics, templates, and deletion handlers.
	if (ui.tabsAccess && ui.tabIPC) {
		const int ipcIndex = ui.tabsAccess->indexOf(ui.tabIPC);
		if (ipcIndex >= 0) {
			auto* legacyIPC = ui.tabIPC;
			auto* nativeIPC = new QWidget(ui.tabsAccess);
			auto* ipcLayout = new QVBoxLayout(nativeIPC);
			auto* ipcHint = new QLabel(tr("Configure access to NT IPC objects such as ALPC ports and process context. Use $:program.exe to target a process."), nativeIPC);
			ipcHint->setWordWrap(true);
			ipcLayout->addWidget(ipcHint);
			ui.treeIPC = new QTreeWidget(nativeIPC);
			ui.treeIPC->setColumnCount(4);
			ui.treeIPC->setHeaderLabels(QStringList() << tr("Type") << tr("Program") << tr("Access") << tr("Path"));
			ui.treeIPC->setSortingEnabled(true);
			ipcLayout->addWidget(ui.treeIPC, 1);
			auto* ipcActions = new QHBoxLayout();
			ui.btnAddIPC = new QToolButton(nativeIPC);
			ui.btnAddIPC->setText(tr("Add IPC Path"));
			ui.btnDelIPC = new QPushButton(tr("Remove"), nativeIPC);
			ui.chkShowIPCTmpl = new QCheckBox(tr("Show Templates"), nativeIPC);
			ipcActions->addWidget(ui.btnAddIPC);
			ipcActions->addWidget(ui.btnDelIPC);
			ipcActions->addWidget(ui.chkShowIPCTmpl);
			ipcActions->addStretch();
			ipcLayout->addLayout(ipcActions);
			ui.tabsAccess->removeTab(ipcIndex);
			ui.tabsAccess->insertTab(ipcIndex, nativeIPC, tr("IPC"));
			ui.tabIPC = nativeIPC;
			legacyIPC->deleteLater();
		}
	}

	// Replace Resource Access > Wnd with native M3 controls while preserving
	// window-class access columns, templates, and no-rename policy behavior.
	if (ui.tabsAccess && ui.tabWnd) {
		const int wndIndex = ui.tabsAccess->indexOf(ui.tabWnd);
		if (wndIndex >= 0) {
			auto* legacyWnd = ui.tabWnd;
			auto* nativeWnd = new QWidget(ui.tabsAccess);
			auto* wndLayout = new QVBoxLayout(nativeWnd);
			auto* wndHint = new QLabel(tr("Configure which processes can access desktop objects such as windows."), nativeWnd);
			wndHint->setWordWrap(true);
			wndLayout->addWidget(wndHint);
			ui.treeWnd = new QTreeWidget(nativeWnd);
			ui.treeWnd->setColumnCount(4);
			ui.treeWnd->setHeaderLabels(QStringList() << tr("Type") << tr("Program") << tr("Access") << tr("Wnd Class"));
			ui.treeWnd->setSortingEnabled(true);
			wndLayout->addWidget(ui.treeWnd, 1);
			auto* wndActions = new QHBoxLayout();
			ui.btnAddWnd = new QToolButton(nativeWnd);
			ui.btnAddWnd->setText(tr("Add Wnd Class"));
			ui.btnDelWnd = new QPushButton(tr("Remove"), nativeWnd);
			ui.chkShowWndTmpl = new QCheckBox(tr("Show Templates"), nativeWnd);
			wndActions->addWidget(ui.btnAddWnd);
			wndActions->addWidget(ui.btnDelWnd);
			wndActions->addWidget(ui.chkShowWndTmpl);
			wndActions->addStretch();
			wndLayout->addLayout(wndActions);
			ui.chkNoWindowRename = new QCheckBox(tr("Do not alter window class names created by sandboxed programs"), nativeWnd);
			wndLayout->addWidget(ui.chkNoWindowRename);
			ui.tabsAccess->removeTab(wndIndex);
			ui.tabsAccess->insertTab(wndIndex, nativeWnd, tr("Wnd"));
			ui.tabWnd = nativeWnd;
			legacyWnd->deleteLater();
		}
	}

	// Replace Resource Access > COM with native M3 controls while retaining
	// COM virtualization policy, access columns, templates, and deletion.
	if (ui.tabsAccess && ui.tabCOM) {
		const int comIndex = ui.tabsAccess->indexOf(ui.tabCOM);
		if (comIndex >= 0) {
			auto* legacyCOM = ui.tabCOM;
			auto* nativeCOM = new QWidget(ui.tabsAccess);
			auto* comLayout = new QVBoxLayout(nativeCOM);
			auto* comHint = new QLabel(tr("Configure which processes can access COM objects."), nativeCOM);
			comHint->setWordWrap(true);
			comLayout->addWidget(comHint);
			ui.treeCOM = new QTreeWidget(nativeCOM);
			ui.treeCOM->setColumnCount(4);
			ui.treeCOM->setHeaderLabels(QStringList() << tr("Type") << tr("Program") << tr("Access") << tr("Class Id"));
			ui.treeCOM->setSortingEnabled(true);
			comLayout->addWidget(ui.treeCOM, 1);
			auto* comActions = new QHBoxLayout();
			ui.btnAddCOM = new QToolButton(nativeCOM);
			ui.btnAddCOM->setText(tr("Add COM Object"));
			ui.btnDelCOM = new QPushButton(tr("Remove"), nativeCOM);
			ui.chkShowCOMTmpl = new QCheckBox(tr("Show Templates"), nativeCOM);
			comActions->addWidget(ui.btnAddCOM);
			comActions->addWidget(ui.btnDelCOM);
			comActions->addWidget(ui.chkShowCOMTmpl);
			comActions->addStretch();
			comLayout->addLayout(comActions);
			ui.chkOpenCOM = new QCheckBox(tr("Do not virtualize COM; open access to the host COM infrastructure (not recommended)"), nativeCOM);
			comLayout->addWidget(ui.chkOpenCOM);
			ui.tabsAccess->removeTab(comIndex);
			ui.tabsAccess->insertTab(comIndex, nativeCOM, tr("COM"));
			ui.tabCOM = nativeCOM;
			legacyCOM->deleteLater();
		}
	}

	// Replace Resource Access > Access Policies with native M3 controls while
	// retaining privacy/rule-specificity dependencies and boxed-access policy.
	if (ui.tabsAccess && ui.tabPolicy) {
		const int policyIndex = ui.tabsAccess->indexOf(ui.tabPolicy);
		if (policyIndex >= 0) {
			auto* legacyPolicy = ui.tabPolicy;
			auto* nativePolicy = new QWidget(ui.tabsAccess);
			auto* policyLayout = new QVBoxLayout(nativePolicy);
			ui.lblMode = new QLabel(tr("Access Mode"), nativePolicy);
			ui.lblMode->setProperty("m3NativeSurface", true);
			ui.lblMode->setStyleSheet("font-weight: 600;");
			policyLayout->addWidget(ui.lblMode);
			ui.chkPrivacy = new QCheckBox(tr("Privacy Mode: block file and registry access except generic system locations"), nativePolicy);
			ui.chkPrivacy->setToolTip(tr("Privacy Mode allows only generic Windows locations unless explicit access is granted."));
			policyLayout->addWidget(ui.chkPrivacy);
			ui.lblPolicy = new QLabel(tr("Rule Policies"), nativePolicy);
			ui.lblPolicy->setProperty("m3NativeSurface", true);
			ui.lblPolicy->setStyleSheet("font-weight: 600;");
			policyLayout->addWidget(ui.lblPolicy);
			ui.chkUseSpecificity = new QCheckBox(tr("Prioritize rules by specificity and process match level"), nativePolicy);
			ui.chkUseSpecificity->setToolTip(tr("More specific paths and stronger process matches take precedence."));
			policyLayout->addWidget(ui.chkUseSpecificity);
			policyLayout->addWidget(new QLabel(tr("Rule specificity measures the matching path; process-name or group matches have higher priority than global matches."), nativePolicy));
			ui.chkCloseForBox = new QCheckBox(tr("Apply Close rules to binaries located inside the sandbox"), nativePolicy);
			ui.chkNoOpenForBox = new QCheckBox(tr("Apply Open rules only to binaries located outside the sandbox"), nativePolicy);
			policyLayout->addWidget(ui.chkCloseForBox);
			policyLayout->addWidget(ui.chkNoOpenForBox);
			policyLayout->addStretch();
			ui.tabsAccess->removeTab(policyIndex);
			ui.tabsAccess->insertTab(policyIndex, nativePolicy, tr("Access Policies"));
			ui.tabPolicy = nativePolicy;
			legacyPolicy->deleteLater();
		}
	}

	// Replace Network Options > Process Restrictions with native M3 controls
	// while preserving embedded per-program editors and network persistence.
	if (ui.tabsInternet && ui.tabINet) {
		const int inetIndex = ui.tabsInternet->indexOf(ui.tabINet);
		if (inetIndex >= 0) {
			auto* legacyINet = ui.tabINet;
			auto* nativeINet = new QWidget(ui.tabsInternet);
			auto* inetLayout = new QVBoxLayout(nativeINet);
			auto* modeRow = new QHBoxLayout();
			modeRow->addWidget(new QLabel(tr("Network access for unlisted processes:"), nativeINet));
			ui.cmbBlockINet = new QComboBox(nativeINet);
			modeRow->addWidget(ui.cmbBlockINet, 1);
			inetLayout->addLayout(modeRow);
			ui.chkINetBlockPrompt = new QCheckBox(tr("Prompt before allowing a network-access exemption"), nativeINet);
			ui.chkINetBlockMsg = new QCheckBox(tr("Issue message 1307 when a program is denied internet access"), nativeINet);
			inetLayout->addWidget(ui.chkINetBlockPrompt);
			inetLayout->addWidget(ui.chkINetBlockMsg);
			ui.treeINet = new QTreeWidget(nativeINet);
			ui.treeINet->setColumnCount(2);
			ui.treeINet->setHeaderLabels(QStringList() << tr("Name") << tr("Access"));
			ui.treeINet->setSortingEnabled(true);
			inetLayout->addWidget(ui.treeINet, 1);
			auto* inetActions = new QHBoxLayout();
			ui.btnAddINetProg = new QPushButton(tr("Add Program"), nativeINet);
			ui.btnDelINetProg = new QPushButton(tr("Remove"), nativeINet);
			inetActions->addWidget(ui.btnAddINetProg);
			inetActions->addWidget(ui.btnDelINetProg);
			inetActions->addStretch();
			inetLayout->addLayout(inetActions);
			ui.tabsInternet->removeTab(inetIndex);
			ui.tabsInternet->insertTab(inetIndex, nativeINet, tr("Process Restrictions"));
			ui.tabINet = nativeINet;
			legacyINet->deleteLater();
		}
	}

	// Replace Network Options > Firewall with native M3 controls while
	// preserving embedded rule editors, test fields, and WFP warning behavior.
	if (ui.tabsInternet && ui.tabNetFw) {
		const int fwIndex = ui.tabsInternet->indexOf(ui.tabNetFw);
		if (fwIndex >= 0) {
			auto* legacyFw = ui.tabNetFw;
			auto* nativeFw = new QWidget(ui.tabsInternet);
			auto* fwLayout = new QVBoxLayout(nativeFw);
			ui.lblNoWfp = new QLabel(tr("Caution: Windows Filtering Platform may be unavailable; rules then apply only in user mode and can be bypassed."), nativeFw);
			ui.lblNoWfp->setWordWrap(true);
			ui.lblNoWfp->setProperty("m3NativeSurface", true);
			fwLayout->addWidget(ui.lblNoWfp);
			ui.treeNetFw = new QTreeWidget(nativeFw);
			ui.treeNetFw->setColumnCount(5);
			ui.treeNetFw->setHeaderLabels(QStringList() << tr("Program") << tr("Action") << tr("Port") << tr("IP") << tr("Protocol"));
			ui.treeNetFw->setSortingEnabled(true);
			fwLayout->addWidget(ui.treeNetFw, 1);
			auto* fwActions = new QHBoxLayout();
			ui.btnAddFwRule = new QPushButton(tr("Add Rule"), nativeFw);
			ui.btnDelFwRule = new QPushButton(tr("Remove"), nativeFw);
			ui.chkShowNetFwTmpl = new QCheckBox(tr("Show Templates"), nativeFw);
			fwActions->addWidget(ui.btnAddFwRule);
			fwActions->addWidget(ui.btnDelFwRule);
			fwActions->addWidget(ui.chkShowNetFwTmpl);
			fwActions->addStretch();
			fwLayout->addLayout(fwActions);
			auto* testRow = new QHBoxLayout();
			testRow->addWidget(new QLabel(tr("Test program"), nativeFw));
			ui.txtProgFwTest = new QLineEdit(nativeFw);
			testRow->addWidget(ui.txtProgFwTest);
			testRow->addWidget(new QLabel(tr("Port"), nativeFw));
			ui.txtPortFwTest = new QLineEdit(nativeFw);
			testRow->addWidget(ui.txtPortFwTest);
			testRow->addWidget(new QLabel(tr("IP"), nativeFw));
			ui.txtIPFwTest = new QLineEdit(nativeFw);
			testRow->addWidget(ui.txtIPFwTest);
			testRow->addWidget(new QLabel(tr("Protocol"), nativeFw));
			ui.cmbProtFwTest = new QComboBox(nativeFw);
			testRow->addWidget(ui.cmbProtFwTest);
			ui.btnClearFwTest = new QToolButton(nativeFw);
			ui.btnClearFwTest->setText(tr("Clear"));
			testRow->addWidget(ui.btnClearFwTest);
			fwLayout->addLayout(testRow);
			ui.tabsInternet->removeTab(fwIndex);
			ui.tabsInternet->insertTab(fwIndex, nativeFw, tr("Network Firewall"));
			ui.tabNetFw = nativeFw;
			legacyFw->deleteLater();
		}
	}

	// Replace Network Options > DNS Filter with native M3 controls while
	// preserving per-process domain rules, delegates, and edit/delete behavior.
	if (ui.tabsInternet && ui.tabDNS) {
		const int dnsIndex = ui.tabsInternet->indexOf(ui.tabDNS);
		if (dnsIndex >= 0) {
			auto* legacyDns = ui.tabDNS;
			auto* nativeDns = new QWidget(ui.tabsInternet);
			auto* dnsLayout = new QVBoxLayout(nativeDns);
			auto* dnsHint = new QLabel(tr("Block individual domains per process; leave IP empty to block or enter an IP address to redirect."), nativeDns);
			dnsHint->setWordWrap(true);
			dnsLayout->addWidget(dnsHint);
			ui.treeDns = new QTreeWidget(nativeDns);
			ui.treeDns->setColumnCount(3);
			ui.treeDns->setHeaderLabels(QStringList() << tr("Program") << tr("Domain") << tr("IP"));
			ui.treeDns->setSortingEnabled(true);
			dnsLayout->addWidget(ui.treeDns, 1);
			auto* dnsActions = new QHBoxLayout();
			ui.btnAddDns = new QPushButton(tr("Add Filter"), nativeDns);
			ui.btnDelDns = new QPushButton(tr("Remove"), nativeDns);
			dnsActions->addWidget(ui.btnAddDns);
			dnsActions->addWidget(ui.btnDelDns);
			dnsActions->addStretch();
			dnsLayout->addLayout(dnsActions);
			ui.tabsInternet->removeTab(dnsIndex);
			ui.tabsInternet->insertTab(dnsIndex, nativeDns, tr("DNS Filter"));
			ui.tabDNS = nativeDns;
			legacyDns->deleteLater();
		}
	}

	// Replace Network Options > Internet Proxy with native M3 controls while
	// preserving embedded proxy editors, ordering, testing, and persistence.
	if (ui.tabsInternet && ui.tabNetProxy) {
		const int proxyIndex = ui.tabsInternet->indexOf(ui.tabNetProxy);
		if (proxyIndex >= 0) {
			auto* legacyProxy = ui.tabNetProxy;
			auto* nativeProxy = new QWidget(ui.tabsInternet);
			auto* proxyLayout = new QVBoxLayout(nativeProxy);
			auto* proxyHint = new QLabel(tr("Sandboxed programs can be forced through preset SOCKS5 proxies."), nativeProxy);
			proxyHint->setWordWrap(true);
			proxyLayout->addWidget(proxyHint);
			ui.treeProxy = new QTreeWidget(nativeProxy);
			ui.treeProxy->setColumnCount(7);
			ui.treeProxy->setHeaderLabels(QStringList() << tr("Program") << tr("IP") << tr("Port") << tr("Auth") << tr("Login") << tr("Password") << tr("Bypass IPs"));
			proxyLayout->addWidget(ui.treeProxy, 1);
			auto* proxyActions = new QHBoxLayout();
			ui.btnAddProxy = new QPushButton(tr("Add Proxy"), nativeProxy);
			ui.btnTestProxy = new QPushButton(tr("Test Proxy"), nativeProxy);
			ui.btnMoveProxyUp = new QPushButton(tr("Move Up"), nativeProxy);
			ui.btnMoveProxyDown = new QPushButton(tr("Move Down"), nativeProxy);
			ui.btnDelProxy = new QPushButton(tr("Remove"), nativeProxy);
			proxyActions->addWidget(ui.btnAddProxy);
			proxyActions->addWidget(ui.btnTestProxy);
			proxyActions->addWidget(ui.btnMoveProxyUp);
			proxyActions->addWidget(ui.btnMoveProxyDown);
			proxyActions->addWidget(ui.btnDelProxy);
			proxyActions->addStretch();
			proxyLayout->addLayout(proxyActions);
			ui.chkProxyResolveHostnames = new QCheckBox(tr("Resolve hostnames via proxy"), nativeProxy);
			ui.chkUseProxyThreads = new QCheckBox(tr("Use in-process proxy relay threads for compatibility"), nativeProxy);
			ui.chkProxyResolveHostnames->setVisible(false);
			ui.chkUseProxyThreads->setVisible(false);
			proxyLayout->addWidget(ui.chkProxyResolveHostnames);
			proxyLayout->addWidget(ui.chkUseProxyThreads);
			ui.tabsInternet->removeTab(proxyIndex);
			ui.tabsInternet->insertTab(proxyIndex, nativeProxy, tr("Internet Proxy"));
			ui.tabNetProxy = nativeProxy;
			legacyProxy->deleteLater();
		}
	}

	// Replace Network Options > Other Options with native M3 controls while
	// preserving adapter binding, port restrictions, and IP persistence.
	if (ui.tabsInternet && ui.tabNetConfig) {
		const int netConfigIndex = ui.tabsInternet->indexOf(ui.tabNetConfig);
		if (netConfigIndex >= 0) {
			auto* legacyNetConfig = ui.tabNetConfig;
			auto* nativeNetConfig = new QWidget(ui.tabsInternet);
			auto* netLayout = new QVBoxLayout(nativeNetConfig);
			ui.lblPorts = new QLabel(tr("Port Blocking"), nativeNetConfig);
			ui.lblPorts->setProperty("m3NativeSurface", true);
			ui.lblPorts->setStyleSheet("font-weight: 600;");
			netLayout->addWidget(ui.lblPorts);
			ui.chkBlockSamba = new QCheckBox(tr("Block common SAMBA ports"), nativeNetConfig);
			ui.chkBlockDns = new QCheckBox(tr("Block DNS, UDP port 53"), nativeNetConfig);
			netLayout->addWidget(ui.chkBlockSamba);
			netLayout->addWidget(ui.chkBlockDns);
			ui.lblNetwork = new QLabel(tr("Network restrictions"), nativeNetConfig);
			ui.lblNetwork->setProperty("m3NativeSurface", true);
			ui.lblNetwork->setStyleSheet("font-weight: 600;");
			netLayout->addWidget(ui.lblNetwork);
			ui.chkBlockNetShare = new QCheckBox(tr("Block network files and folders unless specifically opened"), nativeNetConfig);
			ui.chkBlockNetParam = new QCheckBox(tr("Prevent changes to network and firewall parameters (user mode)"), nativeNetConfig);
			netLayout->addWidget(ui.chkBlockNetShare);
			netLayout->addWidget(ui.chkBlockNetParam);
			ui.lblBind = new QLabel(tr("Bind to Adapter IP"), nativeNetConfig);
			ui.lblBind->setProperty("m3NativeSurface", true);
			ui.lblBind->setStyleSheet("font-weight: 600;");
			netLayout->addWidget(ui.lblBind);
			ui.cmbNIC = new QComboBox(nativeNetConfig);
			netLayout->addWidget(ui.cmbNIC);
			auto* ipRow = new QHBoxLayout();
			ipRow->addWidget(new QLabel(tr("IPv4"), nativeNetConfig));
			ui.txtIPv4 = new QLineEdit(nativeNetConfig);
			ui.txtIPv4->setPlaceholderText(tr("000.000.000.000"));
			ipRow->addWidget(ui.txtIPv4);
			ipRow->addWidget(new QLabel(tr("IPv6"), nativeNetConfig));
			ui.txtIPv6 = new QLineEdit(nativeNetConfig);
			ui.txtIPv6->setPlaceholderText(tr("0000:0000:0000:0000:0000:0000:0000:0000"));
			ipRow->addWidget(ui.txtIPv6);
			netLayout->addLayout(ipRow);
			netLayout->addStretch();
			ui.tabsInternet->removeTab(netConfigIndex);
			ui.tabsInternet->insertTab(netConfigIndex, nativeNetConfig, tr("Other Options"));
			ui.tabNetConfig = nativeNetConfig;
			legacyNetConfig->deleteLater();
		}
	}

	ui.tabs->setTabPosition(QTabWidget::West);

	ui.tabs->setCurrentIndex(0);
	ui.tabs->setTabIcon(0, CSandMan::GetIcon("Config"));
	ui.tabs->setTabIcon(1, CSandMan::GetIcon("Security"));
	ui.tabs->setTabIcon(2, CSandMan::GetIcon("Group"));
	ui.tabs->setTabIcon(3, CSandMan::GetIcon("Control"));
	ui.tabs->setTabIcon(4, CSandMan::GetIcon("Stop"));
	ui.tabs->setTabIcon(5, CSandMan::GetIcon("Start"));
	ui.tabs->setTabIcon(6, CSandMan::GetIcon("Ampel"));
	ui.tabs->setTabIcon(7, CSandMan::GetIcon("Network"));
	ui.tabs->setTabIcon(8, CSandMan::GetIcon("Recover"));
	ui.tabs->setTabIcon(9, CSandMan::GetIcon("Settings"));
	ui.tabs->setTabIcon(10, CSandMan::GetIcon("Advanced"));
	ui.tabs->setTabIcon(11, CSandMan::GetIcon("Compatibility"));
	ui.tabs->setTabIcon(12, CSandMan::GetIcon("Editor"));

	ui.tabsGeneral->setCurrentIndex(0);
	ui.tabsGeneral->setTabIcon(0, CSandMan::GetIcon("Box"));
	ui.tabsGeneral->setTabIcon(1, CSandMan::GetIcon("Folder"));
	ui.tabsGeneral->setTabIcon(2, CSandMan::GetIcon("Move"));
	ui.tabsGeneral->setTabIcon(3, CSandMan::GetIcon("NoAccess"));
	ui.tabsGeneral->setTabIcon(4, CSandMan::GetIcon("EFence"));
	ui.tabsGeneral->setTabIcon(5, CSandMan::GetIcon("Run"));

	ui.tabsSecurity->setCurrentIndex(0);
	ui.tabsSecurity->setTabIcon(0, CSandMan::GetIcon("Shield7"));
	ui.tabsSecurity->setTabIcon(1, CSandMan::GetIcon("Fence"));
	ui.tabsSecurity->setTabIcon(2, CSandMan::GetIcon("Shield15"));
	ui.tabsSecurity->setTabIcon(3, CSandMan::GetIcon("Job"));
	ui.tabsSecurity->setTabIcon(4, CSandMan::GetIcon("Shield12"));

	ui.tabsForce->setCurrentIndex(0);
	ui.tabsForce->setTabIcon(0, CSandMan::GetIcon("Force"));
	ui.tabsForce->setTabIcon(1, CSandMan::GetIcon("Breakout"));

	ui.tabsStop->setCurrentIndex(0);
	ui.tabsStop->setTabIcon(0, CSandMan::GetIcon("Fail"));
	ui.tabsStop->setTabIcon(1, CSandMan::GetIcon("Pass"));
	ui.tabsStop->setTabIcon(2, CSandMan::GetIcon("Policy"));
		
	ui.tabsInternet->setCurrentIndex(0);
	ui.tabsInternet->setTabIcon(0, CSandMan::GetIcon("EthSocket2"));
	ui.tabsInternet->setTabIcon(1, CSandMan::GetIcon("Wall"));
	ui.tabsInternet->setTabIcon(2, CSandMan::GetIcon("DNS"));
	ui.tabsInternet->setTabIcon(3, CSandMan::GetIcon("Proxy"));
	ui.tabsInternet->setTabIcon(4, CSandMan::GetIcon("Network3"));

	ui.tabsAccess->setCurrentIndex(0);
	ui.tabsAccess->setTabIcon(0, CSandMan::GetIcon("Folder"));
	ui.tabsAccess->setTabIcon(1, CSandMan::GetIcon("RegEdit"));
	ui.tabsAccess->setTabIcon(2, CSandMan::GetIcon("Port"));
	ui.tabsAccess->setTabIcon(3, CSandMan::GetIcon("Window"));
	ui.tabsAccess->setTabIcon(4, CSandMan::GetIcon("Objects"));
	//ui.tabsAccess->setTabIcon(0, CSandMan::GetIcon("Rules"));
	ui.tabsAccess->setTabIcon(5, CSandMan::GetIcon("Policy"));

	ui.tabsRecovery->setCurrentIndex(0);
	ui.tabsRecovery->setTabIcon(0, CSandMan::GetIcon("QuickRecovery"));
	ui.tabsRecovery->setTabIcon(1, CSandMan::GetIcon("ImmidiateRecovery"));

	// Native M3 controls for Immediate Recovery: keep the generated layout and
	// object names so persistence, signal wiring, and the later recovery-tab
	// merge continue to use the same pointers without a Designer chrome tax.
	{
		auto replaceRecoveryControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto m3Check = [this](const QString& objectName, const QString& text) {
			auto* control = new QCheckBox(text, this);
			control->setObjectName(objectName);
			control->setProperty("m3NativeSurface", true);
			return control;
		};
		auto m3Button = [this](const QString& objectName, const QString& text) {
			auto* control = new QPushButton(text, this);
			control->setObjectName(objectName);
			control->setProperty("m3NativeSurface", true);
			return control;
		};
		auto m3Tree = [this](const QString& objectName) {
			auto* control = new QTreeWidget(this);
			control->setObjectName(objectName);
			control->setSortingEnabled(true);
			control->setHeaderLabels({ tr("Name") });
			control->setProperty("m3NativeSurface", true);
			return control;
		};

		auto* nativeAutoRecovery = m3Check(
			QStringLiteral("chkAutoRecovery"),
			tr("Enable Immediate Recovery prompt to be able to recover files as soon as they are created."));
		auto* nativeRecIgnore = m3Tree(QStringLiteral("treeRecIgnore"));
		auto* nativeAddIgnore = m3Button(QStringLiteral("btnAddRecIgnore"), tr("Ignore Folder"));
		auto* nativeAddIgnoreExt = m3Button(QStringLiteral("btnAddRecIgnoreExt"), tr("Ignore Extension"));
		auto* nativeDelIgnore = m3Button(QStringLiteral("btnDelRecIgnore"), tr("Remove"));
		replaceRecoveryControl(ui.chkAutoRecovery, nativeAutoRecovery, ui.gridLayout_40);
		replaceRecoveryControl(ui.treeRecIgnore, nativeRecIgnore, ui.gridLayout_40);
		replaceRecoveryControl(ui.btnAddRecIgnore, nativeAddIgnore, ui.gridLayout_40);
		replaceRecoveryControl(ui.btnAddRecIgnoreExt, nativeAddIgnoreExt, ui.gridLayout_40);
		replaceRecoveryControl(ui.btnDelRecIgnore, nativeDelIgnore, ui.gridLayout_40);
		ui.chkAutoRecovery = nativeAutoRecovery;
		ui.treeRecIgnore = nativeRecIgnore;
		ui.btnAddRecIgnore = nativeAddIgnore;
		ui.btnAddRecIgnoreExt = nativeAddIgnoreExt;
		ui.btnDelRecIgnore = nativeDelIgnore;
	}

	// Quick Recovery uses the same safe in-place migration: native controls
	// replace Designer widgets while the generated grid remains the merge point.
	{
		auto replaceQuickRecoveryControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto* nativeRecoveryTree = new QTreeWidget(this);
		nativeRecoveryTree->setObjectName(QStringLiteral("treeRecovery"));
		nativeRecoveryTree->setSortingEnabled(true);
		nativeRecoveryTree->setHeaderLabels({ tr("Name") });
		nativeRecoveryTree->setProperty("m3NativeSurface", true);
		auto* nativeShowTemplates = new QCheckBox(tr("Show Templates"), this);
		nativeShowTemplates->setObjectName(QStringLiteral("chkShowRecoveryTmpl"));
		nativeShowTemplates->setProperty("m3NativeSurface", true);
		auto* nativeAddRecovery = new QPushButton(tr("Add Folder"), this);
		nativeAddRecovery->setObjectName(QStringLiteral("btnAddRecovery"));
		nativeAddRecovery->setProperty("m3NativeSurface", true);
		auto* nativeDelRecovery = new QPushButton(tr("Remove"), this);
		nativeDelRecovery->setObjectName(QStringLiteral("btnDelRecovery"));
		nativeDelRecovery->setProperty("m3NativeSurface", true);
		replaceQuickRecoveryControl(ui.treeRecovery, nativeRecoveryTree, ui.gridLayout_22);
		replaceQuickRecoveryControl(ui.chkShowRecoveryTmpl, nativeShowTemplates, ui.gridLayout_22);
		replaceQuickRecoveryControl(ui.btnAddRecovery, nativeAddRecovery, ui.gridLayout_22);
		replaceQuickRecoveryControl(ui.btnDelRecovery, nativeDelRecovery, ui.gridLayout_22);
		ui.treeRecovery = nativeRecoveryTree;
		ui.chkShowRecoveryTmpl = nativeShowTemplates;
		ui.btnAddRecovery = nativeAddRecovery;
		ui.btnDelRecovery = nativeDelRecovery;
	}

	// Native M3 controls for Dlls & Extensions keep the DLL model and host-image
	// protection handlers intact while replacing the last Designer controls in
	// this child tab.
	{
		auto replaceDllControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto* nativeDllTree = new QTreeWidget(this);
		nativeDllTree->setObjectName(QStringLiteral("treeInjectDll"));
		nativeDllTree->setColumnCount(2);
		nativeDllTree->setHeaderLabels(QStringList() << tr("Name") << tr("Description"));
		nativeDllTree->setSortingEnabled(true);
		nativeDllTree->setProperty("m3NativeSurface", true);
		auto* nativeHostProtect = new QCheckBox(
			tr("Prevent sandboxed programs installed on the host from loading DLLs from the sandbox"), this);
		nativeHostProtect->setObjectName(QStringLiteral("chkHostProtect"));
		nativeHostProtect->setToolTip(ui.chkHostProtect->toolTip());
		nativeHostProtect->setProperty("m3NativeSurface", true);
		auto* nativeHostProtectMsg = new QCheckBox(
			tr("Issue message 1305 when a program tries to load a sandboxed dll"), this);
		nativeHostProtectMsg->setObjectName(QStringLiteral("chkHostProtectMsg"));
		nativeHostProtectMsg->setProperty("m3NativeSurface", true);
		replaceDllControl(ui.treeInjectDll, nativeDllTree, ui.gridLayout_49);
		replaceDllControl(ui.chkHostProtect, nativeHostProtect, ui.gridLayout_49);
		replaceDllControl(ui.chkHostProtectMsg, nativeHostProtectMsg, ui.gridLayout_49);
		ui.treeInjectDll = nativeDllTree;
		ui.chkHostProtect = nativeHostProtect;
		ui.chkHostProtectMsg = nativeHostProtectMsg;
	}

	// Advanced Options > Miscellaneous now owns native M3 controls for its
	// per-process option model, while retaining the generated grid as a
	// temporary seam for the existing event and template logic.
	{
		auto replaceOptionControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto* nativeOptionsTree = new QTreeWidget(this);
		nativeOptionsTree->setObjectName(QStringLiteral("treeOptions"));
		nativeOptionsTree->setColumnCount(3);
		nativeOptionsTree->setHeaderLabels(QStringList() << tr("Option") << tr("Program") << tr("Value"));
		nativeOptionsTree->setSortingEnabled(true);
		nativeOptionsTree->setProperty("m3NativeSurface", true);
		auto* nativeAddOption = new QToolButton(this);
		nativeAddOption->setObjectName(QStringLiteral("btnAddOption"));
		nativeAddOption->setText(tr("Add Option"));
		nativeAddOption->setProperty("m3NativeSurface", true);
		auto* nativeDelOption = new QPushButton(tr("Remove"), this);
		nativeDelOption->setObjectName(QStringLiteral("btnDelOption"));
		nativeDelOption->setProperty("m3NativeSurface", true);
		auto* nativeShowOptions = new QCheckBox(tr("Show Templates"), this);
		nativeShowOptions->setObjectName(QStringLiteral("chkShowOptionsTmpl"));
		nativeShowOptions->setProperty("m3NativeSurface", true);
		replaceOptionControl(ui.treeOptions, nativeOptionsTree, ui.gridLayout_60);
		replaceOptionControl(ui.btnAddOption, nativeAddOption, ui.gridLayout_60);
		replaceOptionControl(ui.btnDelOption, nativeDelOption, ui.gridLayout_60);
		replaceOptionControl(ui.chkShowOptionsTmpl, nativeShowOptions, ui.gridLayout_60);
		ui.treeOptions = nativeOptionsTree;
		ui.btnAddOption = nativeAddOption;
		ui.btnDelOption = nativeDelOption;
		ui.chkShowOptionsTmpl = nativeShowOptions;
	}

	// Advanced Options > Users uses native M3 list and actions while retaining
	// the account-selection and monitor persistence handlers.
	{
		auto replaceUserControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto* nativeUsers = new QListWidget(this);
		nativeUsers->setObjectName(QStringLiteral("lstUsers"));
		nativeUsers->setProperty("m3NativeSurface", true);
		auto* nativeAddUser = new QPushButton(tr("Add User"), this);
		nativeAddUser->setObjectName(QStringLiteral("btnAddUser"));
		nativeAddUser->setProperty("m3NativeSurface", true);
		auto* nativeDelUser = new QPushButton(tr("Remove"), this);
		nativeDelUser->setObjectName(QStringLiteral("btnDelUser"));
		nativeDelUser->setProperty("m3NativeSurface", true);
		auto* nativeMonitorAdmin = new QCheckBox(tr("Restrict Resource Access monitor to administrators only"), this);
		nativeMonitorAdmin->setObjectName(QStringLiteral("chkMonitorAdminOnly"));
		nativeMonitorAdmin->setProperty("m3NativeSurface", true);
		replaceUserControl(ui.lstUsers, nativeUsers, ui.gridLayout_25);
		replaceUserControl(ui.btnAddUser, nativeAddUser, ui.gridLayout_25);
		replaceUserControl(ui.btnDelUser, nativeDelUser, ui.gridLayout_25);
		replaceUserControl(ui.chkMonitorAdminOnly, nativeMonitorAdmin, ui.gridLayout_25);
		ui.lstUsers = nativeUsers;
		ui.btnAddUser = nativeAddUser;
		ui.btnDelUser = nativeDelUser;
		ui.chkMonitorAdminOnly = nativeMonitorAdmin;
	}

	// Advanced Options > Tracing migrates its complete checkbox cluster in one
	// pass, copying the generated text/tooltips so every trace switch keeps its
	// wording while the controls become native M3 surfaces.
	{
		auto migrateTraceCheck = [&](QCheckBox*& control) {
			auto* native = new QCheckBox(control->text(), this);
			native->setObjectName(control->objectName());
			native->setToolTip(control->toolTip());
			native->setProperty("m3NativeSurface", true);
			ui.gridLayout_32->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		migrateTraceCheck(ui.chkDisableMonitor);
		migrateTraceCheck(ui.chkCallTrace);
		migrateTraceCheck(ui.chkFileTrace);
		migrateTraceCheck(ui.chkPipeTrace);
		migrateTraceCheck(ui.chkKeyTrace);
		migrateTraceCheck(ui.chkIpcTrace);
		migrateTraceCheck(ui.chkGuiTrace);
		migrateTraceCheck(ui.chkComTrace);
		migrateTraceCheck(ui.chkNetFwTrace);
		migrateTraceCheck(ui.chkDnsTrace);
		migrateTraceCheck(ui.chkApiTrace);
		migrateTraceCheck(ui.chkHookTrace);
		migrateTraceCheck(ui.chkDbgTrace);
		migrateTraceCheck(ui.chkErrTrace);
	}

	// Config Dump keeps its dynamically supplied tree, but its filter switches
	// and refresh action are native M3 controls so the dump surface no longer
	// depends on Designer chrome for its user-facing controls.
	{
		auto replaceCfgControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto makeCfgCheck = [this](QCheckBox* oldControl) {
			auto* native = new QCheckBox(oldControl->text(), this);
			native->setObjectName(oldControl->objectName());
			native->setChecked(oldControl->isChecked());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto* nativeCfgNoGlobal = makeCfgCheck(ui.chkCfgNoGlobal);
		auto* nativeCfgNoTemplates = makeCfgCheck(ui.chkCfgNoTemplates);
		auto* nativeCfgNoExpand = makeCfgCheck(ui.chkCfgNoExpand);
		auto* nativeCfgUpdate = new QPushButton(ui.btnCfgUpdate->text(), this);
		nativeCfgUpdate->setObjectName(QStringLiteral("btnCfgUpdate"));
		nativeCfgUpdate->setProperty("m3NativeSurface", true);
		replaceCfgControl(ui.chkCfgNoGlobal, nativeCfgNoGlobal, ui.gridLayout_88);
		replaceCfgControl(ui.chkCfgNoTemplates, nativeCfgNoTemplates, ui.gridLayout_88);
		replaceCfgControl(ui.chkCfgNoExpand, nativeCfgNoExpand, ui.gridLayout_88);
		replaceCfgControl(ui.btnCfgUpdate, nativeCfgUpdate, ui.gridLayout_88);
		ui.chkCfgNoGlobal = nativeCfgNoGlobal;
		ui.chkCfgNoTemplates = nativeCfgNoTemplates;
		ui.chkCfgNoExpand = nativeCfgNoExpand;
		ui.btnCfgUpdate = nativeCfgUpdate;
	}

	// App Templates > Templates now uses native M3 filters, tree, and actions;
	// the existing popup menu and template model continue to bind to these
	// object names after replacement.
	{
		auto replaceTemplateControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto* nativeCategories = new QComboBox(this);
		nativeCategories->setObjectName(QStringLiteral("cmbCategories"));
		nativeCategories->setProperty("m3NativeSurface", true);
		for (int i = 0; i < ui.cmbCategories->count(); ++i)
			nativeCategories->addItem(ui.cmbCategories->itemText(i), ui.cmbCategories->itemData(i));
		nativeCategories->setCurrentIndex(ui.cmbCategories->currentIndex());
		auto* nativeTemplateFilter = new QLineEdit(this);
		nativeTemplateFilter->setObjectName(QStringLiteral("txtTemplates"));
		nativeTemplateFilter->setText(ui.txtTemplates->text());
		nativeTemplateFilter->setProperty("m3NativeSurface", true);
		auto* nativeTemplateTree = new QTreeWidget(this);
		nativeTemplateTree->setObjectName(QStringLiteral("treeTemplates"));
		nativeTemplateTree->setColumnCount(2);
		nativeTemplateTree->setHeaderLabels(QStringList() << tr("Category") << tr("Name"));
		nativeTemplateTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
		nativeTemplateTree->setSortingEnabled(true);
		nativeTemplateTree->setProperty("m3NativeSurface", true);
		auto makeTemplateButton = [this](QToolButton* oldControl) {
			auto* native = new QToolButton(this);
			native->setObjectName(oldControl->objectName());
			native->setText(oldControl->text());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto* nativeAddTemplate = makeTemplateButton(ui.btnAddTemplate);
		auto* nativeOpenTemplate = makeTemplateButton(ui.btnOpenTemplate);
		auto* nativeDelTemplate = makeTemplateButton(ui.btnDelTemplate);
		replaceTemplateControl(ui.cmbCategories, nativeCategories, ui.gridLayout_3);
		replaceTemplateControl(ui.txtTemplates, nativeTemplateFilter, ui.gridLayout_3);
		replaceTemplateControl(ui.treeTemplates, nativeTemplateTree, ui.gridLayout_3);
		replaceTemplateControl(ui.btnAddTemplate, nativeAddTemplate, ui.gridLayout_3);
		replaceTemplateControl(ui.btnOpenTemplate, nativeOpenTemplate, ui.gridLayout_3);
		replaceTemplateControl(ui.btnDelTemplate, nativeDelTemplate, ui.gridLayout_3);
		ui.cmbCategories = nativeCategories;
		ui.txtTemplates = nativeTemplateFilter;
		ui.treeTemplates = nativeTemplateTree;
		ui.btnAddTemplate = nativeAddTemplate;
		ui.btnOpenTemplate = nativeOpenTemplate;
		ui.btnDelTemplate = nativeDelTemplate;
	}

	// Template Folders keeps its editable path delegates but receives a native
	// M3 tree host, preserving the item-widget contract used by the folder model.
	{
		auto* nativeFolderTree = new QTreeWidget(this);
		nativeFolderTree->setObjectName(QStringLiteral("treeFolders"));
		nativeFolderTree->setColumnCount(2);
		nativeFolderTree->setHeaderLabels(QStringList() << tr("Name") << tr("Value"));
		nativeFolderTree->setSortingEnabled(true);
		nativeFolderTree->setProperty("m3NativeSurface", true);
		ui.gridLayout_42->replaceWidget(ui.treeFolders, nativeFolderTree);
		ui.treeFolders->deleteLater();
		ui.treeFolders = nativeFolderTree;
	}

	// App Templates > Accessibility has one persisted screen-reader switch;
	// replace it with a native M3 control while retaining the template handler.
	{
		auto* nativeScreenReaders = new QCheckBox(ui.chkScreenReaders->text(), this);
		nativeScreenReaders->setObjectName(QStringLiteral("chkScreenReaders"));
		nativeScreenReaders->setChecked(ui.chkScreenReaders->isChecked());
		nativeScreenReaders->setToolTip(ui.chkScreenReaders->toolTip());
		nativeScreenReaders->setProperty("m3NativeSurface", true);
		ui.gridLayout_43->replaceWidget(ui.chkScreenReaders, nativeScreenReaders);
		ui.chkScreenReaders->deleteLater();
		ui.chkScreenReaders = nativeScreenReaders;
	}

	// Edit ini Section controls are native M3 while the existing code-editor
	// upgrade and save/cancel workflow continue to replace/use these pointers.
	{
		auto replaceIniControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto makeIniCheck = [this](QCheckBox* oldControl) {
			auto* native = new QCheckBox(oldControl->text(), this);
			native->setObjectName(oldControl->objectName());
			native->setCheckState(oldControl->checkState());
			native->setToolTip(oldControl->toolTip());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto makeIniButton = [this](QPushButton* oldControl) {
			auto* native = new QPushButton(oldControl->text(), this);
			native->setObjectName(oldControl->objectName());
			native->setEnabled(oldControl->isEnabled());
			native->setToolTip(oldControl->toolTip());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto* nativeIniTooltips = makeIniCheck(ui.chkEnableTooltips);
		auto* nativeIniComplete = makeIniCheck(ui.chkEnableAutoCompletion);
		auto* nativeIniValidate = makeIniCheck(ui.chkValidateIniKeys);
		auto* nativeEditIni = makeIniButton(ui.btnEditIni);
		auto* nativeSaveIni = makeIniButton(ui.btnSaveIni);
		auto* nativeCancelIni = makeIniButton(ui.btnCancelEdit);
		auto* nativeEditorSettings = makeIniButton(ui.btnEditorSettings);
		auto* nativeIniText = new QPlainTextEdit(this);
		nativeIniText->setObjectName(QStringLiteral("txtIniSection"));
		nativeIniText->setLineWrapMode(QPlainTextEdit::NoWrap);
		nativeIniText->setPlainText(ui.txtIniSection->toPlainText());
		nativeIniText->setProperty("m3NativeSurface", true);
		replaceIniControl(ui.chkEnableTooltips, nativeIniTooltips, ui.gridLayout);
		replaceIniControl(ui.chkEnableAutoCompletion, nativeIniComplete, ui.gridLayout);
		replaceIniControl(ui.chkValidateIniKeys, nativeIniValidate, ui.gridLayout);
		replaceIniControl(ui.btnEditIni, nativeEditIni, ui.gridLayout);
		replaceIniControl(ui.btnSaveIni, nativeSaveIni, ui.gridLayout);
		replaceIniControl(ui.btnCancelEdit, nativeCancelIni, ui.gridLayout);
		replaceIniControl(ui.btnEditorSettings, nativeEditorSettings, ui.gridLayout);
		replaceIniControl(ui.txtIniSection, nativeIniText, ui.gridLayout);
		ui.chkEnableTooltips = nativeIniTooltips;
		ui.chkEnableAutoCompletion = nativeIniComplete;
		ui.chkValidateIniKeys = nativeIniValidate;
		ui.btnEditIni = nativeEditIni;
		ui.btnSaveIni = nativeSaveIni;
		ui.btnCancelEdit = nativeCancelIni;
		ui.btnEditorSettings = nativeEditorSettings;
		ui.txtIniSection = nativeIniText;
	}

	// Advanced Options > Processes now uses native M3 process-hiding controls
	// while keeping the existing process model, templates, and persistence.
	{
		auto replaceProcessControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto makeProcessCheck = [this](QCheckBox* oldControl) {
			auto* native = new QCheckBox(oldControl->text(), this);
			native->setObjectName(oldControl->objectName());
			native->setCheckState(oldControl->checkState());
			native->setToolTip(oldControl->toolTip());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto makeProcessButton = [this](QPushButton* oldControl) {
			auto* native = new QPushButton(oldControl->text(), this);
			native->setObjectName(oldControl->objectName());
			native->setEnabled(oldControl->isEnabled());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto* nativeHideOther = makeProcessCheck(ui.chkHideOtherBoxes);
		auto* nativeHideNonSystem = makeProcessCheck(ui.chkHideNonSystemProcesses);
		auto* nativeShowTemplates = makeProcessCheck(ui.chkShowHiddenProcTmpl);
		auto* nativeBlockWmi = makeProcessCheck(ui.chkBlockWMI);
		auto* nativeHideHost = makeProcessCheck(ui.chkHideHostApps);
		auto* nativeProcessTree = new QTreeWidget(this);
		nativeProcessTree->setObjectName(QStringLiteral("treeHideProc"));
		nativeProcessTree->setColumnCount(2);
		nativeProcessTree->setHeaderLabels(QStringList() << tr("Process") << QString());
		nativeProcessTree->setSortingEnabled(true);
		nativeProcessTree->setMinimumSize(ui.treeHideProc->minimumSize());
		nativeProcessTree->setProperty("m3NativeSurface", true);
		auto* nativeAddProcess = makeProcessButton(ui.btnAddProcess);
		auto* nativeDelProcess = makeProcessButton(ui.btnDelProcess);
		replaceProcessControl(ui.chkHideOtherBoxes, nativeHideOther, ui.gridLayout_86);
		replaceProcessControl(ui.chkHideNonSystemProcesses, nativeHideNonSystem, ui.gridLayout_86);
		replaceProcessControl(ui.chkShowHiddenProcTmpl, nativeShowTemplates, ui.gridLayout_86);
		replaceProcessControl(ui.chkBlockWMI, nativeBlockWmi, ui.gridLayout_86);
		replaceProcessControl(ui.chkHideHostApps, nativeHideHost, ui.gridLayout_86);
		replaceProcessControl(ui.treeHideProc, nativeProcessTree, ui.gridLayout_86);
		replaceProcessControl(ui.btnAddProcess, nativeAddProcess, ui.gridLayout_86);
		replaceProcessControl(ui.btnDelProcess, nativeDelProcess, ui.gridLayout_86);
		ui.chkHideOtherBoxes = nativeHideOther;
		ui.chkHideNonSystemProcesses = nativeHideNonSystem;
		ui.chkShowHiddenProcTmpl = nativeShowTemplates;
		ui.chkBlockWMI = nativeBlockWmi;
		ui.chkHideHostApps = nativeHideHost;
		ui.treeHideProc = nativeProcessTree;
		ui.btnAddProcess = nativeAddProcess;
		ui.btnDelProcess = nativeDelProcess;
	}

	// Advanced Options > Privacy now builds its data-protection controls as
	// native M3 widgets while retaining locale selection and firmware-dump flow.
	{
		auto replacePrivacyControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto makePrivacyCheck = [this](QCheckBox* oldControl) {
			auto* native = new QCheckBox(oldControl->text(), this);
			native->setObjectName(oldControl->objectName());
			native->setCheckState(oldControl->checkState());
			native->setToolTip(oldControl->toolTip());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto* nativeHideFirmware = makePrivacyCheck(ui.chkHideFirmware);
		auto* nativeHideSerial = makePrivacyCheck(ui.chkHideSerial);
		auto* nativeHideUid = makePrivacyCheck(ui.chkHideUID);
		auto* nativeHideMac = makePrivacyCheck(ui.chkHideMac);
		auto* nativeLang = new QComboBox(this);
		nativeLang->setObjectName(QStringLiteral("cmbLangID"));
		nativeLang->setProperty("m3NativeSurface", true);
		for (int i = 0; i < ui.cmbLangID->count(); ++i)
			nativeLang->addItem(ui.cmbLangID->itemText(i), ui.cmbLangID->itemData(i));
		nativeLang->setCurrentIndex(ui.cmbLangID->currentIndex());
		auto* nativeDump = new QToolButton(this);
		nativeDump->setObjectName(QStringLiteral("btnDumpFW"));
		nativeDump->setText(ui.btnDumpFW->text());
		nativeDump->setToolTip(ui.btnDumpFW->toolTip());
		nativeDump->setProperty("m3NativeSurface", true);
		replacePrivacyControl(ui.chkHideFirmware, nativeHideFirmware, ui.gridLayout_29);
		replacePrivacyControl(ui.chkHideSerial, nativeHideSerial, ui.gridLayout_29);
		replacePrivacyControl(ui.chkHideUID, nativeHideUid, ui.gridLayout_29);
		replacePrivacyControl(ui.chkHideMac, nativeHideMac, ui.gridLayout_29);
		replacePrivacyControl(ui.cmbLangID, nativeLang, ui.gridLayout_29);
		replacePrivacyControl(ui.btnDumpFW, nativeDump, ui.gridLayout_29);
		ui.chkHideFirmware = nativeHideFirmware;
		ui.chkHideSerial = nativeHideSerial;
		ui.chkHideUID = nativeHideUid;
		ui.chkHideMac = nativeHideMac;
		ui.cmbLangID = nativeLang;
		ui.btnDumpFW = nativeDump;
	}

	// Various Options > Compatibility now owns native M3 switches for each
	// compatibility workaround while retaining existing persistence handlers.
	{
		auto migrateCompatCheck = [&](QCheckBox*& control) {
			auto* native = new QCheckBox(control->text(), this);
			native->setObjectName(control->objectName());
			native->setCheckState(control->checkState());
			native->setToolTip(control->toolTip());
			native->setProperty("m3NativeSurface", true);
		ui.gridLayout_62->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		migrateCompatCheck(ui.chkPreferExternalManifest);
		migrateCompatCheck(ui.chkRestartOnPCA);
		migrateCompatCheck(ui.chkUseSbieWndStation);
		migrateCompatCheck(ui.chkNoPanic);
		migrateCompatCheck(ui.chkForceRestart);
		migrateCompatCheck(ui.chkUseSbieDeskHack);
		migrateCompatCheck(ui.chkElevateCreateProcessFix);
		migrateCompatCheck(ui.chkComTimeout);
		migrateCompatCheck(ui.chkUseElectronDetection);
	}

	// Advanced Options > Triggers now uses a native M3 event tree and action
	// buttons while preserving popup menus, template filtering, and handlers.
	{
		auto replaceTriggerControl = [](QWidget* oldControl, QWidget* nativeControl, QGridLayout* layout) {
			if (oldControl && nativeControl && layout) {
				layout->replaceWidget(oldControl, nativeControl);
				oldControl->deleteLater();
			}
		};
		auto* nativeTriggerTree = new QTreeWidget(this);
		nativeTriggerTree->setObjectName(QStringLiteral("treeTriggers"));
		nativeTriggerTree->setColumnCount(3);
		nativeTriggerTree->setHeaderLabels(QStringList() << tr("Event") << tr("Action") << QString());
		nativeTriggerTree->setSortingEnabled(true);
		nativeTriggerTree->setProperty("m3NativeSurface", true);
		auto makeTriggerButton = [this](QToolButton* oldControl) {
			auto* native = new QToolButton(this);
			native->setObjectName(oldControl->objectName());
			native->setText(oldControl->text());
			native->setToolTip(oldControl->toolTip());
			native->setProperty("m3NativeSurface", true);
			return native;
		};
		auto* nativeAddRun = makeTriggerButton(ui.btnAddAutoRun);
		auto* nativeAddExec = makeTriggerButton(ui.btnAddAutoExec);
		auto* nativeAddSvc = makeTriggerButton(ui.btnAddAutoSvc);
		auto* nativeAddRecovery = makeTriggerButton(ui.btnAddRecoveryCmd);
		auto* nativeAddDelete = makeTriggerButton(ui.btnAddDeleteCmd);
		auto* nativeAddTerminate = makeTriggerButton(ui.btnAddTerminateCmd);
		auto* nativeDelAuto = makeTriggerButton(ui.btnDelAuto);
		auto* nativeShowTemplates = new QCheckBox(ui.chkShowTriggersTmpl->text(), this);
		nativeShowTemplates->setObjectName(QStringLiteral("chkShowTriggersTmpl"));
		nativeShowTemplates->setCheckState(ui.chkShowTriggersTmpl->checkState());
		nativeShowTemplates->setProperty("m3NativeSurface", true);
		replaceTriggerControl(ui.treeTriggers, nativeTriggerTree, ui.gridLayout_4);
		replaceTriggerControl(ui.btnAddAutoRun, nativeAddRun, ui.gridLayout_4);
		replaceTriggerControl(ui.btnAddAutoExec, nativeAddExec, ui.gridLayout_4);
		replaceTriggerControl(ui.btnAddAutoSvc, nativeAddSvc, ui.gridLayout_4);
		replaceTriggerControl(ui.btnAddRecoveryCmd, nativeAddRecovery, ui.gridLayout_4);
		replaceTriggerControl(ui.btnAddDeleteCmd, nativeAddDelete, ui.gridLayout_4);
		replaceTriggerControl(ui.btnAddTerminateCmd, nativeAddTerminate, ui.gridLayout_4);
		replaceTriggerControl(ui.btnDelAuto, nativeDelAuto, ui.gridLayout_4);
		replaceTriggerControl(ui.chkShowTriggersTmpl, nativeShowTemplates, ui.gridLayout_4);
		ui.treeTriggers = nativeTriggerTree;
		ui.btnAddAutoRun = nativeAddRun;
		ui.btnAddAutoExec = nativeAddExec;
		ui.btnAddAutoSvc = nativeAddSvc;
		ui.btnAddRecoveryCmd = nativeAddRecovery;
		ui.btnAddDeleteCmd = nativeAddDelete;
		ui.btnAddTerminateCmd = nativeAddTerminate;
		ui.btnDelAuto = nativeDelAuto;
		ui.chkShowTriggersTmpl = nativeShowTemplates;
	}

	// Debug Options keeps its dynamically generated checkbox grid but replaces
	// the Designer scroll host with a native M3 surface, preserving dbgLayout.
	{
		auto* nativeDebugScroll = new QScrollArea(this);
		nativeDebugScroll->setObjectName(QStringLiteral("scrollArea"));
		nativeDebugScroll->setWidgetResizable(true);
		nativeDebugScroll->setAccessibleName(tr("Debug options"));
		nativeDebugScroll->setProperty("m3NativeSurface", true);
		nativeDebugScroll->setWidget(ui.dbgWidget);
		ui.gridLayout_87->replaceWidget(ui.scrollArea, nativeDebugScroll);
		ui.scrollArea->deleteLater();
		ui.scrollArea = nativeDebugScroll;
	}

	// Replace the Designer Debug tab host while retaining both existing pages
	// and their dynamic controls/signals.
	{
		auto* nativeDebugTabs = new QTabWidget(this);
		nativeDebugTabs->setObjectName(QStringLiteral("tabsDebug"));
		// Keep the tab strip in the standard readable M3 position; the old
		// bottom strip was easy to miss and put the focus order below content.
		nativeDebugTabs->setTabPosition(QTabWidget::North);
		nativeDebugTabs->setAccessibleName(tr("Debug settings pages"));
		nativeDebugTabs->addTab(ui.tabDebugConfig, tr("Debug Options"));
		nativeDebugTabs->addTab(ui.tabConfigDump, tr("Config Dump"));
		nativeDebugTabs->setCurrentIndex(ui.tabsDebug->currentIndex());
		nativeDebugTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_31->replaceWidget(ui.tabsDebug, nativeDebugTabs);
		ui.tabsDebug->deleteLater();
		ui.tabsDebug = nativeDebugTabs;
	}

	// App Templates now uses a native M3 tab host for Templates, Template
	// Folders, and Accessibility pages while preserving each page's controls.
	{
		auto* nativeTemplateTabs = new QTabWidget(this);
		nativeTemplateTabs->setObjectName(QStringLiteral("tabsTemplates"));
		nativeTemplateTabs->setTabPosition(QTabWidget::West);
		nativeTemplateTabs->setAccessibleName(tr("Application template settings pages"));
		nativeTemplateTabs->addTab(ui.tab_11, tr("Templates"));
		nativeTemplateTabs->addTab(ui.tab_12, tr("Template Folders"));
		nativeTemplateTabs->addTab(ui.tab_13, tr("Accessibility"));
		nativeTemplateTabs->setCurrentIndex(ui.tabsTemplates->currentIndex());
		nativeTemplateTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_5->replaceWidget(ui.tabsTemplates, nativeTemplateTabs);
		ui.tabsTemplates->deleteLater();
		ui.tabsTemplates = nativeTemplateTabs;
	}

	// Various Options now uses a native M3 tab host for Compatibility and
	// Dlls & Extensions while preserving both migrated pages and their state.
	{
		auto* nativeOtherTabs = new QTabWidget(this);
		nativeOtherTabs->setObjectName(QStringLiteral("tabsOther"));
		nativeOtherTabs->setTabPosition(QTabWidget::West);
		nativeOtherTabs->setAccessibleName(tr("Various option pages"));
		nativeOtherTabs->addTab(ui.tabCompat, tr("Compatibility"));
		nativeOtherTabs->addTab(ui.tabDlls, tr("Dlls && Extensions"));
		nativeOtherTabs->setCurrentIndex(ui.tabsOther->currentIndex());
		nativeOtherTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_12->replaceWidget(ui.tabsOther, nativeOtherTabs);
		ui.tabsOther->deleteLater();
		ui.tabsOther = nativeOtherTabs;
	}

	// Advanced Options now uses a native M3 tab host for all advanced pages;
	// dynamic Debug removal and each migrated child continue to target the same
	// object name and page order.
	{
		auto* nativeAdvancedTabs = new QTabWidget(this);
		nativeAdvancedTabs->setObjectName(QStringLiteral("tabsAdvanced"));
		nativeAdvancedTabs->setTabPosition(QTabWidget::West);
		nativeAdvancedTabs->setAccessibleName(tr("Advanced option pages"));
		nativeAdvancedTabs->addTab(ui.tabMisc, tr("Miscellaneous"));
		nativeAdvancedTabs->addTab(ui.tabTriggers, tr("Triggers"));
		nativeAdvancedTabs->addTab(ui.tabProcesses, tr("Processes"));
		nativeAdvancedTabs->addTab(ui.tabPrivacy, tr("Privacy"));
		nativeAdvancedTabs->addTab(ui.tabUsers, tr("Users"));
		nativeAdvancedTabs->addTab(ui.tabTracing, tr("Tracing"));
		nativeAdvancedTabs->addTab(ui.tabDebug, tr("Debug"));
		nativeAdvancedTabs->setCurrentIndex(ui.tabsAdvanced->currentIndex());
		nativeAdvancedTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_121->replaceWidget(ui.tabsAdvanced, nativeAdvancedTabs);
		ui.tabsAdvanced->deleteLater();
		ui.tabsAdvanced = nativeAdvancedTabs;
	}

	// General Options now uses a native M3 tab host for Box, File, Migration,
	// Restrictions, Isolation, and Run pages while preserving page order/state.
	{
		auto* nativeGeneralTabs = new QTabWidget(this);
		nativeGeneralTabs->setObjectName(QStringLiteral("tabsGeneral"));
		nativeGeneralTabs->setTabPosition(QTabWidget::West);
		nativeGeneralTabs->setAccessibleName(tr("General option pages"));
		nativeGeneralTabs->addTab(ui.tabOptions, tr("Box Options"));
		nativeGeneralTabs->addTab(ui.tabFile, tr("File Options"));
		nativeGeneralTabs->addTab(ui.tabMigration, tr("File Migration"));
		nativeGeneralTabs->addTab(ui.tabRestrictions, tr("Restrictions"));
		nativeGeneralTabs->addTab(ui.tabOtherRestrictions, tr("Isolation"));
		nativeGeneralTabs->addTab(ui.tabRun, tr("Run Menu"));
		nativeGeneralTabs->setCurrentIndex(ui.tabsGeneral->currentIndex());
		nativeGeneralTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_9->replaceWidget(ui.tabsGeneral, nativeGeneralTabs);
		ui.tabsGeneral->deleteLater();
		ui.tabsGeneral = nativeGeneralTabs;
	}

	// Security Options now uses a native M3 tab host for Security, Isolation,
	// Protection, Job, and Advanced Security pages while preserving state/order.
	{
		auto* nativeSecurityTabs = new QTabWidget(this);
		nativeSecurityTabs->setObjectName(QStringLiteral("tabsSecurity"));
		nativeSecurityTabs->setTabPosition(QTabWidget::West);
		nativeSecurityTabs->setAccessibleName(tr("Security option pages"));
		nativeSecurityTabs->addTab(ui.tabSecurity, tr("Security"));
		nativeSecurityTabs->addTab(ui.tabIsolation, tr("Security Isolation"));
		nativeSecurityTabs->addTab(ui.tabPrivate, tr("Box Protection"));
		nativeSecurityTabs->addTab(ui.tabJob, tr("Job Object"));
		nativeSecurityTabs->addTab(ui.tabPrivileges, tr("Advanced Security"));
		nativeSecurityTabs->setCurrentIndex(ui.tabsSecurity->currentIndex());
		nativeSecurityTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_7->replaceWidget(ui.tabsSecurity, nativeSecurityTabs);
		ui.tabsSecurity->deleteLater();
		ui.tabsSecurity = nativeSecurityTabs;
	}

	// Program Control now uses a native M3 tab host for Force and Breakout
	// pages; later dynamic Group/Stop/Start pages continue to append normally.
	{
		auto* nativeForceTabs = new QTabWidget(this);
		nativeForceTabs->setObjectName(QStringLiteral("tabsForce"));
		nativeForceTabs->setTabPosition(QTabWidget::West);
		nativeForceTabs->setAccessibleName(tr("Program control pages"));
		nativeForceTabs->addTab(ui.tabForceProgs, tr("Force Programs"));
		nativeForceTabs->addTab(ui.tabBreakout, tr("Breakout Programs"));
		nativeForceTabs->setCurrentIndex(ui.tabsForce->currentIndex());
		nativeForceTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_6->replaceWidget(ui.tabsForce, nativeForceTabs);
		ui.tabsForce->deleteLater();
		ui.tabsForce = nativeForceTabs;
	}

	// Resource Access now uses a native M3 tab host for Files, Registry, IPC,
	// Wnd, COM, and Access Policies pages while preserving state and order.
	{
		auto* nativeAccessTabs = new QTabWidget(this);
		nativeAccessTabs->setObjectName(QStringLiteral("tabsAccess"));
		nativeAccessTabs->addTab(ui.tabFiles, tr("Files"));
		nativeAccessTabs->addTab(ui.tabKeys, tr("Registry"));
		nativeAccessTabs->addTab(ui.tabIPC, tr("IPC"));
		nativeAccessTabs->addTab(ui.tabWnd, tr("Wnd"));
		nativeAccessTabs->addTab(ui.tabCOM, tr("COM"));
		nativeAccessTabs->addTab(ui.tabPolicy, tr("Access Policies"));
		nativeAccessTabs->setCurrentIndex(ui.tabsAccess->currentIndex());
		nativeAccessTabs->setProperty("m3NativeSurface", true);
		ui.gridLayout_11->replaceWidget(ui.tabsAccess, nativeAccessTabs);
		ui.tabsAccess->deleteLater();
		ui.tabsAccess = nativeAccessTabs;
	}

	// Stop Options now uses native M3 controls for its two persisted behavior
	// switches; the surrounding stop-tab merge remains unchanged.
	{
		auto migrateStopCheck = [&](QCheckBox*& control) {
			auto* native = new QCheckBox(control->text(), this);
			native->setObjectName(control->objectName());
			native->setCheckState(control->checkState());
			native->setToolTip(control->toolTip());
			native->setProperty("m3NativeSurface", true);
			ui.gridLayout_37->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		migrateStopCheck(ui.chkNoStopWnd);
		migrateStopCheck(ui.chkLingerLeniency);
	}

	// Leader Programs keeps its model/tree intact while replacing the
	// Designer action controls with native M3 controls and the same handlers.
	{
		auto migrateLeaderButton = [&](QPushButton*& control) {
			auto* native = new QPushButton(control->text(), this);
			native->setObjectName(control->objectName());
			native->setEnabled(control->isEnabled());
			native->setToolTip(control->toolTip());
			native->setProperty("m3NativeSurface", true);
			ui.gridLayout_58->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		auto migrateLeaderCheck = [&](QCheckBox*& control) {
			auto* native = new QCheckBox(control->text(), this);
			native->setObjectName(control->objectName());
			native->setCheckState(control->checkState());
			native->setEnabled(control->isEnabled());
			native->setToolTip(control->toolTip());
			native->setProperty("m3NativeSurface", true);
			ui.gridLayout_58->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		migrateLeaderButton(ui.btnAddLeader);
		migrateLeaderButton(ui.btnDelLeader);
		migrateLeaderCheck(ui.chkShowLeaderTmpl);
	}

	// Start Restrictions replaces its Designer controls while retaining the
	// existing tree and radio semantics used by the start-policy handlers.
	{
		auto migrateStartButton = [&](QPushButton*& control) {
			auto* native = new QPushButton(control->text(), this);
			native->setObjectName(control->objectName());
			native->setEnabled(control->isEnabled());
			native->setToolTip(control->toolTip());
			native->setProperty("m3NativeSurface", true);
			ui.gridLayout_19->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		auto migrateStartCheck = [&](QCheckBox*& control) {
			auto* native = new QCheckBox(control->text(), this);
			native->setObjectName(control->objectName());
			native->setCheckState(control->checkState());
			native->setEnabled(control->isEnabled());
			native->setToolTip(control->toolTip());
			native->setProperty("m3NativeSurface", true);
			ui.gridLayout_19->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		auto migrateStartRadio = [&](QRadioButton*& control) {
			auto* native = new QRadioButton(control->text(), this);
			native->setObjectName(control->objectName());
			native->setChecked(control->isChecked());
			native->setEnabled(control->isEnabled());
			native->setToolTip(control->toolTip());
			native->setAutoExclusive(control->autoExclusive());
			native->setProperty("m3NativeSurface", true);
			ui.gridLayout_23->replaceWidget(control, native);
			control->deleteLater();
			control = native;
		};
		migrateStartButton(ui.btnAddStartProg);
		migrateStartButton(ui.btnDelStartProg);
		migrateStartCheck(ui.chkShowStartTmpl);
		migrateStartCheck(ui.chkStartBlockMsg);
		migrateStartCheck(ui.chkAlertBeforeStart);
		migrateStartRadio(ui.radStartAll);
		migrateStartRadio(ui.radStartExcept);
		migrateStartRadio(ui.radStartSelected);
	}

	ui.tabsOther->setCurrentIndex(0);
	ui.tabsOther->setTabIcon(0, CSandMan::GetIcon("Presets"));
	ui.tabsOther->setTabIcon(1, CSandMan::GetIcon("Dll"));

	ui.tabsAdvanced->setCurrentIndex(0);
	ui.tabsAdvanced->setTabIcon(0, CSandMan::GetIcon("Presets"));
	ui.tabsAdvanced->setTabIcon(1, CSandMan::GetIcon("Trigger"));
	ui.tabsAdvanced->setTabIcon(2, CSandMan::GetIcon("Shield2"));
	ui.tabsAdvanced->setTabIcon(3, CSandMan::GetIcon("Anon"));
	ui.tabsAdvanced->setTabIcon(4, CSandMan::GetIcon("Users"));
	ui.tabsAdvanced->setTabIcon(5, CSandMan::GetIcon("SetLogging"));
	ui.tabsAdvanced->setTabIcon(6, CSandMan::GetIcon("Bug"));

	ui.tabsTemplates->setCurrentIndex(0);
	ui.tabsTemplates->setTabIcon(0, CSandMan::GetIcon("Template"));
	ui.tabsTemplates->setTabIcon(1, CSandMan::GetIcon("Explore"));
	ui.tabsTemplates->setTabIcon(2, CSandMan::GetIcon("Accessibility"));


	int iViewMode = theConf->GetInt("Options/ViewMode", 1);
	int iOptionLayout = theConf->GetInt("Options/NewConfigLayout", 2);
	if (iOptionLayout == 2)
		iOptionLayout = iViewMode != 2 ? 1 : 0;

	if ((QGuiApplication::queryKeyboardModifiers() & Qt::AltModifier) != 0)
		iOptionLayout = !iOptionLayout;

	QWidget* pDummy = new QWidget(this);
	pDummy->setVisible(false);

	// merge recovery tabs
	QWidget* pWidget3 = new QWidget();
	pWidget3->setLayout(ui.gridLayout_10);
	ui.gridLayout_24->addWidget(pWidget3, 1, 0);
	QWidget* pWidget4 = new QWidget();
	pWidget4->setLayout(ui.gridLayout_56);
	ui.gridLayout_24->addWidget(pWidget4, 2, 0);
	delete ui.tabsRecovery;
	ui.gridLayout_24->setContentsMargins(0, 0, 0, 0);

	// collect file options on a new files tab

	QWidget* pWidget = new QWidget();
	QGridLayout* pLayout = new QGridLayout(pWidget);

	QTabWidget* pTabWidget = new QTabWidget();
	pLayout->addWidget(pTabWidget, 0, 0);
	ui.tabs->insertTab(1, pWidget, tr("File Options"));
	ui.tabs->setTabIcon(1, CSandMan::GetIcon("Folder"));

	pTabWidget->addTab(ui.tabsGeneral->widget(1), ui.tabsGeneral->tabText(1));
	pTabWidget->setTabIcon(0, CSandMan::GetIcon("Files"));
	pTabWidget->addTab(ui.tabsGeneral->widget(1), ui.tabsGeneral->tabText(1));
	pTabWidget->setTabIcon(1, CSandMan::GetIcon("Move"));
	pTabWidget->addTab(ui.tabs->widget(9), ui.tabs->tabText(9));
	pTabWidget->setTabIcon(2, CSandMan::GetIcon("Recover"));
	//

	// re structure the UI a bit
	if (iOptionLayout == 1)
	{
		// merge stop tabs
		QWidget* pWidget1 = new QWidget();
		pWidget1->setLayout(ui.gridLayout_57);
		ui.gridLayout_17->addWidget(pWidget1, 1, 0);
		QWidget* pWidget2 = new QWidget();
		pWidget2->setLayout(ui.gridLayout_61);
		ui.gridLayout_17->addWidget(pWidget2, 2, 0);
		QWidget* pWidget3 = new QWidget();
		pWidget3->setLayout(ui.gridLayout_82);
		ui.gridLayout_82->setContentsMargins(3, 3, 3, 3);
		ui.verticalSpacer_40->changeSize(0, 0);
		ui.lblStopOpt->setVisible(false);
		ui.lblStopOpt->setProperty("hidden", true);
		ui.gridLayout_17->addWidget(pWidget3, 3, 0);
		delete ui.tabsStop;
		ui.gridLayout_17->setContentsMargins(0, 0, 0, 0);

		// move stop and restrictions to program tab
		ui.tabsForce->addTab(ui.tabs->widget(5), ui.tabs->tabText(5));
		ui.tabsForce->setTabIcon(2, CSandMan::GetIcon("Stop"));
		ui.tabsForce->addTab(ui.tabs->widget(5), ui.tabs->tabText(5));
		ui.tabsForce->setTabIcon(3, CSandMan::GetIcon("Start"));
		ui.gridLayout_19->setContentsMargins(3, 6, 3, 3);

		// move grouping to program tab
		ui.tabsForce->insertTab(0, ui.tabGroups, tr("Grouping"));
		ui.tabsForce->setTabIcon(0, CSandMan::GetIcon("Group"));
		ui.tabsForce->setCurrentIndex(0);
		ui.gridLayout_18->setContentsMargins(3, 6, 3, 3);
	}

	if (iViewMode != 1 && (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier) == 0)
	{
		if (iOptionLayout == 1) {
			//ui.tabs->removeTab(7); // ini edit
			ui.tabAdvanced->setParent(pDummy); //ui.tabs->removeTab(5); // advanced
			//ui.tabsForce->removeTab(2); // breakout
		}
		else {
			//ui.tabs->removeTab(11); // ini edit
			ui.tabAdvanced->setParent(pDummy); //ui.tabs->removeTab(9); // advanced
			//ui.tabsForce->removeTab(1); // breakout
		}
		ui.tabPrivileges->setParent(pDummy); //ui.tabsSecurity->removeTab(3); // advanced security
		ui.tabIsolation->setParent(pDummy); //ui.tabsSecurity->removeTab(1); // security isolation
		//ui.tabsAccess->removeTab(5); // policy
		ui.treeOptions = NULL;
	}

	foreach(QTreeWidget* pTree, this->findChildren<QTreeWidget*>()) {
		if (pTree == ui.treeFolders) continue;
		pTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
		pTree->setMinimumHeight(50);
	}

	int size = 16.0;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	size *= (QApplication::desktop()->logicalDpiX() / 96.0); // todo Qt6
#endif
	AddIconToLabel(ui.lblAppearance, CSandMan::GetIcon("Design").pixmap(size,size));
	AddIconToLabel(ui.lblBoxType, CSandMan::GetIcon("Maintenance").pixmap(size,size));
	AddIconToLabel(ui.lblNotes, CSandMan::GetIcon("EditIni").pixmap(size,size));
	AddIconToLabel(ui.lblStructure, CSandMan::GetIcon("Structure").pixmap(size,size));
	AddIconToLabel(ui.lblMigration, CSandMan::GetIcon("Move").pixmap(size,size));
	AddIconToLabel(ui.lblDelete, CSandMan::GetIcon("Erase").pixmap(size,size));
	AddIconToLabel(ui.lblRawDisk, CSandMan::GetIcon("Disk").pixmap(size,size));
	AddIconToLabel(ui.lblJob, CSandMan::GetIcon("Job3").pixmap(size,size));
	AddIconToLabel(ui.lblLimit, CSandMan::GetIcon("Job2").pixmap(size,size));
	AddIconToLabel(ui.lblSecurity, CSandMan::GetIcon("Shield5").pixmap(size,size));
	AddIconToLabel(ui.lblElevation, CSandMan::GetIcon("Shield9").pixmap(size,size));
	AddIconToLabel(ui.lblACLs, CSandMan::GetIcon("Ampel").pixmap(size,size));
	AddIconToLabel(ui.lblBoxProtection, CSandMan::GetIcon("BoxConfig").pixmap(size,size));
	AddIconToLabel(ui.lblNetwork, CSandMan::GetIcon("Network").pixmap(size,size));
	AddIconToLabel(ui.lblBind, CSandMan::GetIcon("EthSocket2").pixmap(size,size));
	AddIconToLabel(ui.lblPrinting, CSandMan::GetIcon("Printer").pixmap(size,size));
	AddIconToLabel(ui.lblOther, CSandMan::GetIcon("NoAccess").pixmap(size,size));

	AddIconToLabel(ui.lblStopOpt, CSandMan::GetIcon("Stop").pixmap(size,size));

	AddIconToLabel(ui.lblPorts, CSandMan::GetIcon("Port").pixmap(size,size));

	AddIconToLabel(ui.lblMode, CSandMan::GetIcon("Anon").pixmap(size,size));
	AddIconToLabel(ui.lblPolicy, CSandMan::GetIcon("Policy").pixmap(size,size));

	AddIconToLabel(ui.lblCompatibility, CSandMan::GetIcon("Compatibility").pixmap(size,size));
	//AddIconToLabel(ui.lblComRpc, CSandMan::GetIcon("Objects").pixmap(size,size));

	AddIconToLabel(ui.lblPrivilege, CSandMan::GetIcon("Token").pixmap(size,size));
	AddIconToLabel(ui.lblToken, CSandMan::GetIcon("Sandbox").pixmap(size,size));
	AddIconToLabel(ui.lblIsolation, CSandMan::GetIcon("Fence").pixmap(size,size));
	AddIconToLabel(ui.lblDesktop, CSandMan::GetIcon("Monitor").pixmap(size,size));
	AddIconToLabel(ui.lblAccess, CSandMan::GetIcon("NoAccess").pixmap(size,size));
	AddIconToLabel(ui.lblProtection, CSandMan::GetIcon("EFence").pixmap(size,size));

	AddIconToLabel(ui.lblPrivacyProtection, CSandMan::GetIcon("Anon").pixmap(size,size));
	AddIconToLabel(ui.lblProcessHiding, CSandMan::GetIcon("Cmd").pixmap(size,size));

	AddIconToLabel(ui.lblMonitor, CSandMan::GetIcon("Monitor").pixmap(size,size));
	AddIconToLabel(ui.lblTracing, CSandMan::GetIcon("SetLogging").pixmap(size,size));


	if (theConf->GetBool("Options/AltRowColors", false)) {
		foreach(QTreeWidget* pTree, this->findChildren<QTreeWidget*>()) 
			pTree->setAlternatingRowColors(true);
	}

	// Initialize validation flag from config, fallback to checkbox if not set
	bool defaultValidation = theConf->GetBool("Options/ValidateIniKeys", ui.chkValidateIniKeys->isChecked());
	ui.chkValidateIniKeys->setChecked(defaultValidation);
	m_IniValidationEnabled = defaultValidation;

	int defaultTooltip = theConf->GetInt("Options/EnableIniTooltips", static_cast<int>(CIniHighlighter::GetTooltipMode()));
	ui.chkEnableTooltips->setTristate(true); // Enable tri-state
	ui.chkEnableTooltips->setCheckState(static_cast<Qt::CheckState>(defaultTooltip));
	CIniHighlighter::SetTooltipMode(defaultTooltip); // Initialize the mode

	LoadCompletionConsent();
	int defaultAutoCompletion = theConf->GetInt("Options/EnableAutoCompletion", static_cast<int>(CCodeEdit::GetAutoCompletionMode()));
	if (m_AutoCompletionConsent) { // Consented
		ui.chkEnableAutoCompletion->setTristate(true); // Enable tri-state
		ui.chkEnableAutoCompletion->setCheckState(static_cast<Qt::CheckState>(defaultAutoCompletion));
		CCodeEdit::SetAutoCompletionMode(defaultAutoCompletion); // Initialize the mode
	}
	else {
		CCodeEdit::SetAutoCompletionMode(Qt::Unchecked);
		ui.chkEnableAutoCompletion->setCheckState(Qt::Unchecked);
	}

	// Create initial highlighter and editor
	m_pIniHighlighter = new CIniHighlighter(theGUI->m_DarkTheme, nullptr, m_IniValidationEnabled);
	m_pCodeEdit = new CCodeEdit(m_pIniHighlighter);
	m_pCodeEdit->installEventFilter(this);
	ui.txtIniSection->parentWidget()->layout()->replaceWidget(ui.txtIniSection, m_pCodeEdit);
	delete ui.txtIniSection;
	ui.txtIniSection = nullptr;
	connect(m_pCodeEdit, SIGNAL(textChanged()), this, SLOT(OnIniChanged()));

	// Set fuzzy prefix length bounds from settings data
	CCodeEdit::SetMaxFuzzyPrefixLength(CIniHighlighter::getMaxSettingNameLengthOrDefault());
	CCodeEdit::SetMinFuzzyPrefixLength(CIniHighlighter::getMinSettingNameLengthOrDefault());
	// Pass fuzzy matching toggle from config (no UI checkbox required)
	m_pCodeEdit->SetFuzzyMatchingEnabled(theConf->GetBool("Options/EnableFuzzyMatching", false));

	// Show tooltips when navigating with keyboard
	{
		int iniMode = theConf->GetInt("Options/EnableIniTooltips", static_cast<int>(CIniHighlighter::GetTooltipMode()));
		int popupMode = theConf->GetInt("Options/EnablePopupTooltips", iniMode);
		CCodeEdit::SetPopupTooltipsEnabled(popupMode);
	}

	// Set up autocompletion based on mode
	QCompleter* completer = new QCompleter(this);
	completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
	completer->setFilterMode(Qt::MatchContains);
	
	// Set completer based on mode
	if (CCodeEdit::GetAutoCompletionMode() != CCodeEdit::AutoCompletionMode::Disabled) {
		m_pCodeEdit->SetCompleter(completer);
	}
	else {
		m_pCodeEdit->SetCompleter(nullptr);
	}

	m_pCodeEdit->SetCompletionFilterCallback([](const QString& keyName, const QString& inputKey) -> bool {
		return CIniHighlighter::IsKeyHiddenFromPopup(keyName)
			|| CIniHighlighter::ShouldHideCompletionCandidate(inputKey, keyName, 'p');
	});
	m_pCodeEdit->SetCompletionInsertionCallback([](const QString& candidateKey) -> QString {
		return CIniHighlighter::GetCompletionInsertionText(candidateKey);
	});
	m_pCodeEdit->SetCompletionMatchTextCallback([](const QString& candidateKey) -> QString {
		return CIniHighlighter::GetCompletionMatchText(candidateKey);
	});
	m_pCodeEdit->SetCaseCorrectionCallback([](const QString& wrongKey) -> QString {
		return CIniHighlighter::FindCaseCorrectedKey(wrongKey);
		});
	m_pCodeEdit->SetCaseCorrectionFilterCallback([](const QString& keyName, const QString& inputKey) -> bool {
		return CIniHighlighter::IsKeyHiddenFromContext(keyName, 'c')
			|| CIniHighlighter::ShouldHideCompletionCandidate(inputKey, keyName, 'c');
		});
	const char currentContext = m_Template ? 't' : 's';
	m_pCodeEdit->SetPopupTooltipCallback([currentContext](const QString& keyName) -> QString {
		return CIniHighlighter::GetSettingTooltipForPopup(keyName, QString(), currentContext);
		});
	
	// Update completion model with current settings if auto completion is enabled
	if (CCodeEdit::GetAutoCompletionMode() != CCodeEdit::AutoCompletionMode::Disabled) {
		UpdateAutoCompletion();
	}

	CreateDebug();

	if (m_Template)
	{
		//ui.tabGeneral->setEnabled(false);
		//ui.tabStart->setEnabled(false);
		//ui.tabInternet->setEnabled(false);
		//ui.tabAdvanced->setEnabled(false);
		//ui.tabOther->setEnabled(false);
		//ui.tabTemplates->setEnabled(false);
		//
		//for (int i = 0; i < ui.tabs->count(); i++) 
		//	ui.tabs->setTabEnabled(i, ui.tabs->widget(i)->isEnabled());

		//ui.tabs->setCurrentIndex(ui.tabs->indexOf(ui.tabAccess));

		ui.chkShowGroupTmpl->setEnabled(false);
		ui.chkShowForceTmpl->setEnabled(false);
		ui.chkShowBreakoutTmpl->setEnabled(false);
		ui.chkShowStopTmpl->setEnabled(false);
		ui.chkShowLeaderTmpl->setEnabled(false);
		ui.chkShowStartTmpl->setEnabled(false);
		ui.chkShowFilesTmpl->setEnabled(false);
		ui.chkShowKeysTmpl->setEnabled(false);
		ui.chkShowIPCTmpl->setEnabled(false);
		ui.chkShowWndTmpl->setEnabled(false);
		ui.chkShowCOMTmpl->setEnabled(false);
		ui.chkShowNetFwTmpl->setEnabled(false);
		ui.chkShowRecoveryTmpl->setEnabled(false);
		ui.chkShowRecIgnoreTmpl->setEnabled(false);
		ui.chkShowTriggersTmpl->setEnabled(false);
		ui.chkShowHiddenProcTmpl->setEnabled(false);
		ui.chkShowHostProcTmpl->setEnabled(false);
		ui.chkShowOptionsTmpl->setEnabled(false);

		//ui.chkWithTemplates->setEnabled(false);
	}

	ui.tabs->setCurrentIndex(m_Template ? ui.tabs->count()-1 : 0);
	if(m_Template)
		OnTab();

	//connect(ui.chkWithTemplates, SIGNAL(clicked(bool)), this, SLOT(OnWithTemplates()));

	m_ConfigDirty = true;

	CreateGeneral();

	// Groups
	connect(ui.btnAddGroup, SIGNAL(clicked(bool)), this, SLOT(OnAddGroup()));
	connect(ui.btnAddProg, SIGNAL(clicked(bool)), this, SLOT(OnAddProg()));
	connect(ui.btnDelProg, SIGNAL(clicked(bool)), this, SLOT(OnDelProg()));
	connect(ui.chkShowGroupTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowGroupTmpl()));
	ui.treeGroups->setItemDelegateForColumn(0, new ProgramsDelegate(this, ui.treeGroups, -2, this));
	connect(ui.treeGroups, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnGroupsChanged(QTreeWidgetItem *, int)));
	//

	// Force
	connect(ui.btnForceProg, SIGNAL(clicked(bool)), this, SLOT(OnForceProg()));
	QMenu* pFileBtnMenu = new QMenu(ui.btnForceProg);
	pFileBtnMenu->addAction(tr("Browse for File"), this, SLOT(OnForceBrowseProg()));
	ui.btnForceProg->setPopupMode(QToolButton::MenuButtonPopup);
	ui.btnForceProg->setMenu(pFileBtnMenu);

	connect(ui.btnForceChild, SIGNAL(clicked(bool)), this, SLOT(OnForceChild()));
	pFileBtnMenu = new QMenu(ui.btnForceChild);
	pFileBtnMenu->addAction(tr("Browse for File"), this, SLOT(OnForceBrowseChild()));
	ui.btnForceChild->setPopupMode(QToolButton::MenuButtonPopup);
	ui.btnForceChild->setMenu(pFileBtnMenu);

	connect(ui.btnForceDir, SIGNAL(clicked(bool)), this, SLOT(OnForceDir()));
	connect(ui.btnDelForce, SIGNAL(clicked(bool)), this, SLOT(OnDelForce()));
	connect(ui.chkShowForceTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowForceTmpl()));
	//ui.treeForced->setEditTriggers(QAbstractItemView::DoubleClicked);
	ui.treeForced->setItemDelegateForColumn(0, new NoEditDelegate(this));
	ui.treeForced->setItemDelegateForColumn(1, new ProgramsDelegate(this, ui.treeForced, -1, this));
	connect(ui.treeForced, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnForcedChanged(QTreeWidgetItem *, int)));
	connect(ui.chkDisableForced, SIGNAL(clicked(bool)), this, SLOT(OnForcedChanged()));

	connect(ui.btnBreakoutProg, SIGNAL(clicked(bool)), this, SLOT(OnBreakoutProg()));
	QMenu* pFileBtnMenu2 = new QMenu(ui.btnBreakoutProg);
	pFileBtnMenu2->addAction(tr("Browse for File"), this, SLOT(OnBreakoutBrowse()));
	ui.btnBreakoutProg->setPopupMode(QToolButton::MenuButtonPopup);
	ui.btnBreakoutProg->setMenu(pFileBtnMenu2);
	connect(ui.btnBreakoutDir, SIGNAL(clicked(bool)), this, SLOT(OnBreakoutDir()));
	connect(ui.btnBreakoutDoc, SIGNAL(clicked(bool)), this, SLOT(OnBreakoutDoc()));
	connect(ui.btnDelBreakout, SIGNAL(clicked(bool)), this, SLOT(OnDelBreakout()));
	connect(ui.chkShowBreakoutTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowBreakoutTmpl()));
	//ui.treeBreakout->setEditTriggers(QAbstractItemView::DoubleClicked);
	ui.treeBreakout->setItemDelegateForColumn(0, new NoEditDelegate(this));
	ui.treeBreakout->setItemDelegateForColumn(1, new ProgramsDelegate(this, ui.treeBreakout, -1, this));
	connect(ui.treeBreakout, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnBreakoutChanged(QTreeWidgetItem *, int)));
	//

	// Stop
	connect(ui.btnAddLingering, SIGNAL(clicked(bool)), this, SLOT(OnAddLingering()));
	connect(ui.btnDelStopProg, SIGNAL(clicked(bool)), this, SLOT(OnDelStopProg()));
	connect(ui.chkShowStopTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowStopTmpl()));
	ui.treeStop->setItemDelegateForColumn(0, new ProgramsDelegate(this, ui.treeStop, -1, this));
	connect(ui.treeStop, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnStopChanged()));

	connect(ui.btnAddLeader, SIGNAL(clicked(bool)), this, SLOT(OnAddLeader()));
	connect(ui.btnDelLeader, SIGNAL(clicked(bool)), this, SLOT(OnDelLeader()));
	connect(ui.chkShowLeaderTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowLeaderTmpl()));
	ui.treeLeader->setItemDelegateForColumn(0, new ProgramsDelegate(this, ui.treeLeader, -1, this));
	connect(ui.treeLeader, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnStopChanged()));

	connect(ui.chkNoStopWnd, SIGNAL(clicked(bool)), this, SLOT(OnStopChanged()));
	connect(ui.chkLingerLeniency, SIGNAL(clicked(bool)), this, SLOT(OnStopChanged()));
	//

	// Start
	connect(ui.radStartAll, SIGNAL(clicked(bool)), this, SLOT(OnRestrictStart()));
	connect(ui.radStartExcept, SIGNAL(clicked(bool)), this, SLOT(OnRestrictStart()));
	connect(ui.radStartSelected, SIGNAL(clicked(bool)), this, SLOT(OnRestrictStart()));
	connect(ui.btnAddStartProg, SIGNAL(clicked(bool)), this, SLOT(OnAddStartProg()));
	connect(ui.btnDelStartProg, SIGNAL(clicked(bool)), this, SLOT(OnDelStartProg()));
	//connect(ui.chkShowStartTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowStartTmpl()));
	ui.chkShowStartTmpl->setVisible(false); // todo
	connect(ui.chkStartBlockMsg, SIGNAL(clicked(bool)), this, SLOT(OnStartChanged()));
	ui.treeStart->setItemDelegateForColumn(0, new ProgramsDelegate(this, ui.treeStart, -1, this));
	connect(ui.treeStart, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnStartChanged(QTreeWidgetItem *, int)));
	connect(ui.chkAlertBeforeStart, SIGNAL(clicked(bool)), this, SLOT(OnStartChanged()));
	//

	CreateNetwork();

	CreateAccess();

	// Recovery
	connect(ui.chkAutoRecovery, SIGNAL(clicked(bool)), this, SLOT(OnRecoveryChanged()));
	connect(ui.chkUseIgnoreForQuick, SIGNAL(clicked(bool)), this, SLOT(OnRecoveryChanged()));
	connect(ui.btnAddRecovery, SIGNAL(clicked(bool)), this, SLOT(OnAddRecFolder()));
	connect(ui.btnDelRecovery, SIGNAL(clicked(bool)), this, SLOT(OnDelRecEntry()));
	connect(ui.btnAddRecIgnore, SIGNAL(clicked(bool)), this, SLOT(OnAddRecIgnore()));
	connect(ui.btnAddRecIgnoreExt, SIGNAL(clicked(bool)), this, SLOT(OnAddRecIgnoreExt()));
	connect(ui.btnDelRecIgnore, SIGNAL(clicked(bool)), this, SLOT(OnDelRecIgnoreEntry()));
	connect(ui.chkShowRecoveryTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowRecoveryTmpl()));
	connect(ui.chkShowRecIgnoreTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowRecIgnoreTmpl()));
	//

	CreateAdvanced();

	// Templates
	connect(ui.cmbCategories, SIGNAL(currentIndexChanged(int)), this, SLOT(OnFilterTemplates()));
	connect(ui.txtTemplates, SIGNAL(textChanged(const QString&)), this, SLOT(OnFilterTemplates()));
	//connect(ui.treeTemplates, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(OnTemplateClicked(QTreeWidgetItem*, int)));
	connect(ui.treeTemplates, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(OnTemplateClicked(QTreeWidgetItem*, int)));
	connect(ui.treeTemplates, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(OnTemplateDoubleClicked(QTreeWidgetItem*, int)));
	connect(ui.btnAddTemplate, SIGNAL(clicked(bool)), this, SLOT(OnAddTemplates()));
	QMenu* pTmplBtnMenu = new QMenu(ui.btnAddTemplate);
	for(int i = 1; i < CTemplateWizard::TmplMax; i++)
		pTmplBtnMenu->addAction(tr("Add %1 Template").arg(CTemplateWizard::GetTemplateLabel((CTemplateWizard::ETemplateType)i)), this, SLOT(OnTemplateWizard()))->setData(i);
	ui.btnAddTemplate->setPopupMode(QToolButton::MenuButtonPopup);
	ui.btnAddTemplate->setMenu(pTmplBtnMenu);
	connect(ui.btnOpenTemplate, SIGNAL(clicked(bool)), this, SLOT(OnOpenTemplate()));
	connect(ui.btnDelTemplate, SIGNAL(clicked(bool)), this, SLOT(OnDelTemplates()));
	connect(ui.chkScreenReaders, SIGNAL(clicked(bool)), this, SLOT(OnScreenReaders()));
	//

	connect(ui.tabs, SIGNAL(currentChanged(int)), this, SLOT(OnTab()));

	// edit
	ApplyIniEditFont();

	connect(ui.btnEditIni, SIGNAL(clicked(bool)), this, SLOT(OnEditIni()));
	connect(ui.chkValidateIniKeys, SIGNAL(stateChanged(int)), this, SLOT(OnIniValidationToggled(int)));
	connect(ui.chkEnableTooltips, SIGNAL(stateChanged(int)), this, SLOT(OnTooltipToggled(int)));
	connect(ui.chkEnableAutoCompletion, SIGNAL(stateChanged(int)), this, SLOT(OnAutoCompletionToggled(int)));
	connect(ui.btnEditorSettings, SIGNAL(clicked(bool)), this, SLOT(OnEditorSettings()));
	connect(ui.btnSaveIni, SIGNAL(clicked(bool)), this, SLOT(OnSaveIni()));
	connect(ui.btnCancelEdit, SIGNAL(clicked(bool)), this, SLOT(OnCancelEdit()));
	//connect(ui.txtIniSection, SIGNAL(textChanged()), this, SLOT(OnIniChanged()));

	connect(ui.buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked(bool)), this, SLOT(ok()));
	connect(ui.buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked(bool)), this, SLOT(apply()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	if (ReadOnly)
	{
		ui.btnEditIni->setEnabled(false);
		ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}

	if (theAPI->IsRunningAsAdmin())
	{
		ui.chkDropRights->setEnabled(false);
		ui.chkFakeElevation->setEnabled(false);
	}
	else
		ui.lblAdmin->setVisible(false);

	LoadConfig();

	m_pCurrentTab = ui.tabGeneral;
	UpdateCurrentTab();

	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	
	ui.treeCopy->viewport()->installEventFilter(this);
	ui.treeRun->viewport()->installEventFilter(this);
	ui.treeGroups->viewport()->installEventFilter(this);
	ui.treeForced->viewport()->installEventFilter(this);
	ui.treeBreakout->viewport()->installEventFilter(this);
	ui.treeStop->viewport()->installEventFilter(this);
	ui.treeLeader->viewport()->installEventFilter(this);
	ui.treeStart->viewport()->installEventFilter(this);
	ui.treeINet->viewport()->installEventFilter(this);
	ui.treeNetFw->viewport()->installEventFilter(this);
	ui.treeDns->viewport()->installEventFilter(this);
	ui.treeProxy->viewport()->installEventFilter(this);
	ui.treeFiles->viewport()->installEventFilter(this);
	ui.treeKeys->viewport()->installEventFilter(this);
	ui.treeIPC->viewport()->installEventFilter(this);
	ui.treeWnd->viewport()->installEventFilter(this);
	ui.treeCOM->viewport()->installEventFilter(this);
	ui.treeRecovery->viewport()->installEventFilter(this);
	ui.treeRecIgnore->viewport()->installEventFilter(this);
	//ui.treeAccess->viewport()->installEventFilter(this);
	if(ui.treeOptions) ui.treeOptions->viewport()->installEventFilter(this);
	ui.treeTriggers->viewport()->installEventFilter(this);
	ui.treeHideProc->viewport()->installEventFilter(this);
	ui.treeHostProc->viewport()->installEventFilter(this);
	ui.lstUsers->viewport()->installEventFilter(this);
	ui.treeTemplates->viewport()->installEventFilter(this);
	this->installEventFilter(this); // prevent enter from closing the dialog

	restoreGeometry(theConf->GetBlob("OptionsWindow/Window_Geometry"));

	foreach(QTreeWidget * pTree, this->findChildren<QTreeWidget*>()) {
		QByteArray Columns = theConf->GetBlob("OptionsWindow/" + pTree->objectName() + "_Columns");
		if (!Columns.isEmpty()) 
			pTree->header()->restoreState(Columns);
	}

	if (theAPI->GetGlobalSettings()->GetBool("EditAdminOnly", false) && !IsAdminUser())
	{
		for (int I = 0; I < ui.tabs->count(); I++) {
			QGridLayout* pGrid = qobject_cast<QGridLayout*>(ui.tabs->widget(I)->layout());
			QTabWidget* pSubTabs = pGrid ? qobject_cast<QTabWidget*>(pGrid->itemAt(0)->widget()) : NULL;
			if (!pSubTabs) {
				ui.tabs->widget(I)->setEnabled(false);
			}
			else {
				for (int J = 0; J < pSubTabs->count(); J++) {
					pSubTabs->widget(J)->setEnabled(false);
				}
			}
		}
	}

	int iOptionTree = theConf->GetInt("Options/OptionTree", 2);
	if (iOptionTree == 2)
		iOptionTree = iViewMode == 2 ? 1 : 0;

	if (iOptionTree) {
		m_HoldChange = true;
		OnSetTree();
		m_HoldChange = false;
	}
	else {
		//this->setMinimumHeight(390);

		QWidget* pSearch = AddConfigSearch(ui.tabs);
		ui.horizontalLayout->insertWidget(0, pSearch);
		QTimer::singleShot(0, [this]() {
			m_pSearch->setMaximumWidth(m_pTabs->tabBar()->width());
		});

		QAction* pSetTree = new QAction();
		connect(pSetTree, SIGNAL(triggered()), this, SLOT(OnSetTree()));
		pSetTree->setShortcut(QKeySequence("Ctrl+Alt+T"));
		pSetTree->setShortcutContext(Qt::WidgetWithChildrenShortcut);
		this->addAction(pSetTree);
	}
	m_pSearch->setPlaceholderText(tr("Search for options"));
	
	SetTabOrder(this);
}

void COptionsWindow::ApplyIniEditFont()
{
	QFont font; // defaults to application font
	auto fontName = theConf->GetString("UIConfig/IniFont", "").trimmed();
	if (!fontName.isEmpty()) {
		font.fromString(fontName); // ignore fromString() fail
		//ui.txtIniSection->setFont(font);
		m_pCodeEdit->SetFont(font);
	}
}

void COptionsWindow::UpdateAutoCompletion()
{
	if (CCodeEdit::GetAutoCompletionMode() == CCodeEdit::AutoCompletionMode::Disabled || !m_pCodeEdit || !m_pCodeEdit->GetCompleter())
		return;

	// Get completion candidates from the highlighter
	QStringList candidates = CIniHighlighter::GetCompletionCandidates();

	// Update the completion model
	m_pCodeEdit->UpdateCompletionModel(candidates);
}

void COptionsWindow::OnSetTree()
{
	if (!ui.tabs) return;
	QWidget* pAltView = ConvertToTree(ui.tabs);
	ui.verticalLayout->replaceWidget(ui.tabs, pAltView);
	ui.tabs->deleteLater();
	ui.tabs = NULL;
}

void COptionsWindow::OnOptChanged() 
{
	if (m_HoldChange)
		return;
	m_PendingChanges.Update(sender(), m_pTree);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

COptionsWindow::~COptionsWindow()
{
	theConf->SetBlob("OptionsWindow/Window_Geometry",saveGeometry());

	foreach(QTreeWidget * pTree, this->findChildren<QTreeWidget*>()) 
		theConf->SetBlob("OptionsWindow/" + pTree->objectName() + "_Columns", pTree->header()->saveState());
}

void COptionsWindow::closeEvent(QCloseEvent *e)
{
	emit Closed();
	this->deleteLater();
}

bool COptionsWindow::eventFilter(QObject *source, QEvent *event)
{
	auto isTreeViewport = [source](QAbstractItemView* view) {
		return view && source == view->viewport();
	};

	if (event->type() == QEvent::KeyPress && ((QKeyEvent*)event)->key() == Qt::Key_Escape 
		&& ((QKeyEvent*)event)->modifiers() == Qt::NoModifier
		&& source == m_pCodeEdit)
	{
		return true; // cancel event
	}

	if (event->type() == QEvent::KeyPress && ((QKeyEvent*)event)->key() == Qt::Key_Escape 
		&& ((QKeyEvent*)event)->modifiers() == Qt::NoModifier
		&& (source == ui.treeCopy->viewport()
			|| source == ui.treeINet->viewport() || source == ui.treeNetFw->viewport() 
			// || source == ui.treeAccess->viewport()
			|| source == ui.treeFiles->viewport() || source == ui.treeKeys->viewport() || source == ui.treeIPC->viewport() || source == ui.treeWnd->viewport() || source == ui.treeCOM->viewport() 
			|| (ui.treeOptions && source == ui.treeOptions->viewport())))
	{
		CloseCopyEdit(false);
		CloseINetEdit(false);
		CloseNetFwEdit(false);
		CloseAccessEdit(false);
		CloseOptionEdit(false);
        CloseNetProxyEdit(false);
		return true; // cancel event
	}

	if (event->type() == QEvent::KeyPress && (((QKeyEvent*)event)->key() == Qt::Key_Enter || ((QKeyEvent*)event)->key() == Qt::Key_Return) 
		&& (((QKeyEvent*)event)->modifiers() == Qt::NoModifier || ((QKeyEvent*)event)->modifiers() == Qt::KeypadModifier))
	{
		CloseCopyEdit(true);
		CloseINetEdit(true);
		CloseNetFwEdit(true);
		CloseAccessEdit(true);
		CloseOptionEdit(true);
		CloseNetProxyEdit(true);
		return true; // cancel event
	}

	if (event->type() == QEvent::KeyPress && ((QKeyEvent*)event)->key() == Qt::Key_Delete
		&& ((QKeyEvent*)event)->modifiers() == Qt::NoModifier)
	{
		CloseCopyEdit(true);
		CloseINetEdit(true);
		CloseNetFwEdit(true);
		CloseAccessEdit(true);
		CloseOptionEdit(true);
		CloseNetProxyEdit(true);

		if (isTreeViewport(ui.treeCopy))				OnDelCopyRule();
		else if (isTreeViewport(ui.treeRun))			OnDelCommand();
		else if (isTreeViewport(ui.treeGroups))		OnDelProg();
		else if (isTreeViewport(ui.treeForced))		OnDelForce();
		else if (isTreeViewport(ui.treeBreakout))		OnDelBreakout();
		else if (isTreeViewport(ui.treeStop))			OnDelStopProg();
		else if (isTreeViewport(ui.treeLeader))		OnDelLeader();
		else if (isTreeViewport(ui.treeStart))		OnDelStartProg();
		else if (isTreeViewport(ui.treeINet))			OnDelINetProg();
		else if (isTreeViewport(ui.treeNetFw))		OnDelNetFwRule();
		else if (isTreeViewport(ui.treeDns))			OnDelDnsFilter();
		else if (isTreeViewport(ui.treeProxy))		OnDelNetProxy();
		else if (isTreeViewport(ui.treeFiles))		OnDelFile();
		else if (isTreeViewport(ui.treeKeys))			OnDelKey();
		else if (isTreeViewport(ui.treeIPC))			OnDelIPC();
		else if (isTreeViewport(ui.treeWnd))			OnDelWnd();
		else if (isTreeViewport(ui.treeCOM))			OnDelCOM();
		else if (isTreeViewport(ui.treeRecovery))		OnDelRecEntry();
		else if (isTreeViewport(ui.treeRecIgnore))	OnDelRecIgnoreEntry();
		else if (ui.treeOptions && isTreeViewport(ui.treeOptions)) OnDelOption();
		else if (isTreeViewport(ui.treeTriggers))		OnDelAuto();
		else if (isTreeViewport(ui.treeHideProc))		OnDelProcess();
		else if (isTreeViewport(ui.treeHostProc))		OnDelHostProcess();
		else if (isTreeViewport(ui.lstUsers))			OnDelUser();
		else if (isTreeViewport(ui.treeTemplates))	OnDelTemplates();
		else return QDialog::eventFilter(source, event);

		return true;
	}
	
	if (source == ui.treeCopy->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		CloseCopyEdit();
	}

	if (source == ui.treeINet->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		CloseINetEdit();
	}

	if (source == ui.treeNetFw->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		CloseNetFwEdit();
	}

    if (source == ui.treeProxy->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		CloseNetProxyEdit();
	}

	if (//source == ui.treeAccess->viewport() 
		(source == ui.treeFiles->viewport() || source == ui.treeKeys->viewport() || source == ui.treeIPC->viewport() || source == ui.treeWnd->viewport() || source == ui.treeCOM->viewport())
		&& event->type() == QEvent::MouseButtonPress)
	{
		CloseAccessEdit();
	}


	if ((ui.treeOptions && source == ui.treeOptions->viewport()) && event->type() == QEvent::MouseButtonPress)
	{
		CloseOptionEdit();
	}

	// Tooltip handling
	if (source == m_pCodeEdit && event->type() == QEvent::ToolTip) {
		// Check if tooltips are completely disabled
		if (CIniHighlighter::GetTooltipMode() == CIniHighlighter::TooltipMode::Disabled)
			return false;
		const char currentContext = m_Template ? 't' : 's';

		QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

		// Find the text edit widget inside CCodeEdit
		QTextEdit* pTextEdit = m_pCodeEdit->GetTextEdit();
		if (pTextEdit) {
			// Convert mouse position to text cursor position
			QPoint pos = pTextEdit->viewport()->mapFrom(m_pCodeEdit, helpEvent->pos());
			QTextCursor cursor = pTextEdit->cursorForPosition(pos);

			// Get the current line to check if it's a comment
			QTextBlock block = cursor.block();
			QString currentLine = block.text();

			// Don't show tooltips for comment lines
			if (CIniHighlighter::IsCommentLine(currentLine))
				return false;

			// Template values can identify specialized template metadata.
			int equalsPos = currentLine.indexOf('=');
			if (equalsPos >= 0 && (cursor.position() - block.position()) > equalsPos) {
				const QString settingName = currentLine.left(equalsPos).trimmed();
				const bool isTemplateValue = settingName.compare("Template", Qt::CaseInsensitive) == 0
					|| settingName.compare("TemplateReject", Qt::CaseInsensitive) == 0;
				if (!isTemplateValue || !CIniHighlighter::IsValidTooltipContext(currentLine.left(equalsPos + 1))) {
					QToolTip::hideText();
					return false;
				}

				if (CIniHighlighter::IsSettingsLoaded()) {
					const QString settingValue = currentLine.mid(equalsPos + 1).trimmed();
					QString tooltipText = CIniHighlighter::GetSettingTooltip(settingName, settingValue, currentContext);
					if (!tooltipText.isEmpty()) {
						QToolTip::showText(helpEvent->globalPos(), tooltipText, pTextEdit);
						return true;
					}
				}

				QToolTip::hideText();
				return false;
			}

			// Custom word selection that includes dots and underscores
			int initialPos = cursor.position() - block.position();
			int startPos = initialPos;
			int endPos = initialPos;

			// Move to start of the word
			while (startPos > 0) {
				QChar c = currentLine[startPos - 1];
				if (c.isLetterOrNumber() || c == '_' || c == '.')
					startPos--;
				else
					break;
			}

			// Move to end of the word
			while (endPos < currentLine.length()) {
				QChar c = currentLine[endPos];
				if (c.isLetterOrNumber() || c == '_' || c == '.')
					endPos++;
				else
					break;
			}

			// Show tooltip if it's a valid setting
			if (CIniHighlighter::IsValidTooltipContext(currentLine.left(endPos))) {
				// Only try to show tooltips if settings are loaded
				if (CIniHighlighter::IsSettingsLoaded()) {
					QString settingName = currentLine.mid(startPos, endPos - startPos);
					if (settingName.endsWith('='))
						settingName.chop(1);
					const int equalsIndex = currentLine.indexOf('=');
					const QString settingValue = equalsIndex >= 0 ? currentLine.mid(equalsIndex + 1).trimmed() : QString();
					QString tooltipText = CIniHighlighter::GetSettingTooltip(settingName, settingValue, currentContext);
					if (!tooltipText.isEmpty()) {
						QToolTip::showText(helpEvent->globalPos(), tooltipText, pTextEdit);
						return true;
					}
				}
			}
			QToolTip::hideText();
		}
	}

	return QDialog::eventFilter(source, event);
}

//void COptionsWindow::OnWithTemplates()
//{
//	m_Template = ui.chkWithTemplates->isChecked();
//	ui.buttonBox->setEnabled(!m_Template);
//	LoadConfig();
//}

void COptionsWindow::ReadAdvancedCheck(const QString& Name, QCheckBox* pCheck, const QString& Value)
{
	QString Data = m_pBox->GetText(Name);
	if (Data == Value)			pCheck->setCheckState(Qt::Checked);
	else if (Data.isEmpty())	pCheck->setCheckState(Qt::Unchecked);
	else						pCheck->setCheckState(Qt::PartiallyChecked);
}

int COptionsWindow__GetBoolConfig(const QString& Value)
{
	QString temp = Value.left(1).toLower();
	if (temp == "y")
		return 1;
	else if (temp == "n")
		return 0;
	return -1;
}

void COptionsWindow::ReadGlobalCheck(QCheckBox* pCheck, const QString& Setting, bool bDefault)
{
	int iLocal = COptionsWindow__GetBoolConfig(m_pBox->GetText(Setting));
	int iTemplate = COptionsWindow__GetBoolConfig(m_pBox->GetText(Setting, QString(), false, true, true));
	int iGlobal = COptionsWindow__GetBoolConfig(m_pBox->GetText(Setting, QString(), true));

	bool bTemplate = m_pBox->GetBool(Setting, bDefault, true, true);
	if (iLocal != -1) 
		pCheck->setChecked(iLocal == 1);
	else
		pCheck->setChecked(iTemplate != -1 ? iTemplate == 1 : iGlobal != -1 ? iGlobal == 1 : bDefault);
	QStringList Info;
	Info.append(tr("Box: %1").arg(iLocal == 1 ? "y" : "n"));
	if (iTemplate != -1)	Info.append(tr("Template: %1").arg(iTemplate == 1 ? "y" : "n"));
	if (iGlobal != -1)		Info.append(tr("Global: %1").arg(iGlobal == 1 ? "y" : "n"));
	Info.append(tr("Default: %1").arg(bDefault ? "y" : "n"));
	pCheck->setToolTip(Info.join("\r\n"));
}

void COptionsWindow::WriteGlobalCheck(QCheckBox* pCheck, const QString& Setting, bool bDefault)
{
	bool bLocal = pCheck->isChecked();
	bool bPreset = m_pBox->GetBool(Setting, bDefault, true, true);
	SB_STATUS Status;
	if(bPreset == bLocal)
		Status = m_pBox->DelValue(Setting);
	else 
		Status = m_pBox->SetText(Setting, bLocal ? "y" : "n");

	if (!Status)
		throw Status;
}

void COptionsWindow::LoadConfig()
{
	m_ConfigDirty = false;
	m_StartRadioBaselineLoaded = false;

	m_HoldChange = true;
	int iHighlightPendingChanges = theConf->GetInt("Options/HighlightPendingChanges", 2);
	if (iHighlightPendingChanges == 2)
		iHighlightPendingChanges = theConf->GetInt("Options/ViewMode", 1) != 2 ? 1 : 0;
	m_PendingChanges.SetEnabled(iHighlightPendingChanges != 0, m_pTree);

	LoadTemplates();

	LoadGeneral();

	LoadGroups();

	LoadForced();

	LoadStop();

	LoadStart();

	LoadINetAccess();
	LoadNetFwRules();
	LoadDnsFilter();
	LoadNetProxy();
	LoadNetwork();

	LoadAccessList();

	LoadRecoveryList();

	LoadAdvanced();
	LoadDebug();
	
	UpdateBoxType();

	// Update autocompletion after all settings are loaded
	UpdateAutoCompletion();
	m_PendingChanges.CaptureItemBaselines(m_pTree);
	m_PendingChanges.CaptureCheckboxBaselines();
	m_PendingChanges.CaptureRadioButtonBaselines();
	m_PendingChanges.CaptureValueBaselines();

	m_HoldChange = false;
}

void COptionsWindow::WriteAdvancedCheck(QCheckBox* pCheck, const QString& Name, const QString& Value)
{
	SB_STATUS Status;
	if (pCheck->checkState() == Qt::Checked)		
		Status = m_pBox->SetText(Name, Value);
	else if (pCheck->checkState() == Qt::Unchecked) 
		Status = m_pBox->DelValue(Name);
	
	if (!Status)
		throw Status;
}

void COptionsWindow::WriteAdvancedCheck(QCheckBox* pCheck, const QString& Name, const QString& OnValue, const QString& OffValue)
{
	//if (pCheck->checkState() == Qt::PartiallyChecked)
	//	return;

	if (!pCheck->isEnabled())
		return;

	QString StrValue;
	if (pCheck->checkState() == Qt::Checked)
	{
		if (!OnValue.isEmpty())
			StrValue = OnValue;
	}
	else if (pCheck->checkState() == Qt::Unchecked)
	{
		if (!OffValue.isEmpty())
			StrValue = OffValue;
	}

	QStringList Values = m_pBox->GetTextList(Name, false);
	foreach(const QString & CurValue, Values) {
		if (CurValue.contains(","))
			continue;
		if (!StrValue.isEmpty() && CurValue == StrValue)
			StrValue.clear();
		else
			m_pBox->DelValue(Name, CurValue);
	}

	if (!StrValue.isEmpty()) {
		SB_STATUS Status = m_pBox->AppendText(Name, StrValue);
		if (!Status)
			throw Status;
	}
}

void COptionsWindow::WriteText(const QString& Name, const QString& Value)
{
	SB_STATUS Status = m_pBox->SetText(Name, Value);
	if (!Status)
		throw Status;
}

void COptionsWindow::WriteTextList(const QString& Setting, const QStringList& List)
{
	SB_STATUS Status = m_pBox->UpdateTextList(Setting, List, m_Template);
	if (!Status)
		throw Status;
}

void COptionsWindow::WriteTextSafe(const QString& Name, const QString& Value)
{
	QStringList List = m_pBox->GetTextList(Name, false);

	// clear all non per process (name=program.exe,value) entries 
	for (int i = 0; i < List.count(); i++) {
		if (!List[i].contains(","))
			List.removeAt(i--);
	}

	// Prepend the global entry
	if (!Value.isEmpty()) List.append(Value);

	WriteTextList(Name, List);
}

QString COptionsWindow::ReadTextSafe(const QString& Name, const QString& Default)
{
	QStringList List = m_pBox->GetTextList(Name, false);

	for (int i = 0; i < List.count(); i++) {
		if (!List[i].contains(","))
			return List[i];
	}

	return Default;
}

void COptionsWindow::SaveConfig()
{
	bool UpdatePaths = false;

	m_pBox->SetRefreshOnChange(false);

	try
	{
		if (m_GeneralChanged)
			SaveGeneral();
		if (m_CopyRulesChanged)
			SaveCopyRules();

		if (m_GroupsChanged)
			SaveGroups();

		if (m_ForcedChanged)
			SaveForced();

		if (m_StopChanged)
			SaveStop();

		if (m_StartChanged)
			SaveStart();

		if (m_INetBlockChanged)
			SaveINetAccess();
		if (m_NetFwRulesChanged)
			SaveNetFwRules();
		if (m_DnsFilterChanged)
			SaveDnsFilter();
		if (m_NetProxyChanged)
			SaveNetProxy();
		if (m_NetworkChanged)
			SaveNetwork();

		if (m_AccessChanged) {
			SaveAccessList();
			UpdatePaths = true;
		}

		if (m_RecoveryChanged)
			SaveRecoveryList();

		if (m_AdvancedChanged)
			SaveAdvanced();
		SaveDebug();

		if (m_TemplatesChanged)
			SaveTemplates();

		if (m_FoldersChanged)
			SaveFolders();
	}
	catch (SB_STATUS Status)
	{
		theGUI->CheckResults(QList<SB_STATUS>() << Status, theGUI);
	}

	m_pBox->SetRefreshOnChange(true);
	m_pBox->CommitIniChanges();

	if (UpdatePaths)
		TriggerPathReload();
}

bool COptionsWindow::apply()
{
	if (m_pBox->GetText("Enabled").isEmpty() && !(m_Template && m_pBox->GetName().mid(9, 6).compare("Local_", Qt::CaseInsensitive) == 0)) {
		QMessageBox::critical(this, "Sandboxie-Plus", tr("This sandbox has been deleted hence configuration can not be saved."));
		return false;
	}

	CloseINetEdit();
	CloseNetFwEdit();
	CloseAccessEdit();
	CloseOptionEdit();
    CloseNetProxyEdit();

	if (!ui.btnEditIni->isEnabled())
		SaveIniSection();
	else
	{
		if (m_GeneralChanged) {
			auto pBoxEx = m_pBox.objectCast<CSandBoxPlus>();
			if (ui.chkEncrypt->isChecked() && !QFile::exists(pBoxEx->GetBoxImagePath())) {
				if (m_Password.isEmpty())
					OnSetPassword();
				if (m_Password.isEmpty())
					return false;
				pBoxEx->ImBoxCreate(m_ImageSize / 1024, m_Password);
			}
		}

		SaveConfig();
	}

	LoadConfig();

	UpdateCurrentTab();

	//emit OptionsChanged();

	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

	return true;
}

void COptionsWindow::ok()
{
	if(apply())
		close();
}

void COptionsWindow::reject()
{
	if (m_GeneralChanged
	 || m_CopyRulesChanged
	 || m_GroupsChanged
	 || m_ForcedChanged
	 || m_StopChanged
	 || m_StartChanged
	// ||  m_RestrictionChanged
	 || m_INetBlockChanged
	 || m_NetFwRulesChanged
	 || m_DnsFilterChanged
	 || m_NetProxyChanged
	 || m_AccessChanged
	 || m_TemplatesChanged
	 || m_FoldersChanged
	 || m_RecoveryChanged
	 || m_AdvancedChanged)
	{
		if (QMessageBox("Sandboxie-Plus", tr("Some changes haven't been saved yet, do you really want to close this options window?")
		, QMessageBox::Warning, QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape, QMessageBox::NoButton, this).exec() != QMessageBox::Yes)
			return;
	}

	this->close();
}

void COptionsWindow::showTab(const QString& Name)
{
	QWidget* pWidget = this->findChild<QWidget*>("tab" + Name);

	if (ui.tabs) {
		for (int i = 0; i < ui.tabs->count(); i++) {
			QGridLayout* pGrid = qobject_cast<QGridLayout*>(ui.tabs->widget(i)->layout());
			QTabWidget* pSubTabs = pGrid ? qobject_cast<QTabWidget*>(pGrid->itemAt(0)->widget()) : NULL;
			if(ui.tabs->widget(i) == pWidget)
				ui.tabs->setCurrentIndex(i);
			else if(pSubTabs) {
				for (int j = 0; j < pSubTabs->count(); j++) {
					if (pSubTabs->widget(j) == pWidget) {
						ui.tabs->setCurrentIndex(i);
						pSubTabs->setCurrentIndex(j);
					}
				}
			}
		}
	} 
	else
		m_pStack->setCurrentWidget(pWidget);

	CSandMan::SafeShow(this);
}

void COptionsWindow::SetProgramItem(QString Program, QTreeWidgetItem* pItem, int Column, const QString& Suffix, bool bList)
{
	pItem->setData(Column, Qt::UserRole, Program);
	if (Program.left(1) == "<")
		Program = tr("Group: %1").arg(Program.mid(1, Program.length() - 2));
	else if (Program.isEmpty() || Program == "*")
		Program = tr("All Programs");
	else if(bList)
		m_Programs.insert(Program);
	pItem->setText(Column, Program + Suffix);
}

QString COptionsWindow::SelectProgram(bool bOrGroup)
{
	CComboInputDialog progDialog(this);
	progDialog.setText(tr("Enter program:"));
	progDialog.setEditable(true);

	if (bOrGroup)
	{
		foreach(const QString Group, GetCurrentGroups()){
			QString GroupName = Group.mid(1, Group.length() - 2);
			progDialog.addItem(tr("Group: %1").arg(GroupName), Group);
		}
	}

	foreach(const QString & Name, m_Programs)
		progDialog.addItem(Name, Name);

	progDialog.setValue("");

	if (theGUI->SafeExec(&progDialog) != QDialog::Accepted)
		return QString();

	// Note: pressing enter adds the value to the combo list !
	QString Program = progDialog.value(); 
	int Index = progDialog.findValue(Program);
	if (Index != -1 && progDialog.data().isValid())
		Program = progDialog.data().toString();

	return Program;
}

void COptionsWindow::OnTab(QWidget* pTab)
{
	m_pCurrentTab = pTab;

	if (pTab == ui.tabEdit)
	{
		LoadIniSection();
		//ui.txtIniSection->setReadOnly(true);
	}
	else 
	{
		if (m_ConfigDirty)
			LoadConfig();

		UpdateCurrentTab();
	}
}

void COptionsWindow::UpdateCurrentTab()
{
	if (m_pCurrentTab == ui.tabRestrictions || m_pCurrentTab == ui.tabOptions || m_pCurrentTab == ui.tabGeneral) 
	{
		ui.chkVmRead->setChecked(IsAccessEntrySet(eIPC, "", eReadOnly, "$:*"));
	}
	else if (m_pCurrentTab == ui.tabStart || m_pCurrentTab == ui.tabForce)
	{
		if (IsAccessEntrySet(eIPC, "!<StartRunAccess>", eClosed, "*"))
			ui.radStartSelected->setChecked(true);
		else if (IsAccessEntrySet(eIPC, "<StartRunAccess>", eClosed, "*"))
			ui.radStartExcept->setChecked(true);
		else
			ui.radStartAll->setChecked(true);
		ui.treeStart->clear();
		CopyGroupToList("<StartRunAccess>", ui.treeStart);
		CopyGroupToList("<StartRunAccessDisabled>", ui.treeStart, true);

		OnRestrictStart();
		m_PendingChanges.CaptureItemBaselines(m_pTree, ui.treeStart);
		if (!m_StartRadioBaselineLoaded) {
			m_PendingChanges.CaptureRadioButtonBaseline(ui.radStartAll);
			m_PendingChanges.CaptureRadioButtonBaseline(ui.radStartExcept);
			m_PendingChanges.CaptureRadioButtonBaseline(ui.radStartSelected);
			m_StartRadioBaselineLoaded = true;
		}
	}
	else if (m_pCurrentTab == ui.tabInternet || m_pCurrentTab == ui.tabINet || m_pCurrentTab == ui.tabNetConfig)
	{
		if (!m_INetBlockChanged)
			LoadBlockINet();
	}
	else if (m_pCurrentTab == ui.tabDNS || m_pCurrentTab == ui.tabNetProxy)
	{
		if (!m_HoldChange && !m_pCurrentTab->isEnabled())
			theGUI->CheckCertificate(this, 2);
	}
	else if (m_pCurrentTab == ui.tabCOM) {
		CheckOpenCOM();
	}
	else if (m_pCurrentTab == ui.tabWnd)
	{
		if (IsAccessEntrySet(eWnd, "", eOpen, "*"))
		{
			ui.chkNoWindowRename->setEnabled(false);
			ui.chkNoWindowRename->setChecked(true);
		}
		else
		{
			ui.chkNoWindowRename->setEnabled(true);
			ui.chkNoWindowRename->setChecked(IsAccessEntrySet(eWnd, "", eNoRename, "*"));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Raw section ini Editor
//

void COptionsWindow::SetIniEdit(bool bEnable)
{
	if (m_pTree) {
		m_pTree->setEnabled(!bEnable);
	}
	else {
		for (int i = 0; i < ui.tabs->count() - 1; i++) {
			bool Enabled = ui.tabs->widget(i)->isEnabled();
			ui.tabs->setTabEnabled(i, !bEnable && Enabled);
			ui.tabs->widget(i)->setEnabled(Enabled);
		}
	}
	ui.btnSaveIni->setEnabled(bEnable);
	ui.btnCancelEdit->setEnabled(bEnable);
	//ui.txtIniSection->setReadOnly(!bEnable);
	ui.btnEditIni->setEnabled(!bEnable);
}

void COptionsWindow::OnEditIni()
{
	SetIniEdit(true);
}

void COptionsWindow::OnIniValidationToggled(int state)
{
	m_HoldChange = true;

	m_IniValidationEnabled = (state == Qt::Checked);
	
	// Only save to config if not in a reset-skip context
	if (!m_SkipSaveOnToggle) {
		theConf->SetValue("Options/ValidateIniKeys", m_IniValidationEnabled);
	}

	if (state == Qt::Unchecked) {
		CIniHighlighter::MarkSettingsDirty();
		CIniHighlighter::MarkUserSettingsDirty();
	}

	if (m_pIniHighlighter) {
		delete m_pIniHighlighter;
		m_pIniHighlighter = nullptr;
	}

	QTextEdit* pTextEdit = m_pCodeEdit->GetTextEdit();
	if (pTextEdit) {
		m_pIniHighlighter = new CIniHighlighter(theGUI->m_DarkTheme, pTextEdit->document(), m_IniValidationEnabled);
		m_pIniHighlighter->rehighlight();
		UpdateAutoCompletion();
	}

	m_HoldChange = false;
}

void COptionsWindow::OnTooltipToggled(int state)
{
	m_HoldChange = true;

	// Only save to config if not in a reset-skip context
	if (!m_SkipSaveOnToggle) {
		theConf->SetValue("Options/EnableIniTooltips", state);
	}

	CIniHighlighter::SetTooltipMode(state);

	{
		int iniMode = theConf->GetInt("Options/EnableIniTooltips", static_cast<int>(CIniHighlighter::GetTooltipMode()));
		int popupMode = theConf->GetInt("Options/EnablePopupTooltips", iniMode);
		CCodeEdit::SetPopupTooltipsEnabled(popupMode);
	}

	if (state == Qt::Unchecked) {
		CIniHighlighter::ClearLanguageCache();
		CIniHighlighter::ClearThemeCache();
	}

	m_HoldChange = false;
}

void COptionsWindow::OnAutoCompletionToggled(int state)
{
	m_HoldChange = true;

	// Show consent dialog if enabling and not yet consented
	if (state != Qt::Unchecked && !m_AutoCompletionConsent) {
		int chosenState = ShowConsentDialog();
		
		if (chosenState == Qt::Unchecked) {
			// Cancel - revert the checkbox and return
			ui.chkEnableAutoCompletion->setCheckState(Qt::Unchecked);
			m_HoldChange = false;
			return;
		}
		
		// Consent was given, update UI and state
		ui.chkEnableAutoCompletion->setEnabled(true);
		ui.chkEnableAutoCompletion->setTristate(true);
		ui.chkEnableAutoCompletion->setCheckState(static_cast<Qt::CheckState>(chosenState));
		state = chosenState;
	}

	// Only save to config if not in a reset-skip context
	if (!m_SkipSaveOnToggle) {
		theConf->SetValue("Options/EnableAutoCompletion", state);
	}

	CCodeEdit::SetAutoCompletionMode(state); // Use static method like tooltip

	// Enable or disable the completer based on mode
	if (m_pCodeEdit) {
		if (CCodeEdit::GetAutoCompletionMode() != CCodeEdit::AutoCompletionMode::Disabled) {
			// Create completer if it doesn't exist
			if (!m_pCodeEdit->GetCompleter()) {
				QCompleter* completer = new QCompleter(this);
				completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
				completer->setFilterMode(Qt::MatchContains);
				m_pCodeEdit->SetCompleter(completer);

				// Update completion model with current settings
				UpdateAutoCompletion();
			}
		}
		else {
			// Disable completer
			m_pCodeEdit->SetCompleter(nullptr);
			CCodeEdit::ClearFuzzyCache();
		}
	}

	m_HoldChange = false;
}

void COptionsWindow::OnEditorSettings()
{
	CEditorSettingsWindow editorWindow(this);
	if (theGUI->SafeExec(&editorWindow) == QDialog::Accepted) {
		// Settings were saved by the dialog, now update the current UI to reflect changes
		bool previousConsent = m_AutoCompletionConsent;
		LoadCompletionConsent();
		bool newConsent = m_AutoCompletionConsent;
		
		// If consent was just granted (changed from false to true), show the consent dialog
		if (!previousConsent && newConsent) {
			int chosenState = ShowConsentDialog();
			
			// Save the chosen autocomplete mode to config
			theConf->SetValue("Options/EnableAutoCompletion", chosenState);
		}
		
		// Update the current checkboxes to reflect the new settings
		// Note: OptionsWindow only has UI checkboxes for 3 settings:
		// - ValidateIniKeys (ui.chkValidateIniKeys)
		// - EnableIniTooltips (ui.chkEnableTooltips)
		// - EnableAutoCompletion (ui.chkEnableAutoCompletion)
		// The other 3 settings (EnablePopupTooltips, EnableFuzzyMatching, AutoCompletionConsent)
		// are managed by EditorSettings but don't have corresponding UI in OptionsWindow
		
		// Block signals while updating checkboxes to prevent toggle handlers from being called prematurely
		ui.chkValidateIniKeys->blockSignals(true);
		ui.chkEnableTooltips->blockSignals(true);
		ui.chkEnableAutoCompletion->blockSignals(true);
		
		// Read current values from config (will be defaults if settings were reset/deleted)
		bool defaultValidation = theConf->GetBool("Options/ValidateIniKeys", true);
		ui.chkValidateIniKeys->setChecked(defaultValidation);
		
		int defaultTooltip = theConf->GetInt("Options/EnableIniTooltips", 1); // 1 = BasicInfo
		ui.chkEnableTooltips->setCheckState(static_cast<Qt::CheckState>(defaultTooltip));
		
		int defaultAutoCompletion = theConf->GetInt("Options/EnableAutoCompletion", 0); // 0 = Disabled
		if (m_AutoCompletionConsent) { // Consented
			ui.chkEnableAutoCompletion->setTristate(true);
			ui.chkEnableAutoCompletion->setCheckState(static_cast<Qt::CheckState>(defaultAutoCompletion));
		}
		else {
			ui.chkEnableAutoCompletion->setTristate(false);
			ui.chkEnableAutoCompletion->setCheckState(Qt::Unchecked);
		}
		
		// Unblock signals before calling toggle handlers manually
		ui.chkValidateIniKeys->blockSignals(false);
		ui.chkEnableTooltips->blockSignals(false);
		ui.chkEnableAutoCompletion->blockSignals(false);
		
		// Apply the settings immediately
		// Set skip flag for reset settings to prevent re-saving them to config
		// For non-reset settings, allow normal save behavior
		
		// ValidateIniKeys
		m_SkipSaveOnToggle = editorWindow.WasValidateIniKeysReset();
		OnIniValidationToggled(defaultValidation ? Qt::Checked : Qt::Unchecked);
		m_SkipSaveOnToggle = false;
		
		// EnableIniTooltips
		m_SkipSaveOnToggle = editorWindow.WasEnableIniTooltipsReset();
		OnTooltipToggled(defaultTooltip);
		m_SkipSaveOnToggle = false;
		
		// EnableAutoCompletion
		m_SkipSaveOnToggle = editorWindow.WasEnableAutoCompletionReset();
		OnAutoCompletionToggled(defaultAutoCompletion);
		m_SkipSaveOnToggle = false;
		
		// Apply settings that don't have UI checkboxes in OptionsWindow
		// These are managed via EditorSettings only
		bool fuzzyEnabled = theConf->GetBool("Options/EnableFuzzyMatching", false);
		m_pCodeEdit->SetFuzzyMatchingEnabled(fuzzyEnabled);
		
		// Always update autocompletion list regardless of reset status
		UpdateAutoCompletion();
	}
}

void COptionsWindow::OnSaveIni()
{
	SaveIniSection();
	SetIniEdit(false);
}

void COptionsWindow::OnIniChanged()
{
	if (m_HoldChange)
		return;
	if(ui.btnEditIni->isEnabled())
		SetIniEdit(true);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void COptionsWindow::OnCancelEdit()
{
	SetIniEdit(false);
	LoadIniSection();
}

void COptionsWindow::LoadIniSection()
{
	QString Section;

	// Note: the service only caches sandboxie.ini not templates.ini, hence for global templates we need to load the section through the driver
	if (m_Template && m_pBox->GetName().mid(9, 6).compare("Local_", Qt::CaseInsensitive) != 0)
	{
		m_Settings = m_pBox->GetIniSection(NULL, m_Template);

		for (QList<CSbieIni::SbieIniValue>::const_iterator I = m_Settings.begin(); I != m_Settings.end(); ++I)
			Section += I->Name + "=" + I->Value + "\n";
	}
	else
		Section = m_pBox->SbieIniGetEx(m_pBox->GetName(), "");

	m_HoldChange = true;
	//ui.txtIniSection->setPlainText(Section);
	m_pCodeEdit->SetCode(Section);
	m_HoldChange = false;
}

void COptionsWindow::SaveIniSection()
{
	m_ConfigDirty = true;

	/*m_pBox->SetRefreshOnChange(false);

	// Note: an incremental update would be more elegant but it would change the entry order in the ini,
	//			hence it's better for the user to fully rebuild the section each time.
	//
	for (QList<QPair<QString, QString>>::const_iterator I = m_Settings.begin(); I != m_Settings.end(); ++I)
		m_pBox->DelValue(I->first, I->second);

	//QList<QPair<QString, QString>> NewSettings;
	//QList<QPair<QString, QString>> OldSettings = m_Settings;

	QStringList Section = SplitStr(ui.txtIniSection->toPlainText(), "\n");
	foreach(const QString& Line, Section)
	{
		if (Line.isEmpty())
			return;
		StrPair Settings = Split2(Line, "=");
		
		//if (!OldSettings.removeOne(Settings))
		//	NewSettings.append(Settings);

		m_pBox->AppendText(Settings.first, Settings.second);
	}

	//for (QList<QPair<QString, QString>>::const_iterator I = OldSettings.begin(); I != OldSettings.end(); ++I)
	//	m_pBox->DelValue(I->first, I->second);
	//
	//for (QList<QPair<QString, QString>>::const_iterator I = NewSettings.begin(); I != NewSettings.end(); ++I)
	//	m_pBox->AppendText(I->first, I->second);

	m_pBox->SetRefreshOnChange(true);
	m_pBox->CommitIniChanges();*/

	//m_pBox->GetAPI()->SbieIniSet(m_pBox->GetName(), "", ui.txtIniSection->toPlainText());
	m_pBox->SbieIniSet(m_pBox->GetName(), "", m_pCodeEdit->GetCode());

	//LoadIniSection();
}

#include "OptionsAccess.cpp"
#include "OptionsAdvanced.cpp"
#include "OptionsForce.cpp"
#include "OptionsGeneral.cpp"
#include "OptionsGrouping.cpp"
#include "OptionsNetwork.cpp"
#include "OptionsRecovery.cpp"
#include "OptionsStart.cpp"
#include "OptionsStop.cpp"
#include "OptionsTemplates.cpp"

#include <windows.h>

void COptionsWindow::TriggerPathReload()
{
	//
	// this message makes all boxes reload their path presets
	//

	DWORD bsm_app = BSM_APPLICATIONS;
	BroadcastSystemMessage(BSF_POSTMESSAGE, &bsm_app, WM_DEVICECHANGE, 'sb', 0);
}

// Helper to load/save consent from config
void COptionsWindow::LoadCompletionConsent()
{
	m_AutoCompletionConsent = theConf->GetBool("Options/AutoCompletionConsent", false);
}

void COptionsWindow::SaveCompletionConsent()
{
	theConf->SetValue("Options/AutoCompletionConsent", m_AutoCompletionConsent);
}

QString COptionsWindow::localizedCompletionShortcut()
{
	QKeySequence shortcut = QKeySequence(Qt::CTRL + Qt::Key_Space);
	return shortcut.toString(QKeySequence::NativeText); // Returns the localized shortcut
}

// Show consent dialog and return the chosen autocomplete state
// Returns: Qt::Unchecked (0) if cancelled, Qt::PartiallyChecked (1) for Basic, Qt::Checked (2) for Full
int COptionsWindow::ShowConsentDialog()
{
	QMessageBox consentBox(this);
	consentBox.setWindowTitle(tr("Autocomplete Consent Required"));
	consentBox.setIcon(QMessageBox::Question);
	consentBox.setText(tr("Autocomplete feature requires your consent to proceed."));
	consentBox.setInformativeText(
		tr("If you are unsure about the settings displayed in the autocomplete popup, we strongly recommend consulting the software's documentation or source code before proceeding. Enabling this feature without proper understanding may lead to unintended consequences, for which you will be solely responsible.\n\n"
			"Choose autocomplete mode:\n"
			"%1 Manual: Autocomplete suggestions with %2.\n"
			"%1 While Typing: Autocomplete suggestions while typing.")
		.arg(QChar(0x2022))   // Bullet symbol
		.arg(localizedCompletionShortcut()) // Localized Ctrl+Space
	);

	QPushButton* basicButton = consentBox.addButton(tr("Manual"), QMessageBox::YesRole);
	basicButton->setToolTip(tr("Triggers autocomplete suggestions with %1.").arg(localizedCompletionShortcut()));

	QPushButton* fullButton = consentBox.addButton(tr("While Typing"), QMessageBox::YesRole);
	fullButton->setToolTip(tr("Triggers autocomplete suggestions while typing."));

	QPushButton* cancelButton = consentBox.addButton(tr("Cancel"), QMessageBox::NoRole);
	cancelButton->setToolTip(tr("Keeps autocomplete suggestions disabled."));
	
	consentBox.setDefaultButton(basicButton);
	
	consentBox.exec();
	QAbstractButton* clickedButton = consentBox.clickedButton();
	
	if (clickedButton == basicButton) {
		m_AutoCompletionConsent = true;
		SaveCompletionConsent();
		return Qt::PartiallyChecked; // Basic mode
	}
	else if (clickedButton == fullButton) {
		m_AutoCompletionConsent = true;
		SaveCompletionConsent();
		return Qt::Checked; // Full mode
	}
	else { // Cancel
		m_AutoCompletionConsent = false;
		SaveCompletionConsent();
		return Qt::Unchecked; // Cancelled
	}
}
