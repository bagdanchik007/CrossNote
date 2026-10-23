#include "data/NoteRepository.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

using namespace CrossNote;

class NoteCreationTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsSearchableMarkdownNote();
    void createsPortableUniqueFilenames();
    void indexesMarkdownAddedAfterVaultOpen();
    void keepsFullTextIndexInSyncWithEditsAndTags();
    void removesAndRestoresNotesWithTheirSearchEntries();
    void analyzesWikiLinksAndBacklinks();
    void dailyNotesAreIdempotentPerDate();
    void createsNotesWithPrefilledMarkdown();
    void archivesPreviousVersionsOutsideTheActiveVaultIndex();
    void exportsMarkdownWithoutChangingTheVault();
    void searchTreatsQuerySyntaxAsPlainText();
    void rejectsOverlongTitlesOnSave();
    void tagUpdatesPersistToMarkdown();
    void refreshRejectsDuplicateStableIds();
    void rejectsRepositoryQueriesBeforeVaultOpen();
    void archiveStateSurvivesRepositoryReopen();
    void clearsStaleErrorOutputsOnSuccess();
    void reportsMissingFilesInsteadOfSilentlyHidingNotes();
    void rejectsMutationsBeforeVaultOpen();
    void rejectsEmptyTitleLookup();
    void reportsMissingTrashFiles();
    void rejectsNullSearchOutput();
    void tagChangesAdvanceModificationTime();
    void preservesExternalVersionWhenSavingAfterConflict();
    void renamesNoteWithoutLeavingDuplicateMarkdown();
    void rejectsMarkdownSymlinksOutsideVault();
    void keepsCurrentVaultWhenSwitchFails();
    void unchangedFlagsDoNotEmitReloads();
    void rebuildsSearchIndexFromMarkdownFiles();
    void importsNotesFromNestedDirectories();
    void rejectsInvalidDailyNoteDates();
    void reopensSameVaultWithoutLosingIndexedNotes();
    void returnsCommittedHashFromSave();
};

void NoteCreationTest::createsSearchableMarkdownNote()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));

    const auto created = repository.createNote(QStringLiteral("Projekt Ideen"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QCOMPARE(created->title, QStringLiteral("Projekt Ideen"));
    QVERIFY(QFileInfo::exists(QDir(vault.path()).filePath(created->relativePath)));

    const auto reopened = repository.note(created->id, true, &error);
    QVERIFY2(reopened.has_value(), qPrintable(error));
    QCOMPARE(reopened->title, created->title);
    QVERIFY(reopened->contentHash.size() > 0);

    QStringList matchingIds;
    QVERIFY2(repository.searchIds(QStringLiteral("Ideen"), &matchingIds, &error), qPrintable(error));
    QVERIFY(matchingIds.contains(created->id));
}

void NoteCreationTest::createsPortableUniqueFilenames()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));

    const auto first = repository.createNote(QStringLiteral("Plan"), &error);
    QVERIFY2(first.has_value(), qPrintable(error));
    const auto second = repository.createNote(QStringLiteral("plan"), &error);
    QVERIFY2(second.has_value(), qPrintable(error));
    const auto reserved = repository.createNote(QStringLiteral("CON"), &error);
    QVERIFY2(reserved.has_value(), qPrintable(error));

    QCOMPARE(first->relativePath, QStringLiteral("Plan.md"));
    QCOMPARE(second->relativePath, QStringLiteral("plan-2.md"));
    QCOMPARE(reserved->relativePath, QStringLiteral("_CON.md"));
}

void NoteCreationTest::indexesMarkdownAddedAfterVaultOpen()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));

    QFile externalNote(vault.filePath(QStringLiteral("Imported.md")));
    QVERIFY(externalNote.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray markdown = R"(---
id: 62442507-82f5-4b83-9bf9-f260a8bf22b4
tags: ["external"]
---

# Imported

Borrowed knowledge.
)";
    QCOMPARE(externalNote.write(markdown), qint64(markdown.size()));
    externalNote.close();

    QVERIFY2(repository.refresh(&error), qPrintable(error));
    const auto id = repository.noteIdForTitle(QStringLiteral("Imported"), &error);
    QVERIFY2(id.has_value(), qPrintable(error));
    const auto imported = repository.note(*id, true, &error);
    QVERIFY2(imported.has_value(), qPrintable(error));
    QVERIFY(imported->content.contains(QStringLiteral("Borrowed knowledge")));
    QVERIFY(imported->tags.contains(QStringLiteral("external")));
}

