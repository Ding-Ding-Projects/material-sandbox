#include "stdafx.h"
#include "M3SearchField.h"
#include "RegexBuilderDialog.h"
#include "M3RegexExecutionPolicy.h"
#include "../../MiscHelpers/Common/M3Tokens.h"

#include <QApplication>
#include <QMenu>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointer>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

namespace {
}

CM3SearchField::CM3SearchField(QWidget* parent)
    : QWidget(parent),
      m_lineEdit(new QLineEdit(this)),
      m_clearButton(new QToolButton(this)),
      m_regexButton(new QToolButton(this)),
      m_builder(nullptr),
      m_heightVariant(Control),
      m_regexMode(false),
      m_valid(true)
{
    setObjectName(QStringLiteral("m3SearchField"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
    setProperty("m3SearchSurface", true);

    m_lineEdit->setObjectName(QStringLiteral("m3SearchInput"));
    m_lineEdit->setClearButtonEnabled(false);
    m_lineEdit->setAccessibleName(tr("Search"));
    m_lineEdit->installEventFilter(this);
    setFocusProxy(m_lineEdit);

    m_clearButton->setObjectName(QStringLiteral("m3SearchClearButton"));
    m_clearButton->setText(QString(QChar(0x00D7)));
    m_clearButton->setToolTip(tr("Clear search"));
    m_clearButton->setAccessibleName(m_clearButton->toolTip());
    m_clearButton->setAutoRaise(true);
    m_clearButton->installEventFilter(this);

    m_regexButton->setObjectName(QStringLiteral("m3RegexBuilderButton"));
    m_regexButton->setText(QStringLiteral(".*"));
    m_regexButton->setToolTip(tr("Open regular expression builder"));
    m_regexButton->setAccessibleName(m_regexButton->toolTip());
    m_regexButton->setCheckable(true);
    m_regexButton->setAutoRaise(true);
    m_regexButton->installEventFilter(this);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_lineEdit, 1);
    layout->addWidget(m_clearButton);
    layout->addWidget(m_regexButton);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &CM3SearchField::onTextChanged);
    connect(m_clearButton, &QToolButton::clicked, this, &CM3SearchField::clearSearch);
    connect(m_regexButton, &QToolButton::clicked, this, &CM3SearchField::openRegexBuilder);

    setAccessibleName(tr("Search"));
    setAccessibleDescription(tr("Searches using plain text. A regular expression builder is available beside the field."));
    setHeightVariant(Control);
    rebuildExpression(false);
    updateControls();
}

QLineEdit* CM3SearchField::lineEdit() const { return m_lineEdit; }
QString CM3SearchField::query() const { return m_query; }
QString CM3SearchField::pattern() const { return m_pattern; }
QString CM3SearchField::flags() const { return m_flags; }
bool CM3SearchField::regexMode() const { return m_regexMode; }
QRegularExpression CM3SearchField::expression() const { return m_expression; }
bool CM3SearchField::isValid() const { return m_valid; }
QString CM3SearchField::error() const { return m_error; }

void CM3SearchField::setQuery(const QString& query)
{
    setState(query, query, QString(), false);
}

void CM3SearchField::setState(const QString& query, const QString& pattern, const QString& flags, bool regexMode)
{
    const QString nextQuery = regexMode ? pattern : query;
    const QString nextPattern = pattern.isEmpty() ? nextQuery : pattern;
    if (nextQuery.size() > M3RegexExecutionPolicy::MaximumPatternLength
        || nextPattern.size() > M3RegexExecutionPolicy::MaximumPatternLength) {
        rejectOversizedState(tr("The search input was not applied because patterns are limited to %1 UTF-16 text units.")
                                .arg(M3RegexExecutionPolicy::MaximumPatternLength));
        return;
    }
    if (flags.size() > M3RegexExecutionPolicy::MaximumFlagsLength) {
        rejectOversizedState(tr("The search flags were not applied because flags are limited to %1 UTF-16 text units.")
                                .arg(M3RegexExecutionPolicy::MaximumFlagsLength));
        return;
    }

    m_regexMode = regexMode;
    m_query = nextQuery;
    m_pattern = nextPattern;
    m_flags = flags;
    {
        const QSignalBlocker blocker(m_lineEdit);
        m_lineEdit->setText(m_query);
    }
    rebuildExpression(true);
    updateControls();
}

void CM3SearchField::setPlaceholderText(const QString& placeholder)
{
    m_lineEdit->setPlaceholderText(placeholder);
}

void CM3SearchField::setAccessibleName(const QString& name)
{
    QWidget::setAccessibleName(name.trimmed().isEmpty() ? tr("Search") : name.trimmed());
    updateAccessibleNames();
    updateControls();
}

void CM3SearchField::setAccessibleDescription(const QString& description)
{
    m_accessibleDescription = description.trimmed();
    updateControls();
}

