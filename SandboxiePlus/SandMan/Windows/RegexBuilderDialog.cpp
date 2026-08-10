#include "stdafx.h"
#include "RegexBuilderDialog.h"

#include <QApplication>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QSet>
#include <QStyle>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kMaximumInput = 500;
constexpr int kMaximumFlags = 8;
constexpr int kMaximumMatches = 128;
constexpr int kMaximumCaptures = 32;
QString bounded(const QString& value, int maximum) { return value.left(maximum); }
}

CRegexBuilderDialog::CRegexBuilderDialog(QWidget* parent)
    : QDialog(parent),
      m_patternEdit(new QLineEdit(this)),
      m_flagsEdit(new QLineEdit(this)),
      m_sampleEdit(new QPlainTextEdit(this)),
      m_previewEdit(new QPlainTextEdit(this)),
      m_validationLabel(new QLabel(this)),
      m_applyButton(new QPushButton(tr("Apply"), this))
{
    setObjectName(QStringLiteral("regexBuilderDialog"));
    setWindowTitle(tr("Regular expression builder"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(false);
    setMinimumSize(560, 560);
    resize(680, 660);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("regexBuilderContent"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(16);

    auto* titleRow = new QHBoxLayout();
    auto* title = new QLabel(tr("Build a regular expression"), content);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(qMax(14.0, titleFont.pointSizeF() + 4.0));
    titleFont.setWeight(QFont::Medium);
    title->setFont(titleFont);
    titleRow->addWidget(title, 1);
    auto* closeButton = new QToolButton(content);
    closeButton->setText(QString(QChar(0x00D7)));
    closeButton->setToolTip(tr("Close"));
    closeButton->setAccessibleName(closeButton->toolTip());
    closeButton->setFixedSize(40, 40);
    titleRow->addWidget(closeButton);
    contentLayout->addLayout(titleRow);

    auto* intro = new QLabel(
        tr("Use Qt regular-expression syntax. Evaluation stays local and bounded; invalid patterns are never applied."),
        content);
    intro->setWordWrap(true);
    intro->setObjectName(QStringLiteral("regexBuilderIntroduction"));
    contentLayout->addWidget(intro);

    auto* form = new QFormLayout();
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);
    m_patternEdit->setObjectName(QStringLiteral("regexPatternEdit"));
    m_patternEdit->setMaxLength(kMaximumInput);
    m_patternEdit->setPlaceholderText(tr("Example: ^Sandboxie(?<version>.+)$"));
    m_patternEdit->setAccessibleName(tr("Regular expression pattern"));
    form->addRow(tr("Pattern"), m_patternEdit);
    m_flagsEdit->setObjectName(QStringLiteral("regexFlagsEdit"));
    m_flagsEdit->setMaxLength(kMaximumFlags);
    m_flagsEdit->setPlaceholderText(QStringLiteral("imsxU"));
    m_flagsEdit->setAccessibleName(tr("Regular expression flags"));
    form->addRow(tr("Flags"), m_flagsEdit);
    contentLayout->addLayout(form);

    auto* tokenLabel = new QLabel(tr("Common tokens"), content);
    tokenLabel->setObjectName(QStringLiteral("regexSectionLabel"));
    contentLayout->addWidget(tokenLabel);
    auto* tokenGrid = new QGridLayout();
    tokenGrid->setSpacing(8);
    const QList<QPair<QString, QString>> tokens = {
        { QStringLiteral("."), tr("Any character") },
        { QStringLiteral("\\d"), tr("Digit") },
        { QStringLiteral("\\w"), tr("Word character") },
        { QStringLiteral("\\s"), tr("Whitespace") },
        { QStringLiteral("^"), tr("Start") },
        { QStringLiteral("$"), tr("End") },
        { QStringLiteral("()"), tr("Capture group") },
        { QStringLiteral("(?:)"), tr("Non-capturing group") },
        { QStringLiteral("[]"), tr("Character set") },
        { QStringLiteral("*"), tr("Zero or more") },
        { QStringLiteral("+"), tr("One or more") },
        { QStringLiteral("?"), tr("Optional") }
    };
    for (int i = 0; i < tokens.size(); ++i) {
        auto* button = new QToolButton(content);
        button->setText(tokens.at(i).first);
        button->setToolTip(tokens.at(i).second);
        button->setAccessibleName(tokens.at(i).second);
        button->setProperty("regexToken", tokens.at(i).first);
        button->setProperty("m3", QStringLiteral("chip"));
        button->setMinimumSize(64, 40);
        connect(button, &QToolButton::clicked, this, &CRegexBuilderDialog::insertToken);
        tokenGrid->addWidget(button, i / 4, i % 4);
    }
    contentLayout->addLayout(tokenGrid);

    auto* sampleLabel = new QLabel(tr("Sample text"), content);
    sampleLabel->setBuddy(m_sampleEdit);
    contentLayout->addWidget(sampleLabel);
    m_sampleEdit->setObjectName(QStringLiteral("regexSampleEdit"));
    m_sampleEdit->setPlaceholderText(tr("Type or paste text to test locally"));
    m_sampleEdit->setAccessibleName(tr("Regular expression sample text"));
    m_sampleEdit->setMinimumHeight(90);
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
    m_previewEdit->setMinimumHeight(126);
    m_previewEdit->setAccessibleName(tr("Regular expression match preview"));
    contentLayout->addWidget(m_previewEdit);

    auto* bounds = new QLabel(
        tr("Pattern and sample text are limited to 500 characters. Preview is limited to 128 matches and 32 capture groups per match."),
        content);
    bounds->setWordWrap(true);
    bounds->setObjectName(QStringLiteral("regexBounds"));
    contentLayout->addWidget(bounds);

    auto* actions = new QHBoxLayout();
    actions->addStretch(1);
    auto* cancel = new QPushButton(tr("Cancel"), content);
    cancel->setProperty("m3", QStringLiteral("text"));
    auto* plain = new QPushButton(tr("Keep plain text"), content);
    m_applyButton->setProperty("m3", QStringLiteral("filled"));
    m_applyButton->setDefault(true);
    actions->addWidget(cancel);
    actions->addWidget(plain);
    actions->addWidget(m_applyButton);
    contentLayout->addLayout(actions);

    auto* scroller = new QScrollArea(this);
    scroller->setWidgetResizable(true);
    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroller->setWidget(content);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroller);

    setStyleSheet(QStringLiteral(
        "QDialog#regexBuilderDialog, QWidget#regexBuilderContent { background: palette(window); }"
        "QDialog#regexBuilderDialog { border-radius: 28px; }"
        "QLabel#regexBuilderIntroduction, QLabel#regexBounds { color: palette(mid); }"
        "QLabel#regexSectionLabel { font-weight: 600; }"
        "QPlainTextEdit#regexPreview { font-family: 'Roboto Mono', Consolas, monospace; }"
        "QToolButton[m3='chip'] { border: 1px solid palette(mid); border-radius: 8px; padding: 0 8px; }"));

    connect(closeButton, &QToolButton::clicked, this, &QDialog::reject);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(plain, &QPushButton::clicked, this, &CRegexBuilderDialog::keepPlainText);
    connect(m_applyButton, &QPushButton::clicked, this, &CRegexBuilderDialog::applyPattern);
    connect(m_patternEdit, &QLineEdit::textChanged, this, &CRegexBuilderDialog::updatePreview);
    connect(m_flagsEdit, &QLineEdit::textChanged, this, &CRegexBuilderDialog::updatePreview);
    connect(m_sampleEdit, &QPlainTextEdit::textChanged, this, [this] {
        const QString value = m_sampleEdit->toPlainText();
        if (value.size() > kMaximumInput) {
            QTextCursor cursor = m_sampleEdit->textCursor();
            m_sampleEdit->setPlainText(value.left(kMaximumInput));
            cursor.setPosition(m_sampleEdit->toPlainText().size());
            m_sampleEdit->setTextCursor(cursor);
        }
        updatePreview();
    });
    connect(this, &QDialog::finished, this, [this](int) { restoreOriginFocus(); });
    updatePreview();
}

void CRegexBuilderDialog::setState(const QString& plainText, const QString& pattern,
                                   const QString& flags, bool regexMode)
{
    m_patternEdit->setText(bounded((pattern.isEmpty() && !regexMode) ? plainText : pattern, kMaximumInput));
    m_flagsEdit->setText(bounded(flags, kMaximumFlags));
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
}

int CRegexBuilderDialog::execAnchored(QWidget* origin)
{
    if (!origin)
        return QDialog::Rejected;
    m_origin = origin;
    positionBesideOrigin();
    QTimer::singleShot(0, m_patternEdit, [this] {
        m_patternEdit->setFocus(Qt::OtherFocusReason);
        m_patternEdit->selectAll();
    });
    return exec();
}

void CRegexBuilderDialog::updatePreview()
{
    QString error;
    const QRegularExpression expression = compile(m_patternEdit->text(), m_flagsEdit->text(), &error);
    const bool valid = expression.isValid() && error.isEmpty();
    m_validationLabel->setProperty("valid", valid);
    m_validationLabel->setText(valid ? tr("Pattern is valid.") : tr("Invalid pattern: %1").arg(error));
    m_applyButton->setEnabled(valid);
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);

    if (!valid) {
        m_previewEdit->setPlainText(tr("Fix the pattern to preview matches."));
        return;
    }

    QStringList lines;
    QRegularExpressionMatchIterator iterator = expression.globalMatch(m_sampleEdit->toPlainText());
    int matchCount = 0;
    while (iterator.hasNext() && matchCount < kMaximumMatches) {
        const QRegularExpressionMatch match = iterator.next();
        lines << tr("Match %1: %2").arg(matchCount + 1).arg(match.captured(0));
        const int captures = qMin(match.lastCapturedIndex(), kMaximumCaptures);
        for (int i = 1; i <= captures; ++i)
            lines << tr("  Group %1: %2").arg(i).arg(match.captured(i));
        ++matchCount;
        if (match.capturedLength(0) == 0 && match.capturedStart(0) >= m_sampleEdit->toPlainText().size())
            break;
    }
    if (lines.isEmpty())
        lines << tr("No matches.");
    if (iterator.hasNext())
        lines << tr("Preview stopped after %1 matches.").arg(kMaximumMatches);
    m_previewEdit->setPlainText(lines.join(QLatin1Char('\n')));
}

