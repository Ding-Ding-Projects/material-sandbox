#pragma once

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;

class CRenameSandboxDialog : public QDialog
{
	Q_OBJECT

public:
	CRenameSandboxDialog(const QString& boxName, const QString& alias, bool aliasDisabled, bool hasAliasSetting, QWidget* parent = Q_NULLPTR);
	~CRenameSandboxDialog();

	QString GetBoxName() const { return m_boxName->text().trimmed(); }
	QString GetAlias() const { return m_alias->text().trimmed(); }
	bool IsAliasDisabled() const { return m_aliasDisabled->isChecked(); }
	bool IsAliasHidden() const { return m_hideAlias->isChecked(); }

private slots:
	void OnHideAliasToggled(bool checked);
	void OnAliasTextChanged(const QString& text);

private:
	void UpdateFixedHeight();

	void UpdateAliasDisabledState();

	QLineEdit* m_boxName = nullptr;
	QLabel* m_aliasPrompt = nullptr;
	QLineEdit* m_alias = nullptr;
	QCheckBox* m_aliasDisabled = nullptr;
	QCheckBox* m_hideAlias = nullptr;
};
