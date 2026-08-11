#pragma once

#include <QRegularExpression>
#include <QString>

// One bounded contract is shared by the search field and its anchored builder.
// The PCRE limits prefix the real QRegularExpression dialect, so supported
// groups, alternation, quantifiers, references, and extended syntax remain
// available without a second incomplete regex parser.
namespace M3RegexExecutionPolicy {

constexpr int MaximumPatternLength = 500;
constexpr int MaximumFlagsLength = 8;
constexpr int MaximumSampleLength = 500;
constexpr int MaximumMatches = 128;
constexpr int MaximumCaptures = 32;
constexpr int MaximumPreviewUtf8Bytes = 16 * 1024;
constexpr int MaximumMatchAttempts = 100000;
constexpr int MaximumMatchDepth = 1000;

QRegularExpression invalidExpression();
QString boundedPattern(const QString& pattern);
QRegularExpression compile(const QString& pattern,
                           const QString& flags,
                           QString* error = nullptr);

} // namespace M3RegexExecutionPolicy