void CM3SearchField::setHeightVariant(HeightVariant variant)
{
    m_heightVariant = variant;
    const int height = static_cast<int>(variant);
    setFixedHeight(height);
    m_lineEdit->setFixedHeight(height);
    m_lineEdit->setProperty("m3", variant == Page ? QStringLiteral("search") : QVariant());
    m_clearButton->setFixedSize(qMin(height, 40), qMin(height, 40));
    m_regexButton->setFixedSize(qMin(height, 40), qMin(height, 40));
    m_lineEdit->style()->unpolish(m_lineEdit);
    m_lineEdit->style()->polish(m_lineEdit);
}

void CM3SearchField::setRegexEnabled(bool enabled)
{
    m_regexButton->setVisible(enabled);
    if (!enabled && m_regexMode)
        setQuery(m_query);
}

void CM3SearchField::focusEditor()
{
    QTimer::singleShot(0, m_lineEdit, [this] {
        m_lineEdit->setFocus(Qt::OtherFocusReason);
        m_lineEdit->setCursorPosition(m_lineEdit->text().size());
    });
}

bool CM3SearchField::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_lineEdit || watched == m_clearButton || watched == m_regexButton)
        && (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)) {
        updateFocusState();
    }
    if (watched == m_lineEdit && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Escape) {
            emit escapePressed();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void CM3SearchField::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event && (event->type() == QEvent::EnabledChange
                  || event->type() == QEvent::PaletteChange
                  || event->type() == QEvent::FontChange)) {
        update();
        m_clearButton->update();
        m_regexButton->update();
    }
}

void CM3SearchField::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPalette::ColorGroup colorGroup = isEnabled()
        ? QPalette::Active
        : QPalette::Disabled;
    const QRectF capsule = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = qMax<qreal>(0.0, capsule.height() / 2.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().color(colorGroup, QPalette::AlternateBase));
    painter.drawRoundedRect(capsule, radius, radius);

    const bool invalid = property("m3Invalid").toBool();
    const bool focused = m_lineEdit->hasFocus()
        || m_clearButton->hasFocus()
        || m_regexButton->hasFocus();
    qreal borderWidth = 0.0;
    QColor borderColor;
    if (invalid) {
        const bool dark = qApp && qApp->property("m3Dark").toBool();
        borderColor = M3Tokens::colors(dark).error;
        borderWidth = 2.0;
    } else if (focused) {
        borderColor = palette().color(colorGroup, QPalette::Highlight);
        borderWidth = 3.0;
    }
    if (borderWidth > 0.0) {
        const qreal inset = borderWidth / 2.0;
        const QRectF border = capsule.adjusted(inset, inset, -inset, -inset);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(borderColor, borderWidth));
        painter.drawRoundedRect(border, qMax<qreal>(0.0, radius - inset), qMax<qreal>(0.0, radius - inset));
    }
}

void CM3SearchField::clearSearch()
{
    m_lineEdit->clear();
    focusEditor();
}

void CM3SearchField::onTextChanged(const QString& text)
{
    m_query = text;
    m_pattern = m_query;
    if (text.size() > M3RegexExecutionPolicy::MaximumPatternLength) {
        rejectOversizedState(tr("The search input is over %1 UTF-16 text units. It remains visible but is not applied until it is shortened.")
                                .arg(M3RegexExecutionPolicy::MaximumPatternLength));
        return;
    }
    rebuildExpression(true);
    updateControls();
}

void CM3SearchField::openRegexBuilder()
{
    if (!m_builder) {
        // Parent the anchored builder to its owning search field. A rebuild can
        // retire an app-bar or menu field while the builder is open; parenting
        // here closes that popup with the field instead of leaving a stale
        // dialog attached to the main window.
        m_builder = new CRegexBuilderDialog(this);
        connect(m_builder, &CRegexBuilderDialog::patternApplied,
                this, &CM3SearchField::applyRegexPattern);
        connect(m_builder, &CRegexBuilderDialog::plainTextRequested,
                this, &CM3SearchField::keepPlainText);
        connect(m_builder, &QDialog::rejected,
                this, &CM3SearchField::restoreSearchStateAfterCancellation);
        updateAccessibleNames();
    }
    m_builder->setState(m_query, m_pattern, m_flags, m_regexMode);

    QWidget* popupAncestor = parentWidget();
    while (popupAncestor && !qobject_cast<QMenu*>(popupAncestor)
           && popupAncestor->windowType() != Qt::Popup) {
        popupAncestor = popupAncestor->parentWidget();
    }
    if (!popupAncestor) {
        m_builder->openAnchored(m_regexButton, m_lineEdit);
        return;
    }

    QPointer<CM3SearchField> self(this);
    QPointer<QWidget> popupGuard(popupAncestor);
    const QPoint popupPosition = popupGuard->pos();
    popupGuard->setProperty("m3ChildDialogActive", true);
    m_builder->execAnchored(m_regexButton, m_lineEdit);
    if (popupGuard)
        popupGuard->setProperty("m3ChildDialogActive", false);
    if (!self || !popupGuard)
        return;
    QWidget* restoredPopup = popupGuard.data();

    if (auto* nativeMenu = qobject_cast<QMenu*>(restoredPopup)) {
        nativeMenu->setProperty("m3ResumeMenuSearch", true);
        nativeMenu->popup(popupPosition);
    } else {
        restoredPopup->move(popupPosition);
        restoredPopup->show();
        restoredPopup->raise();
        focusEditor();
    }
}

