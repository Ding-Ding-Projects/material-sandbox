#include "stdafx.h"
#include "RegexBuilderDialog.h"

#include "M3DialogHost.h"

#include <QApplication>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSet>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStyle>
#include <QStringList>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

const int kDialogWidth = 680;
const int kDialogPadding = 24;
const int kAnchorGap = 8;
const int kViewportMargin = 8;
const int kMaximumPatternLength = 500;
const int kMaximumFlagsLength = 8;
const int kMaximumSampleLength = 500;
const int kMaximumMatches = 128;
const int kMaximumCapturesPerMatch = 32;

struct GuidedToken
{
    const char* token;
    const char* description;
};

const GuidedToken kGuidedTokens[] = {
    { "\\d", "Digit" },
    { "\\w", "Word character" },
    { "\\s", "Whitespace" },
    { ".", "Any character" },
    { "+", "One or more" },
    { "*", "Zero or more" },
    { "?", "Optional or lazy" },
    { "[]", "Character class" },
    { "()", "Capture group" },
    { "|", "Alternation" },
    { "^", "Start anchor" },
    { "$", "End anchor" },
    { "\\b", "Word boundary" },
    { "{2,4}", "Repeat two to four times" }
};

QString bounded(const QString& value, int maximum)
{
    return value.left(maximum);
}

QString displayCapture(const QString& value)
{
    QString display = value;
    display.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    display.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
    display.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
    if (display.size() > 120)
        display = display.left(117) + QStringLiteral("...");
    return display;
}

} // namespace

