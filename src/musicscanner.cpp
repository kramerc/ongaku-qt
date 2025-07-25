#include "musicscanner.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>

MusicScanner::MusicScanner(DatabaseManager *dbManager, QObject *parent)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_currentFileIndex(0)
    , m_scanInProgress(false)
    , m_tracksFound(0)
    , m_tracksAdded(0)
    , m_tracksUpdated(0)
    , m_batchSize(10) // Process 10 files per timer tick
{
    // Set default supported formats
    m_supportedFormats = {
        "mp3", "flac", "ogg", "m4a", "mp4", "aac",
        "wma", "wav", "aiff", "ape", "opus"
    };

    // Set up timer for non-blocking file processing
    m_processTimer.setSingleShot(true);
    m_processTimer.setInterval(0); // Process files as fast as possible
    connect(&m_processTimer, &QTimer::timeout, this, &MusicScanner::processBatch);
}

void MusicScanner::setMusicDirectory(const QString &directory)
{
    m_musicDirectory = directory;
}

void MusicScanner::setSupportedFormats(const QStringList &formats)
{
    m_supportedFormats = formats;
}

void MusicScanner::scanLibrary()
{
    if (m_scanInProgress) {
        return;
    }

    if (m_musicDirectory.isEmpty()) {
        emit scanError("Music directory not set");
        return;
    }

    if (!QDir(m_musicDirectory).exists()) {
        emit scanError("Music directory does not exist: " + m_musicDirectory);
        return;
    }

    qDebug() << "Starting library scan in:" << m_musicDirectory;

    m_scanInProgress = true;
    m_tracksFound = 0;
    m_tracksAdded = 0;
    m_tracksUpdated = 0;
    m_currentFileIndex = 0;
    m_filesToProcess.clear();

    emit scanStarted();

    // Find all music files
    findMusicFiles(m_musicDirectory, m_filesToProcess);
    m_tracksFound = m_filesToProcess.size();

    qDebug() << "Found" << m_tracksFound << "music files";

    if (m_tracksFound == 0) {
        m_scanInProgress = false;
        emit scanCompleted(0, 0, 0);
        return;
    }

    // Begin database transaction for better performance
    m_dbManager->beginTransaction();

    // Start processing files
    m_processTimer.start();
}

void MusicScanner::stopScanning()
{
    if (!m_scanInProgress) {
        return;
    }

    m_processTimer.stop();
    m_scanInProgress = false;

    // Commit any pending transactions
    m_dbManager->commitTransaction();

    qDebug() << "Scan stopped by user";
    emit scanCompleted(m_tracksFound, m_tracksAdded, m_tracksUpdated);
}

void MusicScanner::processBatch()
{
    if (!m_scanInProgress) {
        return;
    }

    if (m_currentFileIndex >= m_filesToProcess.size()) {
        // Scanning completed - commit transaction
        m_dbManager->commitTransaction();
        m_scanInProgress = false;
        qDebug() << "Scan completed. Found:" << m_tracksFound
                 << "Added:" << m_tracksAdded << "Updated:" << m_tracksUpdated;
        emit scanCompleted(m_tracksFound, m_tracksAdded, m_tracksUpdated);
        return;
    }

    // Process a batch of files
    int processed = 0;
    while (processed < m_batchSize && m_currentFileIndex < m_filesToProcess.size() && m_scanInProgress) {
        processNextFile();
        processed++;
        m_currentFileIndex++;
    }

    // Update progress after processing the batch
    emit scanProgress(m_currentFileIndex, m_tracksFound);

    // Schedule next batch or complete if we're done
    if (m_scanInProgress) {
        if (m_currentFileIndex < m_filesToProcess.size()) {
            m_processTimer.start();
        } else {
            // We've processed all files, trigger completion
            m_processTimer.start();
        }
    }
}

void MusicScanner::processNextFile()
{
    if (m_currentFileIndex >= m_filesToProcess.size()) {
        return;
    }

    QString filePath = m_filesToProcess[m_currentFileIndex];
    emit trackScanned(filePath);

    try {
        // Check if track already exists in database
        bool trackExists = m_dbManager->trackExists(filePath);

        if (trackExists) {
            // Check if file has been modified since last scan
            MusicTrack existingTrack = m_dbManager->getTrackByPath(filePath);
            QFileInfo fileInfo(filePath);

            if (!isFileNewer(filePath, existingTrack.lastModified)) {
                // File hasn't changed, skip it
                return;
            }
        }

        // Extract metadata
        MusicTrack track = extractMetadata(filePath);

        if (track.filePath.isEmpty()) {
            // Failed to extract metadata, skip file
            return;
        }

        // Add or update track in database
        bool success;
        if (trackExists) {
            success = m_dbManager->updateTrack(track);
            if (success) {
                m_tracksUpdated++;
                emit trackUpdated(track);
            }
        } else {
            success = m_dbManager->addTrack(track);
            if (success) {
                m_tracksAdded++;
                emit trackAdded(track);
            }
        }

        if (!success) {
            qWarning() << "Failed to save track to database:" << filePath;
        }

    } catch (const std::exception &e) {
        qWarning() << "Error processing file" << filePath << ":" << e.what();
    } catch (...) {
        qWarning() << "Unknown error processing file:" << filePath;
    }
}void MusicScanner::findMusicFiles(const QString &directory, QStringList &files)
{
    QStringList nameFilters;
    for (const QString &format : m_supportedFormats) {
        nameFilters << "*." + format;
        nameFilters << "*." + format.toUpper();
    }

    QDirIterator iterator(directory, nameFilters, QDir::Files, QDirIterator::Subdirectories);

    while (iterator.hasNext()) {
        files.append(iterator.next());
    }
}

MusicTrack MusicScanner::extractMetadata(const QString &filePath)
{
    MusicTrack track;

    try {
        TagLib::FileRef fileRef(filePath.toLocal8Bit().constData());

        if (fileRef.isNull()) {
            qWarning() << "Could not read file:" << filePath;
            return track;
        }

        TagLib::Tag *tag = fileRef.tag();
        TagLib::AudioProperties *properties = fileRef.audioProperties();

        track.filePath = filePath;

        if (tag) {
            track.title = QString::fromUtf8(tag->title().toCString(true));
            track.artist = QString::fromUtf8(tag->artist().toCString(true));
            track.album = QString::fromUtf8(tag->album().toCString(true));
            track.genre = QString::fromUtf8(tag->genre().toCString(true));
            track.year = tag->year();
            track.track = tag->track();

            // Clean up empty strings
            if (track.title.isEmpty()) {
                QFileInfo fileInfo(filePath);
                track.title = fileInfo.completeBaseName();
            }
            if (track.artist.isEmpty()) {
                track.artist = "Unknown Artist";
            }
            if (track.album.isEmpty()) {
                track.album = "Unknown Album";
            }
            if (track.genre.isEmpty()) {
                track.genre = "Unknown";
            }
        }

        if (properties) {
            track.duration = properties->lengthInSeconds();
        }

        // Get file information
        QFileInfo fileInfo(filePath);
        track.fileSize = fileInfo.size();
        track.lastModified = fileInfo.lastModified();

    } catch (const std::exception &e) {
        qWarning() << "Exception while extracting metadata from" << filePath << ":" << e.what();
        return MusicTrack(); // Return empty track
    } catch (...) {
        qWarning() << "Unknown exception while extracting metadata from" << filePath;
        return MusicTrack(); // Return empty track
    }

    return track;
}

bool MusicScanner::isFileNewer(const QString &filePath, const QDateTime &dbModified)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.lastModified() > dbModified;
}
