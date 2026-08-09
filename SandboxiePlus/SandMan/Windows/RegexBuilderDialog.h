#pragma once

#include <QDialog>
#include <QPointer>
#include <QRegularExpression>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
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
    void openAnchored(QWidget* origin);

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
    void addGuidedToken();
    void updatePreview();
    void keepPlainText();
    void applyPattern();
    void limitSampleText();

private:
    void positionBesideOrigin();
    void restoreOriginFocus();
    QString previewText(const QRegularExpression& expression) const;

    QLineEdit* m_patternEdit;
    QLineEdit* m_flagsEdit;
    QPlainTextEdit* m_sampleEdit;
    QLabel* m_validationLabel;
    QPlainTextEdit* m_previewEdit;
    QPushButton* m_applyButton;
    QPointer<QWidget> m_origin;
    QString m_error;
    bool m_valid;
};
