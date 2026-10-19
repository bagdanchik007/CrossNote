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

QTEST_GUILESS_MAIN(NoteCreationTest)

#include "NoteCreationTest.moc"
