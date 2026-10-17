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
#include <QUuid>

namespace CrossNote {
namespace {

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
        const QString indexRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                                      .filePath(QStringLiteral("vault-index"));
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
                           "last_opened_at INTEGER, favorite INTEGER NOT NULL DEFAULT 0, tags TEXT NOT NULL DEFAULT '[]')")
        };
        for (const QString &statement : statements) {
            QSqlQuery query(database);
            if (!query.exec(statement)) {
                if (error) *error = sqlError(query);
                return false;
            }
        }
        return true;
    }

    QVector<Note> list(QString *error) const
    {
        QVector<Note> result;
        QSqlQuery query(database);
        query.prepare(QStringLiteral("SELECT id, title, relative_path, created_at, updated_at, "
                                     "last_opened_at, favorite, tags FROM notes ORDER BY updated_at DESC"));
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
            const QString absolute = QDir(vault).filePath(item.relativePath);
            QFile file(absolute);
            if (file.open(QIODevice::ReadOnly))
                item.content = QString::fromUtf8(file.readAll());
            result.append(std::move(item));
        }
        return result;
    }
};

NoteRepository::NoteRepository(QObject *parent)
    : QObject(parent), m_impl(new Impl)
{
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

std::optional<Note> NoteRepository::note(const QString &id, bool includeContent, QString *error) const
{
    QSqlQuery query(m_impl->database);
    query.prepare(QStringLiteral("SELECT title, relative_path, created_at, updated_at, "
                                 "last_opened_at, favorite, tags FROM notes WHERE id = ?"));
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

    if (includeContent) {
        QFile file(QDir(m_impl->vault).filePath(result.relativePath));
        if (!file.open(QIODevice::ReadOnly)) {
            if (error) *error = file.errorString();
            return std::nullopt;
        }
        const Note parsed = parseMarkdownFile(QString::fromUtf8(file.readAll()), id,
                                              result.title, result.relativePath);
        result.content = parsed.content;
        result.frontMatter = parsed.frontMatter;
    }
    return result;
}
