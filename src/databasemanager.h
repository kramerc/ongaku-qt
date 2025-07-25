#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDateTime>

struct MusicTrack {
    int id;
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString genre;
    int year;
    int track;
    int duration; // in seconds
    qint64 fileSize;
    QDateTime lastModified;

    MusicTrack() : id(-1), year(0), track(0), duration(0), fileSize(0) {}
};

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool initialize();
    bool addTrack(const MusicTrack &track);
    bool updateTrack(const MusicTrack &track);
    bool removeTrack(int id);
    bool removeTrackByPath(const QString &filePath);

    QList<MusicTrack> getAllTracks();
    QList<MusicTrack> searchTracks(const QString &searchTerm);
    QList<MusicTrack> getTracksByArtist(const QString &artist);
    QList<MusicTrack> getTracksByAlbum(const QString &album);
    QList<MusicTrack> getTracksByGenre(const QString &genre);

    bool trackExists(const QString &filePath);
    MusicTrack getTrackByPath(const QString &filePath);

    QStringList getAllArtists();
    QStringList getAllAlbums();
    QStringList getAllGenres();

    int getTrackCount();
    void clearDatabase();

    // Transaction support for batch operations
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

private:
    QSqlDatabase m_database;
    bool createTables();
    MusicTrack trackFromQuery(const QSqlQuery &query);
};

#endif // DATABASEMANAGER_H
