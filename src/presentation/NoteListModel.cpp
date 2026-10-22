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
    if (!index.isValid() || index.model() != this || index.column() != 0
        || index.row() < 0 || index.row() >= m_visibleRows.size())
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
QString NoteListModel::tagFilter() const { return m_tagFilter; }

void NoteListModel::setNotes(QVector<Note> notes)
{
    std::sort(notes.begin(), notes.end(), [this](const Note &left, const Note &right) {
        if (m_section == QStringLiteral("recent") && left.lastOpenedAt != right.lastOpenedAt)
            return left.lastOpenedAt > right.lastOpenedAt;
        if (left.updatedAt != right.updatedAt)
            return left.updatedAt > right.updatedAt;
        return left.id < right.id;
    });

    beginResetModel();
    m_notes = std::move(notes);
    m_visibleRows.clear();
    const QDateTime editedAfter = QDateTime::currentDateTimeUtc().addDays(-7);
    for (qsizetype i = 0; i < m_notes.size(); ++i) {
        const Note &note = m_notes.at(i);
        const bool inSection = m_section == QStringLiteral("archive")
            ? note.archived
            : !note.archived && (m_section == QStringLiteral("favorites")
                ? note.favorite
                : m_section == QStringLiteral("recent") ? note.lastOpenedAt.isValid()
                : m_section == QStringLiteral("edited")
                    ? note.updatedAt >= editedAfter
                    : true);
        const QString searchable = note.title + u' ' + note.content + u' '
            + note.tags.join(u' ');
        const bool matchesQuery = m_searchResultsActive
            ? m_searchResultIds.contains(note.id)
            : m_query.isEmpty() || searchable.contains(m_query, Qt::CaseInsensitive);
        const bool matchesTag = m_tagFilter.isEmpty()
            || note.tags.contains(m_tagFilter, Qt::CaseInsensitive);
        if (inSection && matchesQuery && matchesTag)
            m_visibleRows.append(static_cast<int>(i));
    }
    endResetModel();
    emit countChanged();
}

void NoteListModel::setSearchResultIds(const QStringList &ids, bool active)
{
    const QSet<QString> normalized(ids.begin(), ids.end());
    if (m_searchResultsActive == active && m_searchResultIds == normalized)
        return;
    m_searchResultIds = normalized;
    m_searchResultsActive = active;
    setNotes(m_notes);
}

void NoteListModel::setTagFilter(const QString &tag)
{
    const QString normalized = tag.trimmed();
    if (m_tagFilter == normalized)
        return;
    m_tagFilter = normalized;
    emit tagFilterChanged();
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
        && normalized != QStringLiteral("recent") && normalized != QStringLiteral("edited")
        && normalized != QStringLiteral("trash") && normalized != QStringLiteral("archive")) {
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
    static const QRegularExpression frontMatterEnd(QStringLiteral("\\n---"));
    static const QRegularExpression markdownLinks(QStringLiteral("!?\\[[^\\]]*\\]\\([^)]*\\)"));
    static const QRegularExpression markdownDecorations(QStringLiteral("[`*_>#~]"));
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    QString text = note.content;
    if (text.startsWith(QStringLiteral("---"))) {
        const qsizetype end = text.indexOf(frontMatterEnd, 3);
        if (end >= 0)
            text.remove(0, end + 4);
    }
    text.remove(markdownLinks);
    text.remove(markdownDecorations);
    text.replace(whitespace, QStringLiteral(" "));
    return text.trimmed().left(140);
}

} // namespace CrossNote
