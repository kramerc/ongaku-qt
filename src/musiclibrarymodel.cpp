#include "musiclibrarymodel.h"
#include <QIcon>
#include <QFont>
#include <QDebug>
#include <algorithm>

// MusicLibraryItem implementation
MusicLibraryItem::MusicLibraryItem(ItemType type, const QString &data, MusicLibraryItem *parent)
    : m_parentItem(parent), m_type(type), m_text(data)
{
}

MusicLibraryItem::~MusicLibraryItem()
{
    qDeleteAll(m_childItems);
}

void MusicLibraryItem::appendChild(MusicLibraryItem *child)
{
    m_childItems.append(child);
}

void MusicLibraryItem::removeChild(int row)
{
    if (row >= 0 && row < m_childItems.size()) {
        delete m_childItems.takeAt(row);
    }
}

void MusicLibraryItem::clearChildren()
{
    qDeleteAll(m_childItems);
    m_childItems.clear();
}

MusicLibraryItem *MusicLibraryItem::child(int row) const
{
    if (row < 0 || row >= m_childItems.size()) {
        return nullptr;
    }
    return m_childItems.at(row);
}

int MusicLibraryItem::childCount() const
{
    return m_childItems.count();
}

int MusicLibraryItem::columnCount() const
{
    return MusicLibraryModel::ColumnCount;
}

QVariant MusicLibraryItem::data(int column) const
{
    switch (m_type) {
        case TrackItem:
            switch (column) {
                case MusicLibraryModel::TitleColumn:
                    return m_track.title;
                case MusicLibraryModel::ArtistColumn:
                    return m_track.artist;
                case MusicLibraryModel::AlbumColumn:
                    return m_track.album;
                case MusicLibraryModel::GenreColumn:
                    return m_track.genre;
                case MusicLibraryModel::PublisherColumn:
                    return m_track.publisher;
                case MusicLibraryModel::CatalogNumberColumn:
                    return m_track.catalogNumber;
                case MusicLibraryModel::YearColumn:
                    return m_track.year > 0 ? QString::number(m_track.year) : QString();
                case MusicLibraryModel::TrackColumn:
                    return m_track.track > 0 ? QString::number(m_track.track) : QString();
                case MusicLibraryModel::DurationColumn: {
                    int seconds = m_track.duration;
                    int minutes = seconds / 60;
                    seconds %= 60;
                    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
                }
                default:
                    return QVariant();
            }
            break;
        default:
            if (column == MusicLibraryModel::TitleColumn) {
                return m_text;
            }
            return QVariant();
    }
}

int MusicLibraryItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<MusicLibraryItem*>(this));
    }
    return 0;
}

MusicLibraryItem *MusicLibraryItem::parentItem() const
{
    return m_parentItem;
}

// MusicLibraryModel implementation
MusicLibraryModel::MusicLibraryModel(DatabaseManager *dbManager, QObject *parent)
    : QAbstractItemModel(parent)
    , m_dbManager(dbManager)
    , m_sortMode(SortByArtistAlbum)
{
    m_rootItem = new MusicLibraryItem(MusicLibraryItem::RootItem, "Root");
    refreshData();
}

MusicLibraryModel::~MusicLibraryModel()
{
    delete m_rootItem;
}

QVariant MusicLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    MusicLibraryItem *item = getItem(index);

    switch (role) {
        case Qt::DisplayRole:
            return item->data(index.column());

        case Qt::FontRole: {
            QFont font;
            if (item->type() == MusicLibraryItem::ArtistItem) {
                font.setBold(true);
            } else if (item->type() == MusicLibraryItem::AlbumItem) {
                font.setItalic(true);
            }
            return font;
        }

        case Qt::UserRole:
            // Return the track data for easy access
            if (item->type() == MusicLibraryItem::TrackItem) {
                return QVariant::fromValue(item->track());
            }
            return QVariant();

        default:
            return QVariant();
    }
}

