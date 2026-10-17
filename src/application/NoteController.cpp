#include "NoteController.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace CrossNote {

NoteController::NoteController(QObject *parent)
    : QObject(parent), m_repository(), m_noteModel()
{
    connect(&m_repository, &NoteRepository::notesChanged,
            this, &NoteController::reloadNotes);
    connect(&m_repository, &NoteRepository::errorOccurred,
            this, &NoteController::setError);
}

NoteListModel *NoteController::noteModel() { return &m_noteModel; }
QString NoteController::vaultPath() const { return m_repository.vaultPath(); }
QString NoteController::selectedId() const { return m_selectedId; }
QString NoteController::selectedTitle() const { return m_selectedTitle; }
QString NoteController::selectedContent() const { return m_selectedContent; }
QStringList NoteController::selectedTags() const { return m_selectedTags; }
bool NoteController::hasSelection() const { return !m_selectedId.isEmpty(); }
QString NoteController::statusText() const { return m_statusText; }
QString NoteController::errorText() const { return m_errorText; }

void NoteController::initialize()
{
    QSettings settings;
    QString path = settings.value(QStringLiteral("vault/path")).toString();
    if (path.isEmpty()) {
        path = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                   .filePath(QStringLiteral("Vault"));
    }
    QString error;
    if (!m_repository.openVault(path, &error)) {
        setError(error);
        return;
    }
    emit vaultPathChanged();
    reloadNotes();
    setStatus(tr("Notizordner geöffnet"));
}

void NoteController::setVaultPath(const QString &path)
{
    QString error;
    if (!m_repository.openVault(path, &error)) {
        setError(error);
        return;
    }
    QSettings().setValue(QStringLiteral("vault/path"), m_repository.vaultPath());
    m_selectedId.clear();
    m_selectedTitle.clear();
    m_selectedContent.clear();
    m_selectedTags.clear();
    emit vaultPathChanged();
    emit currentNoteChanged();
    reloadNotes();
    clearError();
    setStatus(tr("Notizordner gewechselt"));
}

void NoteController::createNote()
{
    QString error;
    const auto created = m_repository.createNote(tr("Neue Notiz"), &error);
    if (!created) {
        setError(error);
        return;
    }
    openNote(created->id);
    setStatus(tr("Notiz erstellt"));
}

void NoteController::openNote(const QString &id)
{
    if (id == m_selectedId)
        return;
    QString error;
    const auto selected = m_repository.note(id, true, &error);
    if (!selected) {
        if (!error.isEmpty()) setError(error);
        return;
    }
    m_selectedId = selected->id;
    m_selectedTitle = selected->title;
    m_selectedContent = selected->content;
    m_selectedTags = selected->tags;
    emit selectedTagsChanged();
    m_repository.markOpened(id, &error);
    emit currentNoteChanged();
    clearError();
    setStatus(tr("Notiz geöffnet"));
}

void NoteController::saveCurrent(const QString &title, const QString &content)
{
    if (m_selectedId.isEmpty())
        return;
    QString error;
    if (!m_repository.saveNote(m_selectedId, title, content, &error)) {
        setError(error);
        return;
    }
    m_selectedTitle = title.trimmed().isEmpty() ? tr("Unbenannte Notiz") : title.trimmed();
    m_selectedContent = content;
    clearError();
    setStatus(tr("Gespeichert"));
}

void NoteController::deleteCurrent()
{
    if (m_selectedId.isEmpty())
        return;
    const QString previousId = m_selectedId;
    QString error;
    if (!m_repository.moveToTrash(previousId, &error)) {
        setError(error);
        return;
    }
    m_selectedId.clear();
    m_selectedTitle.clear();
    m_selectedContent.clear();
    m_selectedTags.clear();
    emit currentNoteChanged();
    clearError();
    setStatus(tr("Notiz in den Papierkorb verschoben"));
}

void NoteController::setFavorite(const QString &id, bool favorite)
{
    QString error;
    if (!m_repository.setFavorite(id, favorite, &error)) {
        setError(error);
        return;
    }
    clearError();
    setStatus(favorite ? tr("Zu Favoriten hinzugefügt") : tr("Aus Favoriten entfernt"));
}

void NoteController::setTags(const QString &id, const QStringList &tags)
{
    QString error;
    if (!m_repository.setTags(id, tags, &error)) {
        setError(error);
        return;
    }
    clearError();
    setStatus(tr("Tags gespeichert"));
}

void NoteController::setSearchQuery(const QString &query) { m_noteModel.setQuery(query); }
void NoteController::setSection(const QString &section) { m_noteModel.setSection(section); }

void NoteController::restoreFromTrash(const QString &id)
{
    QString error;
    if (!m_repository.restoreFromTrash(id, &error)) {
        setError(error);
        return;
    }
    clearError();
    setStatus(tr("Notiz wiederhergestellt"));
}

void NoteController::reloadNotes()
{
    QString error;
    const QVector<Note> items = m_repository.notes(&error);
    m_noteModel.setNotes(items);
    if (!error.isEmpty())
        setError(error);
}

void NoteController::setStatus(const QString &message)
{
    if (m_statusText == message)
        return;
    m_statusText = message;
    emit statusTextChanged();
}

void NoteController::setError(const QString &message)
{
    if (m_errorText == message)
        return;
    m_errorText = message;
    emit errorTextChanged();
}

void NoteController::clearError()
{
    if (m_errorText.isEmpty())
        return;
    m_errorText.clear();
    emit errorTextChanged();
}

} // namespace CrossNote
