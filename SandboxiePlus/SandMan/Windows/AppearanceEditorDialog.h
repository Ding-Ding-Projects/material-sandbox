#pragma once

#include <QColor>
#include <QDialog>
#include <QFont>

class QComboBox;
class QFontComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;

// A bounded, native editor for the Material chrome typography and seed color.
// Unsupported Word-style properties remain documented rather than being silently
// dropped; the dialog owns only values the widget stack can apply live.
class CAppearanceEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CAppearanceEditorDialog(const QFont& initialFont, const QColor& initialAccent,
        const QColor& initialTextColor = QColor(), const QColor& initialHighlight = QColor(), QWidget* parent = nullptr);

    QFont selectedFont() const;
    int selectedUnderlineStyle() const;
    QColor selectedAccent() const { return m_accent; }
    QColor selectedTextColor() const { return m_textColor; }
    QColor selectedHighlightColor() const { return m_highlight; }

private slots:
    void updatePreview();
    void chooseAccent();
    void chooseTextColor();
    void chooseHighlightColor();
    void resetToShippedDefaults();

private:
    void setFontControls(const QFont& font);
    void setAccent(const QColor& color);

    QFontComboBox* m_family;
    QSpinBox* m_size;
    QComboBox* m_weight;
    QComboBox* m_style;
    QComboBox* m_underline;
    QComboBox* m_capitalization;
    QCheckBox* m_strikeOut;
    QCheckBox* m_overline;
    QDoubleSpinBox* m_letterSpacing;
    QDoubleSpinBox* m_wordSpacing;
    QPushButton* m_accentButton;
    QPushButton* m_textColorButton;
    QPushButton* m_highlightButton;
    QLabel* m_preview;
    QColor m_accent;
    QColor m_textColor;
    QColor m_highlight;
};
