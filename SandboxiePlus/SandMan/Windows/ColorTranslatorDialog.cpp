#include "stdafx.h"
#include "ColorTranslatorDialog.h"

#include <algorithm>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QtMath>

namespace {
double Channel(double value)
{
    value /= 255.0;
    return value <= 0.03928 ? value / 12.92 : qPow((value + 0.055) / 1.055, 2.4);
}
}

CColorTranslatorDialog::CColorTranslatorDialog(const QColor& initial, QWidget* parent)
    : QDialog(parent), m_color(initial.isValid() ? initial : QColor(QStringLiteral("#6750A4")))
{
    setWindowTitle(tr("Material accent color"));
    QFormLayout* form = new QFormLayout(this);
    m_preview = new QLabel(this);
    m_preview->setMinimumHeight(42);
    m_preview->setAccessibleName(tr("Accent color preview"));
    form->addRow(tr("Preview"), m_preview);
    m_hex = new QLineEdit(this);
    m_hex->setMaxLength(9);
    m_hex->setAccessibleName(tr("HEX color"));
    form->addRow(tr("HEX / HEX8"), m_hex);
    m_rgb = new QLineEdit(this);
    m_rgb->setPlaceholderText(QStringLiteral("r, g, b[, a]"));
    m_rgb->setAccessibleName(tr("RGB color"));
    form->addRow(tr("RGB / RGBA"), m_rgb);
    m_hsl = new QLineEdit(this);
    m_hsl->setPlaceholderText(QStringLiteral("h, s%, l%[, a%]"));
    m_hsl->setAccessibleName(tr("HSL color"));
    form->addRow(tr("HSL / HSLA"), m_hsl);
    m_contrast = new QLabel(this);
    m_contrast->setWordWrap(true);
    form->addRow(tr("Contrast"), m_contrast);
    QLabel* help = new QLabel(tr("Edit any representation. Values are converted in both directions; alpha is preserved."), this);
    help->setWordWrap(true);
    form->addRow(QString(), help);
    m_error = new QLabel(this);
    m_error->setAccessibleName(tr("Color validation status"));
    m_error->setStyleSheet(QStringLiteral("color:#B3261E;"));
    m_error->setWordWrap(true);
    form->addRow(QString(), m_error);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    form->addRow(QString(), m_buttons);
    connect(m_hex, &QLineEdit::editingFinished, this, &CColorTranslatorDialog::updateFromHex);
    connect(m_rgb, &QLineEdit::editingFinished, this, &CColorTranslatorDialog::updateFromRgb);
    connect(m_hsl, &QLineEdit::editingFinished, this, &CColorTranslatorDialog::updateFromHsl);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_buttons->button(QDialogButtonBox::Ok)->setDefault(true);
    setTabOrder(m_hex, m_rgb);
    setTabOrder(m_rgb, m_hsl);
    setTabOrder(m_hsl, m_buttons->button(QDialogButtonBox::Ok));
    m_hex->setFocus();
    updateFields();
}

void CColorTranslatorDialog::setColor(const QColor& color)
{
    if (!color.isValid())
        return;
    m_color = color;
    updateFields();
}

void CColorTranslatorDialog::updateFields()
{
    const QSignalBlocker hexBlocker(m_hex);
    const QSignalBlocker rgbBlocker(m_rgb);
    const QSignalBlocker hslBlocker(m_hsl);
    m_hex->setText(m_color.name(QColor::HexArgb).toUpper());
    m_rgb->setText(QStringLiteral("%1, %2, %3, %4").arg(m_color.red()).arg(m_color.green()).arg(m_color.blue()).arg(m_color.alpha()));
    const double hue = m_color.hslHueF() < 0 ? 0.0 : m_color.hslHueF();
    m_hsl->setText(QStringLiteral("%1, %2%, %3%, %4%").arg(qRound(hue * 360.0)).arg(qRound(m_color.hslSaturationF() * 100.0)).arg(qRound(m_color.lightnessF() * 100.0)).arg(qRound(m_color.alphaF() * 100.0)));
    m_preview->setStyleSheet(QStringLiteral("background:%1; border:1px solid palette(mid);").arg(m_color.name(QColor::HexArgb)));
    setInputValid(true);
    updateContrast();
}