Qt::ItemFlags MusicLibraryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant MusicLibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case TitleColumn: return tr("Title");
            case ArtistColumn: return tr("Artist");
            case AlbumColumn: return tr("Album");
            case GenreColumn: return tr("Genre");
            case PublisherColumn: return tr("Publisher");
            case CatalogNumberColumn: return tr("Catalog #");
            case YearColumn: return tr("Year");
            case TrackColumn: return tr("Track");
            case DurationColumn: return tr("Duration");
            default: return QVariant();
        }
    }

    return QVariant();
}

QModelIndex MusicLibraryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    MusicLibraryItem *parentItem = getItem(parent);
    MusicLibraryItem *childItem = parentItem->child(row);

    if (childItem) {
        return createIndex(row, column, childItem);
    }

    return QModelIndex();
}

QModelIndex MusicLibraryModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    MusicLibraryItem *childItem = getItem(index);
    MusicLibraryItem *parentItem = childItem->parentItem();

    if (parentItem == m_rootItem || !parentItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int MusicLibraryModel::rowCount(const QModelIndex &parent) const
{
    MusicLibraryItem *parentItem = getItem(parent);
    return parentItem->childCount();
}

int MusicLibraryModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnCount;
}

void MusicLibraryModel::refreshData()
{
    beginResetModel();
    setupModelData();
    endResetModel();
}

void MusicLibraryModel::searchTracks(const QString &searchTerm)
{
    qDebug() << "MusicLibraryModel::searchTracks called with:" << searchTerm;
    m_currentSearchTerm = searchTerm;
    beginResetModel();

    m_rootItem->clearChildren();

    if (searchTerm.isEmpty()) {
        qDebug() << "Empty search term, showing all tracks";
        setupModelData();
    } else {
        QList<MusicTrack> tracks = m_dbManager->searchTracks(searchTerm);
        qDebug() << "Database returned" << tracks.size() << "tracks for search";

        // For search results, show flat list of tracks
        for (const MusicTrack &track : tracks) {
            MusicLibraryItem *trackItem = new MusicLibraryItem(MusicLibraryItem::TrackItem, track.title, m_rootItem);
            trackItem->setTrack(track);
            m_rootItem->appendChild(trackItem);
        }
    }

    endResetModel();
}

void MusicLibraryModel::showAllTracks()
{
    m_currentSearchTerm.clear();
    refreshData();
}

MusicTrack MusicLibraryModel::getTrack(const QModelIndex &index) const
{
    MusicLibraryItem *item = getItem(index);
    if (item && item->type() == MusicLibraryItem::TrackItem) {
        return item->track();
    }
    return MusicTrack();
}

void MusicLibraryModel::setSortMode(SortMode mode)
{
    if (m_sortMode != mode) {
        m_sortMode = mode;
        refreshData();
    }
}

void MusicLibraryModel::addTrackToModel(const MusicTrack &track)
{
    // Only add if we're not in search mode or if the track matches the search
    if (!m_currentSearchTerm.isEmpty()) {
        QString searchLower = m_currentSearchTerm.toLower();
        if (!track.title.toLower().contains(searchLower) &&
            !track.artist.toLower().contains(searchLower) &&
            !track.album.toLower().contains(searchLower) &&
            !track.genre.toLower().contains(searchLower)) {
            return; // Track doesn't match current search, don't add
        }
    }

    // For simplicity during scanning, we'll do a lightweight refresh
    // This could be optimized further by inserting into the correct position
    refreshData();
}

void MusicLibraryModel::updateTrackInModel(const MusicTrack &track)
{
    // For updates, also do a refresh for now
    // In a more sophisticated implementation, we'd find and update the specific item
    refreshData();
}

void MusicLibraryModel::setupModelData()
{
    qDebug() << "setupModelData() called";
    m_rootItem->clearChildren();

    QList<MusicTrack> tracks;
    if (m_currentSearchTerm.isEmpty()) {
        tracks = m_dbManager->getAllTracks();
        qDebug() << "Getting all tracks, found:" << tracks.size();
    } else {
        tracks = m_dbManager->searchTracks(m_currentSearchTerm);
        qDebug() << "Getting search tracks for '" << m_currentSearchTerm << "', found:" << tracks.size();
    }

    switch (m_sortMode) {
        case SortByArtistAlbum:
            buildArtistAlbumTree(tracks);
            break;
        case SortByAlbum:
            buildAlbumTree(tracks);
            break;
        case SortByGenre:
            buildGenreTree(tracks);
            break;
        case SortByYear:
            buildYearTree(tracks);
            break;
    }
}