CRegexBuilderDialog::CRegexBuilderDialog(QWidget* parent)
    : QDialog(parent),
      m_patternEdit(new QLineEdit(this)),
      m_flagsEdit(new QLineEdit(this)),
      m_sampleEdit(new QPlainTextEdit(this)),
      m_validationLabel(new QLabel(this)),
      m_previewEdit(new QPlainTextEdit(this)),
      m_applyButton(new QPushButton(tr("Apply pattern"), this)),
      m_valid(true)
{
    setObjectName(QStringLiteral("regexBuilderDialog"));
    setWindowTitle(tr("Regex builder"));
    setModal(false);
    setWindowModality(Qt::NonModal);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setSizeGripEnabled(true);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("regexBuilderContent"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(kDialogPadding, kDialogPadding,
                                      kDialogPadding, kDialogPadding);
    contentLayout->setSpacing(16);

    auto* introduction = new QLabel(
        tr("Build a Qt QRegularExpression, test it locally, and return it to this search field."),
        content);
    introduction->setObjectName(QStringLiteral("regexBuilderIntroduction"));
    introduction->setWordWrap(true);
    contentLayout->addWidget(introduction);

    auto* form = new QFormLayout();
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_patternEdit->setObjectName(QStringLiteral("regexPatternEdit"));
    m_patternEdit->setMaxLength(kMaximumPatternLength);
    m_patternEdit->setMinimumHeight(56);
    m_patternEdit->setAccessibleName(tr("Regular expression pattern"));
    m_patternEdit->setAccessibleDescription(
        tr("Qt QRegularExpression pattern, limited to 500 characters."));
    auto* patternLabel = new QLabel(tr("Pattern"), content);
    patternLabel->setBuddy(m_patternEdit);
    form->addRow(patternLabel, m_patternEdit);

    m_flagsEdit->setObjectName(QStringLiteral("regexFlagsEdit"));
    m_flagsEdit->setMaxLength(kMaximumFlagsLength);
    m_flagsEdit->setMinimumHeight(56);
    m_flagsEdit->setPlaceholderText(QStringLiteral("i"));
    m_flagsEdit->setAccessibleName(tr("Regular expression flags"));
    m_flagsEdit->setAccessibleDescription(
        tr("Optional flags i, m, s, x, and U, without duplicates."));
    auto* flagsLabel = new QLabel(tr("Flags"), content);
    flagsLabel->setBuddy(m_flagsEdit);
    form->addRow(flagsLabel, m_flagsEdit);
    contentLayout->addLayout(form);

    auto* tokenLabel = new QLabel(tr("Guided tokens"), content);
    tokenLabel->setObjectName(QStringLiteral("regexSectionLabel"));
    contentLayout->addWidget(tokenLabel);

    auto* tokenGrid = new QGridLayout();
    tokenGrid->setContentsMargins(0, 0, 0, 0);
    tokenGrid->setHorizontalSpacing(8);
    tokenGrid->setVerticalSpacing(8);
    const int tokenCount = static_cast<int>(sizeof(kGuidedTokens) / sizeof(kGuidedTokens[0]));
    for (int index = 0; index < tokenCount; ++index) {
        const GuidedToken& guided = kGuidedTokens[index];
        auto* button = new QToolButton(content);
        const QString token = QString::fromLatin1(guided.token);
        const QString description = tr(guided.description);
        button->setText(token);
        button->setProperty("regexToken", token);
        button->setProperty("m3", QStringLiteral("chip"));
        button->setMinimumSize(48, 40);
        button->setToolTip(description);
        button->setAccessibleName(tr("Insert %1: %2").arg(token, description));
        connect(button, &QToolButton::clicked,
                this, &CRegexBuilderDialog::addGuidedToken);
        tokenGrid->addWidget(button, index / 7, index % 7);
    }
    contentLayout->addLayout(tokenGrid);

    auto* sampleLabel = new QLabel(tr("Sample text"), content);
    sampleLabel->setBuddy(m_sampleEdit);
    contentLayout->addWidget(sampleLabel);
    m_sampleEdit->setObjectName(QStringLiteral("regexSampleEdit"));
    m_sampleEdit->setPlaceholderText(tr("Type or paste text to test locally"));
    m_sampleEdit->setAccessibleName(tr("Regular expression sample text"));
    m_sampleEdit->setAccessibleDescription(tr("Local sample text, limited to 500 characters."));
    m_sampleEdit->setMinimumHeight(96);
    contentLayout->addWidget(m_sampleEdit);

    m_validationLabel->setObjectName(QStringLiteral("regexValidation"));
    m_validationLabel->setWordWrap(true);
    m_validationLabel->setAccessibleName(tr("Pattern validation"));
    contentLayout->addWidget(m_validationLabel);

    auto* previewLabel = new QLabel(tr("Matches and capture groups"), content);
    previewLabel->setBuddy(m_previewEdit);
    contentLayout->addWidget(previewLabel);
    m_previewEdit->setObjectName(QStringLiteral("regexPreview"));
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setMinimumHeight(128);
    m_previewEdit->setAccessibleName(tr("Regular expression match preview"));
    m_previewEdit->setAccessibleDescription(
        tr("At most 128 matches and 32 capture groups per match are shown."));
    contentLayout->addWidget(m_previewEdit);

    auto* bounds = new QLabel(
        tr("Evaluation stays on this computer. Pattern and sample inputs are each limited to 500 characters; previews are limited to 128 matches and 32 capture groups per match. Inputs are not persisted by this dialog."),
        content);
    bounds->setObjectName(QStringLiteral("regexBounds"));
    bounds->setWordWrap(true);
    contentLayout->addWidget(bounds);

    auto* actions = new QHBoxLayout();
    actions->setContentsMargins(0, 4, 0, 0);
    actions->setSpacing(8);
    actions->addStretch(1);
    auto* cancelButton = new QPushButton(tr("Cancel"), content);
    cancelButton->setProperty("m3", QStringLiteral("text"));
    cancelButton->setAccessibleDescription(tr("Close without changing the search field."));
    auto* plainButton = new QPushButton(tr("Keep plain text"), content);
    plainButton->setAccessibleDescription(
        tr("Use the current pattern characters as literal plain text."));
    m_applyButton->setProperty("m3", QStringLiteral("filled"));
    m_applyButton->setDefault(true);
    m_applyButton->setAccessibleDescription(
        tr("Apply the valid pattern and flags to the originating search field."));
    actions->addWidget(cancelButton);
    actions->addWidget(plainButton);
    actions->addWidget(m_applyButton);
    contentLayout->addLayout(actions);

    auto* scroller = new QScrollArea(this);
    scroller->setObjectName(QStringLiteral("regexBuilderScroller"));
    scroller->setWidgetResizable(true);
    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroller->setWidget(content);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(scroller);

    setStyleSheet(QStringLiteral(
        "QDialog#regexBuilderDialog, QWidget#regexBuilderContent {"
        " background: palette(window); }"
        "QDialog#regexBuilderDialog { border-radius: 28px; }"
        "QScrollArea#regexBuilderScroller { border: 0; background: transparent; }"
        "QLabel#regexBuilderIntroduction, QLabel#regexBounds { color: palette(mid); }"
        "QLabel#regexSectionLabel { font-weight: 600; }"
        "QLabel#regexValidation[valid=\"true\"] { color: #2E6B12; }"
        "QLabel#regexValidation[valid=\"false\"] { color: palette(link); }"
        "QPlainTextEdit#regexPreview { font-family: 'Roboto Mono', Consolas, monospace; }"
        "QToolButton[m3=\"chip\"] { border: 1px solid palette(mid); border-radius: 8px;"
        " padding: 0 8px; font-family: 'Roboto Mono', Consolas, monospace; }"));

    connect(m_patternEdit, &QLineEdit::textChanged,
            this, &CRegexBuilderDialog::updatePreview);
    connect(m_flagsEdit, &QLineEdit::textChanged,
            this, &CRegexBuilderDialog::updatePreview);
    connect(m_sampleEdit, &QPlainTextEdit::textChanged,
            this, &CRegexBuilderDialog::limitSampleText);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(plainButton, &QPushButton::clicked,
            this, &CRegexBuilderDialog::keepPlainText);
    connect(m_applyButton, &QPushButton::clicked,
            this, &CRegexBuilderDialog::applyPattern);
    connect(this, &QDialog::finished, this, [this](int) { restoreOriginFocus(); });

    auto* applyReturn = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(applyReturn, &QShortcut::activated, this, &CRegexBuilderDialog::applyPattern);
    auto* applyEnter = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Enter), this);
    connect(applyEnter, &QShortcut::activated, this, &CRegexBuilderDialog::applyPattern);

    M3DialogHost::Install(this);
    updatePreview();
}

