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

QTEST_GUILESS_MAIN(NoteCreationTest)

#include "NoteCreationTest.moc"