void MusicLibraryModel::buildArtistAlbumTree(const QList<MusicTrack> &tracks)
{
    QMap<QString, MusicLibraryItem*> artistItems;
    QMap<QString, MusicLibraryItem*> albumItems;

    for (const MusicTrack &track : tracks) {
        // Get or create artist item
        MusicLibraryItem *artistItem = artistItems.value(track.artist);
        if (!artistItem) {
            artistItem = new MusicLibraryItem(MusicLibraryItem::ArtistItem, track.artist, m_rootItem);
            artistItems[track.artist] = artistItem;
            m_rootItem->appendChild(artistItem);
        }

        // Get or create album item under artist
        QString albumKey = track.artist + " - " + track.album;
        MusicLibraryItem *albumItem = albumItems.value(albumKey);
        if (!albumItem) {
            albumItem = new MusicLibraryItem(MusicLibraryItem::AlbumItem, track.album, artistItem);
            albumItems[albumKey] = albumItem;
            artistItem->appendChild(albumItem);
        }

        // Create track item
        MusicLibraryItem *trackItem = new MusicLibraryItem(MusicLibraryItem::TrackItem, track.title, albumItem);
        trackItem->setTrack(track);
        albumItem->appendChild(trackItem);
    }
}

void MusicLibraryModel::buildAlbumTree(const QList<MusicTrack> &tracks)
{
    QMap<QString, MusicLibraryItem*> albumItems;

    for (const MusicTrack &track : tracks) {
        // Get or create album item
        MusicLibraryItem *albumItem = albumItems.value(track.album);
        if (!albumItem) {
            albumItem = new MusicLibraryItem(MusicLibraryItem::AlbumItem, track.album, m_rootItem);
            albumItems[track.album] = albumItem;
            m_rootItem->appendChild(albumItem);
        }

        // Create track item
        MusicLibraryItem *trackItem = new MusicLibraryItem(MusicLibraryItem::TrackItem, track.title, albumItem);
        trackItem->setTrack(track);
        albumItem->appendChild(trackItem);
    }
}

void MusicLibraryModel::buildGenreTree(const QList<MusicTrack> &tracks)
{
    QMap<QString, MusicLibraryItem*> genreItems;

    for (const MusicTrack &track : tracks) {
        // Get or create genre item
        MusicLibraryItem *genreItem = genreItems.value(track.genre);
        if (!genreItem) {
            genreItem = new MusicLibraryItem(MusicLibraryItem::ArtistItem, track.genre, m_rootItem);
            genreItems[track.genre] = genreItem;
            m_rootItem->appendChild(genreItem);
        }

        // Create track item
        MusicLibraryItem *trackItem = new MusicLibraryItem(MusicLibraryItem::TrackItem, track.title, genreItem);
        trackItem->setTrack(track);
        genreItem->appendChild(trackItem);
    }
}

void MusicLibraryModel::buildYearTree(const QList<MusicTrack> &tracks)
{
    QMap<int, MusicLibraryItem*> yearItems;

    for (const MusicTrack &track : tracks) {
        int year = track.year > 0 ? track.year : 0;
        QString yearText = year > 0 ? QString::number(year) : "Unknown Year";

        // Get or create year item
        MusicLibraryItem *yearItem = yearItems.value(year);
        if (!yearItem) {
            yearItem = new MusicLibraryItem(MusicLibraryItem::ArtistItem, yearText, m_rootItem);
            yearItems[year] = yearItem;
            m_rootItem->appendChild(yearItem);
        }

        // Create track item
        MusicLibraryItem *trackItem = new MusicLibraryItem(MusicLibraryItem::TrackItem, track.title, yearItem);
        trackItem->setTrack(track);
        yearItem->appendChild(trackItem);
    }
}

QString MusicLibraryModel::formatDuration(int seconds) const
{
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

MusicLibraryItem *MusicLibraryModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        MusicLibraryItem *item = static_cast<MusicLibraryItem*>(index.internalPointer());
        if (item) {
            return item;
        }
    }
    return m_rootItem;
}
