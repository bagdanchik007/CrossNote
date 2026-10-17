#pragma once

#include "data/NoteRepository.h"
#include "presentation/NoteListModel.h"

#include <QByteArray>
#include <QObject>
#include <QString>

namespace CrossNote {

class NoteController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(NoteListModel *noteModel READ noteModel CONSTANT)
    Q_PROPERTY(QString vaultPath READ vaultPath NOTIFY vaultPathChanged)
    Q_PROPERTY(QString selectedId READ selectedId NOTIFY currentNoteChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY currentNoteChanged)
    Q_PROPERTY(QString selectedContent READ selectedContent NOTIFY currentNoteChanged)
    Q_PROPERTY(QStringList selectedTags READ selectedTags NOTIFY selectedTagsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY currentNoteChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit NoteController(QObject *parent = nullptr);

    [[nodiscard]] NoteListModel *noteModel();
    [[nodiscard]] QString vaultPath() const;
    [[nodiscard]] QString selectedId() const;
    [[nodiscard]] QString selectedTitle() const;
    [[nodiscard]] QString selectedContent() const;
    [[nodiscard]] QStringList selectedTags() const;
    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString errorText() const;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void setVaultPath(const QString &path);
    Q_INVOKABLE void createNote();
    Q_INVOKABLE void openNote(const QString &id);
    Q_INVOKABLE void saveCurrent(const QString &title, const QString &content);
    Q_INVOKABLE void deleteCurrent();
    Q_INVOKABLE void setFavorite(const QString &id, bool favorite);
    Q_INVOKABLE void setTags(const QString &id, const QStringList &tags);
    Q_INVOKABLE void setSearchQuery(const QString &query);
    Q_INVOKABLE void setSection(const QString &section);
    Q_INVOKABLE void restoreFromTrash(const QString &id);

signals:
    void vaultPathChanged();
    void currentNoteChanged();
    void selectedTagsChanged();
    void statusTextChanged();
    void errorTextChanged();

private:
    void reloadNotes();
    void setStatus(const QString &message);
    void setError(const QString &message);
    void clearError();

    NoteRepository m_repository;
    NoteListModel m_noteModel;
    QString m_selectedId;
    QString m_selectedTitle;
    QString m_selectedContent;
    QByteArray m_selectedHash;
    QString m_conflictPath;
    QStringList m_selectedTags;
    QString m_statusText;
    QString m_errorText;
};

} // namespace CrossNote