void CRegexBuilderDialog::applyPattern()
{
    QString error;
    const QRegularExpression expression = compile(m_patternEdit->text(), m_flagsEdit->text(), &error);
    if (!expression.isValid() || !error.isEmpty())
        return;
    emit patternApplied(m_patternEdit->text(), m_flagsEdit->text());
    accept();
}

void CRegexBuilderDialog::keepPlainText()
{
    emit plainTextRequested(m_patternEdit->text());
    accept();
}

void CRegexBuilderDialog::insertToken()
{
    auto* button = qobject_cast<QToolButton*>(sender());
    if (!button)
        return;
    const QString token = button->property("regexToken").toString();
    int position = m_patternEdit->cursorPosition();
    QString value = m_patternEdit->text();
    value.insert(position, token);
    m_patternEdit->setText(value.left(kMaximumInput));
    if (token == QStringLiteral("()") || token == QStringLiteral("[]"))
        ++position;
    else if (token == QStringLiteral("(?:)"))
        position += 3;
    else
        position += token.size();
    m_patternEdit->setCursorPosition(qMin(position, m_patternEdit->text().size()));
    m_patternEdit->setFocus(Qt::OtherFocusReason);
}

void CRegexBuilderDialog::positionBesideOrigin()
{
    if (!m_origin)
        return;
    const QPoint anchor = m_origin->mapToGlobal(QPoint(0, m_origin->height()));
    QScreen* screen = QGuiApplication::screenAt(anchor);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    QRect available = screen ? screen->availableGeometry() : QRect(anchor, size());
    QPoint target(anchor.x() + m_origin->width() - width(), anchor.y() + 8);
    target.setX(qBound(available.left(), target.x(), available.right() - width()));
    target.setY(qBound(available.top(), target.y(), available.bottom() - height()));
    move(target);
}

