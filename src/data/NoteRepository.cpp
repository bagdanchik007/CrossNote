#include "NoteRepository.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSet>
#include <QUuid>

#include <algorithm>

namespace CrossNote {
namespace {

constexpr int CurrentSchemaVersion = 3;

QString safeAbsolutePath(const QString &vault, const QString &relativePath)
{
    if (relativePath.isEmpty() || QDir::isAbsolutePath(relativePath))
        return {};
    const QString clean = QDir::cleanPath(relativePath);
    if (clean == QStringLiteral("..") || clean.startsWith(QStringLiteral("../")))
        return {};
    const QString root = QDir::fromNativeSeparators(QDir(vault).absolutePath());
    const QString candidate = QDir::fromNativeSeparators(QDir(root).absoluteFilePath(clean));
    const QString prefix = root.endsWith(u'/') ? root : root + u'/';
    Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#ifdef Q_OS_WIN
    sensitivity = Qt::CaseInsensitive;
#endif
    if (!candidate.startsWith(prefix, sensitivity))
        return {};
    const QString canonical = QDir::fromNativeSeparators(QFileInfo(candidate).canonicalFilePath());
    if (!canonical.isEmpty() && canonical != root && !canonical.startsWith(prefix, sensitivity))
        return {};
    return candidate;
}

QString sqlError(const QSqlQuery &query)
{
    return query.lastError().text();
}

QString normalizedTags(const QStringList &tags)
{
    QStringList result;
    for (const QString &tag : tags) {
        const QString cleaned = tag.simplified();
        if (!cleaned.isEmpty() && !result.contains(cleaned, Qt::CaseInsensitive))
            result.append(cleaned);
    }
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(result)).toJson(QJsonDocument::Compact));
}

QStringList parseTags(const QString &json)
{
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    QStringList tags;
    for (const QJsonValue &value : document.array()) {
        if (value.isString())
            tags.append(value.toString());
    }
    return tags;
}

} // namespace

class NoteRepository::Impl
{
public:
    QString vault;
    QString indexRootOverride;
    QString connectionName;
    QSqlDatabase database;

    ~Impl()
    {
        if (database.isValid()) {
            const QString name = connectionName;
            database.close();
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
        }
    }

