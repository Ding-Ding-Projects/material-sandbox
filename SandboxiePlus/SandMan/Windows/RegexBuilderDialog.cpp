#include "stdafx.h"
#include "RegexBuilderDialog.h"

#include "M3DialogHost.h"
#include "M3RegexExecutionPolicy.h"

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
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QStringList>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>

namespace {

constexpr int kDialogWidth = 680;
constexpr int kDialogPadding = 24;
constexpr int kAnchorGap = 8;
constexpr int kViewportMargin = 8;
constexpr int kMinimumInteractiveTarget = 40;
constexpr int kMinimumAnchoredHeight = 240;
constexpr int kResponsiveLayoutWidth = 520;
constexpr int kTokenButtonWidth = 72;
constexpr int kPreviewReservedUtf8Bytes = 256;

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
        const int selectedUnits = cursor.hasSelection()
            ? cursor.selectionEnd() - cursor.selectionStart()
            : 0;
        const int removedUnits = qMin(current.size(), qMax(selectedUnits, qMax(0, replacementLength)));
        return current.size() - removedUnits + inserted.size() <= m_maximumUtf16Units;
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

QString displayCapture(const QString& value)
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

} // namespace

CRegexBuilderDialog::CRegexBuilderDialog(QWidget* parent)
    : QDialog(parent),
      m_patternEdit(new QLineEdit(this)),
      m_flagsEdit(new QLineEdit(this)),
      m_sampleEdit(new BoundedPlainTextEdit(
          M3RegexExecutionPolicy::MaximumSampleLength,
          [this](const QString& message) { announceInputRejection(message); }, this)),
      m_previewEdit(new QPlainTextEdit(this)),
      m_validationLabel(new QLabel(this)),
      m_tokenGrid(new QGridLayout()),
      m_fieldsLayout(new QBoxLayout(QBoxLayout::LeftToRight)),
      m_actionsLayout(new QBoxLayout(QBoxLayout::LeftToRight)),
      m_cancelButton(new QPushButton(tr("Cancel"), this)),
      m_plainButton(new QPushButton(tr("Keep plain text"), this)),
      m_applyButton(new QPushButton(tr("Apply"), this)),
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
    setWindowTitle(tr("Regular expression builder"));
    setWindowFlags(Qt::Dialog);
    setModal(false);
    setWindowModality(Qt::NonModal);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setSizeGripEnabled(true);
    setMinimumSize(0, 0);
    resize(kDialogWidth, 640);

    // Install the shared title host before adding builder content. This keeps
    // the dialog on one layout tree and prevents an already-owned layout from
    // being wrapped a second time when the dialog is shown.
    M3DialogHost::Install(this);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("regexBuilderContent"));
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(kDialogPadding, kDialogPadding, kDialogPadding, kDialogPadding);
    contentLayout->setSpacing(16);

    auto* intro = new QLabel(
        tr("Use Qt regular-expression syntax. Plain text remains the default until you apply a valid pattern with bounded engine execution."),
        content);
    intro->setObjectName(QStringLiteral("regexBuilderIntroduction"));
    intro->setWordWrap(true);
    contentLayout->addWidget(intro);

    auto* patternLabel = new QLabel(tr("Pattern"), content);
    patternLabel->setBuddy(m_patternEdit);
    contentLayout->addWidget(patternLabel);
    m_patternEdit->setObjectName(QStringLiteral("regexPatternEdit"));
    m_patternEdit->setMinimumHeight(48);
    m_patternEdit->setMinimumWidth(0);
    m_patternEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_patternEdit->setPlaceholderText(tr("Example: ^Sandboxie(?<version>.+)$"));
    m_patternEdit->setAccessibleName(tr("Regular expression pattern"));
    m_patternEdit->setAccessibleDescription(
        tr("Qt QRegularExpression pattern. Entries over 500 UTF-16 text units stay visible and are rejected with an announced validation message."));
    contentLayout->addWidget(m_patternEdit);

    auto* tokenLabel = new QLabel(tr("Guided tokens"), content);
    tokenLabel->setObjectName(QStringLiteral("regexSectionLabel"));
    contentLayout->addWidget(tokenLabel);
    m_tokenGrid->setContentsMargins(0, 0, 0, 0);
    m_tokenGrid->setHorizontalSpacing(8);
    m_tokenGrid->setVerticalSpacing(8);
    const QList<QPair<QString, QString>> tokens = {
        { QStringLiteral("."), tr("Any character") },
        { QStringLiteral("\\d"), tr("Digit") },
        { QStringLiteral("\\w"), tr("Word character") },
        { QStringLiteral("\\s"), tr("Whitespace") },
        { QStringLiteral("^"), tr("Start anchor") },
        { QStringLiteral("$"), tr("End anchor") },
        { QStringLiteral("()"), tr("Capture group") },
        { QStringLiteral("(?:)"), tr("Non-capturing group") },
        { QStringLiteral("[]"), tr("Character class") },
        { QStringLiteral("*"), tr("Zero or more") },
        { QStringLiteral("+"), tr("One or more") },
        { QStringLiteral("?"), tr("Optional") },
        { QStringLiteral("|"), tr("Alternation") },
        { QStringLiteral("{2,4}"), tr("Repeat two to four times") }
    };
    for (const QPair<QString, QString>& token : tokens) {
        auto* button = new QToolButton(content);
        button->setObjectName(QStringLiteral("regexGuidedToken"));
        button->setText(token.first);
        button->setToolTip(token.second);
        button->setAccessibleName(tr("Insert %1: %2").arg(token.first, token.second));
        button->setAccessibleDescription(
            tr("Inserts this token at the pattern cursor without exceeding the 500-character limit."));
        button->setProperty("regexToken", token.first);
        button->setProperty("m3", QStringLiteral("chip"));
        button->setMinimumSize(kMinimumInteractiveTarget, kMinimumInteractiveTarget);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(button, &QToolButton::clicked, this, &CRegexBuilderDialog::insertToken);
        m_tokenButtons.append(button);
    }
    contentLayout->addLayout(m_tokenGrid);

    auto* flagsColumn = new QWidget(content);
    flagsColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* flagsLayout = new QVBoxLayout(flagsColumn);
    flagsLayout->setContentsMargins(0, 0, 0, 0);
    flagsLayout->setSpacing(6);
    auto* flagsLabel = new QLabel(tr("Flags"), flagsColumn);
    flagsLabel->setBuddy(m_flagsEdit);
    m_flagsEdit->setObjectName(QStringLiteral("regexFlagsEdit"));
    m_flagsEdit->setMinimumHeight(48);
    m_flagsEdit->setMinimumWidth(0);
    m_flagsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_flagsEdit->setPlaceholderText(QStringLiteral("imsxU"));
    m_flagsEdit->setAccessibleName(tr("Regular expression flags"));
    m_flagsEdit->setAccessibleDescription(
        tr("Optional flags i, m, s, x, and U without duplicates. Entries over 8 UTF-16 text units remain visible and are rejected."));
    flagsLayout->addWidget(flagsLabel);
    flagsLayout->addWidget(m_flagsEdit);

    auto* sampleColumn = new QWidget(content);
    sampleColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* sampleLayout = new QVBoxLayout(sampleColumn);
    sampleLayout->setContentsMargins(0, 0, 0, 0);
    sampleLayout->setSpacing(6);
    auto* sampleLabel = new QLabel(tr("Sample text"), sampleColumn);
    sampleLabel->setBuddy(m_sampleEdit);
    m_sampleEdit->setObjectName(QStringLiteral("regexSampleEdit"));
    m_sampleEdit->setPlaceholderText(tr("Type or paste text to test locally"));
    m_sampleEdit->setAccessibleName(tr("Regular expression sample text"));
    m_sampleEdit->setAccessibleDescription(
        tr("Local sample text. Entries over 500 UTF-16 text units are rejected before insertion and announced."));
    m_sampleEdit->setMinimumHeight(96);
    m_sampleEdit->setMinimumWidth(0);
    m_sampleEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sampleLayout->addWidget(sampleLabel);
    sampleLayout->addWidget(m_sampleEdit);

    m_fieldsLayout->setContentsMargins(0, 0, 0, 0);
    m_fieldsLayout->setSpacing(12);
    m_fieldsLayout->addWidget(flagsColumn, 1);
    m_fieldsLayout->addWidget(sampleColumn, 2);
    contentLayout->addLayout(m_fieldsLayout);

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
    m_previewEdit->setMinimumWidth(0);
    m_previewEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_previewEdit->setAccessibleName(tr("Regular expression match preview"));
    m_previewEdit->setAccessibleDescription(
        tr("A bounded local preview. At most 128 matches, 32 capture groups per match, and 16 KiB of UTF-8 output are shown."));
    contentLayout->addWidget(m_previewEdit);

    auto* bounds = new QLabel(
        tr("Pattern and sample inputs are each limited to 500 UTF-16 text units. PCRE limits matching to 100000 attempts and depth 1000. Previews are limited to 128 matches, 32 capture groups per match, and 16 KiB of UTF-8 output. Valid Qt groups, alternation, and quantifiers remain available."),
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
    for (QPushButton* button : {m_cancelButton, m_plainButton, m_applyButton}) {
        button->setMinimumHeight(kMinimumInteractiveTarget);
        button->setMinimumWidth(kMinimumInteractiveTarget);
        button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    }
    m_applyButton->setAutoDefault(false);
    m_applyButton->setDefault(false);
    m_cancelButton->setAccessibleDescription(tr("Close without changing the originating search field."));
    m_plainButton->setAccessibleDescription(tr("Use the current pattern characters as literal plain-text search text."));
    m_applyButton->setAccessibleDescription(tr("Apply the valid bounded pattern and flags to the originating search field."));
    m_actionsLayout->setContentsMargins(0, 4, 0, 0);
    m_actionsLayout->setSpacing(8);
    m_actionsLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
    auto* shell = findChild<QWidget*>(QStringLiteral("m3DialogShell"));
    auto* shellLayout = shell ? qobject_cast<QVBoxLayout*>(shell->layout()) : nullptr;
    Q_ASSERT(shellLayout);
    if (shellLayout)
        shellLayout->addWidget(scroller);

    setStyleSheet(QStringLiteral(
        "QDialog#regexBuilderDialog, QWidget#regexBuilderContent { background: palette(window); color: palette(window-text); }"
        "QDialog#regexBuilderDialog { border-radius: 28px; }"
        "QScrollArea#regexBuilderScroller { border: 0; background: palette(window); }"
        "QLabel#regexBuilderIntroduction, QLabel#regexBounds { color: palette(text); }"
        "QLabel#regexSectionLabel { color: palette(window-text); font-weight: 600; }"
        "QLineEdit#regexPatternEdit, QLineEdit#regexFlagsEdit, QPlainTextEdit#regexSampleEdit {"
        " background: palette(base); color: palette(text); border: 1px solid palette(mid); border-radius: 8px; padding: 0 12px; }"
        "QPlainTextEdit#regexSampleEdit { padding: 8px 12px; }"
        "QLineEdit#regexPatternEdit:focus, QLineEdit#regexFlagsEdit:focus, QPlainTextEdit#regexSampleEdit:focus, QPlainTextEdit#regexPreview:focus {"
        " border: 2px solid palette(highlight); }"
        "QLabel#regexValidation { border-radius: 12px; padding: 12px 16px; background: palette(alternate-base); }"
        "QLabel#regexValidation[valid=\"false\"] { border: 1px solid #B3261E; }"
        "QPlainTextEdit#regexPreview { background: palette(alternate-base); color: palette(text); border: 1px solid palette(mid);"
        " border-radius: 12px; padding: 8px 12px; font-family: 'Roboto Mono', Consolas, monospace; }"
        "QToolButton#regexGuidedToken { background: transparent; color: palette(window-text); border: 1px solid palette(mid);"
        " border-radius: 8px; padding: 0 8px; font-family: 'Roboto Mono', Consolas, monospace; }"
        "QToolButton#regexGuidedToken:hover, QToolButton#regexGuidedToken:focus { background: palette(alternate-base); border-color: palette(highlight); }"
        "QPushButton#regexCancelButton, QPushButton#regexPlainButton, QPushButton#regexApplyButton { border-radius: 20px; padding: 0 20px; font-weight: 600; }"
        "QPushButton#regexCancelButton { background: transparent; color: palette(link); border: 0; }"
        "QPushButton#regexPlainButton { background: transparent; color: palette(window-text); border: 1px solid palette(mid); }"
        "QPushButton#regexApplyButton { background: palette(highlight); color: palette(highlighted-text); border: 0; }"));

    connect(m_patternEdit, &QLineEdit::textChanged, this, &CRegexBuilderDialog::updatePreview);
    connect(m_flagsEdit, &QLineEdit::textChanged, this, &CRegexBuilderDialog::updatePreview);
    connect(m_sampleEdit, &QPlainTextEdit::textChanged, this, &CRegexBuilderDialog::updatePreview);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_plainButton, &QPushButton::clicked, this, &CRegexBuilderDialog::keepPlainText);
    connect(m_applyButton, &QPushButton::clicked, this, &CRegexBuilderDialog::applyPattern);
    connect(this, &QDialog::finished, this, [this](int) { restoreOriginFocus(); });

    auto* applyShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(applyShortcut, &QShortcut::activated, this, &CRegexBuilderDialog::applyPattern);

    if (auto* closeButton = findChild<QPushButton*>(QStringLiteral("m3DialogClose"))) {
        closeButton->setMinimumSize(kMinimumInteractiveTarget, kMinimumInteractiveTarget);
        closeButton->setAccessibleDescription(tr("Close the regular expression builder without changing the search field."));
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

void CRegexBuilderDialog::openAnchored(QWidget* origin, QWidget* focusReturnTarget)
{
    if (!origin)
        return;
    m_origin = origin;
    m_focusReturnTarget = focusReturnTarget ? focusReturnTarget : origin;
    m_restoreOriginFocus = true;
    m_userResized = false;
    watchOriginGeometry();
    show();
    m_userResized = false;
    raise();
    activateWindow();
    updateResponsiveLayout();
    positionBesideOrigin();
    m_patternEdit->setFocus(Qt::OtherFocusReason);
    m_patternEdit->setCursorPosition(m_patternEdit->text().size());
}

int CRegexBuilderDialog::execAnchored(QWidget* origin, QWidget* focusReturnTarget)
{
    if (!origin)
        return QDialog::Rejected;
    m_origin = origin;
    m_focusReturnTarget = focusReturnTarget ? focusReturnTarget : origin;
    m_restoreOriginFocus = true;
    m_userResized = false;
    watchOriginGeometry();
    updateResponsiveLayout();
    positionBesideOrigin();
    QTimer::singleShot(0, m_patternEdit, [this] {
        m_patternEdit->setFocus(Qt::OtherFocusReason);
        m_patternEdit->selectAll();
    });
    return exec();
}

QString CRegexBuilderDialog::pattern() const { return m_patternEdit->text(); }
QString CRegexBuilderDialog::flags() const { return m_flagsEdit->text(); }
QString CRegexBuilderDialog::sampleText() const { return m_sampleEdit->toPlainText(); }
bool CRegexBuilderDialog::isPatternValid() const { return m_valid; }
QString CRegexBuilderDialog::patternError() const { return m_error; }

QRegularExpression CRegexBuilderDialog::compile(const QString& pattern,
                                                 const QString& flags,
                                                 QString* error)
{
    return M3RegexExecutionPolicy::compile(pattern, flags, error);
}

void CRegexBuilderDialog::updatePreview()
{
    m_error.clear();
    const QRegularExpression expression = compile(pattern(), flags(), &m_error);
    m_valid = m_error.isEmpty() && expression.isValid();
    m_previewSafe = m_valid;

    QString validationText;
    QString preview;
    if (!m_valid) {
        validationText = tr("Pattern cannot be applied: %1").arg(m_error);
        preview = tr("Fix the pattern, flags, or input limit before previewing matches. No expression is exposed while this state is invalid.");
    } else {
        validationText = tr("Pattern is valid. A bounded local preview is available.");
        preview = previewText(expression);
    }

    m_applyButton->setEnabled(m_valid && m_previewSafe);
    m_applyButton->setAccessibleDescription(m_valid
        ? tr("Apply this valid bounded pattern and flags to the originating search field.")
        : tr("Apply is unavailable until the pattern, flags, and bounded-execution checks are valid."));
    m_validationLabel->setProperty("valid", m_valid);
    m_previewEdit->setProperty("previewSafe", m_previewSafe);
    announceValidation(validationText);
    m_previewEdit->setPlainText(preview);
    m_previewEdit->setAccessibleDescription(m_previewSafe
        ? tr("Bounded local regular-expression match preview. It is limited to 128 matches, 32 capture groups per match, and 16 KiB of UTF-8 output.")
        : tr("Regular-expression preview is unavailable until the pattern is valid and safe for bounded evaluation."));
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);
    m_previewEdit->style()->unpolish(m_previewEdit);
    m_previewEdit->style()->polish(m_previewEdit);
}

