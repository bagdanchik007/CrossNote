#include "MarkdownMetrics.h"

#include <QRegularExpression>

namespace CrossNote {

MarkdownMetrics measureMarkdown(const QString &markdown)
{
    MarkdownMetrics metrics;
    metrics.characters = markdown.size();

    static const QRegularExpression token(QStringLiteral("\\S+"));
    auto words = token.globalMatch(markdown);
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
