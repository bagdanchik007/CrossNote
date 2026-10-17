#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

namespace CrossNote {

struct Note
{
    QString id;
    QString title;
    QString relativePath;
    QString content;
    QStringList tags;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime lastOpenedAt;
    bool favorite = false;
};

} // namespace CrossNote