void CM3SearchField::restoreSearchStateAfterCancellation()
{
    updateControls();
}

void CM3SearchField::applyRegexPattern(const QString& pattern, const QString& flags)
{
    setState(pattern, pattern, flags, true);
    focusEditor();
}

void CM3SearchField::keepPlainText(const QString& text)
{
    setState(text, text, QString(), false);
    focusEditor();
}

void CM3SearchField::rebuildExpression(bool notify)
{
    m_error.clear();
    if (m_regexMode) {
        m_expression = compileRegex(m_pattern, m_flags, &m_error);
        m_valid = m_expression.isValid() && m_error.isEmpty();
        if (!m_valid)
            m_expression = M3RegexExecutionPolicy::invalidExpression();
    } else {
        m_expression = QRegularExpression(QRegularExpression::escape(m_query),
                                          QRegularExpression::CaseInsensitiveOption);
        m_valid = true;
    }

    if (notify)
        emit searchChanged(m_query, m_regexMode, m_expression, m_flags, m_valid, m_error);
}

void CM3SearchField::updateControls()
{
    m_clearButton->setVisible(!m_query.isEmpty());
    m_regexButton->setChecked(m_regexMode);
    const bool invalid = !m_valid;
    setProperty("m3Invalid", invalid);
    m_lineEdit->setProperty("m3Invalid", invalid);
    m_clearButton->setProperty("m3Invalid", invalid);
    m_regexButton->setProperty("m3Invalid", invalid);
    const QString fieldName = QWidget::accessibleName().trimmed().isEmpty()
        ? tr("Search")
        : QWidget::accessibleName().trimmed();
    const QString modeDescription = !m_valid
        ? tr("Invalid regular expression: %1").arg(m_error)
        : (m_regexMode
            ? tr("Regular expression mode is on. Active flags: %1.")
                  .arg(m_flags.isEmpty() ? tr("none") : m_flags)
            : tr("Plain-text search is on. Matching ignores letter case."));
    QString description = m_accessibleDescription;
    if (!description.isEmpty())
        description.append(QLatin1Char(' '));
    description.append(modeDescription);
    m_lineEdit->setAccessibleDescription(description);
    m_lineEdit->setToolTip(m_valid ? QString() : modeDescription);
    m_clearButton->setAccessibleDescription(tr("Clears the current query in %1.").arg(fieldName));
    m_regexButton->setAccessibleDescription(modeDescription);
    QWidget::setAccessibleDescription(description);
    updateFocusState();
}

void CM3SearchField::updateFocusState()
{
    const bool anyFocused = m_lineEdit->hasFocus()
        || m_clearButton->hasFocus()
        || m_regexButton->hasFocus();
    for (QWidget* control : { static_cast<QWidget*>(this), static_cast<QWidget*>(m_lineEdit),
                              static_cast<QWidget*>(m_clearButton), static_cast<QWidget*>(m_regexButton) }) {
        const bool focused = control == this ? anyFocused : control->hasFocus();
        control->setProperty("m3Focus", focused);
        control->style()->unpolish(control);
        control->style()->polish(control);
        control->update();
    }
}

void CM3SearchField::updateAccessibleNames()
{
    const QString fieldName = QWidget::accessibleName().trimmed().isEmpty()
        ? tr("Search")
        : QWidget::accessibleName().trimmed();
    m_lineEdit->setAccessibleName(fieldName);
    m_clearButton->setAccessibleName(tr("Clear %1").arg(fieldName));
    m_clearButton->setToolTip(tr("Clear %1").arg(fieldName));
    m_regexButton->setAccessibleName(tr("Open regular expression builder for %1").arg(fieldName));
    m_regexButton->setToolTip(tr("Open regular expression builder for %1").arg(fieldName));
    if (m_builder) {
        m_builder->setAccessibleName(
            tr("Regular expression builder for %1").arg(fieldName));
    }
}

void CM3SearchField::rejectOversizedState(const QString& message)
{
    m_error = message;
    m_valid = false;
    m_expression = M3RegexExecutionPolicy::invalidExpression();
    updateControls();
    emit searchChanged(m_query, m_regexMode, m_expression, m_flags, m_valid, m_error);
}

QRegularExpression CM3SearchField::compileRegex(const QString& pattern, const QString& flags, QString* error)
{
    return M3RegexExecutionPolicy::compile(pattern, flags, error);
}