void NoteCreationTest::keepsFullTextIndexInSyncWithEditsAndTags()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Research"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));

    auto current = repository.note(created->id, true, &error);
    QVERIFY2(current.has_value(), qPrintable(error));
    QVERIFY2(repository.saveNote(created->id, created->title, QStringLiteral("first draft"),
                                 current->contentHash, &error), qPrintable(error));

    QStringList ids;
    QVERIFY2(repository.searchIds(QStringLiteral("draft"), &ids, &error), qPrintable(error));
    QVERIFY(ids.contains(created->id));

    current = repository.note(created->id, true, &error);
    QVERIFY2(current.has_value(), qPrintable(error));
    QVERIFY2(repository.saveNote(created->id, created->title, QStringLiteral("final manuscript"),
                                 current->contentHash, &error), qPrintable(error));
    QVERIFY2(repository.setTags(created->id, {QStringLiteral("roadmap")}, &error), qPrintable(error));

    QVERIFY2(repository.searchIds(QStringLiteral("draft"), &ids, &error), qPrintable(error));
    QVERIFY(!ids.contains(created->id));
    QVERIFY2(repository.searchIds(QStringLiteral("manuscript"), &ids, &error), qPrintable(error));
    QVERIFY(ids.contains(created->id));
    QVERIFY2(repository.searchIds(QStringLiteral("roadmap"), &ids, &error), qPrintable(error));
    QVERIFY(ids.contains(created->id));
}

void NoteCreationTest::removesAndRestoresNotesWithTheirSearchEntries()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Archive entry"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));

    QStringList ids;
    QVERIFY2(repository.searchIds(QStringLiteral("Archive"), &ids, &error), qPrintable(error));
    QVERIFY(ids.contains(created->id));

    QVERIFY2(repository.moveToTrash(created->id, &error), qPrintable(error));
    QVERIFY2(repository.searchIds(QStringLiteral("Archive"), &ids, &error), qPrintable(error));
    QVERIFY(!ids.contains(created->id));
    QCOMPARE(repository.trashedNotes(&error).size(), 1);

    QVERIFY2(repository.restoreFromTrash(created->id, &error), qPrintable(error));
    QVERIFY2(repository.searchIds(QStringLiteral("Archive"), &ids, &error), qPrintable(error));
    QVERIFY(ids.contains(created->id));
    QCOMPARE(repository.trashedNotes(&error).size(), 0);
}

void NoteCreationTest::analyzesWikiLinksAndBacklinks()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto target = repository.createNote(QStringLiteral("Architecture"), &error);
    QVERIFY2(target.has_value(), qPrintable(error));
    const auto source = repository.createNote(QStringLiteral("Design notes"), &error);
    QVERIFY2(source.has_value(), qPrintable(error));

    auto current = repository.note(source->id, true, &error);
    QVERIFY2(current.has_value(), qPrintable(error));
    QVERIFY2(repository.saveNote(source->id, source->title,
                                 QStringLiteral("See [[Architecture]], [[Future]] and [[future]]."),
                                 current->contentHash, &error), qPrintable(error));

    const auto sourceLinks = repository.analyzeLinks(source->id, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(sourceLinks.outgoing, QStringList({QStringLiteral("Architecture"), QStringLiteral("Future")}));
    QCOMPARE(sourceLinks.missing, QStringList({QStringLiteral("Future")}));

    const auto targetLinks = repository.analyzeLinks(target->id, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(targetLinks.backlinks, QStringList({QStringLiteral("Design notes")}));
}

void NoteCreationTest::dailyNotesAreIdempotentPerDate()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const QDate day(2026, 6, 13);
    const auto first = repository.dailyNote(day, &error);
    QVERIFY2(first.has_value(), qPrintable(error));
    const auto second = repository.dailyNote(day, &error);
    QVERIFY2(second.has_value(), qPrintable(error));

    QCOMPARE(first->id, second->id);
    QCOMPARE(first->title, QStringLiteral("2026-06-13"));
    QCOMPARE(repository.notes(&error).size(), 1);
}