void CRegexBuilderDialog::applyPattern()
{
    if (!m_valid || !m_previewSafe)
        return;
    m_restoreOriginFocus = false;
    emit patternApplied(pattern(), flags());
    accept();
}

void CRegexBuilderDialog::keepPlainText()
{
    m_restoreOriginFocus = false;
    emit plainTextRequested(pattern());
    accept();
}

void CRegexBuilderDialog::insertToken()
{
    auto* button = qobject_cast<QToolButton*>(sender());
    if (!button)
        return;
    const QString token = button->property("regexToken").toString();
    const int cursor = m_patternEdit->cursorPosition();
    QString next = m_patternEdit->text();
    next.insert(cursor, token);
    if (next.size() > M3RegexExecutionPolicy::MaximumPatternLength) {
        announceInputRejection(tr("The guided token was not inserted because the pattern would exceed %1 UTF-16 text units.")
                             .arg(M3RegexExecutionPolicy::MaximumPatternLength));
        return;
    }
    m_patternEdit->setText(next);
    m_patternEdit->setCursorPosition(qMin(cursor + token.size(), next.size()));
    m_patternEdit->setFocus(Qt::OtherFocusReason);
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

    m_fullscreenFallback = !enoughRoomToAnchor;
    setProperty("fullscreenFallback", m_fullscreenFallback);
    setSizeGripEnabled(!m_fullscreenFallback);
    const int finalWidth = m_fullscreenFallback
        ? viewport.width()
        : (m_userResized ? qMin(qMax(kMinimumInteractiveTarget, width()), viewport.width()) : desiredWidth);
    const int finalHeight = m_fullscreenFallback
        ? viewport.height()
        : (m_userResized ? qMin(qMax(kMinimumAnchoredHeight, height()), sideSpace)
                         : qMin(desiredHeight, sideSpace));

    m_positioning = true;
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
    if (!m_restoreOriginFocus || !m_focusReturnTarget)
        return;
    const QPointer<QWidget> focusReturnTarget = m_focusReturnTarget;
    QTimer::singleShot(0, this, [focusReturnTarget] {
        if (focusReturnTarget && focusReturnTarget->isVisible() && focusReturnTarget->isEnabled())
            focusReturnTarget->setFocus(Qt::OtherFocusReason);
    });
}

