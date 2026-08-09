#include "stdafx.h"
#include "AppearanceEditorDialog.h"
#include "ColorTranslatorDialog.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>

namespace {
const QColor kShippedAccent(QStringLiteral("#6750A4"));
const int kShippedPointSize = 0;
const int kShippedWeight = QFont::Normal;
const int kShippedStyle = QFont::StyleNormal;
}

CAppearanceEditorDialog::CAppearanceEditorDialog(const QFont& initialFont, const QColor& initialAccent,
    const QColor& initialTextColor, const QColor& initialHighlight, QWidget* parent)
    : QDialog(parent), m_family(new QFontComboBox(this)), m_size(new QSpinBox(this)),
      m_weight(new QComboBox(this)), m_style(new QComboBox(this)),
      m_underline(new QComboBox(this)), m_capitalization(new QComboBox(this)),
      m_strikeOut(new QCheckBox(tr("Strikeout"), this)), m_overline(new QCheckBox(tr("Overline"), this)),
      m_letterSpacing(new QDoubleSpinBox(this)), m_wordSpacing(new QDoubleSpinBox(this)),
      m_accentButton(new QPushButton(this)), m_textColorButton(new QPushButton(this)),
      m_highlightButton(new QPushButton(this)), m_preview(new QLabel(this)),
      m_accent(initialAccent.isValid() ? initialAccent : kShippedAccent),
      m_textColor(initialTextColor.isValid() ? initialTextColor : QColor(Qt::black)),
      m_highlight(initialHighlight.isValid() ? initialHighlight : QColor(Qt::transparent))
{
    setWindowTitle(tr("Material appearance editor"));
    setModal(true);
    resize(560, 420);

    QFormLayout* form = new QFormLayout(this);
    QLabel* intro = new QLabel(tr("Live preview applies to Material typography and the accent seed. Font family choices come from installed fonts; unsupported Word-style properties are intentionally not hidden and are documented below."), this);
    intro->setWordWrap(true);
    intro->setProperty("secondary", true);
    form->addRow(QString(), intro);

    m_family->setFontFilters(QFontComboBox::ScalableFonts | QFontComboBox::MonospacedFonts);
    m_family->setAccessibleName(tr("UI font family"));
    form->addRow(tr("Family"), m_family);

    m_size->setRange(8, 72);
    m_size->setSpecialValueText(tr("Use application default"));
    m_size->setSuffix(tr(" pt"));
    m_size->setAccessibleName(tr("UI font size"));
    form->addRow(tr("Size"), m_size);

    const QList<QPair<QString, int>> weights = {
        {tr("Thin"), QFont::Thin}, {tr("Extra light"), QFont::ExtraLight},
        {tr("Light"), QFont::Light}, {tr("Normal"), QFont::Normal},
        {tr("Medium"), QFont::Medium}, {tr("Demi bold"), QFont::DemiBold},
        {tr("Bold"), QFont::Bold}, {tr("Extra bold"), QFont::ExtraBold},
        {tr("Black"), QFont::Black}
    };
    for (const auto& weight : weights) m_weight->addItem(weight.first, weight.second);
    m_weight->setAccessibleName(tr("UI font weight"));
    form->addRow(tr("Weight"), m_weight);

    m_style->addItem(tr("Normal"), QFont::StyleNormal);
    m_style->addItem(tr("Italic"), QFont::StyleItalic);
    m_style->addItem(tr("Oblique"), QFont::StyleOblique);
    m_style->setAccessibleName(tr("UI font style"));
    form->addRow(tr("Style"), m_style);

    const QList<QPair<QString, int>> underlines = {
        {tr("None"), 0}, {tr("Single"), 1}
    };
    for (const auto& value : underlines) m_underline->addItem(value.first, value.second);
    m_underline->setAccessibleName(tr("Underline style"));
    form->addRow(tr("Underline (single line)"), m_underline);
    m_strikeOut->setAccessibleName(tr("Strikeout"));
    m_overline->setAccessibleName(tr("Overline"));
    QHBoxLayout* decorations = new QHBoxLayout();
    decorations->addWidget(m_strikeOut); decorations->addWidget(m_overline); decorations->addStretch();
    form->addRow(tr("Decorations"), decorations);

    m_capitalization->addItem(tr("Mixed case"), QFont::MixedCase);
    m_capitalization->addItem(tr("Small caps"), QFont::SmallCaps);
    m_capitalization->addItem(tr("All uppercase"), QFont::AllUppercase);
    m_capitalization->addItem(tr("All lowercase"), QFont::AllLowercase);
    m_capitalization->addItem(tr("Capitalize words"), QFont::Capitalize);
    m_capitalization->setAccessibleName(tr("Capitalization"));
    form->addRow(tr("Capitalization"), m_capitalization);
    for (QDoubleSpinBox* spin : {m_letterSpacing, m_wordSpacing}) {
        spin->setRange(-20.0, 100.0); spin->setDecimals(1); spin->setSingleStep(0.5); spin->setSuffix(tr(" px"));
    }
    m_letterSpacing->setAccessibleName(tr("Letter spacing"));
    m_wordSpacing->setAccessibleName(tr("Word spacing"));
    form->addRow(tr("Letter spacing"), m_letterSpacing);
    form->addRow(tr("Word spacing"), m_wordSpacing);

    m_accentButton->setText(tr("Choose accent seed"));
    m_accentButton->setAccessibleName(tr("Material accent seed"));
    form->addRow(tr("Accent"), m_accentButton);
    m_textColorButton->setText(tr("Choose text color"));
    m_textColorButton->setAccessibleName(tr("Preview text color"));
    form->addRow(tr("Text color"), m_textColorButton);
    m_highlightButton->setText(tr("Choose highlight"));
    m_highlightButton->setAccessibleName(tr("Preview highlight color"));
    form->addRow(tr("Highlight"), m_highlightButton);

    m_preview->setMinimumHeight(80);
    m_preview->setWordWrap(true);
    m_preview->setText(tr("Material 3 live preview\nSandboxie-Plus / 安全隔離工具"));
    m_preview->setAccessibleName(tr("Material appearance live preview"));
    form->addRow(tr("Preview"), m_preview);

    QLabel* unsupported = new QLabel(tr("Not represented by this global native slice: line-height, baseline offset, superscript, subscript, rich underline patterns, text effects, and per-element overrides. Line-height, baseline, superscript, and subscript controls are intentionally not fabricated because Qt's application font cannot apply them consistently to every widget. Qt 6.8 can represent font-specific variable axes, but this global editor does not expose or persist them until they are verified across application widget styles. Per-page font overrides are available from a tab's Edit tab page typography action and are persisted for that target."), this);
    unsupported->setWordWrap(true);
    unsupported->setProperty("secondary", true);
    form->addRow(tr("Unsupported properties"), unsupported);

    QHBoxLayout* actions = new QHBoxLayout();
    QPushButton* reset = new QPushButton(tr("Reset to shipped defaults"), this);
    actions->addWidget(reset);
    actions->addStretch();
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    actions->addWidget(buttons);
    form->addRow(QString(), actions);

    setFontControls(initialFont);
    setAccent(m_accent);
    connect(m_family, &QFontComboBox::currentFontChanged, this, &CAppearanceEditorDialog::updatePreview);
    connect(m_size, qOverload<int>(&QSpinBox::valueChanged), this, &CAppearanceEditorDialog::updatePreview);
    connect(m_weight, qOverload<int>(&QComboBox::currentIndexChanged), this, &CAppearanceEditorDialog::updatePreview);
    connect(m_style, qOverload<int>(&QComboBox::currentIndexChanged), this, &CAppearanceEditorDialog::updatePreview);
    connect(m_underline, qOverload<int>(&QComboBox::currentIndexChanged), this, &CAppearanceEditorDialog::updatePreview);
    connect(m_capitalization, qOverload<int>(&QComboBox::currentIndexChanged), this, &CAppearanceEditorDialog::updatePreview);
    connect(m_strikeOut, &QCheckBox::toggled, this, &CAppearanceEditorDialog::updatePreview);
    connect(m_overline, &QCheckBox::toggled, this, &CAppearanceEditorDialog::updatePreview);
    connect(m_letterSpacing, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CAppearanceEditorDialog::updatePreview);
    connect(m_wordSpacing, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CAppearanceEditorDialog::updatePreview);
    connect(m_accentButton, &QPushButton::clicked, this, &CAppearanceEditorDialog::chooseAccent);
    connect(m_textColorButton, &QPushButton::clicked, this, &CAppearanceEditorDialog::chooseTextColor);
    connect(m_highlightButton, &QPushButton::clicked, this, &CAppearanceEditorDialog::chooseHighlightColor);
    connect(reset, &QPushButton::clicked, this, &CAppearanceEditorDialog::resetToShippedDefaults);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    setTabOrder(m_family, m_size);
    setTabOrder(m_size, m_weight);
    setTabOrder(m_weight, m_style);
    setTabOrder(m_style, m_underline);
    setTabOrder(m_underline, m_capitalization);
    setTabOrder(m_capitalization, m_letterSpacing);
    setTabOrder(m_letterSpacing, m_wordSpacing);
    setTabOrder(m_wordSpacing, m_accentButton);
    setTabOrder(m_accentButton, reset);
    setTabOrder(reset, buttons->button(QDialogButtonBox::Ok));
    updatePreview();
}

