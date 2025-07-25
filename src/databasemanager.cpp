#include "databasemanager.h"
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::initialize()
{
    // Create database directory if it doesn't exist
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);

    // Setup database connection
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dataPath + "/vibeqt.db");

    if (!m_database.open()) {
        qWarning() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }

    return createTables();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_database);

    QString createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS tracks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT UNIQUE NOT NULL,
            title TEXT,
            artist TEXT,
            album TEXT,
            genre TEXT,
            year INTEGER,
            track_number INTEGER,
            duration INTEGER,
            file_size INTEGER,
            last_modified DATETIME,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createTableSQL)) {
        qWarning() << "Failed to create tracks table:" << query.lastError().text();
        return false;
    }

    // Create indexes for better search performance
    QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_artist ON tracks(artist)",
        "CREATE INDEX IF NOT EXISTS idx_album ON tracks(album)",
        "CREATE INDEX IF NOT EXISTS idx_genre ON tracks(genre)",
        "CREATE INDEX IF NOT EXISTS idx_title ON tracks(title)",
        "CREATE INDEX IF NOT EXISTS idx_file_path ON tracks(file_path)"
    };

    for (const QString &indexSQL : indexes) {
        if (!query.exec(indexSQL)) {
            qWarning() << "Failed to create index:" << query.lastError().text();
        }
    }

    return true;
}

bool DatabaseManager::addTrack(const MusicTrack &track)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO tracks (file_path, title, artist, album, genre, year, track_number,
                           duration, file_size, last_modified)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(track.filePath);
    query.addBindValue(track.title);
    query.addBindValue(track.artist);
    query.addBindValue(track.album);
    query.addBindValue(track.genre);
    query.addBindValue(track.year);
    query.addBindValue(track.track);
    query.addBindValue(track.duration);
    query.addBindValue(track.fileSize);
    query.addBindValue(track.lastModified);

    if (!query.exec()) {
        qWarning() << "Failed to add track:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::updateTrack(const MusicTrack &track)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        UPDATE tracks SET title=?, artist=?, album=?, genre=?, year=?, track_number=?,
                         duration=?, file_size=?, last_modified=?, updated_at=CURRENT_TIMESTAMP
        WHERE file_path=?
    )");

    query.addBindValue(track.title);
    query.addBindValue(track.artist);
    query.addBindValue(track.album);
    query.addBindValue(track.genre);
    query.addBindValue(track.year);
    query.addBindValue(track.track);
    query.addBindValue(track.duration);
    query.addBindValue(track.fileSize);
    query.addBindValue(track.lastModified);
    query.addBindValue(track.filePath);

    return query.exec();
}

bool DatabaseManager::removeTrack(int id)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM tracks WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

bool DatabaseManager::removeTrackByPath(const QString &filePath)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM tracks WHERE file_path = ?");
    query.addBindValue(filePath);
    return query.exec();
}

QList<MusicTrack> DatabaseManager::getAllTracks()
{
    QList<MusicTrack> tracks;
    QSqlQuery query("SELECT * FROM tracks ORDER BY artist, album, track_number", m_database);

    if (!query.exec()) {
        qWarning() << "getAllTracks query failed:" << query.lastError().text();
        return tracks;
    }

    while (query.next()) {
        tracks.append(trackFromQuery(query));
    }

    return tracks;
}

QList<MusicTrack> DatabaseManager::searchTracks(const QString &searchTerm)
{
    QList<MusicTrack> tracks;
    QSqlQuery query(m_database);

    query.prepare(R"(
        SELECT * FROM tracks
        WHERE title LIKE ? OR artist LIKE ? OR album LIKE ? OR genre LIKE ?
        ORDER BY artist, album, track_number
    )");

    QString searchPattern = "%" + searchTerm + "%";
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);

    if (!query.exec()) {
        qWarning() << "Search query failed:" << query.lastError().text();
        return tracks;
    }

    while (query.next()) {
        tracks.append(trackFromQuery(query));
    }

    qDebug() << "Search for '" << searchTerm << "' returned" << tracks.size() << "tracks";
    return tracks;
}

QList<MusicTrack> DatabaseManager::getTracksByArtist(const QString &artist)
{
    QList<MusicTrack> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM tracks WHERE artist = ? ORDER BY album, track_number");
    query.addBindValue(artist);

    while (query.next()) {
        tracks.append(trackFromQuery(query));
    }

    return tracks;
}

QList<MusicTrack> DatabaseManager::getTracksByAlbum(const QString &album)
{
    QList<MusicTrack> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM tracks WHERE album = ? ORDER BY track_number");
    query.addBindValue(album);

    while (query.next()) {
        tracks.append(trackFromQuery(query));
    }

    return tracks;
}

QList<MusicTrack> DatabaseManager::getTracksByGenre(const QString &genre)
{
    QList<MusicTrack> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM tracks WHERE genre = ? ORDER BY artist, album, track_number");
    query.addBindValue(genre);

    while (query.next()) {
        tracks.append(trackFromQuery(query));
    }

    return tracks;
}

bool DatabaseManager::trackExists(const QString &filePath)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT 1 FROM tracks WHERE file_path = ?");
    query.addBindValue(filePath);
    query.exec();
    return query.next();
}

MusicTrack DatabaseManager::getTrackByPath(const QString &filePath)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM tracks WHERE file_path = ?");
    query.addBindValue(filePath);

    if (query.exec() && query.next()) {
        return trackFromQuery(query);
    }

    return MusicTrack();
}

QStringList DatabaseManager::getAllArtists()
{
    QStringList artists;
    QSqlQuery query("SELECT DISTINCT artist FROM tracks WHERE artist IS NOT NULL AND artist != '' ORDER BY artist", m_database);

    while (query.next()) {
        artists.append(query.value(0).toString());
    }

    return artists;
}

QStringList DatabaseManager::getAllAlbums()
{
    QStringList albums;
    QSqlQuery query("SELECT DISTINCT album FROM tracks WHERE album IS NOT NULL AND album != '' ORDER BY album", m_database);

    while (query.next()) {
        albums.append(query.value(0).toString());
    }

    return albums;
}

QStringList DatabaseManager::getAllGenres()
{
    QStringList genres;
    QSqlQuery query("SELECT DISTINCT genre FROM tracks WHERE genre IS NOT NULL AND genre != '' ORDER BY genre", m_database);

    while (query.next()) {
        genres.append(query.value(0).toString());
    }

    return genres;
}

int DatabaseManager::getTrackCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM tracks", m_database);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void DatabaseManager::clearDatabase()
{
    QSqlQuery query("DELETE FROM tracks", m_database);
    query.exec();
}

MusicTrack DatabaseManager::trackFromQuery(const QSqlQuery &query)
{
    MusicTrack track;
    track.id = query.value("id").toInt();
    track.filePath = query.value("file_path").toString();
    track.title = query.value("title").toString();
    track.artist = query.value("artist").toString();
    track.album = query.value("album").toString();
    track.genre = query.value("genre").toString();
    track.year = query.value("year").toInt();
    track.track = query.value("track_number").toInt();
    track.duration = query.value("duration").toInt();
    track.fileSize = query.value("file_size").toLongLong();
    track.lastModified = query.value("last_modified").toDateTime();
    return track;
}

bool DatabaseManager::beginTransaction()
{
    return m_database.transaction();
}

bool DatabaseManager::commitTransaction()
{
    return m_database.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    return m_database.rollback();
}
