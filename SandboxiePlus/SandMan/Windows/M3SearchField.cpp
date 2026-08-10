#include "stdafx.h"
#include "M3SearchField.h"

#include "RegexBuilderDialog.h"

#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTimer>
#include <QVariant>

namespace {

const int kMaximumPatternLength = 500;
const int kMaximumFlagsLength = 8;
const int kMinimumInteractiveTarget = 40;

bool IsDarkPalette(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

QColor ErrorColor(const QPalette& palette)
{
    return IsDarkPalette(palette) ? QColor(QStringLiteral("#F2B8B5"))
                                  : QColor(QStringLiteral("#B3261E"));
}

QColor ErrorContainerColor(const QPalette& palette)
{
    return IsDarkPalette(palette) ? QColor(QStringLiteral("#8C1D18"))
                                  : QColor(QStringLiteral("#F9DEDC"));
}

QColor OnErrorContainerColor(const QPalette& palette)
{
    return IsDarkPalette(palette) ? QColor(QStringLiteral("#F9DEDC"))
                                  : QColor(QStringLiteral("#410E0B"));
}

QString BoundedText(const QString& text)
{
    return text.left(kMaximumPatternLength);
}

QRegularExpression InvalidExpression()
{
    return QRegularExpression(QStringLiteral("("));
}

class M3SearchActionButton final : public QPushButton
{
public:
    enum Kind {
        Clear,
        RegexBuilder
    };

    M3SearchActionButton(Kind kind, QWidget* parent)
        : QPushButton(parent),
          m_kind(kind)
    {
        setFixedSize(kMinimumInteractiveTarget, kMinimumInteractiveTarget);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setAutoDefault(false);
        setDefault(false);
        setFlat(true);
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void changeEvent(QEvent* event) override
    {
        QPushButton::changeEvent(event);
        if (event->type() == QEvent::EnabledChange) {
            setCursor(isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor);
        }
        if (event->type() == QEvent::EnabledChange
            || event->type() == QEvent::PaletteChange
            || event->type() == QEvent::FontChange) {
            update();
        }
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const int requestedDiameter = property("m3VisualDiameter").toInt();
        const qreal diameter = qBound<qreal>(1.0,
            requestedDiameter > 0 ? requestedDiameter : kMinimumInteractiveTarget,
            qMin(width(), height()));
        const QRectF buttonRect((width() - diameter) / 2.0,
                                (height() - diameter) / 2.0,
                                diameter,
                                diameter);
        const qreal radius = buttonRect.height() / 2.0;
        const bool regexMode = property("regexMode").toBool();
        const bool searchValid = property("searchValid").toBool();
        const bool activeRegex = m_kind == RegexBuilder && regexMode;
        const QPalette::ColorGroup colorGroup = isEnabled()
            ? QPalette::Active
            : QPalette::Disabled;
        QColor foreground = palette().color(colorGroup, QPalette::ButtonText);

        if (m_kind == RegexBuilder && !searchValid && isEnabled()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(ErrorContainerColor(palette()));
            painter.drawRoundedRect(buttonRect, radius, radius);
            foreground = OnErrorContainerColor(palette());
        } else if (activeRegex && isEnabled()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(palette().color(QPalette::Highlight));
            painter.drawRoundedRect(buttonRect, radius, radius);
            foreground = palette().color(QPalette::HighlightedText);
        } else if (m_kind == RegexBuilder && isEnabled()) {
            foreground = palette().color(QPalette::Highlight);
        }

        if (isEnabled() && (underMouse() || isDown())) {
            QColor stateColor = palette().color(QPalette::Text);
            if (m_kind == RegexBuilder && !searchValid)
                stateColor = OnErrorContainerColor(palette());
            else if (activeRegex)
                stateColor = palette().color(QPalette::HighlightedText);
            stateColor.setAlpha(isDown() ? 31 : 20);
            painter.setPen(Qt::NoPen);
            painter.setBrush(stateColor);
            painter.drawRoundedRect(buttonRect, radius, radius);
        }

        if (hasFocus()) {
            const QRectF focusRect = buttonRect.adjusted(2.0, 2.0, -2.0, -2.0);
            QColor focusColor = palette().color(QPalette::Highlight);
            if (m_kind == RegexBuilder && !searchValid)
                focusColor = OnErrorContainerColor(palette());
            else if (activeRegex)
                focusColor = palette().color(QPalette::HighlightedText);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(focusColor, 3.0));
            painter.drawRoundedRect(focusRect,
                                    focusRect.height() / 2.0,
                                    focusRect.height() / 2.0);
        }

        painter.setPen(QPen(foreground, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (m_kind == Clear) {
            const QPointF center(width() / 2.0, height() / 2.0);
            const qreal halfExtent = 4.5;
            painter.drawLine(center + QPointF(-halfExtent, -halfExtent),
                             center + QPointF(halfExtent, halfExtent));
            painter.drawLine(center + QPointF(halfExtent, -halfExtent),
                             center + QPointF(-halfExtent, halfExtent));
        } else {
            QFont actionFont(font());
            actionFont.setFamily(QStringLiteral("Roboto Mono"));
            actionFont.setStyleHint(QFont::TypeWriter);
            actionFont.setWeight(QFont::Medium);
            painter.setFont(actionFont);
            painter.drawText(buttonRect, Qt::AlignCenter, QStringLiteral(".*"));
        }
    }

private:
    Kind m_kind;
};

}

CM3SearchField::CM3SearchField(QWidget* parent)
    : QWidget(parent),
      m_lineEdit(new QLineEdit(this)),
      m_clearButton(new M3SearchActionButton(M3SearchActionButton::Clear, this)),
      m_regexButton(new M3SearchActionButton(M3SearchActionButton::RegexBuilder, this)),
      m_regexBuilder(nullptr),
      m_heightVariant(Control),
      m_regexMode(false),
      m_valid(true)
{
    setObjectName(QStringLiteral("m3SearchField"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    setFocusProxy(m_lineEdit);
    setAttribute(Qt::WA_Hover, true);
    setProperty("m3SearchSurface", true);

    m_lineEdit->setObjectName(QStringLiteral("m3SearchInput"));
    m_lineEdit->setMaxLength(kMaximumPatternLength);
    m_lineEdit->setClearButtonEnabled(false);
    m_lineEdit->setMinimumWidth(0);
    m_lineEdit->setFrame(false);
    m_lineEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_lineEdit->installEventFilter(this);

    m_clearButton->setObjectName(QStringLiteral("m3SearchClearButton"));
    m_clearButton->installEventFilter(this);

    m_regexButton->setObjectName(QStringLiteral("m3RegexBuilderButton"));
    m_regexButton->installEventFilter(this);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addSpacing(24);
    layout->addWidget(m_lineEdit, 1);
    layout->addWidget(m_clearButton);
    layout->addWidget(m_regexButton);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &CM3SearchField::onTextEdited);
    connect(m_clearButton, &QPushButton::clicked, this, &CM3SearchField::clearSearch);
    connect(m_regexButton, &QPushButton::clicked, this, &CM3SearchField::openRegexBuilder);

    setAccessibleName(tr("Search"));
    setAccessibleDescription(tr("Searches using plain text. A regular expression builder is available beside the field."));
    setHeightVariant(Control);
    rebuildExpression(false);
    updateControlState();
}

QLineEdit* CM3SearchField::lineEdit() const
{
    return m_lineEdit;
}

QString CM3SearchField::query() const
{
    return m_query;
}

QString CM3SearchField::pattern() const
{
    return m_pattern;
}

QString CM3SearchField::flags() const
{
    return m_flags;
}

bool CM3SearchField::regexMode() const
{
    return m_regexMode;
}

QRegularExpression CM3SearchField::expression() const
{
    return m_expression;
}

bool CM3SearchField::isValid() const
{
    return m_valid;
}

QString CM3SearchField::error() const
{
    return m_error;
}

CM3SearchField::HeightVariant CM3SearchField::heightVariant() const
{
    return m_heightVariant;
}

void CM3SearchField::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange
        || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::FontChange) {
        if (event->type() == QEvent::FontChange)
            setHeightVariant(m_heightVariant);
        update();
        m_clearButton->update();
        m_regexButton->update();
    }
}

bool CM3SearchField::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_lineEdit || watched == m_clearButton || watched == m_regexButton)
        && (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)) {
        update();
    }
    return QWidget::eventFilter(watched, event);
}