void CRegexBuilderDialog::setState(const QString& plainText,
                                   const QString& pattern,
                                   const QString& flags,
                                   bool regexMode)
{
    const QString boundedPlainText = bounded(plainText, kMaximumPatternLength);
    const QString initialPattern = pattern.isEmpty() && !regexMode ? boundedPlainText : pattern;

    const QSignalBlocker patternBlocker(m_patternEdit);
    const QSignalBlocker flagsBlocker(m_flagsEdit);
    m_patternEdit->setText(bounded(initialPattern, kMaximumPatternLength));
    m_flagsEdit->setText(bounded(flags, kMaximumFlagsLength));
    updatePreview();
}

void CRegexBuilderDialog::openAnchored(QWidget* origin)
{
    if (!origin)
        return;

    m_origin = origin;
    show();
    raise();
    activateWindow();
    positionBesideOrigin();
    m_patternEdit->setFocus(Qt::OtherFocusReason);
    m_patternEdit->setCursorPosition(m_patternEdit->text().size());
}

QString CRegexBuilderDialog::pattern() const
{
    return m_patternEdit->text();
}

QString CRegexBuilderDialog::flags() const
{
    return m_flagsEdit->text();
}

QString CRegexBuilderDialog::sampleText() const
{
    return m_sampleEdit->toPlainText();
}

bool CRegexBuilderDialog::isPatternValid() const
{
    return m_valid;
}

QString CRegexBuilderDialog::patternError() const
{
    return m_error;
}

QRegularExpression CRegexBuilderDialog::compile(const QString& pattern,
                                                 const QString& flags,
                                                 QString* error)
{
    if (error)
        error->clear();
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

void CRegexBuilderDialog::addGuidedToken()
{
    auto* button = qobject_cast<QToolButton*>(sender());
    if (!button)
        return;

    const QString token = button->property("regexToken").toString();
    int cursor = m_patternEdit->cursorPosition();
    QString next = m_patternEdit->text();
    next.insert(cursor, token);
    next = bounded(next, kMaximumPatternLength);
    m_patternEdit->setText(next);
    cursor = qMin(cursor + token.size(), next.size());
    m_patternEdit->setCursorPosition(cursor);
    m_patternEdit->setFocus(Qt::OtherFocusReason);
}

void CRegexBuilderDialog::updatePreview()
{
    m_error.clear();
    const QRegularExpression expression = compile(pattern(), flags(), &m_error);
    m_valid = m_error.isEmpty() && expression.isValid();
    m_applyButton->setEnabled(m_valid);

    m_validationLabel->setProperty("valid", m_valid);
    m_validationLabel->setText(m_valid
        ? tr("Valid Qt QRegularExpression")
        : tr("Invalid pattern: %1").arg(m_error));
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);

    m_previewEdit->setPlainText(m_valid
        ? previewText(expression)
        : tr("Fix the pattern or flags to preview matches."));
}

