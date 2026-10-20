#pragma once

#include <QByteArray>
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
    QString frontMatter;
    QByteArray contentHash;
    QStringList tags;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime lastOpenedAt;
    bool favorite = false;
    bool archived = false;
};

} // namespace CrossNote