void CM3SearchField::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPalette::ColorGroup colorGroup = isEnabled()
        ? QPalette::Active
        : QPalette::Disabled;
    const QRectF capsuleRect(0.0, 0.0, width(), height());
    const qreal radius = height() / 2.0;
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().color(colorGroup, QPalette::AlternateBase));
    painter.drawRoundedRect(capsuleRect, radius, radius);

    QColor borderColor;
    qreal borderWidth = 0.0;
    if (!m_valid) {
        borderColor = isEnabled()
            ? ErrorColor(palette())
            : palette().color(QPalette::Disabled, QPalette::Text);
        borderWidth = 2.0;
    } else if (m_lineEdit->hasFocus()) {
        borderColor = palette().color(QPalette::Highlight);
        borderWidth = 3.0;
    }

    if (borderWidth > 0.0) {
        const qreal inset = borderWidth / 2.0;
        const QRectF borderRect = capsuleRect.adjusted(inset, inset, -inset, -inset);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(borderColor, borderWidth));
        painter.drawRoundedRect(borderRect,
                                qMax<qreal>(0.0, radius - inset),
                                qMax<qreal>(0.0, radius - inset));
    }

    const int leftInset = m_heightVariant == Page ? 16 : (m_heightVariant == Control ? 16 : 12);
    const qreal iconSize = m_heightVariant == Page ? 24.0 : 20.0;
    const QPointF iconOrigin(leftInset, (height() - iconSize) / 2.0);
    const QRectF lens(iconOrigin.x() + 3.0,
                      iconOrigin.y() + 3.0,
                      iconSize - 9.0,
                      iconSize - 9.0);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(palette().color(colorGroup, QPalette::PlaceholderText), 2.0,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawEllipse(lens);
    painter.drawLine(QPointF(lens.right() - 0.5, lens.bottom() - 0.5),
                     QPointF(iconOrigin.x() + iconSize - 2.5,
                             iconOrigin.y() + iconSize - 2.5));
}

