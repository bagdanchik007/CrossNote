#include "MarkdownMetrics.h"

#include <QRegularExpression>
#include <QStringList>

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
    bool inCodeFence = false;
    QChar activeFence;
    for (const QString &line : body.split(u'\n')) {
        const QString trimmed = line.trimmed();
        const QChar fence = trimmed.isEmpty() ? QChar() : trimmed.front();
        if ((fence == u'`' || fence == u'~') && trimmed.startsWith(QString(3, fence))) {
            if (!inCodeFence) {
                inCodeFence = true;
                activeFence = fence;
            } else if (fence == activeFence) {
                inCodeFence = false;
                activeFence = {};
            }
            continue;
        }
        if (inCodeFence)
            continue;
        const auto match = task.match(line);
        if (match.hasMatch()) {
            ++metrics.tasks;
            if (match.captured(1).compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0)
                ++metrics.completedTasks;
        }
    }
    return metrics;
}

} // namespace CrossNote
