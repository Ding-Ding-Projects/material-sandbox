#include "stdafx.h"
#include "RenameSandboxDialog.h"
#include "../MiscHelpers/Common/Settings.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

CRenameSandboxDialog::CRenameSandboxDialog(const QString& boxName, const QString& alias, bool aliasDisabled, bool hasAliasSetting, QWidget* parent)
	: QDialog(parent)
{
	Q_UNUSED(hasAliasSetting);

	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::CustomizeWindowHint;
	flags &= ~Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags);

	setWindowTitle(tr("Rename sandbox"));
	auto* form = new QFormLayout(this);
	m_boxName = new QLineEdit(this);
	m_boxName->setAccessibleName(tr("Sandbox name"));
	form->addRow(tr("Name"), m_boxName);
	m_aliasPrompt = new QLabel(tr("Alias"), this);
	m_alias = new QLineEdit(this);
	m_alias->setAccessibleName(tr("Sandbox alias"));
	form->addRow(m_aliasPrompt, m_alias);
	m_aliasDisabled = new QCheckBox(tr("Disable alias display"), this);
	m_aliasDisabled->setAccessibleName(tr("Disable alias display"));
	form->addRow(QString(), m_aliasDisabled);
	m_hideAlias = new QCheckBox(tr("Hide alias input"), this);
	m_hideAlias->setAccessibleName(tr("Hide alias input"));
	form->addRow(QString(), m_hideAlias);
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	form->addRow(QString(), buttons);

	m_boxName->setText(boxName);
	m_alias->setText(alias);
	m_aliasDisabled->setChecked(aliasDisabled);

	m_aliasDisabled->setToolTip(tr("When enabled, alias display is disabled for this sandbox."));
	m_hideAlias->setToolTip(tr("Hide alias input in this dialog. This preference is remembered."));

	const bool hideAlias = theConf->GetBool("Options/HideAliasInput", true);
	m_hideAlias->setChecked(hideAlias);

	connect(m_hideAlias, &QCheckBox::toggled, this, &CRenameSandboxDialog::OnHideAliasToggled);
	connect(m_alias, &QLineEdit::textChanged, this, &CRenameSandboxDialog::OnAliasTextChanged);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	UpdateAliasDisabledState();
	OnHideAliasToggled(hideAlias);
	UpdateFixedHeight();
}

CRenameSandboxDialog::~CRenameSandboxDialog()
{
	theConf->SetValue("Options/HideAliasInput", IsAliasHidden());
}

void CRenameSandboxDialog::OnHideAliasToggled(bool checked)
{
	m_aliasPrompt->setVisible(!checked);
	m_alias->setVisible(!checked);
	layout()->activate();
	UpdateFixedHeight();
}

void CRenameSandboxDialog::OnAliasTextChanged(const QString& text)
{
	Q_UNUSED(text);
	UpdateAliasDisabledState();
}

void CRenameSandboxDialog::UpdateAliasDisabledState()
{
	const bool hasAliasText = !m_alias->text().trimmed().isEmpty();
	m_aliasDisabled->setEnabled(hasAliasText);
	if (!hasAliasText)
		m_aliasDisabled->setChecked(false);
}

void CRenameSandboxDialog::UpdateFixedHeight()
{
	const int h = sizeHint().height();
	if (height() != h)
		resize(width(), h);
	setMinimumHeight(h);
	setMaximumHeight(h);
}