void CM3SearchField::setQuery(const QString& query)
{
    const QString boundedQuery = BoundedText(query);
    setState(boundedQuery,
             boundedQuery,
             m_flags,
             m_regexMode);
}

void CM3SearchField::setState(const QString& query, const QString& pattern, const QString& flags, bool regexMode)
{
    m_regexMode = regexMode;
    m_pattern = m_regexMode ? BoundedText(pattern) : BoundedText(query);
    m_flags = flags;
    m_query = m_regexMode ? m_pattern : BoundedText(query);

    {
        const QSignalBlocker blocker(m_lineEdit);
        m_lineEdit->setText(m_query);
    }

    rebuildExpression(true);
    updateControlState();
}

void CM3SearchField::setPlaceholderText(const QString& placeholder)
{
    m_lineEdit->setPlaceholderText(placeholder);
}

void CM3SearchField::setAccessibleName(const QString& name)
{
    QWidget::setAccessibleName(name.trimmed().isEmpty() ? tr("Search") : name.trimmed());
    updateAccessibleNames();
    updateControlState();
}

void CM3SearchField::setAccessibleDescription(const QString& description)
{
    m_accessibleDescription = description;
    updateControlState();
}

void CM3SearchField::setHeightVariant(HeightVariant variant)
{
    m_heightVariant = variant;
    const int height = static_cast<int>(variant);
    const int leftInset = variant == Page ? 16 : (variant == Control ? 16 : 12);
    const int rightInset = variant == Page ? 8 : (variant == Control ? 4 : 6);
    const int glyphWidth = variant == Page ? 24 : 20;
    const int editorInset = variant == Page ? 8 : (variant == Control ? 4 : 8);
    const int actionDiameter = variant == Page ? 40 : (variant == Control ? 36 : 28);
    setFixedHeight(height);
    setMinimumWidth(leftInset + glyphWidth + (2 * editorInset)
                    + (2 * kMinimumInteractiveTarget) + rightInset + 1);
    if (layout()) {
        layout()->setContentsMargins(leftInset, 0, rightInset, 0);
        QLayoutItem* glyphItem = layout()->itemAt(0);
        if (glyphItem && glyphItem->spacerItem())
            glyphItem->spacerItem()->changeSize(glyphWidth, kMinimumInteractiveTarget,
                                                 QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout()->invalidate();
    }
    m_lineEdit->setFixedHeight(height);
    m_lineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit#m3SearchInput { min-width: 0; min-height: %1px; max-height: %1px; "
        "border: 0; border-radius: 0; padding: 0; background: transparent; "
        "color: palette(text); selection-background-color: palette(highlight); "
        "selection-color: palette(highlighted-text); }"
        "QLineEdit#m3SearchInput:focus, QLineEdit#m3SearchInput[m3Focus=\"true\"] { "
        "border: 0; padding: 0; background: transparent; }")
        .arg(height));
    m_lineEdit->setTextMargins(editorInset, 0, editorInset, 0);
    m_lineEdit->setProperty("m3", QVariant());
    setProperty("m3SearchHeight", height);
    setProperty("m3SearchRadius", height / 2);
    m_clearButton->setProperty("m3VisualDiameter", actionDiameter);
    m_regexButton->setProperty("m3VisualDiameter", actionDiameter);

    QFont editorFont = font();
    editorFont.setPixelSize(variant == Page ? 16 : 14);
    editorFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.25);
    m_lineEdit->setFont(editorFont);

    QFont regexFont = font();
    regexFont.setPixelSize(variant == Menu ? 12 : 14);
    m_regexButton->setFont(regexFont);

    update();
    updateGeometry();
}

