#include "MarkdownMetrics.h"

#include <QRegularExpression>

namespace CrossNote {

MarkdownMetrics measureMarkdown(const QString &markdown)
{
    MarkdownMetrics metrics;
    QString body = markdown;
    if (body.startsWith(QStringLiteral("---\n"))) {
        const qsizetype frontMatterEnd = body.indexOf(QStringLiteral("\n---"), 4);
        if (frontMatterEnd >= 0)
            body.remove(0, frontMatterEnd + 4);
    }
    metrics.characters = body.size();
    for (const QChar character : body) {
        if (!character.isSpace())
            ++metrics.charactersWithoutWhitespace;
    }

    static const QRegularExpression token(
        QStringLiteral("[\\p{L}\\p{N}]+(?:['’][\\p{L}\\p{N}]+)*"));
    auto words = token.globalMatch(body);
    while (words.hasNext()) {
        words.next();
        ++metrics.words;
    }

    static const QRegularExpression task(QStringLiteral("^\\s*[-*+]\\s+\\[([ xX])\\]"),
                                         QRegularExpression::MultilineOption);
    auto tasks = task.globalMatch(markdown);
    while (tasks.hasNext()) {
        const auto match = tasks.next();
        ++metrics.tasks;
        if (match.captured(1).compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0)
            ++metrics.completedTasks;
    }
    return metrics;
}

} // namespace CrossNote