void CRegexBuilderDialog::updateResponsiveLayout(int availableWidth)
{
    const int layoutWidth = availableWidth >= 0 ? availableWidth : width();
    const bool narrow = layoutWidth > 0 && layoutWidth < kResponsiveLayoutWidth;
    m_fieldsLayout->setDirection(narrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    m_actionsLayout->setDirection(narrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    m_actionsLayout->setAlignment(narrow ? Qt::AlignRight : Qt::AlignRight | Qt::AlignVCenter);
    reflowGuidedTokens(layoutWidth);
}

void CRegexBuilderDialog::reflowGuidedTokens(int availableWidth)
{
    if (m_reflowingTokens)
        return;
    m_reflowingTokens = true;
    const int tokenAreaWidth = qMax(1, (availableWidth >= 0 ? availableWidth : width()) - 2 * kDialogPadding);
    const int columns = qMax(1, (tokenAreaWidth + m_tokenGrid->horizontalSpacing())
                                   / (kTokenButtonWidth + m_tokenGrid->horizontalSpacing()));
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
    QTimer::singleShot(0, this, [this] {
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
    announceValidation(message);
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);
}

QString CRegexBuilderDialog::previewText(const QRegularExpression& expression) const
{
    QRegularExpressionMatchIterator iterator = expression.globalMatch(sampleText());
    QStringList lines;
    int previewUtf8Bytes = 0;
    int renderedMatches = 0;
    bool outputBoundReached = false;
    const auto appendLine = [&lines, &previewUtf8Bytes, &outputBoundReached](const QString& line) {
        const int lineBytes = line.toUtf8().size() + 1;
        if (outputBoundReached
            || previewUtf8Bytes + lineBytes > M3RegexExecutionPolicy::MaximumPreviewUtf8Bytes - kPreviewReservedUtf8Bytes) {
            outputBoundReached = true;
            return false;
        }
        lines << line;
        previewUtf8Bytes += lineBytes;
        return true;
    };

    while (iterator.hasNext() && renderedMatches < M3RegexExecutionPolicy::MaximumMatches && !outputBoundReached) {
        const QRegularExpressionMatch match = iterator.next();
        const int matchNumber = renderedMatches + 1;
        if (!appendLine(tr("Match %1 | index %2 | length %3 | %4")
                            .arg(matchNumber)
                            .arg(match.capturedStart(0))
                            .arg(match.capturedLength(0))
                            .arg(displayCapture(match.captured(0))))) {
            break;
        }
        ++renderedMatches;
        const int captures = qMin(expression.captureCount(), M3RegexExecutionPolicy::MaximumCaptures);
        for (int capture = 1; capture <= captures && !outputBoundReached; ++capture) {
            if (match.capturedStart(capture) < 0)
                continue;
            appendLine(tr("  Group %1 | index %2 | length %3 | %4")
                           .arg(capture)
                           .arg(match.capturedStart(capture))
                           .arg(match.capturedLength(capture))
                           .arg(displayCapture(match.captured(capture))));
        }
    }

    if (renderedMatches == 0 && outputBoundReached)
        return tr("Preview reached its 16 KiB UTF-8 output bound before a complete match could be shown.");
    if (renderedMatches == 0)
        return tr("No matches.");
    if (iterator.hasNext())
        appendLine(tr("Preview stopped after %1 matches.").arg(M3RegexExecutionPolicy::MaximumMatches));
    if (outputBoundReached)
        lines << tr("Preview stopped at the 16 KiB UTF-8 output limit.");
    lines.prepend(tr("%1 match(es) shown.").arg(renderedMatches));
    return lines.join(QLatin1Char('\n'));
}
