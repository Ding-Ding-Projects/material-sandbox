#pragma once

#include <QDialog>
#include "SbiePlusAPI.h"

class QComboBox;
class QLineEdit;
class QCheckBox;
class QPushButton;

class CExtractDialog : public QDialog
{
	Q_OBJECT

public:
	CExtractDialog(const QString& Name, QWidget *parent = Q_NULLPTR);
	~CExtractDialog();

	QString GetName() const { return m_name->text(); }
	QString GetRoot() const;
	void ShowNoCrypt() const { m_noCrypt->setVisible(true); }
	bool IsNoCrypt() const { return m_noCrypt->isChecked(); }

private slots:
	void OnAccept();

private:
	QLineEdit* m_name = nullptr;
	QComboBox* m_root = nullptr;
	QPushButton* m_browse = nullptr;
	QCheckBox* m_noCrypt = nullptr;
};
