#pragma once

#include <QColor>
#include <QDialog>

class QLineEdit;
class QLabel;
class QDialogButtonBox;

class CColorTranslatorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CColorTranslatorDialog(const QColor& initial, QWidget* parent = nullptr);
    QColor color() const { return m_color; }

private slots:
    void updateFromHex();
    void updateFromRgb();
    void updateFromHsl();

private:
    void setColor(const QColor& color);
    void updateFields();
    void updateContrast();
    void setInputValid(bool valid, const QString& message = QString());
    static double contrastRatio(const QColor& first, const QColor& second);

    QColor m_color;
    QLineEdit* m_hex;
    QLineEdit* m_rgb;
    QLineEdit* m_hsl;
    QLabel* m_preview;
    QLabel* m_contrast;
    QLabel* m_error;
    QDialogButtonBox* m_buttons;
};
