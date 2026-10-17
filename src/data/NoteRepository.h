#pragma once

#include "domain/Note.h"

#include <QObject>
#include <QString>
#include <QVector>

#include <optional>

class QSqlDatabase;

namespace CrossNote {

class NoteRepository final : public QObject
{
    Q_OBJECT

public:
    explicit NoteRepository(QObject *parent = nullptr);
    ~NoteRepository() override;

    [[nodiscard]] bool openVault(const QString &vaultPath, QString *error = nullptr);
    [[nodiscard]] QString vaultPath() const;
    [[nodiscard]] QVector<Note> notes(QString *error = nullptr) const;
    [[nodiscard]] std::optional<Note> note(const QString &id, bool includeContent = true,
                                           QString *error = nullptr) const;

    [[nodiscard]] std::optional<Note> createNote(const QString &title, QString *error = nullptr);
    [[nodiscard]] bool saveNote(const QString &id, const QString &title,
                                const QString &content, QString *error = nullptr);
    [[nodiscard]] bool setFavorite(const QString &id, bool favorite,
                                   QString *error = nullptr);
    [[nodiscard]] bool setTags(const QString &id, const QStringList &tags,
                               QString *error = nullptr);
    [[nodiscard]] bool markOpened(const QString &id, QString *error = nullptr);
    [[nodiscard]] bool moveToTrash(const QString &id, QString *error = nullptr);
    [[nodiscard]] bool restoreFromTrash(const QString &id, QString *error = nullptr);
    [[nodiscard]] bool refresh(QString *error = nullptr);

signals:
    void vaultChanged(const QString &path);
    void notesChanged();
    void errorOccurred(const QString &message);

private:
    class Impl;
    Impl *m_impl = nullptr;
};

} // namespace CrossNote