void CRegexBuilderDialog::restoreOriginFocus()
{
    if (!m_origin)
        return;
    QPointer<QWidget> origin = m_origin;
    QTimer::singleShot(0, origin, [origin] { if (origin) origin->setFocus(Qt::OtherFocusReason); });
}

QRegularExpression CRegexBuilderDialog::compile(const QString& pattern, const QString& flags, QString* error)
{
    if (pattern.size() > kMaximumInput) {
        if (error) *error = tr("Pattern is too long.");
        return QRegularExpression();
    }
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    QSet<QChar> seen;
    for (const QChar flag : flags) {
        if (seen.contains(flag)) {
            if (error) *error = tr("Flag '%1' appears more than once.").arg(flag);
            return QRegularExpression();
        }
        seen.insert(flag);
        switch (flag.unicode()) {
        case 'i': options |= QRegularExpression::CaseInsensitiveOption; break;
        case 'm': options |= QRegularExpression::MultilineOption; break;
        case 's': options |= QRegularExpression::DotMatchesEverythingOption; break;
        case 'x': options |= QRegularExpression::ExtendedPatternSyntaxOption; break;
        case 'U': options |= QRegularExpression::InvertedGreedinessOption; break;
        default:
            if (error) *error = tr("Unsupported flag '%1'. Use i, m, s, x, or U.").arg(flag);
            return QRegularExpression();
        }
    }
    const QRegularExpression expression(pattern, options);
    if (!expression.isValid() && error)
        *error = expression.errorString();
    return expression;
}