void CRegexBuilderDialog::keepPlainText()
{
    const QString text = pattern();
    emit plainTextRequested(text);
    accept();
}

void CRegexBuilderDialog::applyPattern()
{
    if (!m_applyButton->isEnabled() || !m_valid)
        return;
    emit patternApplied(pattern(), flags());
    accept();
}

void CRegexBuilderDialog::limitSampleText()
{
    QString text = m_sampleEdit->toPlainText();
    if (text.size() > kMaximumSampleLength) {
        const QSignalBlocker blocker(m_sampleEdit);
        text = bounded(text, kMaximumSampleLength);
        m_sampleEdit->setPlainText(text);
        QTextCursor cursor = m_sampleEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_sampleEdit->setTextCursor(cursor);
    }
    updatePreview();
}

void CRegexBuilderDialog::positionBesideOrigin()
{
    if (!m_origin)
        return;

    const QRect originRect(m_origin->mapToGlobal(QPoint(0, 0)), m_origin->size());
    QScreen* screen = QGuiApplication::screenAt(originRect.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    const QRect viewport = screen->availableGeometry().adjusted(
        kViewportMargin, kViewportMargin, -kViewportMargin, -kViewportMargin);
    const int desiredWidth = qMin(kDialogWidth, viewport.width());
    const int desiredHeight = qMin(qMax(sizeHint().height(), 420),
                                   qRound(viewport.height() * 0.88));
    const int belowSpace = qMax(0, viewport.bottom() - originRect.bottom() - kAnchorGap + 1);
    const int aboveSpace = qMax(0, originRect.top() - viewport.top() - kAnchorGap);
    const bool placeBelow = belowSpace >= desiredHeight || belowSpace >= aboveSpace;
    const int sideSpace = placeBelow ? belowSpace : aboveSpace;
    const int finalHeight = qMax(1, qMin(desiredHeight, sideSpace));

    resize(desiredWidth, finalHeight);
    const int maximumX = viewport.right() - width() + 1;
    const int x = qBound(viewport.left(), originRect.left(), qMax(viewport.left(), maximumX));
    const int y = placeBelow
        ? originRect.bottom() + kAnchorGap + 1
        : originRect.top() - kAnchorGap - height();
    move(x, qBound(viewport.top(), y, qMax(viewport.top(), viewport.bottom() - height() + 1)));
}

void CRegexBuilderDialog::restoreOriginFocus()
{
    if (!m_origin)
        return;
    QPointer<QWidget> origin = m_origin;
    QTimer::singleShot(0, this, [origin]() {
        if (origin)
            origin->setFocus(Qt::OtherFocusReason);
    });
}

QString CRegexBuilderDialog::previewText(const QRegularExpression& expression) const
{
    const QString sample = sampleText();
    QRegularExpressionMatchIterator iterator = expression.globalMatch(sample);
    QStringList lines;
    int matchCount = 0;

    // QRegularExpression advances zero-width global matches itself; the hard cap also
    // bounds both those results and ordinary matches.
    while (iterator.hasNext() && matchCount < kMaximumMatches) {
        const QRegularExpressionMatch match = iterator.next();
        ++matchCount;
        lines << tr("Match %1 | index %2 | length %3 | %4")
                     .arg(matchCount)
                     .arg(match.capturedStart(0))
                     .arg(match.capturedLength(0))
                     .arg(displayCapture(match.captured(0)));

        const int captureCount = qMin(expression.captureCount(), kMaximumCapturesPerMatch);
        for (int capture = 1; capture <= captureCount; ++capture) {
            if (match.capturedStart(capture) < 0)
                continue;
            lines << tr("  Group %1 | index %2 | length %3 | %4")
                         .arg(capture)
                         .arg(match.capturedStart(capture))
                         .arg(match.capturedLength(capture))
                         .arg(displayCapture(match.captured(capture)));
        }
    }

    if (matchCount == 0)
        return tr("Valid | no matches in the sample text.");
    if (iterator.hasNext())
        lines << tr("Preview stopped after %1 matches.").arg(kMaximumMatches);
    lines.prepend(tr("Valid | %1 match(es) shown.").arg(matchCount));
    return lines.join(QLatin1Char('\n'));
}
