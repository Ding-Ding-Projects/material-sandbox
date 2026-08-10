#include "stdafx.h"
#include "RegexBuilderDialog.h"

#include "M3DialogHost.h"

#include <QAccessible>
#include <QBoxLayout>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMimeData>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QSet>
#include <QSizePolicy>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStyle>
#include <QStringList>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>

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
const int kMaximumPreviewUtf8Bytes = 16 * 1024;
const int kPreviewReservedUtf8Bytes = 256;
const int kMinimumInteractiveTarget = 40;
const int kMinimumAnchoredHeight = 240;
const int kResponsiveLayoutWidth = 520;
const int kTokenButtonWidth = 72;

class BoundedPlainTextEdit final : public QPlainTextEdit
{
public:
    typedef std::function<void(const QString&)> RejectHandler;

    BoundedPlainTextEdit(int maximumUtf16Units,
                         const RejectHandler& rejectHandler,
                         QWidget* parent)
        : QPlainTextEdit(parent),
          m_maximumUtf16Units(maximumUtf16Units),
          m_rejectHandler(rejectHandler)
    {
    }

protected:
    void insertFromMimeData(const QMimeData* source) override
    {
        if (source && source->hasText() && !canInsert(source->text(), 0)) {
            rejectInput();
            return;
        }
        QPlainTextEdit::insertFromMimeData(source);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        const QString inserted = event ? event->text() : QString();
        if (!inserted.isEmpty()
            && event->key() != Qt::Key_Backspace
            && event->key() != Qt::Key_Delete
            && !canInsert(inserted, 0)) {
            rejectInput();
            event->accept();
            return;
        }
        QPlainTextEdit::keyPressEvent(event);
    }

    void inputMethodEvent(QInputMethodEvent* event) override
    {
        if (event && !event->commitString().isEmpty()
            && !canInsert(event->commitString(), event->replacementLength())) {
            rejectInput();
            event->accept();
            return;
        }
        QPlainTextEdit::inputMethodEvent(event);
    }

private:
    bool canInsert(const QString& inserted, int replacementLength) const
    {
        const QString current = toPlainText();
        const QTextCursor cursor = textCursor();
        const int selectedUtf16Units = cursor.hasSelection()
            ? cursor.selectionEnd() - cursor.selectionStart()
            : 0;
        const int removedUtf16Units = qMin(current.size(), qMax(selectedUtf16Units,
                                                                  qMax(0, replacementLength)));
        return current.size() - removedUtf16Units + inserted.size()
            <= m_maximumUtf16Units;
    }

    void rejectInput()
    {
        if (m_rejectHandler) {
            m_rejectHandler(QObject::tr(
                "The sample input was not inserted because it would exceed %1 UTF-16 text units.")
                                .arg(m_maximumUtf16Units));
        }
    }

    int m_maximumUtf16Units;
    RejectHandler m_rejectHandler;
};

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

QRegularExpression InvalidExpression()
{
    // A default-constructed QRegularExpression is a valid empty pattern and can
    // match every position.  Invalid input must never escape as that expression.
    return QRegularExpression(QStringLiteral("["));
}

QString DisplayCapture(const QString& value)
{
    QString display = value;
    display.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    display.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
    display.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
    if (display.size() > 120) {
        QString prefix = display.left(96);
        if (!prefix.isEmpty() && prefix.at(prefix.size() - 1).isHighSurrogate())
            prefix.chop(1);
        display = prefix + QObject::tr("... [capture truncated]");
    }
    return display;
}

bool IsSimplePreviewEscape(const QChar character)
{
    switch (character.unicode()) {
    case 'd': case 'D': case 's': case 'S': case 'w': case 'W':
    case 'b': case 'B': case 'A': case 'Z': case 'z':
    case 'n': case 'r': case 't': case 'f': case 'v':
    case '\\': case '.': case '^': case '$': case '(':
    case ')': case '[': case ']': case '{': case '}':
    case '|': case '*': case '+': case '?': case '-':
        return true;
    default:
        return false;
    }
}

