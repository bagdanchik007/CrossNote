#pragma once

#include <QString>

namespace CrossNote {

struct MarkdownMetrics
{
    int words = 0;
    int characters = 0;
    int charactersWithoutWhitespace = 0;
    int tasks = 0;
    int completedTasks = 0;

    [[nodiscard]] int openTasks() const { return tasks - completedTasks; }
    [[nodiscard]] int completionPercent() const
    {
        return tasks == 0 ? 0 : completedTasks * 100 / tasks;
    }
};

[[nodiscard]] MarkdownMetrics measureMarkdown(const QString &markdown);

} // namespace CrossNote
