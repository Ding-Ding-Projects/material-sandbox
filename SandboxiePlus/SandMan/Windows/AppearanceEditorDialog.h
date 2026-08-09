#pragma once

#include <QColor>
#include <QDialog>
#include <QFont>

class QComboBox;
class QFontComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

// A bounded, native editor for the Material chrome typography and seed color.
// Unsupported Word-style properties remain documented rather than being silently
// dropped; the dialog owns only values the widget stack can apply live.
class CAppearanceEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CAppearanceEditorDialog(const QFont& initialFont, const QColor& initialAccent, QWidget* parent = nullptr);

    QFont selectedFont() const;
    QColor selectedAccent() const { return m_accent; }

private slots:
    void updatePreview();
    void chooseAccent();
    void resetToShippedDefaults();

private:
    void setFontControls(const QFont& font);
    void setAccent(const QColor& color);

    QFontComboBox* m_family;
    QSpinBox* m_size;
    QComboBox* m_weight;
    QComboBox* m_style;
    QPushButton* m_accentButton;
    QLabel* m_preview;
    QColor m_accent;
};
