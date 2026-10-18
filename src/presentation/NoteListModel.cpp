#include "NoteListModel.h"

#include <QRegularExpression>
#include <QVariant>

#include <algorithm>
#include <utility>

namespace CrossNote {

NoteListModel::NoteListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int NoteListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleRows.size();
}

QVariant NoteListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleRows.size())
        return {};

    const Note &note = m_notes.at(m_visibleRows.at(index.row()));
    switch (role) {
    case IdRole: return note.id;
    case TitleRole: return note.title;
    case PreviewRole: return previewFor(note);
    case UpdatedAtRole: return note.updatedAt;
    case FavoriteRole: return note.favorite;
    case TagsRole: return note.tags;
    default: return {};
    }
}

QHash<int, QByteArray> NoteListModel::roleNames() const
{
    return {
        {IdRole, "noteId"},
        {TitleRole, "title"},
        {PreviewRole, "preview"},
        {UpdatedAtRole, "updatedAt"},
        {FavoriteRole, "favorite"},
        {TagsRole, "tags"}
    };
}

int NoteListModel::count() const { return m_visibleRows.size(); }
QString NoteListModel::query() const { return m_query; }
QString NoteListModel::section() const { return m_section; }

void NoteListModel::setNotes(QVector<Note> notes)
{
    std::sort(notes.begin(), notes.end(), [](const Note &left, const Note &right) {
        return left.updatedAt > right.updatedAt;
    });

    beginResetModel();
    m_notes = std::move(notes);
    m_visibleRows.clear();
    for (qsizetype i = 0; i < m_notes.size(); ++i) {
        const Note &note = m_notes.at(i);
        const bool inSection = m_section == QStringLiteral("favorites")
            ? note.favorite
            : m_section == QStringLiteral("recent") ? note.lastOpenedAt.isValid() : true;
        const QString searchable = note.title + u' ' + note.content + u' '
            + note.tags.join(u' ');
        const bool matchesQuery = m_searchResultsActive
            ? m_searchResultIds.contains(note.id)
            : m_query.isEmpty() || searchable.contains(m_query, Qt::CaseInsensitive);
        if (inSection && matchesQuery)
            m_visibleRows.append(static_cast<int>(i));
    }
    endResetModel();
    emit countChanged();
}

void NoteListModel::setSearchResultIds(const QStringList &ids, bool active)
{
    m_searchResultIds = QSet<QString>(ids.begin(), ids.end());
    if (m_searchResultsActive == active && !active)
        return;
    m_searchResultsActive = active;
    setNotes(m_notes);
}

void NoteListModel::setQuery(const QString &query)
{
    const QString normalized = query.trimmed();
    if (m_query == normalized)
        return;
    m_query = normalized;
    emit queryChanged();
    setNotes(m_notes);
}

void NoteListModel::setSection(const QString &section)
{
    const QString normalized = section.trimmed().toLower();
    if (normalized != QStringLiteral("notes")
        && normalized != QStringLiteral("favorites")
        && normalized != QStringLiteral("recent") && normalized != QStringLiteral("trash")) {
        return;
    }
    if (m_section == normalized)
        return;
    m_section = normalized;
    emit sectionChanged();
    setNotes(m_notes);
}

QString NoteListModel::previewFor(const Note &note) const
{
    QString text = note.content;
    if (text.startsWith(QStringLiteral("---"))) {
        const qsizetype end = text.indexOf(QStringLiteral("\n---"), 3);
        if (end >= 0)
            text.remove(0, end + 4);
    }
    text.remove(QRegularExpression(QStringLiteral("!?\\[[^\\]]*\\]\\([^)]*\\)")));
    text.remove(QRegularExpression(QStringLiteral("[`*_>#~]")));
    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return text.trimmed().left(140);
}

} // namespace CrossNote