void CColorTranslatorDialog::setInputValid(bool valid, const QString& message)
{
    m_error->setText(valid ? QString() : message);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

void CColorTranslatorDialog::updateFromHex()
{
    const QColor candidate(m_hex->text().trimmed());
    if (candidate.isValid()) setColor(candidate);
    else setInputValid(false, tr("Enter a valid HEX or HEX8 color."));
}

void CColorTranslatorDialog::updateFromRgb()
{
    const QStringList values = m_rgb->text().split(QRegularExpression(QStringLiteral("\\s*,\\s*")), Qt::SkipEmptyParts);
    if (values.size() < 3 || values.size() > 4) { setInputValid(false, tr("RGB needs three channels and optional alpha.")); return; }
    bool ok[4] = { false, false, false, true };
    int channels[4] = { values[0].toInt(&ok[0]), values[1].toInt(&ok[1]), values[2].toInt(&ok[2]), values.size() == 4 ? values[3].toInt(&ok[3]) : 255 };
    if (ok[0] && ok[1] && ok[2] && ok[3] && std::all_of(channels, channels + 4, [](int value) { return value >= 0 && value <= 255; }))
        setColor(QColor(channels[0], channels[1], channels[2], channels[3]));
    else setInputValid(false, tr("RGB channels must be integers from 0 to 255."));
}

void CColorTranslatorDialog::updateFromHsl()
{
    const QStringList values = m_hsl->text().split(QRegularExpression(QStringLiteral("\\s*,\\s*")), Qt::SkipEmptyParts);
    if (values.size() < 3 || values.size() > 4) { setInputValid(false, tr("HSL needs hue, saturation, lightness, and optional alpha.")); return; }
    if (!values[1].trimmed().endsWith('%') || !values[2].trimmed().endsWith('%') || (values.size() == 4 && !values[3].trimmed().endsWith('%'))) { setInputValid(false, tr("HSL saturation, lightness, and alpha must use percent units.")); return; }
    bool ok[4] = { false, false, false, true };
    const double h = values[0].toDouble(&ok[0]);
    const auto parsePercent = [](const QString& value, bool* ok) {
        QString normalized = value.trimmed();
        normalized.remove('%');
        return normalized.toDouble(ok);
    };
    const double s = parsePercent(values[1], &ok[1]);
    const double l = parsePercent(values[2], &ok[2]);
    const double a = values.size() == 4 ? parsePercent(values[3], &ok[3]) : 100.0;
    if (ok[0] && ok[1] && ok[2] && ok[3] && h >= 0 && h <= 360 && s >= 0 && s <= 100 && l >= 0 && l <= 100 && a >= 0 && a <= 100)
        setColor(QColor::fromHslF(h / 360.0, s / 100.0, l / 100.0, a / 100.0));
    else setInputValid(false, tr("HSL hue must be 0–360; other channels must be 0–100%."));
}

double CColorTranslatorDialog::contrastRatio(const QColor& first, const QColor& second)
{
    const double firstLum = 0.2126 * Channel(first.red()) + 0.7152 * Channel(first.green()) + 0.0722 * Channel(first.blue());
    const double secondLum = 0.2126 * Channel(second.red()) + 0.7152 * Channel(second.green()) + 0.0722 * Channel(second.blue());
    return (qMax(firstLum, secondLum) + 0.05) / (qMin(firstLum, secondLum) + 0.05);
}

void CColorTranslatorDialog::updateContrast()
{
    m_contrast->setText(tr("Against white: %1:1 · against black: %2:1").arg(contrastRatio(m_color, Qt::white), 0, 'f', 2).arg(contrastRatio(m_color, Qt::black), 0, 'f', 2));
}
