#include "stdafx.h"
#include "M3SearchField.h"

#include "RegexBuilderDialog.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QVariant>

namespace {

const int kMaximumPatternLength = 500;
const int kMaximumFlagsLength = 8;
const int kMinimumInteractiveTarget = 40;

QString BoundedText(const QString& text)
{
    return text.left(kMaximumPatternLength);
}

}

CM3SearchField::CM3SearchField(QWidget* parent)
    : QWidget(parent),
      m_lineEdit(new QLineEdit(this)),
      m_clearButton(new QPushButton(QString(QChar(0x00D7)), this)),
      m_regexButton(new QPushButton(QStringLiteral(".*"), this)),
      m_regexBuilder(nullptr),
      m_heightVariant(Control),
      m_regexMode(false),
      m_valid(true)
{
    setObjectName(QStringLiteral("m3SearchField"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_lineEdit->setObjectName(QStringLiteral("m3SearchInput"));
    m_lineEdit->setMaxLength(kMaximumPatternLength);
    m_lineEdit->setClearButtonEnabled(false);
    m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_clearButton->setObjectName(QStringLiteral("m3SearchClearButton"));
    m_clearButton->setFixedSize(kMinimumInteractiveTarget, kMinimumInteractiveTarget);
    m_clearButton->setStyleSheet(QStringLiteral(
        "QPushButton#m3SearchClearButton { min-width: 40px; max-width: 40px; min-height: 40px; max-height: 40px; padding: 0; border-radius: 20px; }"));
    m_clearButton->setAccessibleName(tr("Clear search"));
    m_clearButton->setAccessibleDescription(tr("Clears the current search query."));
    m_clearButton->setToolTip(tr("Clear search"));

    m_regexButton->setObjectName(QStringLiteral("m3RegexBuilderButton"));
    m_regexButton->setFixedSize(kMinimumInteractiveTarget, kMinimumInteractiveTarget);
    m_regexButton->setStyleSheet(QStringLiteral(
        "QPushButton#m3RegexBuilderButton { min-width: 40px; max-width: 40px; min-height: 40px; max-height: 40px; padding: 0; border-radius: 20px; }"));
    m_regexButton->setAccessibleName(tr("Open regular expression builder"));
    m_regexButton->setToolTip(tr("Open regular expression builder"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
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
    QWidget::setAccessibleName(name);
    m_lineEdit->setAccessibleName(name);
    m_lineEdit->setAccessibleDescription(m_accessibleDescription);
}

void CM3SearchField::setAccessibleDescription(const QString& description)
{
    m_accessibleDescription = description;
    QWidget::setAccessibleDescription(description);
    m_lineEdit->setAccessibleDescription(description);
}

void CM3SearchField::setHeightVariant(HeightVariant variant)
{
    m_heightVariant = variant;
    const int height = static_cast<int>(variant);
    setFixedHeight(height);
    m_lineEdit->setFixedHeight(height);
    m_lineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit#m3SearchInput { min-height: %1px; max-height: %1px; }").arg(height));
    m_lineEdit->setProperty("m3", variant == Page ? QVariant(QStringLiteral("search")) : QVariant());
    setProperty("m3SearchHeight", height);

    m_lineEdit->style()->unpolish(m_lineEdit);
    m_lineEdit->style()->polish(m_lineEdit);
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
    connect(m_regexBuilder, &CRegexBuilderDialog::patternApplied,
            this, &CM3SearchField::applyRegexPattern);
    connect(m_regexBuilder, &CRegexBuilderDialog::plainTextRequested,
            this, &CM3SearchField::keepPlainText);
}

void CM3SearchField::rebuildExpression(bool notify)
{
    m_error.clear();

    if (m_regexMode) {
        m_expression = compileRegex(m_pattern, m_flags, &m_error);
        m_valid = m_error.isEmpty() && m_expression.isValid();
        if (!m_valid)
            m_expression = QRegularExpression();
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

void CM3SearchField::updateControlState()
{
    const bool hasQuery = !m_query.isEmpty();
    m_clearButton->setEnabled(hasQuery);
    m_clearButton->setToolTip(hasQuery ? tr("Clear search") : tr("Search is already empty."));
    m_regexButton->setProperty("regexMode", m_regexMode);
    m_regexButton->setProperty("searchValid", m_valid);
    m_lineEdit->setProperty("regexMode", m_regexMode);
    m_lineEdit->setProperty("searchValid", m_valid);

    QString modeDescription;
    if (!m_valid) {
        modeDescription = tr("Invalid regular expression: %1").arg(m_error);
    } else if (m_regexMode) {
        modeDescription = tr("Regular expression mode is on. Active flags: %1.")
                              .arg(m_flags.isEmpty() ? tr("none") : m_flags);
    } else {
        modeDescription = tr("Plain-text search is on. Matching ignores letter case.");
    }

    m_regexButton->setAccessibleDescription(modeDescription);
    m_lineEdit->setAccessibleDescription(m_accessibleDescription + QStringLiteral(" ") + modeDescription);
    m_lineEdit->setToolTip(m_valid ? QString() : modeDescription);

    m_regexButton->style()->unpolish(m_regexButton);
    m_regexButton->style()->polish(m_regexButton);
    m_lineEdit->style()->unpolish(m_lineEdit);
    m_lineEdit->style()->polish(m_lineEdit);
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
        return QRegularExpression();
    }
    if (flags.size() > kMaximumFlagsLength) {
        if (error)
            *error = tr("Flags are limited to %1 characters.").arg(kMaximumFlagsLength);
        return QRegularExpression();
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    QSet<QChar> seenFlags;
    for (const QChar flag : flags) {
        if (seenFlags.contains(flag)) {
            if (error)
                *error = tr("Flag '%1' appears more than once.").arg(flag);
            return QRegularExpression();
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
            return QRegularExpression();
        }
    }

    const QRegularExpression expression(pattern, options);
    if (!expression.isValid() && error)
        *error = expression.errorString();
    return expression;
}
