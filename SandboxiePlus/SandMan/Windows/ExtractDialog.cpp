#include "stdafx.h"
#include "ExtractDialog.h"
#include "SandMan.h"
#include "../MiscHelpers/Common/Settings.h"
#include "../MiscHelpers/Common/Common.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>


CExtractDialog::CExtractDialog(const QString& Name, QWidget *parent)
	: QDialog(parent)
{
	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::CustomizeWindowHint;
	//flags &= ~Qt::WindowContextHelpButtonHint;
	//flags &= ~Qt::WindowSystemMenuHint;
	//flags &= ~Qt::WindowMinMaxButtonsHint;
	//flags |= Qt::WindowMinimizeButtonHint;
	//flags &= ~Qt::WindowCloseButtonHint;
	flags &= ~Qt::WindowContextHelpButtonHint;
	//flags &= ~Qt::WindowSystemMenuHint;
	setWindowFlags(flags);

	setWindowTitle(tr("Sandbox import"));
	auto* form = new QFormLayout();
	m_name = new QLineEdit(this);
	m_name->setAccessibleName(tr("Sandbox name"));
	form->addRow(tr("Name"), m_name);
	m_root = new QComboBox(this);
	m_root->setEditable(true);
	m_root->setAccessibleName(tr("Sandbox root directory"));
	form->addRow(tr("Root"), m_root);
	m_browse = new QPushButton(tr("Browse…"), this);
	m_browse->setAccessibleName(tr("Browse for sandbox root directory"));
	form->addRow(QString(), m_browse);
	m_noCrypt = new QCheckBox(tr("Do not encrypt imported content"), this);
	m_noCrypt->setVisible(false);
	form->addRow(QString(), m_noCrypt);
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	form->addRow(QString(), buttons);
	setLayout(form);

	m_name->setText(Name);

	QString Location = theAPI->GetGlobalSettings()->GetText("FileRootPath", "\\??\\%SystemDrive%\\Sandbox\\%USER%\\%SANDBOX%");
    m_root->addItem(Location/*.replace("%SANDBOX%", field("boxName").toString())*/);
    QStringList StdLocations = QStringList() 
        << "\\??\\%SystemDrive%\\Sandbox\\%USER%\\%SANDBOX%" 
        << "\\??\\%SystemDrive%\\Sandbox\\%SANDBOX%" 
        << "\\??\\%SystemDrive%\\Users\\%USER%\\Sandbox\\%SANDBOX%";
    foreach(auto StdLocation, StdLocations) {
        if (StdLocation != Location)
            m_root->addItem(StdLocation);
    }

	connect(m_browse, &QPushButton::clicked, this, [this]() {
        QString FilePath = QFileDialog::getExistingDirectory(this, tr("Select Directory"));
	    if (!FilePath.isEmpty())
		    m_root->setCurrentText(FilePath.replace("/", "\\"));
    });
	connect(buttons, &QDialogButtonBox::accepted, this, &CExtractDialog::OnAccept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	//restoreGeometry(theConf->GetBlob("ExtractDialog/Window_Geometry"));
}

CExtractDialog::~CExtractDialog()
{
	//theConf->SetBlob("ExtractDialog/Window_Geometry", saveGeometry());
}

void CExtractDialog::OnAccept()
{
	CSandBoxPtr pBox = theAPI->GetBoxByName(m_name->text());
	if (!pBox.isNull()) {
		QMessageBox::warning(this, "Sandboxie-Plus", tr("This name is already in use, please select an alternative box name"));
		return;
	}

	accept();
}

QString CExtractDialog::GetRoot() const
{
	QString Location = theAPI->GetGlobalSettings()->GetText("FileRootPath", "\\??\\%SystemDrive%\\Sandbox\\%USER%\\%SANDBOX%");
	if (Location == m_root->currentText())
		return "";
	return m_root->currentText();
}
