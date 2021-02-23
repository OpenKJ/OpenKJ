#ifndef TABLEMODELPLAYLISTSONGSNEW_H
#define TABLEMODELPLAYLISTSONGSNEW_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QItemDelegate>
#include <QStyleOptionViewItem>
#include "tablemodelbreaksongs.h"


struct PlaylistSong {
    int id{0};
    int breakSongId{0};
    int position{0};
    QString artist;
    QString title;
    QString filename;
    QString path;
    int duration;
};

class ItemDelegatePlaylistSongs : public QItemDelegate
{
    Q_OBJECT
private:
    int m_currentSong;
    QImage m_iconDelete;
    QImage m_iconPlaying;
    int m_curFontHeight;
    void resizeIconsForFont(const QFont &font);
public:
    explicit ItemDelegatePlaylistSongs(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int currentSong() const;
    void setCurrentSong(int value);
};

class TableModelPlaylistSongs : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {COL_ID=0,COL_POSITION,COL_ARTIST,COL_TITLE,COL_FILENAME,COL_DURATION,COL_PATH};
    explicit TableModelPlaylistSongs(TableModelBreakSongs &breakSongsModel, QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setCurrentPlaylist(const int playlistId);
    void setCurrentSongPos(const int currentPos);;
    void savePlaylistChanges();
    void moveSong(const int oldPosition, const int newPosition);
    void addSong(const int songId);
    void insertSong(const int songId, const int position);
    void deleteSong(const int position);
    int currentPlaylist() const;
    int getSongIdByFilePath(const QString &filePath) const;
    int numSongs() const;
    int randomizePlaylist(const int currentpos);
    int getPlSongIdAtPos(const int position) const;
    int getSongPositionById(const int plSongId) const;


private:
    std::vector<PlaylistSong> m_songs;
    TableModelBreakSongs &m_breakSongsModel;
    int m_curPlaylistId;
    int m_currentSongPos;

signals:
    void bmSongMoved(int oldPos, int newPos);
    void bmPlSongsMoved(const int startRow, const int startCol, const int endRow, const int endCol);

};


#endif // TABLEMODELPLAYLISTSONGSNEW_H
