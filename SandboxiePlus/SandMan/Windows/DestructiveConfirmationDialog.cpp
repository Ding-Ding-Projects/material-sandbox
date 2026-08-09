#include "stdafx.h"
#include "DestructiveConfirmationDialog.h"
#include "M3DialogHost.h"

#include "../MiscHelpers/Common/Settings.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSlider>
#include <QVariantAnimation>
#include <QVBoxLayout>

CDestructiveConfirmationDialog::CDestructiveConfirmationDialog(QWidget* parent, const QString& action, const QStringList& affectedItems)
	: QDialog(parent)
{
	setWindowTitle(tr("Confirm irreversible action: %1").arg(action));
	setModal(true);
	setMinimumWidth(520);
	setAttribute(Qt::WA_DeleteOnClose, false);

	// This preference is intentionally read-only here.  The gate remains
	// complete with reduced motion: progress still reports exact state, while
	// the visual pulses are replaced with immediate state changes.
	m_reducedMotion = theConf && theConf->GetBool("UIConfig/ReducedMotion", false);

	auto* layout = new QVBoxLayout(this);
	auto* title = new QLabel(tr("This action is irreversible and will permanently change local data."), this);
	title->setWordWrap(true);
	title->setStyleSheet(QStringLiteral("font-weight: 700; font-size: 15px;"));
	layout->addWidget(title);

	auto* target = new QLabel(tr("Action: %1\nAffected item(s): %2").arg(action, affectedItems.join(tr(", "))), this);
	target->setWordWrap(true);
	target->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(target);

	m_firstKey = new QCheckBox(tr("I understand that the selected data will be permanently changed or deleted."), this);
	m_firstKey->setObjectName(QStringLiteral("destructiveKeyContent"));
	m_firstKey->setAccessibleName(tr("Confirm permanent data change"));
	layout->addWidget(m_firstKey);

	m_secondKey = new QCheckBox(tr("I confirm that this action applies only to the items listed above."), this);
	m_secondKey->setObjectName(QStringLiteral("destructiveKeyTarget"));
	m_secondKey->setAccessibleName(tr("Confirm affected action targets"));
	layout->addWidget(m_secondKey);

	auto* instruction = new QLabel(tr("Move the confirmation slider across its full range to authorize this action."), this);
	instruction->setWordWrap(true);
	layout->addWidget(instruction);

	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setObjectName(QStringLiteral("destructiveConfirmationSlider"));
	m_slider->setRange(0, 100);
	m_slider->setValue(0);
	m_slider->setEnabled(false);
	m_slider->setAccessibleName(tr("Full-range destructive action confirmation"));
	m_slider->setAccessibleDescription(tr("Both confirmation checkboxes must be selected before this slider can authorize removal."));
	layout->addWidget(m_slider);

	m_progress = new QProgressBar(this);
	m_progress->setObjectName(QStringLiteral("destructiveConfirmationProgress"));
	m_progress->setRange(0, 100);
	m_progress->setValue(0);
	m_progress->setTextVisible(true);
	m_progress->setFormat(tr("Authorization progress: %p%"));
	layout->addWidget(m_progress);

	m_state = new QLabel(tr("Authorization is waiting for both confirmations."), this);
	m_state->setWordWrap(true);
	m_state->setAccessibleName(tr("Authorization status"));
	layout->addWidget(m_state);

	m_completion = new QLabel(tr("Authorization complete. The removal is ready to run."), this);
	m_completion->setObjectName(QStringLiteral("destructiveConfirmationComplete"));
	m_completion->setWordWrap(true);
	m_completion->setVisible(false);
	layout->addWidget(m_completion);

	auto* buttons = new QDialogButtonBox(this);
	m_cancel = buttons->addButton(tr("Emergency exit"), QDialogButtonBox::RejectRole);
	m_cancel->setObjectName(QStringLiteral("destructiveEmergencyExit"));
	m_cancel->setToolTip(tr("Cancel safely without removing anything (Escape)"));
	m_confirm = buttons->addButton(tr("Authorize action"), QDialogButtonBox::AcceptRole);
	m_confirm->setObjectName(QStringLiteral("destructiveAuthorize"));
	m_confirm->setEnabled(false);
	m_confirm->setToolTip(tr("Select both confirmations and complete the slider first"));
	layout->addWidget(buttons);

	m_progressAnimation = new QVariantAnimation(this);
	m_progressAnimation->setDuration(m_reducedMotion ? 0 : 220);
	connect(m_progressAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
		m_progress->setValue(value.toInt());
	});
	connect(m_firstKey, &QCheckBox::toggled, this, [this](bool) { UpdateAuthorizationState(); });
	connect(m_secondKey, &QCheckBox::toggled, this, [this](bool) { UpdateAuthorizationState(); });
	connect(m_slider, &QSlider::valueChanged, this, [this](int value) {
		m_progressAnimation->stop();
		m_progressAnimation->setStartValue(m_progress->value());
		m_progressAnimation->setEndValue(value);
		m_progressAnimation->start();
		if (value >= 100 && m_firstKey->isChecked() && m_secondKey->isChecked())
			CompleteAuthorization();
		else {
			m_authorized = false;
			m_confirm->setEnabled(false);
			m_completion->setVisible(false);
			m_state->setText(tr("Authorization progress is %1%. Complete the full range.").arg(value));
		}
	});
	connect(m_confirm, &QPushButton::clicked, this, &QDialog::accept);
	connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	setTabOrder(m_firstKey, m_secondKey);
	setTabOrder(m_secondKey, m_slider);
	setTabOrder(m_slider, m_cancel);
	setTabOrder(m_cancel, m_confirm);
	m_cancel->setDefault(true);
	resize(sizeHint());
	M3DialogHost::Install(this);
}

