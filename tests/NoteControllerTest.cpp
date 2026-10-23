#include "application/NoteController.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using namespace CrossNote;

class NoteControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void createsAndSelectsNotes();
    void savesContentAndRefreshesSearchResults();
    void failedVaultSwitchKeepsCurrentNote();
    void reopensSameNoteFromLatestDiskContent();
    void reportsMissingNoteOpeningFailure();
    void tagEditDoesNotCreateFalseExternalConflict();
    void persistsBoundedEditorAndThemePreferences();

private:
    std::unique_ptr<QTemporaryDir> m_sandbox;
    std::unique_ptr<QTemporaryDir> m_testVault;
    std::unique_ptr<QTemporaryDir> m_testIndex;
    QString m_vaultPath;
    QString m_indexPath;
};

void NoteControllerTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("CrossNoteTests"));
    QCoreApplication::setApplicationName(QStringLiteral("NoteControllerTest"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    m_sandbox = std::make_unique<QTemporaryDir>(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("controller-settings-XXXXXX")));
    QVERIFY(m_sandbox->isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_sandbox->path());
}

void NoteControllerTest::init()
{
    QSettings().clear();
    m_testVault = std::make_unique<QTemporaryDir>(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("controller-vault-XXXXXX")));
    m_testIndex = std::make_unique<QTemporaryDir>(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("controller-index-XXXXXX")));
    QVERIFY(m_testVault->isValid());
    QVERIFY(m_testIndex->isValid());
    m_vaultPath = m_testVault->path();
    m_indexPath = m_testIndex->path();
}

void NoteControllerTest::createsAndSelectsNotes()
{
    NoteController controller(m_indexPath);
    controller.setVaultPath(m_vaultPath);
    QVERIFY2(controller.errorText().isEmpty(), qPrintable(controller.errorText()));
    QVERIFY(controller.createNote(QStringLiteral("Controller note")));
    QVERIFY(controller.hasSelection());
    QCOMPARE(controller.selectedTitle(), QStringLiteral("Controller note"));
    QVERIFY(!controller.selectedId().isEmpty());
    QCOMPARE(controller.noteModel()->count(), 1);
}

void NoteControllerTest::savesContentAndRefreshesSearchResults()
{
    NoteController controller(m_indexPath);
    controller.setVaultPath(m_vaultPath);
    QVERIFY(controller.createNote(QStringLiteral("Draft")));
    QVERIFY(controller.saveCurrent(QStringLiteral("Final title"), QStringLiteral("needle phrase")));
    QCOMPARE(controller.selectedTitle(), QStringLiteral("Final title"));
    controller.setSearchQuery(QStringLiteral("needle"));
    QCOMPARE(controller.noteModel()->count(), 1);
    controller.setSearchQuery(QStringLiteral("absent"));
    QCOMPARE(controller.noteModel()->count(), 0);
    QVERIFY(controller.errorText().isEmpty());
    controller.setSearchQuery(QString());
    QCOMPARE(controller.noteModel()->count(), 1);
}

void NoteControllerTest::failedVaultSwitchKeepsCurrentNote()
{
    NoteController controller(m_indexPath);
    controller.setVaultPath(m_vaultPath);
    QVERIFY(controller.createNote(QStringLiteral("Still here")));
    const QString selectedId = controller.selectedId();
    const QString invalidPath = m_sandbox->filePath(QStringLiteral("regular-file"));
    QFile file(invalidPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    controller.setVaultPath(invalidPath);
    QVERIFY(!controller.errorText().isEmpty());
    QCOMPARE(controller.vaultPath(), QFileInfo(m_vaultPath).canonicalFilePath());
    QCOMPARE(controller.selectedId(), selectedId);
    QCOMPARE(controller.selectedTitle(), QStringLiteral("Still here"));
}

void NoteControllerTest::reopensSameNoteFromLatestDiskContent()
{
    NoteController controller(m_indexPath);
    controller.setVaultPath(m_vaultPath);
    QVERIFY(controller.createNote(QStringLiteral("External update")));
    QVERIFY(controller.saveCurrent(QStringLiteral("External update"), QStringLiteral("first")));
    const QString id = controller.selectedId();
    const QString noteFile = QDir(m_vaultPath).filePath(QStringLiteral("External-update.md"));
    QFile file(noteFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray contents = file.readAll();
    file.close();
    contents.replace("first", "second");
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(contents), qint64(contents.size()));
    file.close();

    controller.openNote(id);
    QCOMPARE(controller.selectedContent(), QStringLiteral("second"));
}

void NoteControllerTest::reportsMissingNoteOpeningFailure()
{
    NoteController controller(m_indexPath);
    controller.setVaultPath(m_vaultPath);
    QVERIFY(!controller.openNote(QStringLiteral("missing-id")));
    QVERIFY(!controller.errorText().isEmpty());
    QVERIFY(!controller.openNoteByTitle(QStringLiteral("Missing title")));
    QVERIFY(controller.statusText().contains(QStringLiteral("Missing title")));
}

void NoteControllerTest::tagEditDoesNotCreateFalseExternalConflict()
{
    NoteController controller(m_indexPath);
    controller.setVaultPath(m_vaultPath);
    QVERIFY(controller.createNote(QStringLiteral("Tagged draft")));
    QVERIFY(controller.saveCurrent(QStringLiteral("Tagged draft"), QStringLiteral("first body")));
    controller.setTags(controller.selectedId(), {QStringLiteral("work")});
    QVERIFY(controller.errorText().isEmpty());
    QVERIFY(controller.saveCurrent(QStringLiteral("Tagged draft"), QStringLiteral("second body")));
    QCOMPARE(controller.statusText(), QStringLiteral("Gespeichert"));
    QCOMPARE(controller.selectedTags(), QStringList({QStringLiteral("work")}));
}

void NoteControllerTest::persistsBoundedEditorAndThemePreferences()
{
    {
        NoteController controller(m_indexPath);
        controller.setEditorFontSize(100);
        controller.setDarkTheme(true);
        QCOMPARE(controller.editorFontSize(), 32);
        QVERIFY(controller.darkTheme());
    }
    NoteController reopened(m_indexPath);
    QCOMPARE(reopened.editorFontSize(), 32);
    QVERIFY(reopened.darkTheme());
    reopened.setEditorFontSize(1);
    QCOMPARE(reopened.editorFontSize(), 12);
}

QTEST_GUILESS_MAIN(NoteControllerTest)

#include "NoteControllerTest.moc"
