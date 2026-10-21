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

    QFile markdown(vault.filePath(created->relativePath));
    QVERIFY(markdown.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(markdown.readAll());
    QVERIFY(text.contains(QStringLiteral("portable")));
    QCOMPARE(text.count(QStringLiteral("portable")), 1);
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

QTEST_GUILESS_MAIN(NoteCreationTest)

#include "NoteCreationTest.moc"