void CAppearanceEditorDialog::setFontControls(const QFont& font)
{
    const QString family = font.family().isEmpty() ? QFontDatabase::systemFont(QFontDatabase::GeneralFont).family() : font.family();
    const int familyIndex = m_family->findText(family, Qt::MatchFixedString);
    if (familyIndex >= 0) m_family->setCurrentIndex(familyIndex);
    m_size->setValue(font.pointSizeF() > 0.0 ? qRound(font.pointSizeF()) : kShippedPointSize);
    const int weightIndex = m_weight->findData(font.weight());
    if (weightIndex >= 0) m_weight->setCurrentIndex(weightIndex);
    const int styleIndex = m_style->findData(font.style());
    if (styleIndex >= 0) m_style->setCurrentIndex(styleIndex);
    const int underlineIndex = m_underline->findData(font.underline());
    if (underlineIndex >= 0) m_underline->setCurrentIndex(underlineIndex);
    const int capitalizationIndex = m_capitalization->findData(font.capitalization());
    if (capitalizationIndex >= 0) m_capitalization->setCurrentIndex(capitalizationIndex);
    m_strikeOut->setChecked(font.strikeOut());
    m_overline->setChecked(font.overline());
    m_letterSpacing->setValue(font.letterSpacing());
    m_wordSpacing->setValue(font.wordSpacing());
}

