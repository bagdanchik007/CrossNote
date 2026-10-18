#include "NoteController.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace CrossNote {

NoteController::NoteController(QObject *parent)
    : QObject(parent), m_repository(), m_noteModel()
{
    m_editorFontSize = qBound(12, QSettings().value(QStringLiteral("editor/fontSize"), 16).toInt(), 32);
    connect(&m_repository, &NoteRepository::notesChanged,
            this, &NoteController::reloadNotes);
    connect(&m_repository, &NoteRepository::errorOccurred,
            this, &NoteController::setError);
    connect(&m_repository, &NoteRepository::externalVersionBackedUp, this,
            [this](const QString &path) { m_conflictPath = path; });
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
int NoteController::editorFontSize() const { return m_editorFontSize; }

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
    m_selectedHash.clear();
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
    m_selectedHash = selected->contentHash;
    m_selectedTags = selected->tags;
    emit selectedTagsChanged();
    m_repository.markOpened(id, &error);
    emit currentNoteChanged();
    clearError();
    setStatus(tr("Notiz geöffnet"));
}

void NoteController::openNoteByTitle(const QString &title)
{
    QString error;
    const auto id = m_repository.noteIdForTitle(title, &error);
    if (!id) {
        if (!error.isEmpty())
            setError(error);
        else
            setStatus(tr("Verlinkte Notiz nicht gefunden: %1").arg(title));
        return;
    }
    openNote(*id);
}

void NoteController::saveCurrent(const QString &title, const QString &content)
{
    if (m_selectedId.isEmpty())
        return;
    QString error;
    if (!m_repository.saveNote(m_selectedId, title, content, m_selectedHash, &error)) {
        setError(error);
        return;
    }
    m_selectedTitle = title.trimmed().isEmpty() ? tr("Unbenannte Notiz") : title.trimmed();
    m_selectedContent = content;
    const auto saved = m_repository.note(m_selectedId, true);
    if (saved) m_selectedHash = saved->contentHash;
    clearError();
    if (!m_conflictPath.isEmpty()) {
        setStatus(tr("Externe Fassung als Konfliktkopie gesichert: %1").arg(m_conflictPath));
        m_conflictPath.clear();
    } else {
        setStatus(tr("Gespeichert"));
    }
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
    m_selectedHash.clear();
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
    if (id == m_selectedId) {
        m_selectedTags = tags;
        emit selectedTagsChanged();
    }
    clearError();
    setStatus(tr("Tags gespeichert"));
}

void NoteController::setSearchQuery(const QString &query)
{
    const QString normalized = query.trimmed();
    m_noteModel.setQuery(normalized);
    if (normalized.isEmpty()) {
        m_noteModel.setSearchResultIds({}, false);
        return;
    }

    QStringList ids;
    QString error;
    if (!m_repository.searchIds(normalized, &ids, &error)) {
        m_noteModel.setSearchResultIds({}, false);
        setError(error);
        return;
    }
    m_noteModel.setSearchResultIds(ids, true);
    clearError();
}

void NoteController::setTagFilter(const QString &tag)
{
    m_noteModel.setTagFilter(tag);
}

void NoteController::rebuildSearchIndex()
{
    QString error;
    if (!m_repository.rebuildSearchIndex(&error)) {
        setError(error);
        return;
    }
    clearError();
    setStatus(tr("Volltextindex wurde aus den Markdown-Dateien neu aufgebaut"));
}

void NoteController::setSection(const QString &section)
{
    m_noteModel.setSection(section);
    if (m_noteModel.section() == QStringLiteral("trash")) {
        m_selectedId.clear();
        m_selectedTitle.clear();
        m_selectedContent.clear();
        m_selectedHash.clear();
        m_selectedTags.clear();
        emit selectedTagsChanged();
        emit currentNoteChanged();
    }
    reloadNotes();
}

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

void NoteController::setEditorFontSize(int size)
{
    const int boundedSize = qBound(12, size, 32);
    if (m_editorFontSize == boundedSize)
        return;

    m_editorFontSize = boundedSize;
    QSettings().setValue(QStringLiteral("editor/fontSize"), m_editorFontSize);
    emit editorFontSizeChanged();
}

void NoteController::reloadNotes()
{
    QString error;
    const QVector<Note> items = m_noteModel.section() == QStringLiteral("trash")
        ? m_repository.trashedNotes(&error) : m_repository.notes(&error);
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
