#pragma once

#include <QtWidgets/QMainWindow>
#include "SbiePlusAPI.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;

class CBoxImageWindow : public QDialog
{
	Q_OBJECT

public:
	enum EAction {
		eNew,
		eMount,
		eChange,
		eExport,
		eImport
	};

	CBoxImageWindow(EAction Action, QWidget *parent = Q_NULLPTR);
	~CBoxImageWindow();

	QString		GetPassword() const { return m_Password; }
	QString		GetNewPassword() const { return m_NewPassword; }
	void        SetForce(bool force);
	void		SetImageSize(quint64 uSize) const { return ui.txtImageSize->setText(QString::number(uSize / 1024)); }
	quint64		GetImageSize() const { return ui.txtImageSize->text().toULongLong() * 1024; }
	bool		UseProtection() const { return ui.chkProtect->isChecked(); }
	void		SetAutoUnMount(bool bSet) { ui.chkAutoLock->setChecked(bSet); }
	bool        AutoUnMount() const { return ui.chkAutoLock->isChecked(); }

private slots:
	void		OnShowPassword();
	void		OnImageSize();
	void		CheckPassword();

private:
	struct Controls {
		QLabel* lblIcon = nullptr;
		QLabel* lblInfo = nullptr;
		QLabel* lblPassword = nullptr;
		QLabel* lblNewPassword = nullptr;
		QLabel* lblRepeatPassword = nullptr;
		QLabel* lblImageSize = nullptr;
		QLabel* lblImageSizeKb = nullptr;
		QLabel* lblCipher = nullptr;
		QLineEdit* txtPassword = nullptr;
		QLineEdit* txtNewPassword = nullptr;
		QLineEdit* txtRepeatPassword = nullptr;
		QLineEdit* txtImageSize = nullptr;
		QComboBox* cmbCipher = nullptr;
		QCheckBox* chkShow = nullptr;
		QCheckBox* chkProtect = nullptr;
		QCheckBox* chkAutoLock = nullptr;
		QDialogButtonBox* buttonBox = nullptr;
	} ui;

	EAction m_Action;
	QString m_Password;
	QString m_NewPassword;
};
