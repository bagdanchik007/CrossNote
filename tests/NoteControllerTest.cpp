#include "application/NoteController.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

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

private:
    QTemporaryDir m_sandbox;
    QString m_vaultPath;
};

void NoteControllerTest::initTestCase()
{
    QVERIFY(m_sandbox.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("CrossNoteTests"));
    QCoreApplication::setApplicationName(QStringLiteral("NoteControllerTest"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_sandbox.path());
}

void NoteControllerTest::init()
{
    QSettings().clear();
    m_vaultPath = m_sandbox.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QVERIFY(QDir().mkpath(m_vaultPath));
}

void NoteControllerTest::createsAndSelectsNotes()
{
    NoteController controller;
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
    NoteController controller;
    controller.setVaultPath(m_vaultPath);
    QVERIFY(controller.createNote(QStringLiteral("Draft")));
    QVERIFY(controller.saveCurrent(QStringLiteral("Final title"), QStringLiteral("needle phrase")));
    QCOMPARE(controller.selectedTitle(), QStringLiteral("Final title"));
    controller.setSearchQuery(QStringLiteral("needle"));
    QCOMPARE(controller.noteModel()->count(), 1);
    controller.setSearchQuery(QStringLiteral("absent"));
    QCOMPARE(controller.noteModel()->count(), 0);
    QVERIFY(controller.errorText().isEmpty());
}

void NoteControllerTest::failedVaultSwitchKeepsCurrentNote()
{
    NoteController controller;
    controller.setVaultPath(m_vaultPath);
    QVERIFY(controller.createNote(QStringLiteral("Still here")));
    const QString selectedId = controller.selectedId();
    const QString invalidPath = m_sandbox.filePath(QStringLiteral("regular-file"));
    QFile file(invalidPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    controller.setVaultPath(invalidPath);
    QVERIFY(!controller.errorText().isEmpty());
    QCOMPARE(controller.vaultPath(), QFileInfo(m_vaultPath).canonicalFilePath());
    QCOMPARE(controller.selectedId(), selectedId);
    QCOMPARE(controller.selectedTitle(), QStringLiteral("Still here"));
}

QTEST_GUILESS_MAIN(NoteControllerTest)

#include "NoteControllerTest.moc"
