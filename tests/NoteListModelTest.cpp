#include "presentation/NoteListModel.h"

#include <QDateTime>
#include <QSignalSpy>
#include <QtTest>

using namespace CrossNote;

class NoteListModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsInvalidIndexesAndParents();
    void sortsNotesByModificationTimeWithStableTieBreaks();
    void filtersFavoritesAndTagsTogether();
    void filtersRecentlyOpenedAndEditedNotes();
    void archiveSectionExcludesArchivedNotesFromActiveSections();
    void appliesFullTextIdsAlongsideLocalFilters();
    void ignoresUnknownSectionsAndNormalizesKnownSections();
    void createsReadableMarkdownPreviews();
    void skipsResetForIdenticalSearchIds();
    void sortsRecentNotesByOpenTime();
    void matchesLocalQueryAcrossTitleContentAndTags();
    void emitsCountSignalOnlyForVisibleCountChanges();

private:
    static Note note(QString id, QString title, QDateTime updatedAt);
};

Note NoteListModelTest::note(QString id, QString title, QDateTime updatedAt)
{
    Note result;
    result.id = std::move(id);
    result.title = std::move(title);
    result.updatedAt = updatedAt;
    return result;
}

void NoteListModelTest::rejectsInvalidIndexesAndParents()
{
    NoteListModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
    QVERIFY(!model.data(QModelIndex(), NoteListModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(0, 0), NoteListModel::TitleRole).isValid());

    Note item = note(QStringLiteral("id"), QStringLiteral("Title"), QDateTime::currentDateTimeUtc());
    model.setNotes({item});
    NoteListModel foreign;
    foreign.setNotes({item});
    QVERIFY(!model.data(foreign.index(0, 0), NoteListModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(0, 1), NoteListModel::TitleRole).isValid());
}

void NoteListModelTest::sortsNotesByModificationTimeWithStableTieBreaks()
{
    NoteListModel model;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    model.setNotes({note(QStringLiteral("z"), QStringLiteral("Zeta"), now),
                    note(QStringLiteral("a"), QStringLiteral("Alpha"), now),
                    note(QStringLiteral("old"), QStringLiteral("Older"), now.addDays(-1))});

    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("a"));
    QCOMPARE(model.data(model.index(1, 0), NoteListModel::IdRole).toString(), QStringLiteral("z"));
    QCOMPARE(model.data(model.index(2, 0), NoteListModel::IdRole).toString(), QStringLiteral("old"));
}

void NoteListModelTest::filtersFavoritesAndTagsTogether()
{
    NoteListModel model;
    Note favorite = note(QStringLiteral("fav"), QStringLiteral("Favorite"), QDateTime::currentDateTimeUtc());
    favorite.favorite = true;
    favorite.tags = {QStringLiteral("work")};
    Note other = note(QStringLiteral("other"), QStringLiteral("Other"), QDateTime::currentDateTimeUtc());
    other.favorite = true;
    other.tags = {QStringLiteral("home")};
    model.setNotes({favorite, other});
    model.setSection(QStringLiteral("favorites"));
    model.setTagFilter(QStringLiteral("work"));

    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("fav"));
}

void NoteListModelTest::filtersRecentlyOpenedAndEditedNotes()
{
    NoteListModel model;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    Note recent = note(QStringLiteral("recent"), QStringLiteral("Recent"), now);
    recent.lastOpenedAt = now;
    Note openedLongAgo = note(QStringLiteral("opened"), QStringLiteral("Opened"), now.addDays(-30));
    openedLongAgo.lastOpenedAt = now.addDays(-1);
    Note oldEdit = note(QStringLiteral("old"), QStringLiteral("Old"), now.addDays(-8));
    model.setNotes({oldEdit, openedLongAgo, recent});

    model.setSection(QStringLiteral("recent"));
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("recent"));
    model.setSection(QStringLiteral("edited"));
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("recent"));
}

void NoteListModelTest::archiveSectionExcludesArchivedNotesFromActiveSections()
{
    NoteListModel model;
    Note active = note(QStringLiteral("active"), QStringLiteral("Active"), QDateTime::currentDateTimeUtc());
    Note archived = note(QStringLiteral("archived"), QStringLiteral("Archived"), QDateTime::currentDateTimeUtc());
    archived.archived = true;
    model.setNotes({active, archived});

    QCOMPARE(model.count(), 1);
    model.setSection(QStringLiteral("archive"));
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("archived"));
    model.setSection(QStringLiteral("favorites"));
    QCOMPARE(model.count(), 0);
}

