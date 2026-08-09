#pragma once

#include <QDialog>
#include "SbiePlusAPI.h"

class QComboBox;
class QCheckBox;

class CCompressDialog : public QDialog
{
	Q_OBJECT

public:
	CCompressDialog(QWidget *parent = Q_NULLPTR);
	~CCompressDialog();

	QString GetFormat();
	int GetLevel();
	bool MakeSolid();

	void SetMustEncrypt();
	bool UseEncryption();

private slots:
	void OnFormatChanged(int index);

private:
	QComboBox* m_format = nullptr;
	QComboBox* m_compression = nullptr;
	QCheckBox* m_solid = nullptr;
	QCheckBox* m_encrypt = nullptr;
};