bool CDestructiveConfirmationDialog::Confirm(QWidget* parent, const QStringList& sandboxNames)
{
	return ConfirmAction(parent, QObject::tr("Remove sandbox content"), sandboxNames);
}

bool CDestructiveConfirmationDialog::ConfirmAction(QWidget* parent, const QString& action, const QStringList& affectedItems)
{
	CDestructiveConfirmationDialog dialog(parent, action, affectedItems);
	QWidget* origin = parent ? parent->focusWidget() : nullptr;
	const bool confirmed = dialog.exec() == QDialog::Accepted && dialog.m_authorized;
	if (origin)
		origin->setFocus(Qt::OtherFocusReason);
	return confirmed;
}

void CDestructiveConfirmationDialog::UpdateAuthorizationState()
{
	const bool ready = m_firstKey->isChecked() && m_secondKey->isChecked();
	m_slider->setEnabled(ready && !m_authorized);
	if (!ready) {
		m_slider->setValue(0);
		m_progress->setValue(0);
		m_authorized = false;
		m_confirm->setEnabled(false);
		m_completion->setVisible(false);
		m_state->setText(tr("Authorization is waiting for both confirmations."));
	} else if (!m_authorized) {
		m_state->setText(tr("Both confirmations are present. Complete the full-range slider."));
	}
}

void CDestructiveConfirmationDialog::CompleteAuthorization()
{
	m_authorized = true;
	m_slider->setEnabled(false);
	m_confirm->setEnabled(true);
	m_state->setText(tr("Authorization complete; the action is ready to run."));
	m_completion->setVisible(true);

	if (m_reducedMotion) {
		return;
	}
	auto* effect = new QGraphicsOpacityEffect(m_completion);
	m_completion->setGraphicsEffect(effect);
	effect->setOpacity(0.0);
	auto* animation = new QPropertyAnimation(effect, "opacity", effect);
	animation->setDuration(260);
	animation->setStartValue(0.0);
	animation->setEndValue(1.0);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void CDestructiveConfirmationDialog::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape) {
		reject();
		return;
	}
	QDialog::keyPressEvent(event);
}
