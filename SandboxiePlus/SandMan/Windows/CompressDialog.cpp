#include "stdafx.h"
#include "CompressDialog.h"
#include "SandMan.h"
#include "../MiscHelpers/Common/Settings.h"
#include "../MiscHelpers/Common/Common.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>


CCompressDialog::CCompressDialog(QWidget *parent)
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

	setWindowTitle(tr("Sandbox export"));
	auto* form = new QFormLayout(this);
	m_format = new QComboBox(this);
	m_format->setAccessibleName(tr("Export format"));
	form->addRow(tr("Format"), m_format);
	m_compression = new QComboBox(this);
	m_compression->setAccessibleName(tr("Compression level"));
	form->addRow(tr("Compression"), m_compression);
	m_solid = new QCheckBox(tr("Create solid archive"), this);
	m_solid->setAccessibleName(tr("Create solid archive"));
	form->addRow(QString(), m_solid);
	m_encrypt = new QCheckBox(tr("Encrypt archive"), this);
	m_encrypt->setAccessibleName(tr("Encrypt archive"));
	form->addRow(QString(), m_encrypt);
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	form->addRow(QString(), buttons);
	setLayout(form);

	connect(m_format, qOverload<int>(&QComboBox::currentIndexChanged), this, &CCompressDialog::OnFormatChanged);

	m_format->addItem(tr("7-Zip"), ".7z");
	m_format->addItem(tr("Zip"), ".zip");

	m_compression->addItem(tr("Store"), 0);
	m_compression->addItem(tr("Fastest"), 1);
	m_compression->addItem(tr("Fast"), 3);
	m_compression->addItem(tr("Normal"), 5);
	m_compression->addItem(tr("Maximum"), 7);
	m_compression->addItem(tr("Ultra"), 9);
	m_compression->setCurrentIndex(m_compression->findData(theConf->GetInt("Options/ExportCompression", 3)));

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	OnFormatChanged(m_format->currentIndex());

	//restoreGeometry(theConf->GetBlob("CompressDialog/Window_Geometry"));
}

CCompressDialog::~CCompressDialog()
{
	//theConf->SetBlob("CompressDialog/Window_Geometry", saveGeometry());
}

void CCompressDialog::OnFormatChanged(int index)
{
	m_solid->setEnabled(index == 0);
	m_encrypt->setEnabled(index == 0);
}

QString CCompressDialog::GetFormat()
{
	return m_format->currentData().toString();
}

int CCompressDialog::GetLevel()
{
	return m_compression->currentData().toInt();
}

bool CCompressDialog::MakeSolid()
{
	return m_solid->isChecked();
}

void CCompressDialog::SetMustEncrypt()
{
	m_encrypt->setChecked(true);
	m_encrypt->setEnabled(false);
}

bool CCompressDialog::UseEncryption()
{
	return m_encrypt->isChecked();
}