bool IsHardBoundedPreviewPattern(const QString& pattern, QString* reason)
{
    // QRegularExpression has no cross-version execution-time budget.  The
    // preview and Apply therefore permit only a deliberately fixed-width grammar:
    // literals, classes, groups, anchors, dot and known single-character
    // escapes.  It excludes every operator that can introduce unbounded or
    // ambiguous backtracking before calling globalMatch on the UI thread.
    bool insideClass = false;
    for (int index = 0; index < pattern.size(); ++index) {
        const QChar character = pattern.at(index);

        if (character == QLatin1Char('\\')) {
            if (++index >= pattern.size() || !IsSimplePreviewEscape(pattern.at(index))) {
                if (reason)
                    *reason = QObject::tr("this pattern uses an escape outside the preview-safe subset");
                return false;
            }
            continue;
        }

        if (insideClass) {
            if (character == QLatin1Char('[')) {
                if (reason)
                    *reason = QObject::tr("nested or POSIX character classes are not previewed");
                return false;
            }
            if (character == QLatin1Char(']'))
                insideClass = false;
            continue;
        }

        switch (character.unicode()) {
        case '[':
            insideClass = true;
            break;
        case '*': case '+': case '?': case '{': case '}': case '|':
            if (reason)
                *reason = QObject::tr("quantifiers, alternation, and advanced groups need a bounded execution contract");
            return false;
        case ']':
            if (reason)
                *reason = QObject::tr("this pattern contains an unmatched character-class delimiter");
            return false;
        default:
            break;
        }
    }

    if (insideClass) {
        if (reason)
            *reason = QObject::tr("this pattern contains an unterminated character class");
        return false;
    }
    return true;
}

} // namespace