    bool open(const QString &path, QString *error)
    {
        const QString cleaned = QDir::cleanPath(path.trimmed());
        if (cleaned.isEmpty()) {
            if (error) *error = QStringLiteral("Bitte wähle einen Notizordner aus.");
            return false;
        }
        if (!QDir().mkpath(cleaned)) {
            if (error) *error = QStringLiteral("Der Notizordner kann nicht angelegt werden.");
            return false;
        }
        const QFileInfo vaultInfo(cleaned);
        if (!vaultInfo.isDir() || !vaultInfo.isReadable() || !vaultInfo.isWritable()) {
            if (error) *error = QStringLiteral("Der Notizordner ist nicht lesbar oder beschreibbar.");
            return false;
        }

        vault = vaultInfo.canonicalFilePath();
        if (vault.isEmpty())
            vault = vaultInfo.absoluteFilePath();
        if (!QDir(vault).mkpath(QStringLiteral(".trash"))) {
            if (error) *error = QStringLiteral("Der Papierkorb im Notizordner kann nicht angelegt werden.");
            return false;
        }

        const QByteArray hash = QCryptographicHash::hash(vault.toUtf8(), QCryptographicHash::Sha256).toHex();
        const QString indexRoot = indexRootOverride.isEmpty()
            ? QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                  .filePath(QStringLiteral("vault-index"))
            : indexRootOverride;
        if (!QDir().mkpath(indexRoot)) {
            if (error) *error = QStringLiteral("Der lokale Suchindex kann nicht angelegt werden.");
            return false;
        }

        if (database.isValid()) {
            database.close();
            const QString oldName = connectionName;
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(oldName);
        }
        connectionName = QStringLiteral("crossnote-%1")
                             .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(QDir(indexRoot).filePath(QString::fromLatin1(hash) + QStringLiteral(".sqlite")));
        if (!database.open()) {
            if (error) *error = database.lastError().text();
            return false;
        }

        QSqlQuery pragma(database);
        if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
            if (error) *error = sqlError(pragma);
            return false;
        }
        if (!pragma.exec(QStringLiteral("PRAGMA busy_timeout = 5000"))) {
            if (error) *error = sqlError(pragma);
            return false;
        }
        if (!pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL")) || !pragma.next()
            || pragma.value(0).toString().compare(QStringLiteral("wal"), Qt::CaseInsensitive) != 0) {
            if (error) *error = pragma.lastError().text().isEmpty()
                ? QStringLiteral("Der lokale Datenbankmodus WAL konnte nicht aktiviert werden.")
                : sqlError(pragma);
            return false;
        }
        if (!createSchema(error))
            return false;
        return true;
    }

    bool createSchema(QString *error)
    {
        const QStringList statements = {
            QStringLiteral("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER NOT NULL)"),
            QStringLiteral("CREATE TABLE IF NOT EXISTS notes ("
                           "id TEXT PRIMARY KEY, title TEXT NOT NULL, relative_path TEXT NOT NULL UNIQUE, "
                           "created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL, "
                           "last_opened_at INTEGER, favorite INTEGER NOT NULL DEFAULT 0, tags TEXT NOT NULL DEFAULT '[]')"),
            QStringLiteral("CREATE TABLE IF NOT EXISTS trashed_notes (id TEXT PRIMARY KEY, title TEXT NOT NULL, relative_path TEXT NOT NULL, trash_path TEXT NOT NULL, created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL, last_opened_at INTEGER, favorite INTEGER NOT NULL DEFAULT 0, tags TEXT NOT NULL DEFAULT '[]', deleted_at INTEGER NOT NULL)")
        };
        for (const QString &statement : statements) {
            QSqlQuery query(database);
            if (!query.exec(statement)) {
                if (error) *error = sqlError(query);
                return false;
            }
        }

        QSqlQuery currentVersion(database);
        if (!currentVersion.exec(QStringLiteral("SELECT MAX(version) FROM schema_version"))) {
            if (error) *error = sqlError(currentVersion);
            return false;
        }
        if (!currentVersion.next()) {
            if (error) *error = QStringLiteral("Das Datenbankschema konnte nicht gelesen werden.");
            return false;
        }
        const bool hasVersion = !currentVersion.value(0).isNull();
        int version = currentVersion.value(0).toInt();
        currentVersion.finish();
        if (version > CurrentSchemaVersion) {
            if (error) *error = QStringLiteral("Die lokale Datenbank stammt aus einer neueren CrossNote-Version.");
            return false;
        }
        if (!hasVersion) {
            QSqlQuery initializeVersion(database);
            if (!initializeVersion.exec(QStringLiteral("INSERT INTO schema_version (version) VALUES (1)"))) {
                if (error) *error = sqlError(initializeVersion);
                return false;
            }
            initializeVersion.finish();
            version = 1;
        }

        if (version < 2) {
            QSqlQuery createSearchIndex(database);
            if (!createSearchIndex.exec(QStringLiteral(
                    "CREATE VIRTUAL TABLE IF NOT EXISTS note_search USING fts5("
                    "note_id UNINDEXED, title, content, tags, tokenize='unicode61 remove_diacritics 2')"))) {
                if (error) *error = sqlError(createSearchIndex);
                return false;
            }
            createSearchIndex.finish();

            QSqlQuery sourceNotes(database);
            if (!sourceNotes.exec(QStringLiteral("SELECT id, title, relative_path, tags FROM notes"))) {
                if (error) *error = sqlError(sourceNotes);
                return false;
            }
            while (sourceNotes.next()) {
                const QString id = sourceNotes.value(0).toString();
                const QString title = sourceNotes.value(1).toString();
                const QString path = safeAbsolutePath(vault, sourceNotes.value(2).toString());
                QString content;
                QFile noteFile(path);
                if (!path.isEmpty() && noteFile.open(QIODevice::ReadOnly))
                    content = QString::fromUtf8(noteFile.readAll());

                QString indexError;
                if (!replaceSearchDocument(id, title, content, sourceNotes.value(3).toString(), &indexError)) {
                    if (error) *error = indexError;
                    return false;
                }
            }
            sourceNotes.finish();

            QSqlQuery updateVersion(database);
            if (!updateVersion.exec(QStringLiteral("INSERT INTO schema_version (version) VALUES (2)"))) {
                const QString message = updateVersion.lastError().text().isEmpty()
                    ? database.lastError().text() : sqlError(updateVersion);
                if (error) *error = message;
                return false;
            }
            updateVersion.finish();
            version = 2;
        }

        if (version < 3) {
            QSqlQuery addArchivedColumn(database);
            if (!addArchivedColumn.exec(QStringLiteral(
                    "ALTER TABLE notes ADD COLUMN archived INTEGER NOT NULL DEFAULT 0"))) {
                if (error) *error = sqlError(addArchivedColumn);
                return false;
            }
            addArchivedColumn.finish();
            QSqlQuery updateVersion(database);
            if (!updateVersion.exec(QStringLiteral("INSERT INTO schema_version (version) VALUES (3)"))) {
                if (error) *error = sqlError(updateVersion);
                return false;
            }
            updateVersion.finish();
        }
        return true;
    }

    bool replaceSearchDocument(const QString &id, const QString &title, const QString &content,
                               const QString &tags, QString *error)
    {
        QSqlQuery remove(database);
        remove.prepare(QStringLiteral("DELETE FROM note_search WHERE note_id = ?"));
        remove.addBindValue(id);
        if (!remove.exec()) {
            if (error) *error = sqlError(remove);
            return false;
        }
        remove.finish();

        QSqlQuery insert(database);
        insert.prepare(QStringLiteral(
            "INSERT INTO note_search (note_id, title, content, tags) VALUES (?, ?, ?, ?)"));
        insert.addBindValue(id);
        insert.addBindValue(title);
        insert.addBindValue(content);
        insert.addBindValue(tags);
        if (!insert.exec()) {
            if (error) *error = sqlError(insert);
            return false;
        }
        insert.finish();
        return true;
    }

    bool removeSearchDocument(const QString &id, QString *error)
    {
        QSqlQuery remove(database);
        remove.prepare(QStringLiteral("DELETE FROM note_search WHERE note_id = ?"));
        remove.addBindValue(id);
        if (!remove.exec()) {
            if (error) *error = sqlError(remove);
            return false;
        }
        return true;
    }

    QVector<Note> list(QString *error) const
    {
        QVector<Note> result;
        QSqlQuery query(database);
        query.prepare(QStringLiteral("SELECT id, title, relative_path, created_at, updated_at, "
                                     "last_opened_at, favorite, tags, archived FROM notes ORDER BY updated_at DESC"));
        if (!query.exec()) {
            if (error) *error = sqlError(query);
            return result;
        }
        while (query.next()) {
            Note item;
            item.id = query.value(0).toString();
            item.title = query.value(1).toString();
            item.relativePath = query.value(2).toString();
            item.createdAt = QDateTime::fromMSecsSinceEpoch(query.value(3).toLongLong(), Qt::UTC);
            item.updatedAt = QDateTime::fromMSecsSinceEpoch(query.value(4).toLongLong(), Qt::UTC);
            if (!query.value(5).isNull())
                item.lastOpenedAt = QDateTime::fromMSecsSinceEpoch(query.value(5).toLongLong(), Qt::UTC);
            item.favorite = query.value(6).toBool();
            item.tags = parseTags(query.value(7).toString());
            item.archived = query.value(8).toBool();
            const QString absolute = safeAbsolutePath(vault, item.relativePath);
            if (absolute.isEmpty())
                continue;
            QFile file(absolute);
            if (file.open(QIODevice::ReadOnly))
                item.content = QString::fromUtf8(file.readAll());
            result.append(std::move(item));
        }
        return result;
    }
};

NoteRepository::NoteRepository(QObject *parent)
    : NoteRepository(QString(), parent)
{
}

NoteRepository::NoteRepository(const QString &indexRoot, QObject *parent)
    : QObject(parent), m_impl(new Impl), m_watcher(this), m_refreshTimer(this)
{
    m_impl->indexRootOverride = indexRoot;
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(250);
    const auto scheduleRefresh = [this](const QString &) { m_refreshTimer.start(); };
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, scheduleRefresh);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, scheduleRefresh);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this] {
        QString error;
        if (!refresh(&error) && !error.isEmpty())
            emit errorOccurred(error);
    });
}

NoteRepository::~NoteRepository() { delete m_impl; }

bool NoteRepository::openVault(const QString &vaultPath, QString *error)
{
    if (!m_impl->open(vaultPath, error))
        return false;
    emit vaultChanged(m_impl->vault);
    return refresh(error);
}

QString NoteRepository::vaultPath() const { return m_impl->vault; }
QVector<Note> NoteRepository::notes(QString *error) const { return m_impl->list(error); }

bool NoteRepository::searchIds(const QString &query, QStringList *ids, QString *error) const
{
    if (!ids) {
        if (error) *error = QStringLiteral("Für Suchergebnisse wurde kein Ziel übergeben.");
        return false;
    }
    ids->clear();
    if (query.trimmed().isEmpty())
        return true;

    QStringList terms;
    const QRegularExpression tokenPattern(QStringLiteral("[\\p{L}\\p{N}_]+"));
    auto matches = tokenPattern.globalMatch(query);
    while (matches.hasNext()) {
        const QString token = matches.next().captured();
        terms.append(QStringLiteral("\"%1\"*").arg(token));
    }
    if (terms.isEmpty())
        return true;

    QSqlQuery search(m_impl->database);
    search.prepare(QStringLiteral("SELECT note_id FROM note_search WHERE note_search MATCH ? ORDER BY rank"));
    search.addBindValue(terms.join(QStringLiteral(" AND ")));
    if (!search.exec()) {
        if (error) *error = sqlError(search);
        return false;
    }
    while (search.next())
        ids->append(search.value(0).toString());
    return true;
}

