#ifndef MUSICLIBRARYMODEL_H
#define MUSICLIBRARYMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QStringList>
#include "databasemanager.h"

class MusicLibraryItem
{
public:
    enum ItemType {
        RootItem,
        ArtistItem,
        AlbumItem,
        TrackItem
    };

    explicit MusicLibraryItem(ItemType type, const QString &data = QString(),
                             MusicLibraryItem *parent = nullptr);
    ~MusicLibraryItem();

    void appendChild(MusicLibraryItem *child);
    void removeChild(int row);
    void clearChildren();

    MusicLibraryItem *child(int row) const;
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    MusicLibraryItem *parentItem() const;
    void setParentItem(MusicLibraryItem *parent) { m_parentItem = parent; }

    ItemType type() const { return m_type; }
    QString text() const { return m_text; }
    void setText(const QString &text) { m_text = text; }

    // For track items
    MusicTrack track() const { return m_track; }
    void setTrack(const MusicTrack &track) { m_track = track; }

private:
    QList<MusicLibraryItem*> m_childItems;
    MusicLibraryItem *m_parentItem;
    ItemType m_type;
    QString m_text;
    MusicTrack m_track;
};

class MusicLibraryModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column {
        TitleColumn = 0,
        ArtistColumn,
        AlbumColumn,
        GenreColumn,
        YearColumn,
        TrackColumn,
        DurationColumn,
        ColumnCount
    };

    explicit MusicLibraryModel(DatabaseManager *dbManager, QObject *parent = nullptr);
    ~MusicLibraryModel();

    // QAbstractItemModel interface
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                       int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                     const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // Custom methods
    void refreshData();
    void searchTracks(const QString &searchTerm);
    void showAllTracks();
    MusicTrack getTrack(const QModelIndex &index) const;
    void addTrackToModel(const MusicTrack &track);
    void updateTrackInModel(const MusicTrack &track);

    enum SortMode {
        SortByArtistAlbum,
        SortByAlbum,
        SortByGenre,
        SortByYear
    };

    void setSortMode(SortMode mode);

private:
    DatabaseManager *m_dbManager;
    MusicLibraryItem *m_rootItem;
    SortMode m_sortMode;
    QString m_currentSearchTerm;

    void setupModelData();
    void buildArtistAlbumTree(const QList<MusicTrack> &tracks);
    void buildAlbumTree(const QList<MusicTrack> &tracks);
    void buildGenreTree(const QList<MusicTrack> &tracks);
    void buildYearTree(const QList<MusicTrack> &tracks);

    QString formatDuration(int seconds) const;
    MusicLibraryItem *getItem(const QModelIndex &index) const;
};

#endif // MUSICLIBRARYMODEL_H