CRegexBuilderDialog::CRegexBuilderDialog(QWidget* parent)
    : QDialog(parent),
      m_patternEdit(new QLineEdit(this)),
      m_flagsEdit(new QLineEdit(this)),
      m_sampleEdit(new BoundedPlainTextEdit(
          kMaximumSampleLength,
          [this](const QString& message) { announceInputRejection(message); },
          this)),
      m_validationLabel(new QLabel(this)),
      m_previewEdit(new QPlainTextEdit(this)),
      m_tokenGrid(new QGridLayout()),
      m_flagsSampleLayout(new QBoxLayout(QBoxLayout::LeftToRight)),
      m_actionsLayout(new QBoxLayout(QBoxLayout::LeftToRight)),
      m_cancelButton(new QPushButton(tr("Cancel"), this)),
      m_plainButton(new QPushButton(tr("Keep plain text"), this)),
      m_applyButton(new QPushButton(tr("Apply pattern"), this)),
      m_valid(false),
      m_previewSafe(false),
      m_restoreOriginFocus(true),
      m_repositionScheduled(false),
      m_positioning(false),
      m_reflowingTokens(false),
      m_userResized(false),
      m_fullscreenFallback(false)
{
    setObjectName(QStringLiteral("regexBuilderDialog"));
    setWindowTitle(tr("Regex builder"));
    setModal(false);
    setWindowModality(Qt::NonModal);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setSizeGripEnabled(true);
    setMinimumSize(0, 0);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("regexBuilderContent"));
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(kDialogPadding, kDialogPadding,
                                      kDialogPadding, kDialogPadding);
    contentLayout->setSpacing(16);

    auto* header = new QWidget(content);
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);
    auto* eyebrow = new QLabel(tr("Search tool"), header);
    eyebrow->setObjectName(QStringLiteral("regexBuilderEyebrow"));
    eyebrow->setAccessibleName(tr("Regex builder category"));
    headerLayout->addWidget(eyebrow);
    auto* introduction = new QLabel(
        tr("Qt QRegularExpression dialect - plain text stays the default until you apply a pattern."),
        header);
    introduction->setObjectName(QStringLiteral("regexBuilderIntroduction"));
    introduction->setWordWrap(true);
    headerLayout->addWidget(introduction);
    contentLayout->addWidget(header);

    auto* tokenLabel = new QLabel(tr("Guided tokens"), content);
    tokenLabel->setObjectName(QStringLiteral("regexSectionLabel"));
    contentLayout->addWidget(tokenLabel);

    m_tokenGrid->setContentsMargins(0, 0, 0, 0);
    m_tokenGrid->setHorizontalSpacing(8);
    m_tokenGrid->setVerticalSpacing(8);
    const int tokenCount = static_cast<int>(sizeof(kGuidedTokens) / sizeof(kGuidedTokens[0]));
    for (int index = 0; index < tokenCount; ++index) {
        const GuidedToken& guided = kGuidedTokens[index];
        auto* button = new QToolButton(content);
        const QString token = QString::fromLatin1(guided.token);
        const QString description = tr(guided.description);
        button->setText(token);
        button->setObjectName(QStringLiteral("regexGuidedToken"));
        button->setProperty("regexToken", token);
        button->setProperty("m3", QStringLiteral("chip"));
        button->setMinimumSize(kMinimumInteractiveTarget, kMinimumInteractiveTarget);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setToolTip(description);
        button->setAccessibleName(tr("Insert %1: %2").arg(token, description));
        button->setAccessibleDescription(tr("Inserts this token at the pattern cursor without exceeding the 500-character limit."));
        connect(button, &QToolButton::clicked,
                this, &CRegexBuilderDialog::addGuidedToken);
        m_tokenButtons.append(button);
    }
    contentLayout->addLayout(m_tokenGrid);

    m_patternEdit->setObjectName(QStringLiteral("regexPatternEdit"));
    m_patternEdit->setMinimumHeight(56);
    m_patternEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_patternEdit->setAccessibleName(tr("Regular expression pattern"));
    m_patternEdit->setAccessibleDescription(
        tr("Qt QRegularExpression pattern. Entries over 500 UTF-16 text units are rejected and announced; they are not silently shortened."));
    auto* patternLabel = new QLabel(tr("Pattern"), content);
    patternLabel->setBuddy(m_patternEdit);
    contentLayout->addWidget(patternLabel);
    contentLayout->addWidget(m_patternEdit);

    auto* flagsColumn = new QWidget(content);
    flagsColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* flagsLayout = new QVBoxLayout(flagsColumn);
    flagsLayout->setContentsMargins(0, 0, 0, 0);
    flagsLayout->setSpacing(6);
    m_flagsEdit->setObjectName(QStringLiteral("regexFlagsEdit"));
    m_flagsEdit->setMinimumHeight(56);
    m_flagsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_flagsEdit->setPlaceholderText(QStringLiteral("i"));
    m_flagsEdit->setAccessibleName(tr("Regular expression flags"));
    m_flagsEdit->setAccessibleDescription(
        tr("Optional flags i, m, s, x, and U, without duplicates. Entries over 8 UTF-16 text units are rejected and announced."));
    auto* flagsLabel = new QLabel(tr("Flags"), flagsColumn);
    flagsLabel->setBuddy(m_flagsEdit);
    flagsLayout->addWidget(flagsLabel);
    flagsLayout->addWidget(m_flagsEdit);

    auto* sampleColumn = new QWidget(content);
    sampleColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* sampleLayout = new QVBoxLayout(sampleColumn);
    sampleLayout->setContentsMargins(0, 0, 0, 0);
    sampleLayout->setSpacing(6);
    m_sampleEdit->setObjectName(QStringLiteral("regexSampleEdit"));
    m_sampleEdit->setPlaceholderText(tr("Type or paste text to test locally"));
    m_sampleEdit->setAccessibleName(tr("Regular expression sample text"));
    m_sampleEdit->setAccessibleDescription(
        tr("Local sample text. Entries over 500 UTF-16 text units are rejected and announced before they are inserted; they are not silently shortened."));
    m_sampleEdit->setMinimumHeight(96);
    m_sampleEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* sampleLabel = new QLabel(tr("Sample text"), sampleColumn);
    sampleLabel->setBuddy(m_sampleEdit);
    sampleLayout->addWidget(sampleLabel);
    sampleLayout->addWidget(m_sampleEdit);

    m_flagsSampleLayout->setContentsMargins(0, 0, 0, 0);
    m_flagsSampleLayout->setSpacing(12);
    m_flagsSampleLayout->addWidget(flagsColumn, 1);
    m_flagsSampleLayout->addWidget(sampleColumn, 2);
    contentLayout->addLayout(m_flagsSampleLayout);

    m_validationLabel->setObjectName(QStringLiteral("regexValidation"));
    m_validationLabel->setWordWrap(true);
    m_validationLabel->setAccessibleName(tr("Pattern validation"));
    m_validationLabel->setAccessibleDescription(tr("Live pattern validation status."));
    contentLayout->addWidget(m_validationLabel);

    auto* previewLabel = new QLabel(tr("Matches and capture groups"), content);
    previewLabel->setBuddy(m_previewEdit);
    contentLayout->addWidget(previewLabel);
    m_previewEdit->setObjectName(QStringLiteral("regexPreview"));
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setMinimumHeight(128);
    m_previewEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_previewEdit->setAccessibleName(tr("Regular expression match preview"));
    m_previewEdit->setAccessibleDescription(
        tr("A bounded local preview. At most 128 matches, 32 capture groups per match, and 16 KiB of UTF-8 output are shown."));
    contentLayout->addWidget(m_previewEdit);

    auto* bounds = new QLabel(
        tr("This builder keeps samples only for the current dialog and writes no pattern or sample to disk. It evaluates only the current dialog text on this computer. Applying a pattern updates the originating search field. Pattern and sample inputs are each limited to 500 UTF-16 text units; previews are limited to 128 matches, 32 capture groups per match, and 16 KiB of UTF-8 output. Quantifiers, alternation, and advanced groups stay available for editing but are not applied until this field has a bounded execution contract."),
        content);
    bounds->setObjectName(QStringLiteral("regexBounds"));
    bounds->setWordWrap(true);
    contentLayout->addWidget(bounds);

    m_cancelButton->setObjectName(QStringLiteral("regexCancelButton"));
    m_plainButton->setObjectName(QStringLiteral("regexPlainButton"));
    m_applyButton->setObjectName(QStringLiteral("regexApplyButton"));
    m_cancelButton->setProperty("m3", QStringLiteral("text"));
    m_plainButton->setProperty("m3", QStringLiteral("outlined"));
    m_applyButton->setProperty("m3", QStringLiteral("filled"));
    m_cancelButton->setMinimumHeight(kMinimumInteractiveTarget);
    m_plainButton->setMinimumHeight(kMinimumInteractiveTarget);
    m_applyButton->setMinimumHeight(kMinimumInteractiveTarget);
    m_cancelButton->setMinimumWidth(kMinimumInteractiveTarget);
    m_plainButton->setMinimumWidth(kMinimumInteractiveTarget);
    m_applyButton->setMinimumWidth(kMinimumInteractiveTarget);
    m_cancelButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_plainButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_applyButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_applyButton->setAutoDefault(false);
    m_applyButton->setDefault(false);
    m_cancelButton->setAccessibleDescription(tr("Close without changing the originating search field."));
    m_plainButton->setAccessibleDescription(
        tr("Use the current pattern characters as literal plain-text search text."));
    m_applyButton->setAccessibleDescription(
        tr("Apply the valid pattern and flags to this builder's originating search field."));
    m_actionsLayout->setContentsMargins(0, 4, 0, 0);
    m_actionsLayout->setSpacing(8);
    m_actionsLayout->setAlignment(Qt::AlignRight);
    m_actionsLayout->addWidget(m_cancelButton);
    m_actionsLayout->addWidget(m_plainButton);
    m_actionsLayout->addWidget(m_applyButton);
    contentLayout->addLayout(m_actionsLayout);

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
        " background: palette(window); color: palette(window-text); }"
        "QDialog#regexBuilderDialog { border-radius: 28px; }"
        "QScrollArea#regexBuilderScroller { border: 0; background: palette(window); }"
        "QLabel#regexBuilderEyebrow { color: palette(highlight); font-size: 11px; font-weight: 600;"
        " letter-spacing: 1px; text-transform: uppercase; }"
        "QLabel#regexBuilderIntroduction, QLabel#regexBounds { color: palette(text); }"
        "QLabel#regexSectionLabel { color: palette(window-text); font-weight: 600; }"
        "QLineEdit#regexPatternEdit, QLineEdit#regexFlagsEdit, QPlainTextEdit#regexSampleEdit {"
        " background: palette(base); color: palette(text); border: 1px solid palette(mid);"
        " border-radius: 4px; padding: 0 12px; }"
        "QPlainTextEdit#regexSampleEdit { padding: 8px 12px; }"
        "QLineEdit#regexPatternEdit:focus, QLineEdit#regexFlagsEdit:focus,"
        " QPlainTextEdit#regexSampleEdit:focus, QPlainTextEdit#regexPreview:focus {"
        " border: 2px solid palette(highlight); }"
        "QLabel#regexValidation { border-radius: 12px; padding: 12px 16px; }"
        "QLabel#regexValidation[valid=\"true\"][m3Dark=\"false\"] { background: #D7F5C4; color: #0B2000; }"
        "QLabel#regexValidation[valid=\"true\"][m3Dark=\"true\"] { background: #254D14; color: #CFF7B4; }"
        "QLabel#regexValidation[valid=\"false\"][m3Dark=\"false\"] { background: #F9DEDC; color: #410E0B;"
        " border: 1px solid #B3261E; font-weight: 600; }"
        "QLabel#regexValidation[valid=\"false\"][m3Dark=\"true\"] { background: #8C1D18; color: #F9DEDC;"
        " border: 1px solid #F2B8B5; font-weight: 600; }"
        "QPlainTextEdit#regexPreview { background: palette(alternate-base); color: palette(text);"
        " border: 1px solid palette(mid); border-radius: 12px; padding: 8px 12px;"
        " font-family: 'Roboto Mono', Consolas, monospace; }"
        "QPlainTextEdit#regexPreview[previewSafe=\"false\"][m3Dark=\"false\"] {"
        " background: #F9DEDC; color: #410E0B; border-color: #B3261E; }"
        "QPlainTextEdit#regexPreview[previewSafe=\"false\"][m3Dark=\"true\"] {"
        " background: #8C1D18; color: #F9DEDC; border-color: #F2B8B5; }"
        "QToolButton#regexGuidedToken { background: transparent; color: palette(window-text);"
        " border: 1px solid palette(mid); border-radius: 8px; padding: 0 8px;"
        " font-family: 'Roboto Mono', Consolas, monospace; }"
        "QToolButton#regexGuidedToken:hover, QToolButton#regexGuidedToken:focus { background: palette(alternate-base);"
        " border-color: palette(highlight); }"
        "QPushButton#regexCancelButton, QPushButton#regexPlainButton, QPushButton#regexApplyButton {"
        " border-radius: 20px; padding: 0 20px; font-weight: 600; }"
        "QPushButton#regexCancelButton { background: transparent; color: palette(link); border: 0; }"
        "QPushButton#regexPlainButton { background: transparent; color: palette(window-text); border: 1px solid palette(mid); }"
        "QPushButton#regexApplyButton { background: palette(highlight); color: palette(highlighted-text); border: 0; }"
        "QPushButton#regexCancelButton:hover, QPushButton#regexPlainButton:hover { background: palette(alternate-base); }"
        "QPushButton#regexApplyButton:hover { background: palette(highlight); }"
        "QPushButton#regexCancelButton:focus, QPushButton#regexPlainButton:focus, QPushButton#regexApplyButton:focus {"
        " outline: 2px solid palette(highlight); }"));

    connect(m_patternEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        updatePreview();
    });
    connect(m_flagsEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        updatePreview();
    });
    connect(m_sampleEdit, &QPlainTextEdit::textChanged, this, [this]() {
        updatePreview();
    });
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_plainButton, &QPushButton::clicked,
            this, &CRegexBuilderDialog::keepPlainText);
    connect(m_applyButton, &QPushButton::clicked,
            this, &CRegexBuilderDialog::applyPattern);
    connect(this, &QDialog::finished, this, [this](int) { restoreOriginFocus(); });

    auto* applyReturn = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(applyReturn, &QShortcut::activated, this, &CRegexBuilderDialog::applyPattern);
    auto* applyEnter = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Enter), this);
    connect(applyEnter, &QShortcut::activated, this, &CRegexBuilderDialog::applyPattern);

    M3DialogHost::Install(this);
    if (auto* closeButton = findChild<QPushButton*>(QStringLiteral("m3DialogClose"))) {
        closeButton->setMinimumSize(kMinimumInteractiveTarget, kMinimumInteractiveTarget);
        closeButton->setAccessibleDescription(tr("Close the regex builder without changing the search field."));
        setTabOrder(closeButton, m_patternEdit);
        setTabOrder(m_applyButton, closeButton);
    }

    setTabOrder(m_patternEdit, m_flagsEdit);
    setTabOrder(m_flagsEdit, m_sampleEdit);
    setTabOrder(m_sampleEdit, m_previewEdit);
    QWidget* previous = m_previewEdit;
    for (QToolButton* token : m_tokenButtons) {
        setTabOrder(previous, token);
        previous = token;
    }
    setTabOrder(previous, m_cancelButton);
    setTabOrder(m_cancelButton, m_plainButton);
    setTabOrder(m_plainButton, m_applyButton);

    updateResponsiveLayout();
    updatePreview();
}

