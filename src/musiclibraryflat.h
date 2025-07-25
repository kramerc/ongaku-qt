#ifndef MUSICLIBRARYFLAT_H
#define MUSICLIBRARYFLAT_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include "databasemanager.h"

class MusicLibraryFlatModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        TitleColumn = 0,
        ArtistColumn,
        AlbumColumn,
        GenreColumn,
        PublisherColumn,
        CatalogNumberColumn,
        YearColumn,
        TrackColumn,
        DurationColumn,
        ColumnCount
    };

    explicit MusicLibraryFlatModel(DatabaseManager *dbManager, QObject *parent = nullptr);
    ~MusicLibraryFlatModel() = default;

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // Custom methods
    void refreshData();
    void searchTracks(const QString &searchTerm);
    void showAllTracks();
    MusicTrack getTrack(const QModelIndex &index) const;
    MusicTrack getTrack(int row) const;

private:
    DatabaseManager *m_dbManager;
    QList<MusicTrack> m_tracks;
    QString m_currentSearchTerm;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;

    QString formatDuration(int seconds) const;
    void sortTracks();
    bool trackLessThan(const MusicTrack &left, const MusicTrack &right) const;
};

// Proxy model for additional filtering if needed
class MusicLibraryFlatProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit MusicLibraryFlatProxyModel(QObject *parent = nullptr);

    void setSearchFilter(const QString &searchTerm);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_searchTerm;
};

#endif // MUSICLIBRARYFLAT_H