QVector<Note> NoteRepository::trashedNotes(QString *error) const
{
    QVector<Note> result;
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("SELECT id, title, trash_path, created_at, updated_at, last_opened_at, favorite, tags FROM trashed_notes ORDER BY deleted_at DESC"));
    if (!query.exec()) {
        if (error) *error = sqlError(query);
        return result;
    }
    while (query.next()) {
        Note item;
        item.id = query.value(0).toString();
        item.title = query.value(1).toString();
        item.relativePath = query.value(2).toString();
        item.createdAt = QDateTime::fromMSecsSinceEpoch(query.value(3).toLongLong(), Qt::UTC);
        item.updatedAt = QDateTime::fromMSecsSinceEpoch(query.value(4).toLongLong(), Qt::UTC);
        if (!query.value(5).isNull())
            item.lastOpenedAt = QDateTime::fromMSecsSinceEpoch(query.value(5).toLongLong(), Qt::UTC);
        item.favorite = query.value(6).toBool();
        item.tags = parseTags(query.value(7).toString());
        const QString absolute = safeAbsolutePath(m_impl->vault, item.relativePath);
        QFile file(absolute);
        if (absolute.isEmpty() || !file.open(QIODevice::ReadOnly))
            continue;
        const QByteArray bytes = file.readAll();
        item.content = QString::fromUtf8(bytes);
        item.contentHash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
        result.append(std::move(item));
    }
    return result;
}
namespace {

Note parseMarkdownFile(const QString &text, const QString &fallbackId,
                       const QString &fallbackTitle, const QString &relativePath)
{
    Note result;
    result.id = fallbackId;
    result.title = fallbackTitle;
    result.relativePath = relativePath;

    QString body = text;
    if (body.startsWith(QStringLiteral("---"))) {
        const QRegularExpression divider(QStringLiteral("(?:^|\\n)---[ \\t]*(?:\\r?\\n|$)"));
        const auto match = divider.match(body, 3);
        if (match.hasMatch()) {
            result.frontMatter = body.left(match.capturedEnd());
            const QString metadata = body.mid(3, match.capturedStart() - 3);
            const auto idMatch = QRegularExpression(QStringLiteral("(?m)^id:\\s*([^\\r\\n]+)"))
                                     .match(metadata);
            if (idMatch.hasMatch()) {
                const QUuid parsedId(idMatch.captured(1).trimmed());
                if (!parsedId.isNull())
                    result.id = parsedId.toString(QUuid::WithoutBraces);
            }
            const auto tagsMatch = QRegularExpression(QStringLiteral("(?m)^tags:\\s*(\\[[^\\r\\n]*\\])"))
                                       .match(metadata);
            if (tagsMatch.hasMatch()) {
                const QJsonDocument tagsDoc = QJsonDocument::fromJson(tagsMatch.captured(1).toUtf8());
                for (const QJsonValue &tag : tagsDoc.array()) {
                    if (tag.isString())
                        result.tags.append(tag.toString());
                }
            }
            body = body.mid(match.capturedEnd());
        }
    }

    const auto heading = QRegularExpression(QStringLiteral("(?m)^#\\s+(.+?)\\s*$")).match(body);
    if (heading.hasMatch()) {
        result.title = heading.captured(1).trimmed();
        body.remove(heading.capturedStart(), heading.capturedLength());
    }
    result.content = body.trimmed();
    return result;
}

} // namespace

bool NoteRepository::rebuildSearchIndex(QString *error)
{
    if (!m_impl->database.transaction()) {
        if (error) *error = m_impl->database.lastError().text();
        return false;
    }

    QSqlQuery clear(m_impl->database);
    if (!clear.exec(QStringLiteral("DELETE FROM note_search"))) {
        m_impl->database.rollback();
        if (error) *error = sqlError(clear);
        return false;
    }
    QSqlQuery source(m_impl->database);
    if (!source.exec(QStringLiteral("SELECT id, title, relative_path, tags FROM notes"))) {
        m_impl->database.rollback();
        if (error) *error = sqlError(source);
        return false;
    }
    while (source.next()) {
        const QString id = source.value(0).toString();
        const QString title = source.value(1).toString();
        const QString path = safeAbsolutePath(m_impl->vault, source.value(2).toString());
        QFile file(path);
        if (path.isEmpty() || !file.open(QIODevice::ReadOnly)) {
            m_impl->database.rollback();
            if (error) *error = path.isEmpty()
                ? QStringLiteral("Ein Notizpfad liegt außerhalb des geöffneten Vaults.")
                : file.errorString();
            return false;
        }
        const Note parsed = parseMarkdownFile(QString::fromUtf8(file.readAll()), id, title,
                                              source.value(2).toString());
        QString indexError;
        if (!m_impl->replaceSearchDocument(id, title, parsed.content,
                                           source.value(3).toString(), &indexError)) {
            m_impl->database.rollback();
            if (error) *error = indexError;
            return false;
        }
    }
    if (!m_impl->database.commit()) {
        if (error) *error = m_impl->database.lastError().text();
        m_impl->database.rollback();
        return false;
    }
    emit notesChanged();
    return true;
}

std::optional<Note> NoteRepository::note(const QString &id, bool includeContent, QString *error) const
{
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("SELECT title, relative_path, created_at, updated_at, "
                                 "last_opened_at, favorite, tags, archived FROM notes WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = sqlError(query);
        return std::nullopt;
    }
    if (!query.next())
        return std::nullopt;

    Note result;
    result.id = id;
    result.title = query.value(0).toString();
    result.relativePath = query.value(1).toString();
    result.createdAt = QDateTime::fromMSecsSinceEpoch(query.value(2).toLongLong(), Qt::UTC);
    result.updatedAt = QDateTime::fromMSecsSinceEpoch(query.value(3).toLongLong(), Qt::UTC);
    if (!query.value(4).isNull())
        result.lastOpenedAt = QDateTime::fromMSecsSinceEpoch(query.value(4).toLongLong(), Qt::UTC);
    result.favorite = query.value(5).toBool();
    result.tags = parseTags(query.value(6).toString());
    result.archived = query.value(7).toBool();

    if (includeContent) {
        const QString absolute = safeAbsolutePath(m_impl->vault, result.relativePath);
        if (absolute.isEmpty()) {
            if (error) *error = QStringLiteral("Der Notizpfad liegt außerhalb des Vaults.");
            return std::nullopt;
        }
        QFile file(absolute);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error) *error = file.errorString();
            return std::nullopt;
        }
        const QByteArray fileBytes = file.readAll();
        const Note parsed = parseMarkdownFile(QString::fromUtf8(fileBytes), id,
                                              result.title, result.relativePath);
        result.content = parsed.content;
        result.frontMatter = parsed.frontMatter;
        result.contentHash = QCryptographicHash::hash(fileBytes, QCryptographicHash::Sha256);
    }
    return result;
}