void CRegexBuilderDialog::setState(const QString& plainText,
                                   const QString& pattern,
                                   const QString& flags,
                                   bool regexMode)
{
    const QString initialPattern = pattern.isEmpty() && !regexMode ? plainText : pattern;
    const QSignalBlocker patternBlocker(m_patternEdit);
    const QSignalBlocker flagsBlocker(m_flagsEdit);
    const QSignalBlocker sampleBlocker(m_sampleEdit);
    m_patternEdit->setText(initialPattern);
    m_flagsEdit->setText(flags);
    m_sampleEdit->clear();
    updatePreview();
}

void CRegexBuilderDialog::openAnchored(QWidget* origin)
{
    if (!origin)
        return;

    m_origin = origin;
    m_restoreOriginFocus = true;
    m_userResized = false;
    watchOriginGeometry();
    show();
    // Showing a dialog may emit a layout-driven resize.  It is not a user resize.
    m_userResized = false;
    raise();
    activateWindow();
    updateResponsiveLayout();
    positionBesideOrigin();

    // The builder has one intentional entry point: the pattern editor.  Its
    // result/cancel paths choose their destination separately in restoreOriginFocus.
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
    if (pattern.isEmpty()) {
        if (error)
            *error = tr("Enter a pattern before applying it.");
        return InvalidExpression();
    }
    if (pattern.size() > kMaximumPatternLength) {
        if (error)
            *error = tr("Patterns are limited to %1 UTF-16 text units. Reduce the input; it was not shortened.")
                         .arg(kMaximumPatternLength);
        return InvalidExpression();
    }
    if (flags.size() > kMaximumFlagsLength) {
        if (error)
            *error = tr("Flags are limited to %1 UTF-16 text units. Reduce the input; it was not shortened.")
                         .arg(kMaximumFlagsLength);
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
    if (!expression.isValid()) {
        if (error)
            *error = expression.errorString();
        return InvalidExpression();
    }
    return expression;
}

void CRegexBuilderDialog::addGuidedToken()
{
    auto* button = qobject_cast<QToolButton*>(sender());
    if (!button)
        return;

    const QString token = button->property("regexToken").toString();
    const int cursor = m_patternEdit->cursorPosition();
    QString next = m_patternEdit->text();
    next.insert(cursor, token);
    if (next.size() > kMaximumPatternLength) {
        announceInputRejection(tr("The guided token was not inserted because the pattern would exceed %1 UTF-16 text units.")
                             .arg(kMaximumPatternLength));
        return;
    }

    m_patternEdit->setText(next);
    m_patternEdit->setCursorPosition(qMin(cursor + token.size(), next.size()));
    m_patternEdit->setFocus(Qt::OtherFocusReason);
}

void CRegexBuilderDialog::updatePreview()
{
    m_error.clear();
    m_previewSafe = false;
    const QString sample = sampleText();
    QRegularExpression expression = InvalidExpression();

    if (sample.size() > kMaximumSampleLength) {
        m_error = tr("Sample text is limited to %1 UTF-16 text units. Reduce the input; it was not shortened.")
                      .arg(kMaximumSampleLength);
    } else {
        expression = compile(pattern(), flags(), &m_error);
    }

    m_valid = m_error.isEmpty() && expression.isValid();
    QString validationText;
    QString preview;
    if (!m_valid) {
        validationText = tr("Invalid pattern: %1").arg(m_error);
        preview = tr("Fix the pattern, flags, or input limit before previewing matches. No expression is exposed while this state is invalid.");
    } else {
        QString previewReason;
        m_previewSafe = IsHardBoundedPreviewPattern(pattern(), &previewReason);
        if (m_previewSafe) {
            validationText = tr("Valid Qt QRegularExpression - bounded local preview available.");
            preview = previewText(expression);
        } else {
            validationText = tr("Valid Qt QRegularExpression - unavailable for this unbounded search field.");
            preview = tr("This valid pattern is not previewed or applied because %1. Quantifiers, alternation, and advanced groups remain available for editing, but this field needs a bounded execution contract before it can run them.")
                          .arg(previewReason);
        }
    }

    m_applyButton->setEnabled(m_valid && m_previewSafe);
    if (!m_valid) {
        m_applyButton->setAccessibleDescription(
            tr("Apply is unavailable until the pattern, flags, and input limits are valid."));
    } else if (!m_previewSafe) {
        m_applyButton->setAccessibleDescription(
            tr("Apply is unavailable because this valid pattern needs a bounded execution contract before this search field can run it."));
    } else {
        m_applyButton->setAccessibleDescription(
            tr("Apply this valid bounded pattern and flags to this builder's originating search field."));
    }

    const bool m3Dark = palette().color(QPalette::Window).lightness() < 128;
    m_validationLabel->setProperty("valid", m_valid);
    m_validationLabel->setProperty("m3Dark", m3Dark);
    m_previewEdit->setProperty("previewSafe", m_previewSafe);
    m_previewEdit->setProperty("m3Dark", m3Dark);
    announceValidation(validationText);
    m_previewEdit->setPlainText(preview);
    m_previewEdit->setAccessibleDescription(m_previewSafe
        ? tr("Bounded local regular-expression match preview. It is limited to 128 matches, 32 capture groups per match, and 16 KiB of UTF-8 output.")
        : tr("Regular-expression preview status. The current valid pattern is not executed or applied because this search field has no bounded execution contract for it."));

    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);
    m_previewEdit->style()->unpolish(m_previewEdit);
    m_previewEdit->style()->polish(m_previewEdit);
}