void NoteCreationTest::createsNotesWithPrefilledMarkdown()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const QString body = QStringLiteral("## Agenda\n\n- [ ] Confirm requirements\n- [ ] Assign an owner");
    const auto created = repository.createNote(QStringLiteral("Planning"), body, &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QCOMPARE(created->content, body);

    const auto reopened = repository.note(created->id, true, &error);
    QVERIFY2(reopened.has_value(), qPrintable(error));
    QCOMPARE(reopened->content, body);
    QStringList ids;
    QVERIFY2(repository.searchIds(QStringLiteral("requirements"), &ids, &error), qPrintable(error));
    QVERIFY(ids.contains(created->id));
}

void NoteCreationTest::archivesPreviousVersionsOutsideTheActiveVaultIndex()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Revision"), QStringLiteral("original phrase"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY2(repository.saveNote(created->id, created->title, QStringLiteral("updated phrase"),
                                 created->contentHash, &error), qPrintable(error));

    const QString backupDirectory = vault.filePath(QStringLiteral(".backups/%1").arg(created->id));
    const QStringList backups = QDir(backupDirectory).entryList({QStringLiteral("*.md")}, QDir::Files);
    QCOMPARE(backups.size(), 1);
    QFile previous(QDir(backupDirectory).filePath(backups.first()));
    QVERIFY(previous.open(QIODevice::ReadOnly));
    QVERIFY(QString::fromUtf8(previous.readAll()).contains(QStringLiteral("original phrase")));

    QCOMPARE(repository.notes(&error).size(), 1);
    QStringList ids;
    QVERIFY2(repository.searchIds(QStringLiteral("original"), &ids, &error), qPrintable(error));
    QVERIFY(!ids.contains(created->id));
}

void NoteCreationTest::exportsMarkdownWithoutChangingTheVault()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QTemporaryDir output(QDir::current().filePath(QStringLiteral("test-export-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    QVERIFY(output.isValid());

    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Export me"), QStringLiteral("Exact markdown bytes"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));

    const QString sourcePath = vault.filePath(created->relativePath);
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::ReadOnly));
    const QByteArray original = source.readAll();
    source.close();

    const QString destination = output.filePath(QStringLiteral("exported-note.md"));
    QVERIFY2(repository.exportNote(created->id, destination, &error), qPrintable(error));
    QFile exported(destination);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    QCOMPARE(exported.readAll(), original);
    QVERIFY(!repository.exportNote(created->id, sourcePath, &error));
    QCOMPARE(repository.notes(&error).size(), 1);
}

void NoteCreationTest::searchTreatsQuerySyntaxAsPlainText()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Searchable"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));

    QStringList ids;
    QVERIFY2(repository.searchIds(QStringLiteral("&&& : !?"), &ids, &error), qPrintable(error));
    QVERIFY(ids.isEmpty());
    QVERIFY2(repository.searchIds(QStringLiteral("Searchable OR missing"), &ids, &error), qPrintable(error));
    QVERIFY(ids.isEmpty());
}

void NoteCreationTest::rejectsOverlongTitlesOnSave()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Bounded title"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY(!repository.saveNote(created->id, QString(161, QLatin1Char('x')),
                                 QStringLiteral("must not be written"), created->contentHash, &error));
    QVERIFY(!error.isEmpty());
    const auto current = repository.note(created->id, true, &error);
    QVERIFY2(current.has_value(), qPrintable(error));
    QCOMPARE(current->title, QStringLiteral("Bounded title"));
}

void NoteCreationTest::tagUpdatesPersistToMarkdown()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Tagged"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY2(repository.setTags(created->id, {QStringLiteral(" portable "), QStringLiteral("PORTABLE")}, &error),
             qPrintable(error));
    const auto tagged = repository.note(created->id, false, &error);
    QVERIFY2(tagged.has_value(), qPrintable(error));
    QCOMPARE(tagged->tags, QStringList({QStringLiteral("portable")}));

    QFile markdown(vault.filePath(created->relativePath));
    QVERIFY(markdown.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(markdown.readAll());
    QVERIFY(text.contains(QStringLiteral("portable")));
    QCOMPARE(text.count(QStringLiteral("portable")), 1);
    const QString backupDirectory = vault.filePath(QStringLiteral(".backups/%1").arg(created->id));
    QCOMPARE(QDir(backupDirectory).entryList({QStringLiteral("*.md")}, QDir::Files).size(), 1);
    QVERIFY2(repository.setTags(created->id, {QStringLiteral("portable")}, &error), qPrintable(error));
    QCOMPARE(QDir(backupDirectory).entryList({QStringLiteral("*.md")}, QDir::Files).size(), 1);
}