std::optional<QString> NoteRepository::noteIdForTitle(const QString &title, QString *error) const
{
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("SELECT id FROM notes WHERE title = ? COLLATE NOCASE "
                                 "ORDER BY updated_at DESC, id LIMIT 1"));
    query.addBindValue(title.trimmed());
    if (!query.exec()) {
        if (error) *error = sqlError(query);
        return std::nullopt;
    }
    if (!query.next())
        return std::nullopt;
    return query.value(0).toString();
}

NoteRepository::LinkAnalysis NoteRepository::analyzeLinks(const QString &noteId, QString *error) const
{
    LinkAnalysis result;
    if (error)
        error->clear();

    QString listError;
    const QVector<Note> allNotes = m_impl->list(&listError);
    if (!listError.isEmpty()) {
        if (error) *error = listError;
        return result;
    }

    const auto target = std::find_if(allNotes.cbegin(), allNotes.cend(), [&noteId](const Note &item) {
        return item.id == noteId;
    });
    if (target == allNotes.cend()) {
        if (error) *error = QStringLiteral("Die ausgewählte Notiz wurde nicht gefunden.");
        return result;
    }

    QSet<QString> knownTitles;
    for (const Note &item : allNotes)
        knownTitles.insert(item.title.simplified().toCaseFolded());

    const QRegularExpression linkPattern(QStringLiteral("\\[\\[\\s*([^\\]]+?)\\s*\\]\\]"));
    QSet<QString> outgoingSeen;
    const QString ownBody = parseMarkdownFile(target->content, target->id, target->title,
                                               target->relativePath).content;
    auto outgoingMatches = linkPattern.globalMatch(ownBody);
    while (outgoingMatches.hasNext()) {
        const QString title = outgoingMatches.next().captured(1).simplified();
        const QString folded = title.toCaseFolded();
        if (title.isEmpty() || outgoingSeen.contains(folded))
            continue;
        outgoingSeen.insert(folded);
        result.outgoing.append(title);
        if (!knownTitles.contains(folded))
            result.missing.append(title);
    }

    QSet<QString> backlinkSeen;
    const QString targetTitle = target->title.simplified().toCaseFolded();
    for (const Note &candidate : allNotes) {
        if (candidate.id == noteId)
            continue;
        const QString body = parseMarkdownFile(candidate.content, candidate.id, candidate.title,
                                               candidate.relativePath).content;
        auto backlinkMatches = linkPattern.globalMatch(body);
        bool linksToTarget = false;
        while (backlinkMatches.hasNext()) {
            if (backlinkMatches.next().captured(1).simplified().toCaseFolded() == targetTitle) {
                linksToTarget = true;
                break;
            }
        }
        const QString foldedTitle = candidate.title.toCaseFolded();
        if (linksToTarget && !backlinkSeen.contains(foldedTitle)) {
            backlinkSeen.insert(foldedTitle);
            result.backlinks.append(candidate.title);
        }
    }

    result.outgoing.sort(Qt::CaseInsensitive);
    result.backlinks.sort(Qt::CaseInsensitive);
    result.missing.sort(Qt::CaseInsensitive);
    return result;
}

bool NoteRepository::refresh(QString *error)
{
    if (!m_impl->database.isOpen()) {
        if (error) *error = QStringLiteral("Der Notizordner ist noch nicht geöffnet.");
        return false;
    }

    if (!m_impl->database.transaction()) {
        if (error) *error = m_impl->database.lastError().text();
        return false;
    }
    const auto rollback = [this] { m_impl->database.rollback(); };

    QSet<QString> seenPaths;
    QDirIterator iterator(m_impl->vault, {QStringLiteral("*.md")}, QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString absolutePath = iterator.next();
        QString relativePath = QDir(m_impl->vault).relativeFilePath(absolutePath);
        relativePath.replace(QDir::separator(), u'/');
        if (relativePath.startsWith(QStringLiteral(".trash/"))
            || relativePath.contains(QStringLiteral("/.trash/"))
            || relativePath.startsWith(QStringLiteral(".backups/"))
            || relativePath.contains(QStringLiteral("/.backups/"))
            || relativePath.startsWith(QStringLiteral(".conflicts/"))
            || relativePath.contains(QStringLiteral("/.conflicts/")))
            continue;
        seenPaths.insert(relativePath);

        QFile file(absolutePath);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QString text = QString::fromUtf8(file.readAll());
        QFileInfo info(file);
        file.close();

        QString existingId;
        qint64 createdAt = info.birthTime().toUTC().toMSecsSinceEpoch();
        bool favorite = false;
        QStringList tags;
        QSqlQuery existing(m_impl->database);
        existing.prepare(QStringLiteral("SELECT id, created_at, favorite, tags FROM notes WHERE relative_path = ?"));
        existing.addBindValue(relativePath);
        if (existing.exec() && existing.next()) {
            existingId = existing.value(0).toString();
            createdAt = existing.value(1).toLongLong();
            favorite = existing.value(2).toBool();
            tags = parseTags(existing.value(3).toString());
        }
        existing.finish();

        const Note parsed = parseMarkdownFile(text, existingId, info.completeBaseName(), relativePath);
        const QString id = parsed.id.isEmpty()
            ? QUuid::createUuid().toString(QUuid::WithoutBraces) : parsed.id;
        const QString title = parsed.title.trimmed().isEmpty() ? info.completeBaseName() : parsed.title;
        if (createdAt <= 0)
            createdAt = info.lastModified().toUTC().toMSecsSinceEpoch();
        if (!parsed.tags.isEmpty())
            tags = parsed.tags;

        QSqlQuery upsert(m_impl->database);
        upsert.prepare(QStringLiteral(
            "INSERT INTO notes (id, title, relative_path, created_at, updated_at, favorite, tags) "
            "VALUES (?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET title=excluded.title, relative_path=excluded.relative_path, "
            "updated_at=excluded.updated_at, tags=excluded.tags"));
        upsert.addBindValue(id);
        upsert.addBindValue(title);
        upsert.addBindValue(relativePath);
        upsert.addBindValue(createdAt);
        upsert.addBindValue(info.lastModified().toUTC().toMSecsSinceEpoch());
        upsert.addBindValue(favorite);
        upsert.addBindValue(normalizedTags(tags));
        if (!upsert.exec()) {
            rollback();
            if (error) *error = sqlError(upsert);
            return false;
        }
        upsert.finish();
        QString indexError;
        if (!m_impl->replaceSearchDocument(id, title, parsed.content, normalizedTags(tags), &indexError)) {
            rollback();
            if (error) *error = indexError;
            return false;
        }
    }

    QSqlQuery allPaths(m_impl->database);
    if (!allPaths.exec(QStringLiteral("SELECT relative_path FROM notes"))) {
        rollback();
        if (error) *error = sqlError(allPaths);
        return false;
    }
    QStringList stalePaths;
    while (allPaths.next()) {
        const QString path = allPaths.value(0).toString();
        if (!seenPaths.contains(path))
            stalePaths.append(path);
    }
    allPaths.finish();
    for (const QString &path : stalePaths) {
        QSqlQuery removeFromSearch(m_impl->database);
        removeFromSearch.prepare(QStringLiteral(
            "DELETE FROM note_search WHERE note_id = (SELECT id FROM notes WHERE relative_path = ?)"));
        removeFromSearch.addBindValue(path);
        if (!removeFromSearch.exec()) {
            rollback();
            if (error) *error = sqlError(removeFromSearch);
            return false;
        }
        QSqlQuery remove(m_impl->database);
        remove.prepare(QStringLiteral("DELETE FROM notes WHERE relative_path = ?"));
        remove.addBindValue(path);
        if (!remove.exec()) {
            rollback();
            if (error) *error = sqlError(remove);
            return false;
        }
    }

    if (!m_impl->database.commit()) {
        if (error) *error = QStringLiteral("Vault-Abgleich: %1").arg(m_impl->database.lastError().text());
        rollback();
        return false;
    }
    updateWatcherPaths();
    emit notesChanged();
    return true;
}
void NoteRepository::updateWatcherPaths()
{
    QStringList previous = m_watcher.files();
    previous.append(m_watcher.directories());
    if (!previous.isEmpty())
        m_watcher.removePaths(previous);
    if (m_impl->vault.isEmpty())
        return;

    QStringList paths{m_impl->vault};
    QDirIterator directories(m_impl->vault, QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                             QDirIterator::Subdirectories);
    while (directories.hasNext()) {
        const QString directory = directories.next();
        const QString relative = QDir(m_impl->vault).relativeFilePath(directory).replace(QDir::separator(), u'/');
        if (relative == QStringLiteral(".trash") || relative.startsWith(QStringLiteral(".trash/"))
            || relative == QStringLiteral(".backups") || relative.startsWith(QStringLiteral(".backups/"))
            || relative == QStringLiteral(".conflicts") || relative.startsWith(QStringLiteral(".conflicts/")))
            continue;
        paths.append(directory);
    }

    QDirIterator files(m_impl->vault, {QStringLiteral("*.md")}, QDir::Files | QDir::NoSymLinks,
                       QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const QString file = files.next();
        const QString relative = QDir(m_impl->vault).relativeFilePath(file).replace(QDir::separator(), u'/');
        if (!relative.startsWith(QStringLiteral(".trash/"))
            && !relative.startsWith(QStringLiteral(".backups/"))
            && !relative.startsWith(QStringLiteral(".conflicts/")))
            paths.append(file);
    }
    m_watcher.addPaths(paths);
}
namespace {

QString safeFileStem(const QString &title)
{
    QString result = title.simplified();
    result.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("-"));
    result.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral("-"));
    result.remove(QRegularExpression(QStringLiteral("^[. -]+|[. ]+$")));
    if (result.isEmpty())
        result = QStringLiteral("Neue-Notiz");
    result = result.left(80);

    static const QRegularExpression reservedName(
        QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$"),
        QRegularExpression::CaseInsensitiveOption);
    if (reservedName.match(result).hasMatch())
        result.prepend(u'_');
    return result;
}