void CRegexBuilderDialog::keepPlainText()
{
    m_restoreOriginFocus = false;
    emit plainTextRequested(pattern());
    accept();
}

void CRegexBuilderDialog::applyPattern()
{
    if (!m_applyButton->isEnabled() || !m_valid || !m_previewSafe)
        return;
    m_restoreOriginFocus = false;
    emit patternApplied(pattern(), flags());
    accept();
}

void CRegexBuilderDialog::changeEvent(QEvent* event)
{
    QDialog::changeEvent(event);
    if (event && event->type() == QEvent::PaletteChange)
        updatePreview();
}

bool CRegexBuilderDialog::eventFilter(QObject* watched, QEvent* event)
{
    bool watchesOrigin = false;
    for (const QPointer<QWidget>& watcher : m_originWatchers) {
        if (watcher.data() == watched) {
            watchesOrigin = true;
            break;
        }
    }

    if (watchesOrigin && isVisible()) {
        switch (event->type()) {
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::ShowToParent:
        case QEvent::WindowStateChange:
        case QEvent::LayoutRequest:
            scheduleReposition();
            break;
        default:
            break;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CRegexBuilderDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updateResponsiveLayout();
    if (!m_positioning && isVisible()) {
        m_userResized = true;
        scheduleReposition();
    }
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
    if (viewport.width() <= 0 || viewport.height() <= 0)
        return;

    const int desiredWidth = qMin(kDialogWidth, viewport.width());
    const int desiredHeight = qMin(qMax(sizeHint().height(), kMinimumAnchoredHeight),
                                   qMax(1, qRound(viewport.height() * 0.88)));
    const int belowSpace = qMax(0, viewport.bottom() - originRect.bottom() - kAnchorGap + 1);
    const int aboveSpace = qMax(0, originRect.top() - viewport.top() - kAnchorGap);
    const bool enoughRoomToAnchor = qMax(belowSpace, aboveSpace) >= kMinimumAnchoredHeight;
    const bool placeBelow = belowSpace >= desiredHeight || belowSpace >= aboveSpace;
    const int sideSpace = placeBelow ? belowSpace : aboveSpace;
    if (!enoughRoomToAnchor && !m_fullscreenFallback)
        m_userResized = false;
    m_fullscreenFallback = !enoughRoomToAnchor;
    setProperty("fullscreenFallback", m_fullscreenFallback);
    setSizeGripEnabled(!m_fullscreenFallback);
    setWindowTitle(m_fullscreenFallback
        ? tr("Regex builder (full screen)")
        : tr("Regex builder"));
    if (auto* title = findChild<QLabel*>(QStringLiteral("m3DialogTitleLabel"))) {
        title->setText(m_fullscreenFallback
            ? tr("Regex builder (full screen)")
            : tr("Regex builder"));
    }

    const int finalWidth = m_fullscreenFallback
        ? viewport.width()
        : (m_userResized ? qMin(qMax(kMinimumInteractiveTarget, width()), viewport.width())
                         : desiredWidth);
    const int finalHeight = m_fullscreenFallback
        ? viewport.height()
        : (m_userResized ? qMin(qMax(kMinimumAnchoredHeight, height()), sideSpace)
                         : qMin(desiredHeight, sideSpace));

    m_positioning = true;
    // Collapse wide rows before resize calculates its minimum width.  Otherwise
    // a narrow viewport can be held open by the previous horizontal action row.
    updateResponsiveLayout(finalWidth);
    resize(qMax(1, finalWidth), qMax(1, finalHeight));
    updateResponsiveLayout();
    if (m_fullscreenFallback) {
        move(viewport.topLeft());
        m_positioning = false;
        return;
    }

    const int maximumX = viewport.right() - width() + 1;
    const int x = qBound(viewport.left(), originRect.left(), qMax(viewport.left(), maximumX));
    const int y = placeBelow
        ? originRect.bottom() + kAnchorGap + 1
        : originRect.top() - kAnchorGap - height();
    move(x, qBound(viewport.top(), y, qMax(viewport.top(), viewport.bottom() - height() + 1)));
    m_positioning = false;
}

void CRegexBuilderDialog::restoreOriginFocus()
{
    clearOriginGeometryWatchers();
    if (!m_restoreOriginFocus || !m_origin)
        return;

    QPointer<QWidget> origin = m_origin;
    QTimer::singleShot(0, this, [origin]() {
        if (origin && origin->isVisible() && origin->isEnabled())
            origin->setFocus(Qt::OtherFocusReason);
    });
}

void CRegexBuilderDialog::updateResponsiveLayout(int availableWidth)
{
    const int layoutWidth = availableWidth >= 0 ? availableWidth : width();
    const bool narrow = layoutWidth > 0 && layoutWidth < kResponsiveLayoutWidth;
    m_flagsSampleLayout->setDirection(narrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    m_actionsLayout->setDirection(narrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    m_actionsLayout->setAlignment(narrow ? Qt::Alignment(Qt::AlignRight)
                                         : Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    reflowGuidedTokens(layoutWidth);
}

void CRegexBuilderDialog::reflowGuidedTokens(int availableWidth)
{
    if (m_reflowingTokens)
        return;
    m_reflowingTokens = true;

    const int tokenAreaWidth = qMax(1, (availableWidth >= 0 ? availableWidth : width())
                                       - 2 * kDialogPadding);
    const int columns = qMax(1, (tokenAreaWidth + m_tokenGrid->horizontalSpacing()) /
                                   (kTokenButtonWidth + m_tokenGrid->horizontalSpacing()));
    while (QLayoutItem* item = m_tokenGrid->takeAt(0))
        delete item;
    for (int index = 0; index < m_tokenButtons.size(); ++index)
        m_tokenGrid->addWidget(m_tokenButtons.at(index), index / columns, index % columns);
    m_tokenGrid->invalidate();
    m_reflowingTokens = false;
}

void CRegexBuilderDialog::watchOriginGeometry()
{
    clearOriginGeometryWatchers();
    for (QWidget* widget = m_origin.data(); widget; widget = widget->parentWidget()) {
        widget->installEventFilter(this);
        m_originWatchers.append(widget);
    }
}

void CRegexBuilderDialog::clearOriginGeometryWatchers()
{
    for (const QPointer<QWidget>& watcher : m_originWatchers) {
        if (watcher)
            watcher->removeEventFilter(this);
    }
    m_originWatchers.clear();
}

void CRegexBuilderDialog::scheduleReposition()
{
    if (!isVisible() || !m_origin || m_repositionScheduled || m_positioning)
        return;
    m_repositionScheduled = true;
    QTimer::singleShot(0, this, [this]() {
        m_repositionScheduled = false;
        if (isVisible() && m_origin)
            positionBesideOrigin();
    });
}

void CRegexBuilderDialog::announceValidation(const QString& text)
{
    m_validationLabel->setText(text);
    m_validationLabel->setAccessibleName(tr("Pattern validation: %1").arg(text));
    m_validationLabel->setAccessibleDescription(tr("Live validation status: %1").arg(text));
    if (m_lastAnnouncedValidation == text)
        return;

    m_lastAnnouncedValidation = text;
    QAccessibleEvent accessibilityEvent(m_validationLabel, QAccessible::NameChanged);
    QAccessible::updateAccessibility(&accessibilityEvent);
}

void CRegexBuilderDialog::announceInputRejection(const QString& message)
{
    m_validationLabel->setProperty("valid", false);
    m_validationLabel->setProperty("m3Dark",
                                   palette().color(QPalette::Window).lightness() < 128);
    announceValidation(message);
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);
}

QString CRegexBuilderDialog::previewText(const QRegularExpression& expression) const
{
    const QString sample = sampleText();
    QRegularExpressionMatchIterator iterator = expression.globalMatch(sample);
    QStringList lines;
    int previewUtf8Bytes = 0;
    int renderedMatchCount = 0;
    bool outputBoundReached = false;

    const auto appendLine = [&lines, &previewUtf8Bytes, &outputBoundReached](const QString& line) {
        const int lineUtf8Bytes = line.toUtf8().size() + 1;
        if (outputBoundReached || previewUtf8Bytes + lineUtf8Bytes
            > kMaximumPreviewUtf8Bytes - kPreviewReservedUtf8Bytes) {
            outputBoundReached = true;
            return false;
        }
        lines << line;
        previewUtf8Bytes += lineUtf8Bytes;
        return true;
    };

    // IsHardBoundedPreviewPattern has already excluded every unbounded or
    // ambiguous operator.  These result limits additionally bound output work.
    while (iterator.hasNext() && renderedMatchCount < kMaximumMatches && !outputBoundReached) {
        const QRegularExpressionMatch match = iterator.next();
        const int nextMatchCount = renderedMatchCount + 1;
        if (!appendLine(tr("Match %1 | index %2 | length %3 | %4")
                            .arg(nextMatchCount)
                            .arg(match.capturedStart(0))
                            .arg(match.capturedLength(0))
                            .arg(DisplayCapture(match.captured(0)))))
            break;
        ++renderedMatchCount;

        const int captureCount = qMin(expression.captureCount(), kMaximumCapturesPerMatch);
        for (int capture = 1; capture <= captureCount && !outputBoundReached; ++capture) {
            if (match.capturedStart(capture) < 0)
                continue;
            appendLine(tr("  Group %1 | index %2 | length %3 | %4")
                           .arg(capture)
                           .arg(match.capturedStart(capture))
                           .arg(match.capturedLength(capture))
                           .arg(DisplayCapture(match.captured(capture))));
        }
    }

    if (renderedMatchCount == 0 && outputBoundReached)
        return tr("Valid | preview reached its 16 KiB UTF-8 output bound before a complete match could be shown.");
    if (renderedMatchCount == 0)
        return tr("Valid | no matches in the sample text.");
    if (iterator.hasNext())
        appendLine(tr("Preview stopped after %1 matches.").arg(kMaximumMatches));
    if (outputBoundReached)
        lines << tr("Preview stopped at the 16 KiB UTF-8 output limit.");
    lines.prepend(tr("Valid | %1 match(es) shown.").arg(renderedMatchCount));
    return lines.join(QLatin1Char('\n'));
}
