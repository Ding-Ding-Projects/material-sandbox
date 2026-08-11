#pragma once

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QRegularExpression>
#include <QString>

class QBoxLayout;
class QEvent;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QToolButton;
class QWidget;

class CRegexBuilderDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit CRegexBuilderDialog(QWidget* parent = nullptr);

    void setState(const QString& plainText,
                  const QString& pattern,
                  const QString& flags,
                  bool regexMode);
    void openAnchored(QWidget* origin, QWidget* focusReturnTarget = nullptr);
    int execAnchored(QWidget* origin, QWidget* focusReturnTarget = nullptr);

    QString pattern() const;
    QString flags() const;
    QString sampleText() const;
    bool isPatternValid() const;
    QString patternError() const;

    static QRegularExpression compile(const QString& pattern,
                                      const QString& flags,
                                      QString* error = nullptr);

signals:
    void patternApplied(QString pattern, QString flags);
    void plainTextRequested(QString text);

private slots:
    void updatePreview();
    void applyPattern();
    void keepPlainText();
    void insertToken();

protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void positionBesideOrigin();
    void restoreOriginFocus();
    void updateResponsiveLayout(int availableWidth = -1);
    void reflowGuidedTokens(int availableWidth = -1);
    void watchOriginGeometry();
    void clearOriginGeometryWatchers();
    void scheduleReposition();
    void announceValidation(const QString& text);
    void announceInputRejection(const QString& message);
    QString previewText(const QRegularExpression& expression) const;

    QPointer<QWidget> m_origin;
    QPointer<QWidget> m_focusReturnTarget;
    QLineEdit* m_patternEdit;
    QLineEdit* m_flagsEdit;
    QPlainTextEdit* m_sampleEdit;
    QPlainTextEdit* m_previewEdit;
    QLabel* m_validationLabel;
    QGridLayout* m_tokenGrid;
    QBoxLayout* m_fieldsLayout;
    QBoxLayout* m_actionsLayout;
    QList<QToolButton*> m_tokenButtons;
    QPushButton* m_cancelButton;
    QPushButton* m_plainButton;
    QPushButton* m_applyButton;
    QList<QPointer<QWidget>> m_originWatchers;
    QString m_error;
    QString m_lastAnnouncedValidation;
    bool m_valid;
    bool m_previewSafe;
    bool m_restoreOriginFocus;
    bool m_repositionScheduled;
    bool m_positioning;
    bool m_reflowingTokens;
    bool m_userResized;
    bool m_fullscreenFallback;
};