QString uniqueNotePath(const QString &vault, const QString &stem, const QSet<QString> &indexedPaths)
{
    const QDir root(vault);
    QString candidate = stem + QStringLiteral(".md");
    QSet<QString> occupiedNames;
    const QStringList files = root.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &file : files)
        occupiedNames.insert(file.toCaseFolded());
    for (const QString &path : indexedPaths)
        occupiedNames.insert(path.toCaseFolded());

    for (int suffix = 2; occupiedNames.contains(candidate.toCaseFolded()); ++suffix)
        candidate = QStringLiteral("%1-%2.md").arg(stem).arg(suffix);
    return candidate;
}

QString markdownForNote(const QString &id, const QString &title, const QString &content,
                        const QStringList &tags, const QString &existingFrontMatter = {})
{
    QStringList metadataLines;
    if (existingFrontMatter.startsWith(QStringLiteral("---"))) {
        const qsizetype end = existingFrontMatter.lastIndexOf(QStringLiteral("---"));
        if (end > 2) {
            const QString body = existingFrontMatter.mid(3, end - 3);
            const QStringList previous = body.split(u'\n');
            bool skippingTagLines = false;
            for (const QString &line : previous) {
                if (line.trimmed().startsWith(QStringLiteral("id:"), Qt::CaseInsensitive))
                    continue;
                if (line.trimmed().startsWith(QStringLiteral("tags:"), Qt::CaseInsensitive)) {
                    skippingTagLines = true;
                    continue;
                }
                if (skippingTagLines && (line.trimmed().startsWith(QStringLiteral("- ")) || line.trimmed().isEmpty()))
                    continue;
                skippingTagLines = false;
                if (!line.trimmed().isEmpty())
                    metadataLines.append(line);
            }
        }
    }
    metadataLines.append(QStringLiteral("id: %1").arg(id));
    const QByteArray tagsJson = QJsonDocument(QJsonArray::fromStringList(tags)).toJson(QJsonDocument::Compact);
    metadataLines.append(QStringLiteral("tags: %1").arg(QString::fromUtf8(tagsJson)));
    return QStringLiteral("---\n%1\n---\n\n# %2\n\n%3\n")
        .arg(metadataLines.join(u'\n'), title.trimmed(), content.trimmed());
}

