#pragma once

#include <QDialog>
#include <QPointer>
#include <QRegularExpression>
#include <QString>

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
    int execAnchored(QWidget* origin);

signals:
    void patternApplied(QString pattern, QString flags);
    void plainTextRequested(QString text);

private slots:
    void updatePreview();
    void applyPattern();
    void keepPlainText();
    void insertToken();

private:
    void positionBesideOrigin();
    void restoreOriginFocus();
    static QRegularExpression compile(const QString& pattern, const QString& flags, QString* error);

    QPointer<QWidget> m_origin;
    QLineEdit* m_patternEdit;
    QLineEdit* m_flagsEdit;
    QPlainTextEdit* m_sampleEdit;
    QPlainTextEdit* m_previewEdit;
    QLabel* m_validationLabel;
    QPushButton* m_applyButton;
};
