#ifndef TABLEMODELPLAYLISTSONGSNEW_H
#define TABLEMODELPLAYLISTSONGSNEW_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QItemDelegate>
#include <QStyleOptionViewItem>
#include <optional>
#include "tablemodelbreaksongs.h"
#include "settings.h"
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>

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
    int m_playingPlSongId{-1};
    QImage m_iconDelete;
    QImage m_iconPlaying;
    int m_curFontHeight{0};
    Settings m_settings;

public:
    explicit ItemDelegatePlaylistSongs(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setPlayingPlSongId(int plSongId);

public slots:
    void resizeIconsForFont(const QFont &font);

};

class TableModelPlaylistSongs : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {COL_ID=0,COL_POSITION,COL_ARTIST,COL_TITLE,COL_FILENAME,COL_DURATION,COL_PATH};
    explicit TableModelPlaylistSongs(TableModelBreakSongs &breakSongsModel, QObject *parent = nullptr);
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData *mimeData(const QModelIndexList &indexes) const override;
    [[nodiscard]] bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    [[nodiscard]] bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] bool isCurrentlyPlayingSong(int plSongId) const;
    void setCurrentPlaylist(int playlistId);
    void setCurrentPosition(int currentPos);
    [[nodiscard]] int currentPosition() const { return m_currentPosition; }
    void savePlaylistChanges();
    void moveSong(int oldPosition, int newPosition);
    void addSong(int songId);
    void insertSong(int songId, int position);
    void deleteSong(int position);
    [[nodiscard]] int currentPlaylist() const;
    int randomizePlaylist();
    [[nodiscard]] int getPlSongIdAtPos(int position) const;
    [[nodiscard]] std::optional<std::reference_wrapper<PlaylistSong>> getPlSong(int plSongId);
    [[nodiscard]] std::optional<std::reference_wrapper<PlaylistSong>> getPlSongByPosition(int position);
    [[nodiscard]] std::optional<std::reference_wrapper<PlaylistSong>> getNextPlSong();
    [[nodiscard]] std::optional<std::reference_wrapper<PlaylistSong>> getCurrentSong();
    [[nodiscard]] int getSongPositionById(int plSongId) const;


private:
    std::string m_loggingPrefix{"[PlaylistSongsModel]"};
    std::shared_ptr<spdlog::logger> m_logger;
    std::vector<PlaylistSong> m_songs;
    TableModelBreakSongs &m_breakSongsModel;
    int m_curPlaylistId{0};
    int m_playingPlaylist{0};
    int m_currentPosition{-1};
    int m_playingPlSongId{-1};
    Settings m_settings;

signals:
    void bmSongMoved(int oldPos, int newPos);
    void bmPlSongsMoved(int startRow, int startCol, int endRow, int endCol);
    void playingPlSongIdChanged(int plSongId);

};


#endif // TABLEMODELPLAYLISTSONGSNEW_H