bool archiveMarkdownVersion(const QString &vault, const QString &noteId,
                            const QByteArray &contents, QString *error)
{
    const QString relativeDirectory = QStringLiteral(".backups/%1").arg(noteId);
    const QString absoluteDirectory = QDir(vault).filePath(relativeDirectory);
    if (!QDir().mkpath(absoluteDirectory)) {
        if (error) *error = QStringLiteral("Der Backup-Ordner für die Notiz kann nicht angelegt werden.");
        return false;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    QString fileName = timestamp + QStringLiteral(".md");
    for (int suffix = 2; QFileInfo::exists(QDir(absoluteDirectory).filePath(fileName)); ++suffix)
        fileName = QStringLiteral("%1-%2.md").arg(timestamp).arg(suffix);

    QSaveFile backup(QDir(absoluteDirectory).filePath(fileName));
    if (!backup.open(QIODevice::WriteOnly) || backup.write(contents) != contents.size() || !backup.commit()) {
        if (error) *error = backup.errorString();
        return false;
    }

    QDir directory(absoluteDirectory);
    const QStringList versions = directory.entryList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);
    constexpr qsizetype MaximumBackups = 10;
    for (qsizetype i = 0; i < versions.size() - MaximumBackups; ++i) {
        if (!directory.remove(versions.at(i))) {
            if (error) *error = QStringLiteral("Ein älteres Notiz-Backup konnte nicht entfernt werden.");
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<Note> NoteRepository::createNote(const QString &title, QString *error)
{
    return createNote(title, QString(), error);
}

std::optional<Note> NoteRepository::createNote(const QString &title, const QString &initialContent,
                                                QString *error)
{
    if (m_impl->vault.isEmpty()) {
        if (error) *error = QStringLiteral("Öffne zuerst einen Notizordner.");
        return std::nullopt;
    }

    const QString cleanTitle = title.simplified().isEmpty() ? QStringLiteral("Neue Notiz") : title.simplified();
    if (cleanTitle.size() > 160) {
        if (error) *error = QStringLiteral("Der Titel darf höchstens 160 Zeichen lang sein.");
        return std::nullopt;
    }
    QSqlQuery existingPaths(m_impl->database);
    if (!existingPaths.exec(QStringLiteral(
            "SELECT relative_path FROM notes UNION SELECT relative_path FROM trashed_notes"))) {
        if (error) *error = sqlError(existingPaths);
        return std::nullopt;
    }
    QSet<QString> indexedPaths;
    while (existingPaths.next())
        indexedPaths.insert(existingPaths.value(0).toString());
    existingPaths.finish();

    const QString relativePath = uniqueNotePath(m_impl->vault, safeFileStem(cleanTitle), indexedPaths);

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QSaveFile file(QDir(m_impl->vault).filePath(relativePath));
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return std::nullopt;
    }
    const QByteArray contents = markdownForNote(id, cleanTitle, initialContent, {}).toUtf8();
    if (file.write(contents) != contents.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return std::nullopt;
    }

    if (!m_impl->database.transaction()) {
        QFile::remove(QDir(m_impl->vault).filePath(relativePath));
        if (error) *error = m_impl->database.lastError().text();
        return std::nullopt;
    }

    QSqlQuery insert(m_impl->database);
    insert.prepare(QStringLiteral("INSERT INTO notes (id, title, relative_path, created_at, updated_at, favorite, tags) "
                                  "VALUES (?, ?, ?, ?, ?, 0, '[]')"));
    insert.addBindValue(id);
    insert.addBindValue(cleanTitle);
    insert.addBindValue(relativePath);
    insert.addBindValue(now.toMSecsSinceEpoch());
    insert.addBindValue(now.toMSecsSinceEpoch());
    if (!insert.exec()) {
        const QString message = sqlError(insert);
        m_impl->database.rollback();
        QFile::remove(QDir(m_impl->vault).filePath(relativePath));
        if (error) *error = message;
        return std::nullopt;
    }
    QString indexError;
    if (!m_impl->replaceSearchDocument(id, cleanTitle, initialContent, QStringLiteral("[]"), &indexError)) {
        m_impl->database.rollback();
        QFile::remove(QDir(m_impl->vault).filePath(relativePath));
        if (error) *error = indexError;
        return std::nullopt;
    }
    if (!m_impl->database.commit()) {
        const QString message = m_impl->database.lastError().text();
        m_impl->database.rollback();
        QFile::remove(QDir(m_impl->vault).filePath(relativePath));
        if (error) *error = message;
        return std::nullopt;
    }

    Note created;
    created.id = id;
    created.title = cleanTitle;
    created.relativePath = relativePath;
    created.content = initialContent;
    created.createdAt = now;
    created.updatedAt = now;
    created.contentHash = QCryptographicHash::hash(contents, QCryptographicHash::Sha256);
    emit notesChanged();
    return created;
}

std::optional<Note> NoteRepository::dailyNote(const QDate &date, QString *error)
{
    if (error)
        error->clear();
    if (!date.isValid()) {
        if (error) *error = QStringLiteral("Das Datum für die Tagesnotiz ist ungültig.");
        return std::nullopt;
    }

    const QString title = date.toString(Qt::ISODate);
    if (const auto existingId = noteIdForTitle(title, error))
        return note(*existingId, true, error);
    if (error && !error->isEmpty())
        return std::nullopt;
    return createNote(title, error);
}

bool NoteRepository::exportNote(const QString &id, const QString &destination, QString *error) const
{
    const QString trimmedDestination = destination.trimmed();
    if (trimmedDestination.isEmpty()) {
        if (error) *error = QStringLiteral("Wähle einen Speicherort für den Markdown-Export aus.");
        return false;
    }

    const auto selected = note(id, false, error);
    if (!selected)
        return false;
    const QString sourcePath = safeAbsolutePath(m_impl->vault, selected->relativePath);
    if (sourcePath.isEmpty()) {
        if (error) *error = QStringLiteral("Die Notizdatei liegt außerhalb des geöffneten Vaults.");
        return false;
    }
    const QString targetPath = QFileInfo(trimmedDestination).absoluteFilePath();
    const QString canonicalSource = QFileInfo(sourcePath).canonicalFilePath();
    const QString canonicalTarget = QFileInfo(targetPath).canonicalFilePath();
    const QString comparableSource = QDir::fromNativeSeparators(
        canonicalSource.isEmpty() ? QFileInfo(sourcePath).absoluteFilePath() : canonicalSource);
    const QString comparableTarget = QDir::fromNativeSeparators(
        canonicalTarget.isEmpty() ? targetPath : canonicalTarget);
#ifdef Q_OS_WIN
    if (comparableSource.compare(comparableTarget, Qt::CaseInsensitive) == 0) {
#else
    if (comparableSource == comparableTarget) {
#endif
        if (error) *error = QStringLiteral("Der Export darf die aktive Notizdatei nicht überschreiben.");
        return false;
    }

    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        if (error) *error = source.errorString();
        return false;
    }
    const QByteArray contents = source.readAll();
    source.close();

    QSaveFile output(targetPath);
    if (!output.open(QIODevice::WriteOnly) || output.write(contents) != contents.size() || !output.commit()) {
        if (error) *error = output.errorString();
        return false;
    }
    return true;
}

bool NoteRepository::saveNote(const QString &id, const QString &title,
                              const QString &content, const QByteArray &expectedHash, QString *error)
{
    const QString normalizedTitle = title.simplified();
    if (normalizedTitle.size() > 160) {
        if (error) *error = QStringLiteral("Der Titel darf höchstens 160 Zeichen lang sein.");
        return false;
    }
    auto current = note(id, true, error);
    if (!current)
        return false;

    const QString cleanTitle = normalizedTitle.isEmpty() ? QStringLiteral("Unbenannte Notiz") : normalizedTitle;
    bool externalConflict = false;
    if (!expectedHash.isEmpty() && current->contentHash != expectedHash) {
        externalConflict = true;
        const QString conflictRoot = QDir(m_impl->vault).filePath(QStringLiteral(".conflicts"));
        if (!QDir().mkpath(conflictRoot)) {
            if (error) *error = QStringLiteral("Die externe Änderung kann nicht gesichert werden.");
            return false;
        }
        const QString conflictName = QStringLiteral("%1-%2.md").arg(id, QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")));
        const QString conflictRelative = QStringLiteral(".conflicts/%1").arg(conflictName);
        QFile original(QDir(m_impl->vault).filePath(current->relativePath));
        if (!original.open(QIODevice::ReadOnly)) {
            if (error) *error = original.errorString();
            return false;
        }
        const QByteArray externalBytes = original.readAll();
        original.close();
        QSaveFile backup(QDir(m_impl->vault).filePath(conflictRelative));
        if (!backup.open(QIODevice::WriteOnly) || backup.write(externalBytes) != externalBytes.size() || !backup.commit()) {
            if (error) *error = backup.errorString();
            return false;
        }
        emit externalVersionBackedUp(conflictRelative);
    }
    QString targetPath = current->relativePath;
    if (safeFileStem(cleanTitle) != QFileInfo(targetPath).completeBaseName()) {
        const QString stem = safeFileStem(cleanTitle);
        targetPath = stem + QStringLiteral(".md");
        int suffix = 2;
        while (QFileInfo::exists(QDir(m_impl->vault).filePath(targetPath))
               && targetPath != current->relativePath) {
            targetPath = QStringLiteral("%1-%2.md").arg(stem).arg(suffix++);
        }
    }

    const QByteArray bytes = markdownForNote(id, cleanTitle, content, current->tags,
                                              current->frontMatter).toUtf8();
    if (!externalConflict && current->contentHash != QCryptographicHash::hash(bytes, QCryptographicHash::Sha256)) {
        QFile previous(QDir(m_impl->vault).filePath(current->relativePath));
        if (!previous.open(QIODevice::ReadOnly)) {
            if (error) *error = previous.errorString();
            return false;
        }
        const QByteArray previousBytes = previous.readAll();
        previous.close();
        if (!archiveMarkdownVersion(m_impl->vault, id, previousBytes, error))
            return false;
    }

    const QString targetAbsolute = QDir(m_impl->vault).filePath(targetPath);
    QSaveFile file(targetAbsolute);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    QSqlQuery update(m_impl->database);
    update.prepare(QStringLiteral("UPDATE notes SET title = ?, relative_path = ?, updated_at = ? WHERE id = ?"));
    update.addBindValue(cleanTitle);
    update.addBindValue(targetPath);
    update.addBindValue(now.toMSecsSinceEpoch());
    update.addBindValue(id);
    if (!update.exec() || update.numRowsAffected() != 1) {
        if (targetPath != current->relativePath)
            QFile::remove(targetAbsolute);
        if (error) *error = update.lastError().text().isEmpty()
            ? QStringLiteral("Die Notiz wurde während des Speicherns geändert.") : sqlError(update);
        return false;
    }
    QString indexError;
    if (!m_impl->replaceSearchDocument(id, cleanTitle, content, normalizedTags(current->tags), &indexError)) {
        if (error) *error = QStringLiteral("Die Notizdatei wurde gespeichert, aber der Suchindex ist veraltet: %1")
            .arg(indexError);
        return false;
    }
    if (targetPath != current->relativePath)
        QFile::remove(QDir(m_impl->vault).filePath(current->relativePath));

    updateWatcherPaths();
    emit notesChanged();
    return true;
}
bool NoteRepository::setFavorite(const QString &id, bool favorite, QString *error)
{
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("UPDATE notes SET favorite = ? WHERE id = ?"));
    query.addBindValue(favorite);
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = sqlError(query);
        return false;
    }
    if (query.numRowsAffected() != 1) {
        if (error) *error = QStringLiteral("Die Notiz wurde nicht gefunden.");
        return false;
    }
    emit notesChanged();
    return true;
}

bool NoteRepository::setArchived(const QString &id, bool archived, QString *error)
{
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("UPDATE notes SET archived = ? WHERE id = ?"));
    query.addBindValue(archived);
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = sqlError(query);
        return false;
    }
    if (query.numRowsAffected() != 1) {
        if (error) *error = QStringLiteral("Die Notiz wurde nicht gefunden.");
        return false;
    }
    emit notesChanged();
    return true;
}

