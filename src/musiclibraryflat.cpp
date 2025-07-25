#include "musiclibraryflat.h"
#include <QFont>
#include <QDebug>
#include <algorithm>

// MusicLibraryFlatModel implementation
MusicLibraryFlatModel::MusicLibraryFlatModel(DatabaseManager *dbManager, QObject *parent)
    : QAbstractTableModel(parent)
    , m_dbManager(dbManager)
    , m_sortColumn(TitleColumn)
    , m_sortOrder(Qt::AscendingOrder)
{
    refreshData();
}

int MusicLibraryFlatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_tracks.size();
}

int MusicLibraryFlatModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnCount;
}

QVariant MusicLibraryFlatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tracks.size()) {
        return QVariant();
    }

    const MusicTrack &track = m_tracks.at(index.row());

    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case TitleColumn:
                    return track.title;
                case ArtistColumn:
                    return track.artist;
                case AlbumColumn:
                    return track.album;
                case GenreColumn:
                    return track.genre;
                case YearColumn:
                    return track.year > 0 ? QString::number(track.year) : QString();
                case TrackColumn:
                    return track.track > 0 ? QString::number(track.track) : QString();
                case DurationColumn:
                    return formatDuration(track.duration);
                default:
                    return QVariant();
            }
            break;

        case Qt::UserRole:
            // Return the track data for easy access
            return QVariant::fromValue(track);

        case Qt::TextAlignmentRole:
            switch (index.column()) {
                case YearColumn:
                case TrackColumn:
                case DurationColumn:
                    return QVariant(Qt::AlignCenter);
                default:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
            }
            break;

        default:
            return QVariant();
    }
}

QVariant MusicLibraryFlatModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case TitleColumn:
                    return "Title";
                case ArtistColumn:
                    return "Artist";
                case AlbumColumn:
                    return "Album";
                case GenreColumn:
                    return "Genre";
                case YearColumn:
                    return "Year";
                case TrackColumn:
                    return "Track";
                case DurationColumn:
                    return "Duration";
                default:
                    return QVariant();
            }
            break;

        case Qt::FontRole: {
            QFont font;
            font.setBold(true);
            return font;
        }

        default:
            return QVariant();
    }
}

bool MusicLibraryFlatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    // For now, we don't allow editing
    return false;
}

Qt::ItemFlags MusicLibraryFlatModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void MusicLibraryFlatModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= ColumnCount) {
        return;
    }

    beginResetModel();
    m_sortColumn = column;
    m_sortOrder = order;
    sortTracks();
    endResetModel();
}

void MusicLibraryFlatModel::refreshData()
{
    beginResetModel();

    if (m_currentSearchTerm.isEmpty()) {
        m_tracks = m_dbManager->getAllTracks();
    } else {
        m_tracks = m_dbManager->searchTracks(m_currentSearchTerm);
    }

    sortTracks();
    endResetModel();
}

void MusicLibraryFlatModel::searchTracks(const QString &searchTerm)
{
    beginResetModel();
    m_currentSearchTerm = searchTerm.trimmed();

    if (m_currentSearchTerm.isEmpty()) {
        m_tracks = m_dbManager->getAllTracks();
    } else {
        m_tracks = m_dbManager->searchTracks(m_currentSearchTerm);
    }

    sortTracks();
    endResetModel();
}

void MusicLibraryFlatModel::showAllTracks()
{
    searchTracks(QString());
}

MusicTrack MusicLibraryFlatModel::getTrack(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_tracks.size()) {
        return MusicTrack();
    }
    return m_tracks.at(index.row());
}

MusicTrack MusicLibraryFlatModel::getTrack(int row) const
{
    if (row < 0 || row >= m_tracks.size()) {
        return MusicTrack();
    }
    return m_tracks.at(row);
}

QString MusicLibraryFlatModel::formatDuration(int seconds) const
{
    if (seconds <= 0) {
        return QString();
    }

    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void MusicLibraryFlatModel::sortTracks()
{
    std::sort(m_tracks.begin(), m_tracks.end(), [this](const MusicTrack &left, const MusicTrack &right) {
        return trackLessThan(left, right);
    });
}

bool MusicLibraryFlatModel::trackLessThan(const MusicTrack &left, const MusicTrack &right) const
{
    QVariant leftValue, rightValue;

    switch (m_sortColumn) {
        case TitleColumn:
            leftValue = left.title;
            rightValue = right.title;
            break;
        case ArtistColumn:
            leftValue = left.artist;
            rightValue = right.artist;
            break;
        case AlbumColumn:
            leftValue = left.album;
            rightValue = right.album;
            break;
        case GenreColumn:
            leftValue = left.genre;
            rightValue = right.genre;
            break;
        case YearColumn:
            leftValue = left.year;
            rightValue = right.year;
            break;
        case TrackColumn:
            leftValue = left.track;
            rightValue = right.track;
            break;
        case DurationColumn:
            leftValue = left.duration;
            rightValue = right.duration;
            break;
        default:
            leftValue = left.title;
            rightValue = right.title;
            break;
    }

    // Handle different data types appropriately
    if (leftValue.userType() == QMetaType::Int) {
        int leftInt = leftValue.toInt();
        int rightInt = rightValue.toInt();
        if (m_sortOrder == Qt::AscendingOrder) {
            return leftInt < rightInt;
        } else {
            return leftInt > rightInt;
        }
    } else {
        QString leftStr = leftValue.toString().toLower();
        QString rightStr = rightValue.toString().toLower();
        if (m_sortOrder == Qt::AscendingOrder) {
            return leftStr < rightStr;
        } else {
            return leftStr > rightStr;
        }
    }
}

// MusicLibraryFlatProxyModel implementation
MusicLibraryFlatProxyModel::MusicLibraryFlatProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(-1); // Filter all columns
}

void MusicLibraryFlatProxyModel::setSearchFilter(const QString &searchTerm)
{
    m_searchTerm = searchTerm.trimmed().toLower();
    invalidateFilter();
}

bool MusicLibraryFlatProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);

    if (m_searchTerm.isEmpty()) {
        return true;
    }

    // Check all columns for the search term
    for (int column = 0; column < sourceModel()->columnCount(); ++column) {
        QModelIndex index = sourceModel()->index(sourceRow, column);
        QString data = sourceModel()->data(index, Qt::DisplayRole).toString().toLower();
        if (data.contains(m_searchTerm)) {
            return true;
        }
    }

    return false;
}

bool MusicLibraryFlatProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // Get the raw data for comparison
    QVariant leftData = sourceModel()->data(left, Qt::DisplayRole);
    QVariant rightData = sourceModel()->data(right, Qt::DisplayRole);

    // Handle numeric columns specially
    if (left.column() == MusicLibraryFlatModel::YearColumn ||
        left.column() == MusicLibraryFlatModel::TrackColumn ||
        left.column() == MusicLibraryFlatModel::DurationColumn) {
        return leftData.toInt() < rightData.toInt();
    }

    // String comparison for other columns
    return QString::localeAwareCompare(leftData.toString(), rightData.toString()) < 0;
}