void NoteCreationTest::refreshRejectsDuplicateStableIds()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const QByteArray markdown = "---\nid: 5e6eb4d4-42c1-4c45-93a9-b70a25a4c5c6\ntags: []\n---\n\n# Shared\n\nBody\n";
    for (const QString &name : {QStringLiteral("first.md"), QStringLiteral("second.md")}) {
        QFile file(vault.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(markdown), qint64(markdown.size()));
        file.close();
    }

    QVERIFY(!repository.refresh(&error));
    QVERIFY(error.contains(QStringLiteral("ID"), Qt::CaseInsensitive));
    QCOMPARE(repository.notes(&error).size(), 0);
    QVERIFY(QFileInfo::exists(vault.filePath(QStringLiteral("first.md"))));
    QVERIFY(QFileInfo::exists(vault.filePath(QStringLiteral("second.md"))));
}

void NoteCreationTest::rejectsRepositoryQueriesBeforeVaultOpen()
{
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QStringList ids{QStringLiteral("stale")};
    QVERIFY(!repository.searchIds(QStringLiteral("anything"), &ids, &error));
    QVERIFY(ids.isEmpty());
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!repository.refresh(&error));
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(repository.notes(&error).isEmpty());
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!repository.note(QStringLiteral("missing"), true, &error).has_value());
    QVERIFY(!error.isEmpty());
}

void NoteCreationTest::archiveStateSurvivesRepositoryReopen()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    QString error;
    QString noteId;
    {
        NoteRepository repository(index.path());
        QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
        const auto created = repository.createNote(QStringLiteral("Archive persists"), &error);
        QVERIFY2(created.has_value(), qPrintable(error));
        noteId = created->id;
        QVERIFY2(repository.setArchived(noteId, true, &error), qPrintable(error));
        QVERIFY(!repository.setArchived(QStringLiteral("missing"), true, &error));
        QVERIFY(!error.isEmpty());
    }
    {
        NoteRepository repository(index.path());
        QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
        const auto reopened = repository.note(noteId, false, &error);
        QVERIFY2(reopened.has_value(), qPrintable(error));
        QVERIFY(reopened->archived);
    }
}

void NoteCreationTest::clearsStaleErrorOutputsOnSuccess()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error = QStringLiteral("stale error");
    QVERIFY(repository.openVault(vault.path(), &error));
    QVERIFY(error.isEmpty());
    error = QStringLiteral("stale error");
    const auto created = repository.createNote(QStringLiteral("No stale errors"), &error);
    QVERIFY(created.has_value());
    QVERIFY(error.isEmpty());
    error = QStringLiteral("stale error");
    QVERIFY(repository.setFavorite(created->id, true, &error));
    QVERIFY(error.isEmpty());
    error = QStringLiteral("stale error");
    QVERIFY(repository.note(created->id, false, &error).has_value());
    QVERIFY(error.isEmpty());
}

void NoteCreationTest::reportsMissingFilesInsteadOfSilentlyHidingNotes()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Temporarily missing"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY(QFile::remove(vault.filePath(created->relativePath)));

    const QVector<Note> visible = repository.notes(&error);
    QVERIFY(visible.isEmpty());
    QVERIFY(error.contains(QStringLiteral("Temporarily missing"), Qt::CaseInsensitive)
            || error.contains(created->relativePath));
}