bool NoteRepository::setTags(const QString &id, const QStringList &tags, QString *error)
{
    const auto current = note(id, true, error);
    if (!current)
        return false;
    const QString serializedTags = normalizedTags(tags);
    const QString notePath = safeAbsolutePath(m_impl->vault, current->relativePath);
    if (notePath.isEmpty()) {
        if (error) *error = QStringLiteral("Der Notizpfad liegt außerhalb des geöffneten Vaults.");
        return false;
    }
    QFile source(notePath);
    if (!source.open(QIODevice::ReadOnly)) {
        if (error) *error = source.errorString();
        return false;
    }
    const QByteArray previousBytes = source.readAll();
    source.close();
    if (QCryptographicHash::hash(previousBytes, QCryptographicHash::Sha256) != current->contentHash) {
        if (error) *error = QStringLiteral("Die Notiz wurde extern geändert. Aktualisiere den Vault und versuche es erneut.");
        return false;
    }
    const QByteArray updatedBytes = markdownForNote(id, current->title, current->content,
                                                    parseTags(serializedTags), current->frontMatter).toUtf8();

    if (!m_impl->database.transaction()) {
        if (error) *error = m_impl->database.lastError().text();
        return false;
    }
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("UPDATE notes SET tags = ? WHERE id = ?"));
    query.addBindValue(serializedTags);
    query.addBindValue(id);
    if (!query.exec()) {
        m_impl->database.rollback();
        if (error) *error = sqlError(query);
        return false;
    }
    if (query.numRowsAffected() != 1) {
        m_impl->database.rollback();
        if (error) *error = QStringLiteral("Die Notiz wurde nicht gefunden.");
        return false;
    }
    QString indexError;
    if (!m_impl->replaceSearchDocument(id, current->title, current->content, serializedTags, &indexError)) {
        m_impl->database.rollback();
        if (error) *error = indexError;
        return false;
    }
    QSaveFile output(notePath);
    if (!output.open(QIODevice::WriteOnly) || output.write(updatedBytes) != updatedBytes.size() || !output.commit()) {
        const QString message = output.errorString();
        m_impl->database.rollback();
        if (error) *error = message;
        return false;
    }
    if (!m_impl->database.commit()) {
        const QString commitError = m_impl->database.lastError().text();
        m_impl->database.rollback();
        QSaveFile restore(notePath);
        const bool restored = restore.open(QIODevice::WriteOnly)
            && restore.write(previousBytes) == previousBytes.size() && restore.commit();
        if (error) *error = restored ? commitError
            : QStringLiteral("Tag-Metadaten konnten nicht gespeichert werden und die Notizdatei ließ sich nicht zurücksetzen: %1")
                  .arg(restore.errorString());
        return false;
    }
    emit notesChanged();
    return true;
}

