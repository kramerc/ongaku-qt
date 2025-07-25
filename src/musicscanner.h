#ifndef MUSICSCANNER_H
#define MUSICSCANNER_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QFileInfo>
#include <QTimer>
#include "databasemanager.h"

class MusicScanner : public QObject
{
    Q_OBJECT

public:
    explicit MusicScanner(DatabaseManager *dbManager, QObject *parent = nullptr);

    void setMusicDirectory(const QString &directory);
    void setSupportedFormats(const QStringList &formats);

public slots:
    void scanLibrary();
    void stopScanning();

signals:
    void scanStarted();
    void scanProgress(int current, int total);
    void trackScanned(const QString &filePath);
    void trackAdded(const MusicTrack &track);
    void trackUpdated(const MusicTrack &track);
    void scanCompleted(int tracksFound, int tracksAdded, int tracksUpdated);
    void scanError(const QString &error);

private slots:
    void processNextFile();

private:
    DatabaseManager *m_dbManager;
    QString m_musicDirectory;
    QStringList m_supportedFormats;
    QStringList m_filesToProcess;
    int m_currentFileIndex;
    bool m_scanInProgress;
    QTimer m_processTimer;

    int m_tracksFound;
    int m_tracksAdded;
    int m_tracksUpdated;
    int m_batchSize; // Number of files to process per timer tick

    void findMusicFiles(const QString &directory, QStringList &files);
    MusicTrack extractMetadata(const QString &filePath);
    bool isFileNewer(const QString &filePath, const QDateTime &dbModified);
    void processBatch();
};

#endif // MUSICSCANNER_H
