#include "stdafx.h"
#include "AppearanceEditorDialog.h"
#include "ColorTranslatorDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

namespace {
const QColor kShippedAccent(QStringLiteral("#6750A4"));
const int kShippedPointSize = 0;
const int kShippedWeight = QFont::Normal;
const int kShippedStyle = QFont::StyleNormal;
}

CAppearanceEditorDialog::CAppearanceEditorDialog(const QFont& initialFont, const QColor& initialAccent, QWidget* parent)
    : QDialog(parent), m_family(new QFontComboBox(this)), m_size(new QSpinBox(this)),
      m_weight(new QComboBox(this)), m_style(new QComboBox(this)),
      m_accentButton(new QPushButton(this)), m_preview(new QLabel(this)),
      m_accent(initialAccent.isValid() ? initialAccent : kShippedAccent)
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

    m_accentButton->setText(tr("Choose accent seed"));
    m_accentButton->setAccessibleName(tr("Material accent seed"));
    form->addRow(tr("Accent"), m_accentButton);

    m_preview->setMinimumHeight(80);
    m_preview->setWordWrap(true);
    m_preview->setText(tr("Material 3 live preview\nSandboxie-Plus / 安全隔離工具"));
    m_preview->setAccessibleName(tr("Material appearance live preview"));
    form->addRow(tr("Preview"), m_preview);

    QLabel* unsupported = new QLabel(tr("Not represented by this native slice: variable-font axes, underline variants, strikethrough, overline, capitalization, small caps, superscript, subscript, text effects, character/word spacing, baseline offset, and per-element overrides. These remain visible as an explicit limitation instead of being silently discarded."), this);
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
    connect(m_accentButton, &QPushButton::clicked, this, &CAppearanceEditorDialog::chooseAccent);
    connect(reset, &QPushButton::clicked, this, &CAppearanceEditorDialog::resetToShippedDefaults);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    setTabOrder(m_family, m_size);
    setTabOrder(m_size, m_weight);
    setTabOrder(m_weight, m_style);
    setTabOrder(m_style, m_accentButton);
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
}

void CAppearanceEditorDialog::setAccent(const QColor& color)
{
    m_accent = color.isValid() ? color : kShippedAccent;
    m_accentButton->setStyleSheet(QStringLiteral("background:%1;").arg(m_accent.name(QColor::HexArgb)));
    m_accentButton->setToolTip(tr("Current accent: %1").arg(m_accent.name(QColor::HexArgb).toUpper()));
}

QFont CAppearanceEditorDialog::selectedFont() const
{
    QFont font(m_family->currentFont());
    font.setPointSize(m_size->value());
    font.setWeight(static_cast<QFont::Weight>(m_weight->currentData().toInt()));
    font.setStyle(static_cast<QFont::Style>(m_style->currentData().toInt()));
    return font;
}

void CAppearanceEditorDialog::updatePreview()
{
    QFont font = selectedFont();
    if (m_size->value() == kShippedPointSize) font.setPointSizeF(QApplication::font().pointSizeF());
    m_preview->setFont(font);
    m_preview->setStyleSheet(QStringLiteral("background:%1; color:%2; border:1px solid palette(mid); padding:12px;").arg(m_accent.name(QColor::HexArgb), m_accent.lightness() < 128 ? QStringLiteral("#FFFFFF") : QStringLiteral("#111111")));
}

void CAppearanceEditorDialog::chooseAccent()
{
    CColorTranslatorDialog editor(m_accent, this);
    if (editor.exec() == QDialog::Accepted) setAccent(editor.color());
}

void CAppearanceEditorDialog::resetToShippedDefaults()
{
    QFont shipped = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    shipped.setWeight(static_cast<QFont::Weight>(kShippedWeight));
    shipped.setStyle(static_cast<QFont::Style>(kShippedStyle));
    setFontControls(shipped);
    setAccent(kShippedAccent);
    updatePreview();
}
