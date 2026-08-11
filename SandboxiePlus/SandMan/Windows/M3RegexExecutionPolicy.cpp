#include "stdafx.h"
#include "M3RegexExecutionPolicy.h"

#include <QObject>
#include <QSet>

namespace M3RegexExecutionPolicy {

namespace {

bool hasLeadingLimitOverrideDirective(const QString& pattern)
{
    // PCRE accepts several consecutive start directives. Walk only that
    // unambiguous prefix so a literal or quoted "(*LIMIT_" elsewhere stays
    // part of the user's regular expression. A later user LIMIT directive
    // would override the hard resource ceiling that this policy provides.
    int cursor = 0;
    while (pattern.mid(cursor, 2) == QStringLiteral("(*")) {
        const int closingParenthesis = pattern.indexOf(QLatin1Char(')'), cursor + 2);
        if (closingParenthesis < 0)
            return false;

        const QString directive = pattern.mid(cursor + 2, closingParenthesis - cursor - 2);
        if (directive.startsWith(QStringLiteral("LIMIT_")))
            return true;
        cursor = closingParenthesis + 1;
    }
    return false;
}

} // namespace

QRegularExpression invalidExpression()
{
    // A default-constructed expression is valid and matches every position.
    // Invalid input must never escape as that match-all expression.
    return QRegularExpression(QStringLiteral("["));
}

QString boundedPattern(const QString& pattern)
{
    // PCRE evaluates the control verbs before the user's unmodified pattern.
    // This leaves capture numbering, extended-mode comments, and top-level
    // alternation semantics exactly as the user entered them.
    return QStringLiteral("(*LIMIT_MATCH=%1)(*LIMIT_DEPTH=%2)%3")
        .arg(MaximumMatchAttempts)
        .arg(MaximumMatchDepth)
        .arg(pattern);
}

QRegularExpression compile(const QString& pattern, const QString& flags, QString* error)
{
    if (error)
        error->clear();
    if (pattern.isEmpty()) {
        if (error)
            *error = QObject::tr("Enter a pattern before applying regular expression mode.");
        return invalidExpression();
    }
    if (pattern.size() > MaximumPatternLength) {
        if (error) {
            *error = QObject::tr("Patterns are limited to %1 UTF-16 text units. Reduce the input; it was not shortened.")
                         .arg(MaximumPatternLength);
        }
        return invalidExpression();
    }
    if (flags.size() > MaximumFlagsLength) {
        if (error) {
            *error = QObject::tr("Flags are limited to %1 UTF-16 text units. Reduce the input; it was not shortened.")
                         .arg(MaximumFlagsLength);
        }
        return invalidExpression();
    }
    if (hasLeadingLimitOverrideDirective(pattern)) {
        if (error) {
            *error = QObject::tr("PCRE LIMIT_* directives are unavailable here because the search builder enforces its own match and depth limits.");
        }
        return invalidExpression();
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    QSet<QChar> seenFlags;
    for (const QChar flag : flags) {
        if (seenFlags.contains(flag)) {
            if (error)
                *error = QObject::tr("Flag '%1' appears more than once.").arg(flag);
            return invalidExpression();
        }
        seenFlags.insert(flag);
        switch (flag.unicode()) {
        case 'i': options |= QRegularExpression::CaseInsensitiveOption; break;
        case 'm': options |= QRegularExpression::MultilineOption; break;
        case 's': options |= QRegularExpression::DotMatchesEverythingOption; break;
        case 'x': options |= QRegularExpression::ExtendedPatternSyntaxOption; break;
        case 'U': options |= QRegularExpression::InvertedGreedinessOption; break;
        default:
            if (error)
                *error = QObject::tr("Unsupported flag '%1'. Use i, m, s, x, or U.").arg(flag);
            return invalidExpression();
        }
    }

    const QRegularExpression expression(boundedPattern(pattern), options);
    if (!expression.isValid()) {
        if (error)
            *error = expression.errorString();
        return invalidExpression();
    }
    return expression;
}

} // namespace M3RegexExecutionPolicy
