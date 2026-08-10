#include "stdafx.h"
#include "M3SearchField.h"
#include "RegexBuilderDialog.h"

#include <QApplication>
#include <QMenu>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSet>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

namespace {
constexpr int kMaximumPatternLength = 500;
constexpr int kMaximumFlagsLength = 8;
QString bounded(const QString& value, int maximum) { return value.left(maximum); }
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

    m_lineEdit->setObjectName(QStringLiteral("m3SearchInput"));
    m_lineEdit->setMaxLength(kMaximumPatternLength);
    m_lineEdit->setClearButtonEnabled(false);
    m_lineEdit->setAccessibleName(tr("Search"));
    m_lineEdit->installEventFilter(this);

    m_clearButton->setObjectName(QStringLiteral("m3SearchClearButton"));
    m_clearButton->setText(QString(QChar(0x00D7)));
    m_clearButton->setToolTip(tr("Clear search"));
    m_clearButton->setAccessibleName(m_clearButton->toolTip());
    m_clearButton->setAutoRaise(true);

    m_regexButton->setObjectName(QStringLiteral("m3RegexBuilderButton"));
    m_regexButton->setText(QStringLiteral(".*"));
    m_regexButton->setToolTip(tr("Open regular expression builder"));
    m_regexButton->setAccessibleName(m_regexButton->toolTip());
    m_regexButton->setCheckable(true);
    m_regexButton->setAutoRaise(true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_lineEdit, 1);
    layout->addWidget(m_clearButton);
    layout->addWidget(m_regexButton);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &CM3SearchField::onTextChanged);
    connect(m_clearButton, &QToolButton::clicked, m_lineEdit, &QLineEdit::clear);
    connect(m_regexButton, &QToolButton::clicked, this, &CM3SearchField::openRegexBuilder);

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
    setState(query, query, m_flags, false);
}

void CM3SearchField::setState(const QString& query, const QString& pattern, const QString& flags, bool regexMode)
{
    m_regexMode = regexMode;
    m_query = bounded(regexMode ? pattern : query, kMaximumPatternLength);
    m_pattern = bounded(pattern.isEmpty() ? m_query : pattern, kMaximumPatternLength);
    m_flags = bounded(flags, kMaximumFlagsLength);
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
    if (watched == m_lineEdit && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Escape) {
            emit escapePressed();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void CM3SearchField::onTextChanged(const QString& text)
{
    m_query = bounded(text, kMaximumPatternLength);
    m_pattern = m_query;
    rebuildExpression(true);
    updateControls();
}

void CM3SearchField::openRegexBuilder()
{
    if (!m_builder) {
        QWidget* owner = window();
        if (auto* sourceMenu = qobject_cast<QMenu*>(owner)) {
            QWidget* menuOwner = sourceMenu->parentWidget();
            owner = menuOwner ? menuOwner->window() : QApplication::activeWindow();
        }
        m_builder = new CRegexBuilderDialog(owner);
        connect(m_builder, &CRegexBuilderDialog::patternApplied,
                this, &CM3SearchField::applyRegexPattern);
        connect(m_builder, &CRegexBuilderDialog::plainTextRequested,
                this, &CM3SearchField::keepPlainText);
    }
    m_builder->setState(m_query, m_pattern, m_flags, m_regexMode);

    QWidget* popupAncestor = parentWidget();
    while (popupAncestor && !qobject_cast<QMenu*>(popupAncestor)
           && !(popupAncestor->windowFlags() & Qt::Popup)) {
        popupAncestor = popupAncestor->parentWidget();
    }
    if (!popupAncestor) {
        m_builder->openAnchored(m_regexButton);
        return;
    }

    const QPoint popupPosition = popupAncestor->pos();
    popupAncestor->setProperty("m3ChildDialogActive", true);
    m_builder->execAnchored(m_regexButton);
    popupAncestor->setProperty("m3ChildDialogActive", false);

    if (auto* nativeMenu = qobject_cast<QMenu*>(popupAncestor)) {
        nativeMenu->setProperty("m3ResumeMenuSearch", true);
        nativeMenu->popup(popupPosition);
    } else {
        popupAncestor->move(popupPosition);
        popupAncestor->show();
        popupAncestor->raise();
        focusEditor();
    }
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
    m_lineEdit->setProperty("m3Invalid", !m_valid);
    m_lineEdit->setToolTip(m_valid ? QString() : m_error);
    m_lineEdit->style()->unpolish(m_lineEdit);
    m_lineEdit->style()->polish(m_lineEdit);
}

QRegularExpression CM3SearchField::compileRegex(const QString& pattern, const QString& flags, QString* error)
{
    if (pattern.size() > kMaximumPatternLength) {
        if (error) *error = tr("Patterns are limited to %1 characters.").arg(kMaximumPatternLength);
        return QRegularExpression();
    }
    if (flags.size() > kMaximumFlagsLength) {
        if (error) *error = tr("Flags are limited to %1 characters.").arg(kMaximumFlagsLength);
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

    QRegularExpression expression(pattern, options);
    if (!expression.isValid() && error)
        *error = expression.errorString();
    return expression;
}
