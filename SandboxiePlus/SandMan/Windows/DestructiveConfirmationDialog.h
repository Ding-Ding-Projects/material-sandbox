#pragma once

#include <QDialog>
#include <QStringList>

class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSlider;
class QVariantAnimation;

// Native, reusable gate for irreversible actions.  The first integration is
// deliberately limited to the Remove Sandbox command; other destructive
// flows must opt in explicitly after their own unsaved-work review.
class CDestructiveConfirmationDialog : public QDialog
{
	Q_OBJECT

public:
	static bool Confirm(QWidget* parent, const QStringList& sandboxNames);
	static bool ConfirmAction(QWidget* parent, const QString& action, const QStringList& affectedItems);

private:
	explicit CDestructiveConfirmationDialog(QWidget* parent, const QString& action, const QStringList& affectedItems);
	void keyPressEvent(QKeyEvent* event) override;

	void UpdateAuthorizationState();
	void CompleteAuthorization();

	QCheckBox* m_firstKey = nullptr;
	QCheckBox* m_secondKey = nullptr;
	QSlider* m_slider = nullptr;
	QProgressBar* m_progress = nullptr;
	QLabel* m_state = nullptr;
	QLabel* m_completion = nullptr;
	QPushButton* m_confirm = nullptr;
	QPushButton* m_cancel = nullptr;
	QVariantAnimation* m_progressAnimation = nullptr;
	bool m_reducedMotion = false;
	bool m_authorized = false;
};