void CM3SearchField::onTextEdited(const QString& text)
{
    m_query = text;
    m_pattern = text;
    rebuildExpression(true);
    updateControlState();
}

void CM3SearchField::clearSearch()
{
    setQuery(QString());
    returnFocusToEditor();
}

void CM3SearchField::openRegexBuilder()
{
    ensureRegexBuilder();
    m_regexBuilder->setState(m_query, m_pattern, m_flags, m_regexMode);
    m_regexBuilder->openAnchored(m_regexButton);
}

void CM3SearchField::applyRegexPattern(const QString& pattern, const QString& flags)
{
    setState(pattern, pattern, flags, true);
    returnFocusToEditor();
}

void CM3SearchField::keepPlainText(const QString& text)
{
    setState(text, text, m_flags, false);
    returnFocusToEditor();
}

void CM3SearchField::ensureRegexBuilder()
{
    if (m_regexBuilder)
        return;

    m_regexBuilder = new CRegexBuilderDialog(this);
    m_regexBuilder->setAccessibleName(
        tr("Regular expression builder for %1").arg(QWidget::accessibleName()));
    connect(m_regexBuilder, &CRegexBuilderDialog::patternApplied,
            this, &CM3SearchField::applyRegexPattern);
    connect(m_regexBuilder, &CRegexBuilderDialog::plainTextRequested,
            this, &CM3SearchField::keepPlainText);
    connect(m_regexBuilder, &QDialog::finished,
            this, [this](int) { returnFocusToEditor(); });
}

void CM3SearchField::rebuildExpression(bool notify)
{
    m_error.clear();

    if (m_regexMode) {
        m_expression = compileRegex(m_pattern, m_flags, &m_error);
        m_valid = m_error.isEmpty() && m_expression.isValid();
        if (!m_valid) {
            m_expression = InvalidExpression();
            Q_ASSERT(!m_expression.isValid());
        }
    } else {
        m_expression = QRegularExpression(QRegularExpression::escape(m_query),
                                          QRegularExpression::CaseInsensitiveOption);
        m_valid = true;
    }

    if (notify) {
        emit searchChanged(m_query,
                           m_regexMode,
                           m_expression,
                           m_flags,
                           m_valid,
                           m_error);
    }
}