void NoteListModelTest::appliesFullTextIdsAlongsideLocalFilters()
{
    NoteListModel model;
    Note matching = note(QStringLiteral("match"), QStringLiteral("Matches"), QDateTime::currentDateTimeUtc());
    matching.tags = {QStringLiteral("work")};
    Note filtered = note(QStringLiteral("filtered"), QStringLiteral("Filtered"), QDateTime::currentDateTimeUtc());
    filtered.tags = {QStringLiteral("home")};
    model.setNotes({matching, filtered});
    model.setSearchResultIds({QStringLiteral("match"), QStringLiteral("filtered")}, true);
    model.setTagFilter(QStringLiteral("work"));

    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("match"));
}

void NoteListModelTest::ignoresUnknownSectionsAndNormalizesKnownSections()
{
    NoteListModel model;
    Note item = note(QStringLiteral("one"), QStringLiteral("One"), QDateTime::currentDateTimeUtc());
    model.setNotes({item});
    model.setSection(QStringLiteral("NOTES"));
    QCOMPARE(model.section(), QStringLiteral("notes"));
    model.setSection(QStringLiteral("not-a-section"));
    QCOMPARE(model.section(), QStringLiteral("notes"));
    QCOMPARE(model.count(), 1);
}

void NoteListModelTest::createsReadableMarkdownPreviews()
{
    NoteListModel model;
    Note item = note(QStringLiteral("one"), QStringLiteral("One"), QDateTime::currentDateTimeUtc());
    item.content = QStringLiteral("---\nid: id\ntags: []\n---\n# Heading\n\n**Useful** [link](https://example.test) ![image](image.png)");
    model.setNotes({item});

    const QString preview = model.data(model.index(0, 0), NoteListModel::PreviewRole).toString();
    QVERIFY(preview.contains(QStringLiteral("Heading")));
    QVERIFY(preview.contains(QStringLiteral("Useful")));
    QVERIFY(!preview.contains(QStringLiteral("https://example.test")));
    QVERIFY(!preview.contains(QStringLiteral("image.png")));
}

void NoteListModelTest::skipsResetForIdenticalSearchIds()
{
    NoteListModel model;
    model.setNotes({note(QStringLiteral("one"), QStringLiteral("One"), QDateTime::currentDateTimeUtc())});
    QSignalSpy countSpy(&model, &NoteListModel::countChanged);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    model.setSearchResultIds({QStringLiteral("one")}, true);
    QCOMPARE(resetSpy.count(), 1);
    model.setSearchResultIds({QStringLiteral("one")}, true);
    QCOMPARE(resetSpy.count(), 1);
    model.setSearchResultIds({}, false);
    QCOMPARE(resetSpy.count(), 2);
    QCOMPARE(countSpy.count(), 0);
}

void NoteListModelTest::sortsRecentNotesByOpenTime()
{
    NoteListModel model;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    Note older = note(QStringLiteral("older"), QStringLiteral("Older"), now);
    older.lastOpenedAt = now.addSecs(-30);
    Note newer = note(QStringLiteral("newer"), QStringLiteral("Newer"), now.addDays(-1));
    newer.lastOpenedAt = now;
    model.setNotes({older, newer});
    model.setSection(QStringLiteral("recent"));
    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("newer"));
    QCOMPARE(model.data(model.index(1, 0), NoteListModel::IdRole).toString(), QStringLiteral("older"));
}

void NoteListModelTest::matchesLocalQueryAcrossTitleContentAndTags()
{
    NoteListModel model;
    Note titleMatch = note(QStringLiteral("title"), QStringLiteral("Planning"), QDateTime::currentDateTimeUtc());
    Note bodyMatch = note(QStringLiteral("body"), QStringLiteral("Other"), QDateTime::currentDateTimeUtc());
    bodyMatch.content = QStringLiteral("Build the roadmap");
    Note tagMatch = note(QStringLiteral("tag"), QStringLiteral("Third"), QDateTime::currentDateTimeUtc());
    tagMatch.tags = {QStringLiteral("Roadmap")};
    model.setNotes({titleMatch, bodyMatch, tagMatch});

    model.setQuery(QStringLiteral("roadmap"));
    QCOMPARE(model.count(), 2);
    model.setQuery(QStringLiteral("planning"));
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), NoteListModel::IdRole).toString(), QStringLiteral("title"));
}

void NoteListModelTest::emitsCountSignalOnlyForVisibleCountChanges()
{
    NoteListModel model;
    Note first = note(QStringLiteral("one"), QStringLiteral("One"), QDateTime::currentDateTimeUtc());
    model.setNotes({first});
    QSignalSpy countSpy(&model, &NoteListModel::countChanged);
    first.title = QStringLiteral("Updated title");
    model.setNotes({first});
    QCOMPARE(countSpy.count(), 0);
    model.setNotes({});
    QCOMPARE(countSpy.count(), 1);
}

QTEST_GUILESS_MAIN(NoteListModelTest)

#include "NoteListModelTest.moc"
