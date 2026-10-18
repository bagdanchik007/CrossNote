#pragma once

#include "domain/Note.h"

#include <QAbstractListModel>
#include <QSet>
#include <QVector>

namespace CrossNote {

class NoteListModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QString section READ section WRITE setSection NOTIFY sectionChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        PreviewRole,
        UpdatedAtRole,
        FavoriteRole,
        TagsRole
    };
    Q_ENUM(Role)

    explicit NoteListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int count() const;
    [[nodiscard]] QString query() const;
    [[nodiscard]] QString section() const;

    void setNotes(QVector<Note> notes);
    void setSearchResultIds(const QStringList &ids, bool active);
    void setQuery(const QString &query);
    void setSection(const QString &section);

signals:
    void countChanged();
    void queryChanged();
    void sectionChanged();

private:
    void rebuildVisibleRows();
    [[nodiscard]] QString previewFor(const Note &note) const;

    QVector<Note> m_notes;
    QVector<int> m_visibleRows;
    QString m_query;
    QString m_section = QStringLiteral("notes");
    QSet<QString> m_searchResultIds;
    bool m_searchResultsActive = false;
};

} // namespace CrossNote
