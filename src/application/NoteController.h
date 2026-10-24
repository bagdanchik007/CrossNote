#pragma once

#include "data/NoteRepository.h"
#include "domain/MarkdownMetrics.h"
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
    Q_PROPERTY(QStringList selectedLinks READ selectedLinks NOTIFY linkAnalysisChanged)
    Q_PROPERTY(QStringList selectedBacklinks READ selectedBacklinks NOTIFY linkAnalysisChanged)
    Q_PROPERTY(QStringList selectedMissingLinks READ selectedMissingLinks NOTIFY linkAnalysisChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY currentNoteChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(int editorFontSize READ editorFontSize NOTIFY editorFontSizeChanged)
    Q_PROPERTY(bool darkTheme READ darkTheme NOTIFY themeChanged)
    Q_PROPERTY(int selectedWordCount READ selectedWordCount NOTIFY metricsChanged)
    Q_PROPERTY(int selectedCharacterCount READ selectedCharacterCount NOTIFY metricsChanged)
    Q_PROPERTY(int selectedCompactCharacterCount READ selectedCompactCharacterCount NOTIFY metricsChanged)
    Q_PROPERTY(int selectedTaskCount READ selectedTaskCount NOTIFY metricsChanged)
    Q_PROPERTY(int selectedCompletedTaskCount READ selectedCompletedTaskCount NOTIFY metricsChanged)
    Q_PROPERTY(int selectedOpenTaskCount READ selectedOpenTaskCount NOTIFY metricsChanged)
    Q_PROPERTY(int selectedTaskCompletionPercent READ selectedTaskCompletionPercent NOTIFY metricsChanged)
    Q_PROPERTY(int selectedReadingMinutes READ selectedReadingMinutes NOTIFY metricsChanged)

public:
    explicit NoteController(QObject *parent = nullptr);
    explicit NoteController(const QString &indexRoot, QObject *parent = nullptr);

    [[nodiscard]] NoteListModel *noteModel();
    [[nodiscard]] QString vaultPath() const;
    [[nodiscard]] QString selectedId() const;
    [[nodiscard]] QString selectedTitle() const;
    [[nodiscard]] QString selectedContent() const;
    [[nodiscard]] QStringList selectedTags() const;
    [[nodiscard]] QStringList selectedLinks() const;
    [[nodiscard]] QStringList selectedBacklinks() const;
    [[nodiscard]] QStringList selectedMissingLinks() const;
    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString errorText() const;
    [[nodiscard]] int editorFontSize() const;
    [[nodiscard]] bool darkTheme() const;
    [[nodiscard]] int selectedWordCount() const;
    [[nodiscard]] int selectedCharacterCount() const;
    [[nodiscard]] int selectedCompactCharacterCount() const;
    [[nodiscard]] int selectedTaskCount() const;
    [[nodiscard]] int selectedCompletedTaskCount() const;
    [[nodiscard]] int selectedOpenTaskCount() const;
    [[nodiscard]] int selectedTaskCompletionPercent() const;
    [[nodiscard]] int selectedReadingMinutes() const;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void setVaultPath(const QString &path);
    Q_INVOKABLE bool createNote(const QString &title = QString());
    Q_INVOKABLE bool createNoteWithTemplate(const QString &title, const QString &templateId);
    Q_INVOKABLE bool openDailyNote();
    Q_INVOKABLE bool exportSelectedNote(const QString &destination);
    Q_INVOKABLE bool openNote(const QString &id);
    Q_INVOKABLE bool openNoteByTitle(const QString &title);
    Q_INVOKABLE bool saveCurrent(const QString &title, const QString &content);
    Q_INVOKABLE void deleteCurrent();
    Q_INVOKABLE void setFavorite(const QString &id, bool favorite);
    Q_INVOKABLE void setArchived(const QString &id, bool archived);
    Q_INVOKABLE void setTags(const QString &id, const QStringList &tags);
    Q_INVOKABLE void setSearchQuery(const QString &query);
    Q_INVOKABLE void setTagFilter(const QString &tag);
    Q_INVOKABLE void rebuildSearchIndex();
    Q_INVOKABLE void setSection(const QString &section);
    Q_INVOKABLE void restoreFromTrash(const QString &id);
    Q_INVOKABLE void setEditorFontSize(int size);
    Q_INVOKABLE void setDarkTheme(bool enabled);

signals:
    void vaultPathChanged();
    void currentNoteChanged();
    void selectedTagsChanged();
    void linkAnalysisChanged();
    void statusTextChanged();
    void errorTextChanged();
    void editorFontSizeChanged();
    void themeChanged();
    void metricsChanged();

private:
    void reloadNotes();
    bool activateCreatedNote(const Note &note);
    void refreshLinkAnalysis();
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
    QStringList m_selectedLinks;
    QStringList m_selectedBacklinks;
    QStringList m_selectedMissingLinks;
    QString m_statusText;
    QString m_errorText;
    int m_editorFontSize = 16;
    bool m_darkTheme = false;
    MarkdownMetrics m_selectedMetrics;
};

} // namespace CrossNote