bool NoteRepository::markOpened(const QString &id, QString *error)
{
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("UPDATE notes SET last_opened_at = ? WHERE id = ?"));
    query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = sqlError(query);
        return false;
    }
    if (query.numRowsAffected() != 1) {
        if (error) *error = QStringLiteral("Die Notiz wurde nicht gefunden.");
        return false;
    }
    emit notesChanged();
    return true;
}
bool NoteRepository::moveToTrash(const QString &id, QString *error)
{
    QSqlQuery lookup(m_impl->database);
    lookup.prepare(QStringLiteral("SELECT title, relative_path, created_at, updated_at, last_opened_at, favorite, tags FROM notes WHERE id = ?"));
    lookup.addBindValue(id);
    if (!lookup.exec() || !lookup.next()) {
        if (error) *error = lookup.lastError().text().isEmpty()
            ? QStringLiteral("Die Notiz wurde nicht gefunden.") : sqlError(lookup);
        return false;
    }

    const QString noteTitle = lookup.value(0).toString();
    const QString originalPath = lookup.value(1).toString();
    const QVariant createdAt = lookup.value(2);
    const QVariant updatedAt = lookup.value(3);
    const QVariant lastOpenedAt = lookup.value(4);
    const QVariant favorite = lookup.value(5);
    const QVariant tags = lookup.value(6);
    lookup.finish();
    const QString trashPath = QStringLiteral(".trash/%1.md").arg(id);
    const QString absoluteOriginal = safeAbsolutePath(m_impl->vault, originalPath);
    const QString absoluteTrash = QDir(m_impl->vault).filePath(trashPath);
    if (absoluteOriginal.isEmpty() || QFileInfo::exists(absoluteTrash) || !QFile::rename(absoluteOriginal, absoluteTrash)) {
        if (error) *error = QStringLiteral("Die Notiz kann nicht in den Papierkorb verschoben werden.");
        return false;
    }

    if (!m_impl->database.transaction()) {
        QFile::rename(absoluteTrash, absoluteOriginal);
        if (error) *error = m_impl->database.lastError().text();
        return false;
    }
    QSqlQuery insert(m_impl->database);
    insert.prepare(QStringLiteral("INSERT INTO trashed_notes (id, title, relative_path, trash_path, created_at, updated_at, last_opened_at, favorite, tags, deleted_at) "
                                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    insert.addBindValue(id);
    insert.addBindValue(noteTitle);
    insert.addBindValue(originalPath);
    insert.addBindValue(trashPath);
    insert.addBindValue(createdAt);
    insert.addBindValue(updatedAt);
    insert.addBindValue(lastOpenedAt);
    insert.addBindValue(favorite);
    insert.addBindValue(tags);
    insert.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    QSqlQuery remove(m_impl->database);
    remove.prepare(QStringLiteral("DELETE FROM notes WHERE id = ?"));
    remove.addBindValue(id);
    QString indexError;
    const bool indexRemoved = m_impl->removeSearchDocument(id, &indexError);
    if (!insert.exec() || !remove.exec() || !indexRemoved || !m_impl->database.commit()) {
        const QString message = !insert.lastError().text().isEmpty() ? sqlError(insert)
            : !remove.lastError().text().isEmpty() ? sqlError(remove)
            : !indexError.isEmpty() ? indexError : m_impl->database.lastError().text();
        m_impl->database.rollback();
        QFile::rename(absoluteTrash, absoluteOriginal);
        if (error) *error = message;
        return false;
    }
    updateWatcherPaths();
    emit notesChanged();
    return true;
}
bool NoteRepository::restoreFromTrash(const QString &id, QString *error)
{
    QSqlQuery lookup(m_impl->database);
    lookup.prepare(QStringLiteral("SELECT title, relative_path, trash_path, created_at, updated_at, last_opened_at, favorite, tags FROM trashed_notes WHERE id = ?"));
    lookup.addBindValue(id);
    if (!lookup.exec() || !lookup.next()) {
        if (error) *error = lookup.lastError().text().isEmpty()
            ? QStringLiteral("Die gelöschte Notiz wurde nicht gefunden.") : sqlError(lookup);
        return false;
    }
    const QString title = lookup.value(0).toString();
    const QString originalPath = lookup.value(1).toString();
    const QString trashPath = lookup.value(2).toString();
    const QVariant createdAt = lookup.value(3);
    const QVariant updatedAt = lookup.value(4);
    const QVariant lastOpenedAt = lookup.value(5);
    const QVariant favorite = lookup.value(6);
    const QVariant tags = lookup.value(7);
    lookup.finish();

    QString restoredPath = originalPath;
    if (QFileInfo::exists(QDir(m_impl->vault).filePath(restoredPath))) {
        const QString stem = safeFileStem(title);
        int suffix = 2;
        do {
            restoredPath = QStringLiteral("%1-%2.md").arg(stem).arg(suffix++);
        } while (QFileInfo::exists(QDir(m_impl->vault).filePath(restoredPath)));
    }
    const QString absoluteTrash = safeAbsolutePath(m_impl->vault, trashPath);
    const QString absoluteRestored = safeAbsolutePath(m_impl->vault, restoredPath);
    if (absoluteTrash.isEmpty() || absoluteRestored.isEmpty()
        || !QFile::rename(absoluteTrash, absoluteRestored)) {
        if (error) *error = QStringLiteral("Die Notiz konnte nicht wiederhergestellt werden.");
        return false;
    }

    if (!m_impl->database.transaction()) {
        QFile::rename(absoluteRestored, absoluteTrash);
        if (error) *error = m_impl->database.lastError().text();
        return false;
    }
    QSqlQuery insert(m_impl->database);
    insert.prepare(QStringLiteral("INSERT INTO notes (id, title, relative_path, created_at, updated_at, last_opened_at, favorite, tags) "
                                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    insert.addBindValue(id);
    insert.addBindValue(title);
    insert.addBindValue(restoredPath);
    insert.addBindValue(createdAt);
    insert.addBindValue(updatedAt);
    insert.addBindValue(lastOpenedAt);
    insert.addBindValue(favorite);
    insert.addBindValue(tags);
    QFile restoredFile(absoluteRestored);
    QString restoredContent;
    if (restoredFile.open(QIODevice::ReadOnly))
        restoredContent = QString::fromUtf8(restoredFile.readAll());
    QString indexError;
    const bool indexUpdated = m_impl->replaceSearchDocument(id, title, restoredContent,
                                                             tags.toString(), &indexError);
    QSqlQuery remove(m_impl->database);
    remove.prepare(QStringLiteral("DELETE FROM trashed_notes WHERE id = ?"));
    remove.addBindValue(id);
    if (!insert.exec() || !remove.exec() || !indexUpdated || !m_impl->database.commit()) {
        const QString message = !insert.lastError().text().isEmpty() ? sqlError(insert)
            : !remove.lastError().text().isEmpty() ? sqlError(remove)
            : !indexError.isEmpty() ? indexError : m_impl->database.lastError().text();
        m_impl->database.rollback();
        QFile::rename(absoluteRestored, absoluteTrash);
        if (error) *error = message;
        return false;
    }
    updateWatcherPaths();
    emit notesChanged();
    return true;
}

} // namespace CrossNote