void NoteCreationTest::rejectsMutationsBeforeVaultOpen()
{
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY(!repository.setFavorite(QStringLiteral("id"), true, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!repository.setArchived(QStringLiteral("id"), true, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!repository.markOpened(QStringLiteral("id"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!repository.moveToTrash(QStringLiteral("id"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!repository.restoreFromTrash(QStringLiteral("id"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!repository.rebuildSearchIndex(&error));
    QVERIFY(!error.isEmpty());
}

void NoteCreationTest::rejectsEmptyTitleLookup()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Not empty"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY(!repository.noteIdForTitle(QStringLiteral("   "), &error));
    QVERIFY(error.isEmpty());
}

void NoteCreationTest::reportsMissingTrashFiles()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Trash integrity"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY2(repository.moveToTrash(created->id, &error), qPrintable(error));
    QVERIFY(QFile::remove(vault.filePath(QStringLiteral(".trash/%1.md").arg(created->id))));
    QVERIFY(repository.trashedNotes(&error).isEmpty());
    QVERIFY(error.contains(QStringLiteral("kann nicht gelesen werden")));
}

void NoteCreationTest::rejectsNullSearchOutput()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    QVERIFY(!repository.searchIds(QStringLiteral("anything"), nullptr, &error));
    QVERIFY(error.contains(QStringLiteral("kein Ziel")));
}

void NoteCreationTest::tagChangesAdvanceModificationTime()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Tag timestamp"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QTest::qWait(2);
    QVERIFY2(repository.setTags(created->id, {QStringLiteral("important")}, &error), qPrintable(error));
    const auto updated = repository.note(created->id, false, &error);
    QVERIFY2(updated.has_value(), qPrintable(error));
    QVERIFY(updated->updatedAt > created->updatedAt);
}

void NoteCreationTest::preservesExternalVersionWhenSavingAfterConflict()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Conflict handling"), QStringLiteral("local"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QFile external(vault.filePath(created->relativePath));
    QVERIFY(external.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray changed = "---\nid: " + created->id.toUtf8()
        + "\ntags: []\n---\n\n# Conflict handling\n\nexternal version\n";
    QCOMPARE(external.write(changed), qint64(changed.size()));
    external.close();
    QSignalSpy conflictSpy(&repository, &NoteRepository::externalVersionBackedUp);
    QVERIFY2(repository.saveNote(created->id, created->title, QStringLiteral("local edit"),
                                 created->contentHash, &error), qPrintable(error));
    QCOMPARE(conflictSpy.count(), 1);
    const QString conflictPath = vault.filePath(conflictSpy.takeFirst().at(0).toString());
    QFile backup(conflictPath);
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QVERIFY(backup.readAll().contains("external version"));
    const auto saved = repository.note(created->id, true, &error);
    QVERIFY2(saved.has_value(), qPrintable(error));
    QCOMPARE(saved->content, QStringLiteral("local edit"));
}

void NoteCreationTest::renamesNoteWithoutLeavingDuplicateMarkdown()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Before rename"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY2(repository.saveNote(created->id, QStringLiteral("After rename"), QStringLiteral("body"),
                                 created->contentHash, &error), qPrintable(error));
    QVERIFY(!QFileInfo::exists(vault.filePath(QStringLiteral("Before rename.md"))));
    QVERIFY(QFileInfo::exists(vault.filePath(QStringLiteral("After-rename.md"))));
    QVERIFY2(repository.refresh(&error), qPrintable(error));
    QCOMPARE(repository.notes(&error).size(), 1);
    const auto reopened = repository.note(created->id, true, &error);
    QVERIFY2(reopened.has_value(), qPrintable(error));
    QCOMPARE(reopened->relativePath, QStringLiteral("After-rename.md"));
}

void NoteCreationTest::rejectsMarkdownSymlinksOutsideVault()
{
#ifdef Q_OS_UNIX
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QTemporaryDir outside(QDir::current().filePath(QStringLiteral("test-outside-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    QVERIFY(outside.isValid());
    QFile external(outside.filePath(QStringLiteral("external.md")));
    QVERIFY(external.open(QIODevice::WriteOnly));
    external.write("# Outside\n");
    external.close();
    QVERIFY(QFile::link(external.fileName(), vault.filePath(QStringLiteral("linked.md"))));
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    QVERIFY(!repository.refresh(&error));
    QVERIFY(error.contains(QStringLiteral("außerhalb")));
#else
    QSKIP("Der Symlink-Test wird auf Unix ausgeführt.");
#endif
}

void NoteCreationTest::keepsCurrentVaultWhenSwitchFails()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QTemporaryDir invalidRoot(QDir::current().filePath(QStringLiteral("test-invalid-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    QVERIFY(invalidRoot.isValid());
    QFile blockingFile(invalidRoot.filePath(QStringLiteral("not-a-directory")));
    QVERIFY(blockingFile.open(QIODevice::WriteOnly));
    blockingFile.close();
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Still available"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY(!repository.openVault(blockingFile.fileName(), &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(repository.vaultPath(), QFileInfo(vault.path()).canonicalFilePath());
    QVERIFY(repository.note(created->id, true, &error).has_value());
}

void NoteCreationTest::unchangedFlagsDoNotEmitReloads()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Flags"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QSignalSpy changedSpy(&repository, &NoteRepository::notesChanged);
    QVERIFY2(repository.setFavorite(created->id, false, &error), qPrintable(error));
    QVERIFY2(repository.setArchived(created->id, false, &error), qPrintable(error));
    QCOMPARE(changedSpy.count(), 0);
    QVERIFY(!repository.setFavorite(QStringLiteral("missing"), true, &error));
    QVERIFY(!error.isEmpty());
}

void NoteCreationTest::rebuildsSearchIndexFromMarkdownFiles()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Index rebuild"), QStringLiteral("oldtoken"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QFile markdown(vault.filePath(created->relativePath));
    QVERIFY(markdown.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray external = "---\nid: " + created->id.toUtf8()
        + "\ntags: []\n---\n\n# Index rebuild\n\nnewtoken\n";
    QCOMPARE(markdown.write(external), qint64(external.size()));
    markdown.close();
    QVERIFY2(repository.rebuildSearchIndex(&error), qPrintable(error));
    QStringList ids;
    QVERIFY2(repository.searchIds(QStringLiteral("newtoken"), &ids, &error), qPrintable(error));
    QVERIFY(ids.contains(created->id));
    QVERIFY2(repository.searchIds(QStringLiteral("oldtoken"), &ids, &error), qPrintable(error));
    QVERIFY(!ids.contains(created->id));
}

void NoteCreationTest::importsNotesFromNestedDirectories()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    QVERIFY(QDir(vault.path()).mkpath(QStringLiteral("Projects/Design")));
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    QFile file(vault.filePath(QStringLiteral("Projects/Design/Overview.md")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray markdown = "---\nid: 2d7dedaa-3080-4d89-a489-18090a990d7c\ntags: [\"design\"]\n---\n\n# Overview\n\nNested content.\n";
    QCOMPARE(file.write(markdown), qint64(markdown.size()));
    file.close();
    QVERIFY2(repository.refresh(&error), qPrintable(error));
    const auto id = repository.noteIdForTitle(QStringLiteral("Overview"), &error);
    QVERIFY2(id.has_value(), qPrintable(error));
    const auto imported = repository.note(*id, true, &error);
    QVERIFY2(imported.has_value(), qPrintable(error));
    QCOMPARE(imported->relativePath, QStringLiteral("Projects/Design/Overview.md"));
    QVERIFY(imported->tags.contains(QStringLiteral("design")));
    QVERIFY(imported->content.contains(QStringLiteral("Nested content")));
}

void NoteCreationTest::rejectsInvalidDailyNoteDates()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    QVERIFY(!repository.dailyNote(QDate(), &error));
    QVERIFY(error.contains(QStringLiteral("ungültig")));
    QCOMPARE(repository.notes(&error).size(), 0);
}

void NoteCreationTest::reopensSameVaultWithoutLosingIndexedNotes()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Reopened vault"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto reopened = repository.note(created->id, true, &error);
    QVERIFY2(reopened.has_value(), qPrintable(error));
    QCOMPARE(reopened->title, created->title);
}

void NoteCreationTest::returnsCommittedHashFromSave()
{
    QTemporaryDir vault(QDir::current().filePath(QStringLiteral("test-vault-XXXXXX")));
    QTemporaryDir index(QDir::current().filePath(QStringLiteral("test-index-XXXXXX")));
    QVERIFY(vault.isValid());
    QVERIFY(index.isValid());
    NoteRepository repository(index.path());
    QString error;
    QVERIFY2(repository.openVault(vault.path(), &error), qPrintable(error));
    const auto created = repository.createNote(QStringLiteral("Hash result"), &error);
    QVERIFY2(created.has_value(), qPrintable(error));
    QByteArray savedHash("stale");
    const QString body = QStringLiteral("saved once");
    QVERIFY2(repository.saveNote(created->id, created->title, body, created->contentHash,
                                 &error, &savedHash), qPrintable(error));
    const auto current = repository.note(created->id, true, &error);
    QVERIFY2(current.has_value(), qPrintable(error));
    QCOMPARE(savedHash, current->contentHash);
}

QTEST_GUILESS_MAIN(NoteCreationTest)

#include "NoteCreationTest.moc"