void CM3SearchField::updateAccessibleNames()
{
    const QString fieldName = QWidget::accessibleName().trimmed().isEmpty()
        ? tr("Search")
        : QWidget::accessibleName().trimmed();
    const QString clearName = tr("Clear %1").arg(fieldName);
    const QString builderName = tr("Open regular expression builder for %1").arg(fieldName);

    m_lineEdit->setAccessibleName(fieldName);
    m_clearButton->setAccessibleName(clearName);
    m_clearButton->setAccessibleDescription(tr("Clears the current query in %1.").arg(fieldName));
    m_clearButton->setToolTip(clearName);
    m_regexButton->setAccessibleName(builderName);
    m_regexButton->setToolTip(builderName);
    if (m_regexBuilder) {
        m_regexBuilder->setAccessibleName(
            tr("Regular expression builder for %1").arg(fieldName));
    }
}

void CM3SearchField::updateControlState()
{
    const bool hasQuery = !m_query.isEmpty();
    m_clearButton->setVisible(hasQuery);
    m_clearButton->setEnabled(hasQuery);
    const QString fieldName = QWidget::accessibleName().trimmed().isEmpty()
        ? tr("Search")
        : QWidget::accessibleName().trimmed();
    m_clearButton->setToolTip(hasQuery
        ? tr("Clear %1").arg(fieldName)
        : tr("%1 is already empty.").arg(fieldName));
    m_regexButton->setProperty("regexMode", m_regexMode);
    m_regexButton->setProperty("searchValid", m_valid);
    m_lineEdit->setProperty("regexMode", m_regexMode);
    m_lineEdit->setProperty("searchValid", m_valid);
    setProperty("regexMode", m_regexMode);
    setProperty("searchValid", m_valid);

    QString modeDescription;
    if (!m_valid) {
        modeDescription = tr("Invalid regular expression: %1").arg(m_error);
    } else if (m_regexMode) {
        modeDescription = tr("Regular expression mode is on. Active flags: %1.")
                              .arg(m_flags.isEmpty() ? tr("none") : m_flags);
    } else {
        modeDescription = tr("Plain-text search is on. Matching ignores letter case.");
    }

    QString completeDescription = m_accessibleDescription.trimmed();
    if (!completeDescription.isEmpty())
        completeDescription.append(QLatin1Char(' '));
    completeDescription.append(modeDescription);

    QWidget::setAccessibleDescription(completeDescription);
    m_regexButton->setAccessibleDescription(modeDescription);
    m_lineEdit->setAccessibleDescription(completeDescription);
    m_lineEdit->setToolTip(m_valid ? QString() : modeDescription);
    setToolTip(m_valid ? QString() : modeDescription);

    m_clearButton->update();
    m_regexButton->update();
    update();
}

void CM3SearchField::returnFocusToEditor()
{
    QTimer::singleShot(0, this, [this]() {
        m_lineEdit->setFocus(Qt::OtherFocusReason);
        m_lineEdit->setCursorPosition(m_lineEdit->text().size());
    });
}

QRegularExpression CM3SearchField::compileRegex(const QString& pattern, const QString& flags, QString* error)
{
    if (pattern.size() > kMaximumPatternLength) {
        if (error)
            *error = tr("Patterns are limited to %1 characters.").arg(kMaximumPatternLength);
        return InvalidExpression();
    }
    if (flags.size() > kMaximumFlagsLength) {
        if (error)
            *error = tr("Flags are limited to %1 characters.").arg(kMaximumFlagsLength);
        return InvalidExpression();
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    QSet<QChar> seenFlags;
    for (const QChar flag : flags) {
        if (seenFlags.contains(flag)) {
            if (error)
                *error = tr("Flag '%1' appears more than once.").arg(flag);
            return InvalidExpression();
        }
        seenFlags.insert(flag);

        switch (flag.unicode()) {
        case 'i':
            options |= QRegularExpression::CaseInsensitiveOption;
            break;
        case 'm':
            options |= QRegularExpression::MultilineOption;
            break;
        case 's':
            options |= QRegularExpression::DotMatchesEverythingOption;
            break;
        case 'x':
            options |= QRegularExpression::ExtendedPatternSyntaxOption;
            break;
        case 'U':
            options |= QRegularExpression::InvertedGreedinessOption;
            break;
        default:
            if (error)
                *error = tr("Unsupported flag '%1'. Use i, m, s, x, or U.").arg(flag);
            return InvalidExpression();
        }
    }

    const QRegularExpression expression(pattern, options);
    if (!expression.isValid() && error)
        *error = expression.errorString();
    return expression;
}