void CAppearanceEditorDialog::setAccent(const QColor& color)
{
    m_accent = color.isValid() ? color : kShippedAccent;
    m_accentButton->setStyleSheet(QStringLiteral("background:%1;").arg(m_accent.name(QColor::HexArgb)));
    m_accentButton->setToolTip(tr("Current accent: %1").arg(m_accent.name(QColor::HexArgb).toUpper()));
    m_textColorButton->setStyleSheet(QStringLiteral("background:%1;").arg(m_textColor.name(QColor::HexArgb)));
    m_highlightButton->setStyleSheet(QStringLiteral("background:%1;").arg(m_highlight.name(QColor::HexArgb)));
}

QFont CAppearanceEditorDialog::selectedFont() const
{
    QFont font(m_family->currentFont());
    font.setPointSize(m_size->value());
    font.setWeight(static_cast<QFont::Weight>(m_weight->currentData().toInt()));
    font.setStyle(static_cast<QFont::Style>(m_style->currentData().toInt()));
    // Qt 6 exposes only a boolean QFont underline; retain the richer style
    // identifier in the editor/configuration while applying its enabled state.
    font.setUnderline(m_underline->currentData().toInt() != 0);
    font.setStrikeOut(m_strikeOut->isChecked());
    font.setOverline(m_overline->isChecked());
    font.setCapitalization(static_cast<QFont::Capitalization>(m_capitalization->currentData().toInt()));
    font.setLetterSpacing(QFont::AbsoluteSpacing, m_letterSpacing->value());
    font.setWordSpacing(m_wordSpacing->value());
    return font;
}

int CAppearanceEditorDialog::selectedUnderlineStyle() const
{
    return m_underline->currentData().toInt();
}

void CAppearanceEditorDialog::setUnderlineStyle(int style)
{
    const int index = m_underline->findData(qBound(0, style, 1));
    if (index >= 0)
        m_underline->setCurrentIndex(index);
}

void CAppearanceEditorDialog::updatePreview()
{
    QFont font = selectedFont();
    if (m_size->value() == kShippedPointSize) font.setPointSizeF(QApplication::font().pointSizeF());
    m_preview->setFont(font);
    const QString text = m_textColor.name(QColor::HexArgb);
    const QString highlight = m_highlight.alpha() == 0 ? QStringLiteral("transparent") : m_highlight.name(QColor::HexArgb);
    m_preview->setStyleSheet(QStringLiteral("background:%1; color:%2; border:1px solid palette(mid); padding:12px; selection-background-color:%3;").arg(m_accent.name(QColor::HexArgb), text, highlight));
}

void CAppearanceEditorDialog::chooseAccent()
{
    CColorTranslatorDialog editor(m_accent, this);
    if (editor.exec() == QDialog::Accepted) setAccent(editor.color());
}

void CAppearanceEditorDialog::chooseTextColor()
{
    CColorTranslatorDialog editor(m_textColor, this);
    if (editor.exec() == QDialog::Accepted) { m_textColor = editor.color(); setAccent(m_accent); updatePreview(); }
}

void CAppearanceEditorDialog::chooseHighlightColor()
{
    CColorTranslatorDialog editor(m_highlight.isValid() ? m_highlight : QColor(Qt::transparent), this);
    if (editor.exec() == QDialog::Accepted) { m_highlight = editor.color(); setAccent(m_accent); updatePreview(); }
}

void CAppearanceEditorDialog::resetToShippedDefaults()
{
    QFont shipped = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    shipped.setWeight(static_cast<QFont::Weight>(kShippedWeight));
    shipped.setStyle(static_cast<QFont::Style>(kShippedStyle));
    setFontControls(shipped);
    m_textColor = QColor(Qt::black);
    m_highlight = QColor(Qt::transparent);
    setAccent(kShippedAccent);
    updatePreview();
}
