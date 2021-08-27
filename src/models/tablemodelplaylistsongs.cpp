#include "tablemodelplaylistsongs.h"

#include <QFileInfo>
#include <QPainter>
#include <QSqlQuery>
#include <QSqlError>
#include <random>
#include <algorithm>
#include <QMimeData>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSvgRenderer>


TableModelPlaylistSongs::TableModelPlaylistSongs(TableModelBreakSongs &breakSongsModel, QObject *parent)
    : QAbstractTableModel(parent), m_breakSongsModel(breakSongsModel)
{
    m_logger = spdlog::get("logger");
}

QVariant TableModelPlaylistSongs::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COL_POSITION:
            return "Position";
        case COL_ARTIST:
            return "Artist";
        case COL_TITLE:
            return "Title";
        case COL_FILENAME:
            return "Filename";
        case COL_DURATION:
            return "Duration";
            default:
                return {};
        }
    }
    return {};
}

int TableModelPlaylistSongs::rowCount([[maybe_unused]]const QModelIndex &parent) const
{
    return static_cast<int>(m_songs.size());
}

int TableModelPlaylistSongs::columnCount([[maybe_unused]]const QModelIndex &parent) const
{
    return 7;
}

QVariant TableModelPlaylistSongs::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    if (role == Qt::UserRole)
        return m_songs.at(index.row()).id;
    if (role == Qt::ForegroundRole)
    {
        if (m_songs.at(index.row()).id == m_playingPlSongId && m_playingPlaylist == m_curPlaylistId)
            return QColor("black");
        return {};
    }
    if (role == Qt::BackgroundRole)
    {
        if (m_songs.at(index.row()).id == m_playingPlSongId && m_playingPlaylist == m_curPlaylistId)
            return (m_settings.theme() == 1) ? QColor(180,180,0) : QColor("yellow");
        return {};
    }
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case COL_ID:
            return m_songs.at(index.row()).id;
        case COL_POSITION:
            return m_songs.at(index.row()).position;
        case COL_ARTIST:
            return m_songs.at(index.row()).artist;
        case COL_TITLE:
            return m_songs.at(index.row()).title;
        case COL_FILENAME:
            return m_songs.at(index.row()).filename;
        case COL_DURATION:
            return QTime(0,0,0,0).addSecs(m_songs.at(index.row()).duration).toString("m:ss");
        case COL_PATH:
            return m_songs.at(index.row()).path;
        }
    }
    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column()) {
        case COL_DURATION:
            return Qt::AlignRight + Qt::AlignVCenter;
        default:
            return Qt::AlignLeft + Qt::AlignVCenter;
        }
    }
    return {};
}

void TableModelPlaylistSongs::setCurrentPlaylist(const int playlistId)
{
    emit layoutAboutToBeChanged();
    m_curPlaylistId = playlistId;
    m_songs.clear();
    QSqlQuery query;
    query.prepare("SELECT bmplsongs.plsongid, bmplsongs.artist, bmplsongs.position, bmsongs.artist, bmsongs.title, bmsongs.Filename, bmsongs.path, bmsongs.Duration FROM bmplsongs INNER JOIN bmsongs ON bmsongs.songid=bmplsongs.Artist WHERE bmplsongs.playlist = :playlistId ORDER BY bmplsongs.position");
    query.bindValue(":playlistId", playlistId);
    query.exec();
    while (query.next())
    {
        m_songs.push_back(PlaylistSong{
                              query.value(0).toInt(),
                              query.value(1).toInt(),
                              query.value(2).toInt(),
                              query.value(3).toString(),
                              query.value(4).toString(),
                              query.value(5).toString(),
                              query.value(6).toString(),
                              query.value(7).toInt()
                          });
    }
    emit layoutChanged();
    m_logger->info("{} Loaded {} songs from db on disk", m_loggingPrefix, m_songs.size());
}

void TableModelPlaylistSongs::setCurrentPosition(const int currentPos)
{
    emit layoutAboutToBeChanged();
    m_currentPosition = currentPos;
    m_playingPlSongId = getPlSongIdAtPos(currentPos);
    m_playingPlaylist = m_curPlaylistId;
    emit playingPlSongIdChanged(m_playingPlSongId);
    emit layoutChanged();
}

void TableModelPlaylistSongs::savePlaylistChanges()
{
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.prepare("DELETE FROM bmplsongs WHERE playlist = :playlist");
    query.bindValue(":playlist", m_curPlaylistId);
    query.exec();
    query.prepare("INSERT INTO bmplsongs (plsongid,playlist,position,artist,title,filename,duration,path)"
                  "VALUES(:plsongid,:playlist,:position,:bmsongid,:bmsongid,:bmsongid,:bmsongid,:bmsongid)");
    std::for_each(m_songs.begin(), m_songs.end(), [&] (PlaylistSong &song) {
        query.bindValue(":plsongid", song.id);
        query.bindValue(":playlist", m_curPlaylistId);
        query.bindValue(":position", song.position);
        query.bindValue(":bmsongid", song.breakSongId);
        query.exec();
    });
    query.exec("COMMIT");
    if (auto error = query.lastError(); error.type() != QSqlError::NoError)
        m_logger->error("{} DB error while saving playlist changes: {}", m_loggingPrefix, error.text().toStdString());
    else
        m_logger->debug("{} Playlist changes saved to db", m_loggingPrefix);
}

void TableModelPlaylistSongs::moveSong(const int oldPosition, const int newPosition)
{
    if (oldPosition == newPosition)
        return;
    emit layoutAboutToBeChanged();
    if (oldPosition > newPosition)
    {
        // moving up
        std::for_each(m_songs.begin(), m_songs.end(), [&oldPosition, &newPosition] (PlaylistSong &song) {
           if (song.position == oldPosition)
               song.position = newPosition;
           else if(song.position >= newPosition && song.position < oldPosition)
               song.position++;
        });
    }
    else
    {
        // moving down
        std::for_each(m_songs.begin(), m_songs.end(), [&oldPosition, &newPosition] (PlaylistSong &song) {
            if (song.position == oldPosition)
                song.position = newPosition;
            else if (song.position > oldPosition && song.position <= newPosition)
                song.position--;
        });
    }
    std::sort(m_songs.begin(), m_songs.end(), [] (PlaylistSong &a, PlaylistSong &b) {
        return (a.position < b.position);
    });
    emit layoutChanged();
}

void TableModelPlaylistSongs::addSong(const int songId)
{
   emit layoutAboutToBeChanged();
   BreakSong bs = m_breakSongsModel.getSong(songId);
   QSqlQuery query;
   query.prepare("INSERT INTO bmplsongs (playlist,position,artist,title,filename,duration,path)"
                 "VALUES(:playlist,:position,:bmsongid,:bmsongid,:bmsongid,:bmsongid,:bmsongid)");
   query.bindValue(":playlist", m_curPlaylistId);
   query.bindValue(":position", (int)m_songs.size());
   query.bindValue(":bmsongid", bs.id);
   query.exec();
   auto plSongId = query.lastInsertId();
   m_songs.emplace_back(PlaylistSong{
                            plSongId.toInt(),
                            bs.id,
                            (int)m_songs.size(),
                            bs.artist,
                            bs.title,
                            bs.filename,
                            bs.path,
                            bs.duration
                        });
   emit layoutChanged();
}

void TableModelPlaylistSongs::insertSong(const int songId, const int position)
{
    addSong(songId);
    moveSong(static_cast<int>(m_songs.size()) - 1, position);
}

void TableModelPlaylistSongs::deleteSong(const int position)
{
    m_logger->debug("{} Songs before delete: {}", m_songs.size());
    emit layoutAboutToBeChanged();
    auto it = std::remove_if(m_songs.begin(), m_songs.end(), [&position] (PlaylistSong &song) {
       return (song.position == position);
    });
    m_songs.erase(it, m_songs.end());
    std::for_each(m_songs.begin(), m_songs.end(), [&position] (PlaylistSong &song) {
       if (song.position > position)
           song.position--;
    });
    emit layoutChanged();
    m_logger->debug("{} Songs after delete: {}", m_songs.size());
}

int TableModelPlaylistSongs::currentPlaylist() const
{
    return m_curPlaylistId;
}

int TableModelPlaylistSongs::randomizePlaylist()
{
    emit layoutAboutToBeChanged();
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_songs.begin(), m_songs.end(), g);
    int songPos{0};
    int newCurPos{-1};
    std::for_each(m_songs.begin(), m_songs.end(), [&] (PlaylistSong &song) {
        if (song.position == m_currentPosition)
        {
            song.position = songPos++;
            newCurPos = song.position;
        }
        else
            song.position = songPos++;
    });
    std::sort(m_songs.begin(), m_songs.end(), [] (const PlaylistSong &a, const PlaylistSong &b) {
        return (a.position < b.position);
    });
    savePlaylistChanges();
    emit layoutChanged();
    setCurrentPosition(newCurPos);
    return newCurPos;
}

int TableModelPlaylistSongs::getPlSongIdAtPos(const int position) const
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&position] (const PlaylistSong& song) {
            return (song.position == position);
    });
    if (it == m_songs.end())
        return -1;
    return it->id;
}

int TableModelPlaylistSongs::getSongPositionById(const int plSongId) const
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&plSongId] (const PlaylistSong& song) {
            return (song.id == plSongId);
    });
    if (it == m_songs.end())
        return -1;
    return it->position;
}

void ItemDelegatePlaylistSongs::resizeIconsForFont(const QFont &font)
{
    QString thm = (m_settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_curFontHeight = QFontMetrics(font).height();
    m_iconDelete = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconPlaying = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconDelete.fill(Qt::transparent);
    m_iconPlaying.fill(Qt::transparent);
    QPainter painterDelete(&m_iconDelete);
    QPainter painterPlaying(&m_iconPlaying);
    QSvgRenderer svgrndrDelete(thm + "actions/16/edit-delete.svg");
    QSvgRenderer svgrndrPlaying(thm + "actions/22/media-playback-start.svg");
    svgrndrDelete.render(&painterDelete);
    svgrndrPlaying.render(&painterPlaying);
}

ItemDelegatePlaylistSongs::ItemDelegatePlaylistSongs(QObject *parent) :
    QItemDelegate(parent)
{
    resizeIconsForFont(m_settings.applicationFont());
}

void ItemDelegatePlaylistSongs::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == TableModelPlaylistSongs::COL_ID)
    {
        if (index.data(Qt::UserRole).toInt() == m_playingPlSongId)
        {
            int topPad = (option.rect.height() - m_curFontHeight) / 2;
            int leftPad = (option.rect.width() - m_curFontHeight) / 2;
            painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconPlaying);
        }
        return;
    }
    if (index.column() == TableModelPlaylistSongs::COL_PATH)
    {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconDelete);
        return;
    }
    QItemDelegate::paint(painter, option, index);
}

void ItemDelegatePlaylistSongs::setPlayingPlSongId(int plSongId) {
    m_playingPlSongId = plSongId;
}

QStringList TableModelPlaylistSongs::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/queuepos";
    types << "application/vnd.bmsongid.list";
    types << "application/plsongids";
    return types;
}

QMimeData *TableModelPlaylistSongs::mimeData(const QModelIndexList &indexes) const
{
    auto mimeData = new QMimeData();
    mimeData->setData("integer/queuepos", indexes.at(0).sibling(indexes.at(0).row(), COL_POSITION).data().toByteArray().data());
    if (indexes.size() > 1)
    {
        QJsonArray jArr;
        std::for_each(indexes.begin(), indexes.end(), [&] (QModelIndex index) {
            // Just using 3 here because it's the first column that's included in the index list
            if (index.column() != 3)
                return;
            jArr.append(index.sibling(index.row(), COL_ID).data().toInt());
        });
        QJsonDocument jDoc(jArr);
        mimeData->setData("application/plsongids", jDoc.toJson());
    }
    return mimeData;
}

bool TableModelPlaylistSongs::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (data->hasFormat("application/plsongids") && action == Qt::MoveAction)
        return true;
    if (data->hasFormat("application/vnd.bmsongid.list") && action == Qt::CopyAction)
        return true;
    return false;
}

bool TableModelPlaylistSongs::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column)

    if (data->hasFormat("application/plsongids") && action == Qt::MoveAction)
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(data->data("application/plsongids"));
        QJsonArray jArr = jDoc.array();
        auto ids = jArr.toVariantList();
        int droprow{0};
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = static_cast<int>(m_songs.size()) - 1;
        if (getSongPositionById(ids.at(0).toInt()) > droprow)
            std::reverse(ids.begin(),ids.end());
        std::for_each(ids.begin(), ids.end(), [&] (auto val) {
            int oldPosition = getSongPositionById(val.toInt());
            if (oldPosition < droprow && droprow != m_songs.size() - 1)
                moveSong(oldPosition, droprow - 1);
            else
                moveSong(oldPosition, droprow);
        });
        if (droprow == rowCount() - 1)
        {
            // moving to bottom
            emit bmPlSongsMoved(droprow - ids.size() + 1, 0, rowCount() - 1, columnCount() - 1);
        }
        else if (getSongPositionById(ids.at(0).toInt()) < droprow)
        {
            // moving down
            emit bmPlSongsMoved(droprow - ids.size(), 0, droprow - 1, columnCount() - 1);
        }
        else
        {
            // moving up
            emit bmPlSongsMoved(droprow, 0, droprow + ids.size() - 1, columnCount() - 1);
        }
        savePlaylistChanges();
        return true;
    }
    if (data->hasFormat("application/vnd.bmsongid.list") && action == Qt::CopyAction)
    {
        QByteArray encodedData = data->data("application/vnd.bmsongid.list");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<int> songids;
        stream >> songids;
        unsigned int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount();
        for (int i = songids.size() -1; i >= 0; i--)
        {
            insertSong(songids.at(i), static_cast<int>(droprow));
        }
        savePlaylistChanges();
        return true;
    }
    return false;
}

Qt::DropActions TableModelPlaylistSongs::supportedDropActions() const
{
       return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags TableModelPlaylistSongs::flags([[maybe_unused]]const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

std::optional<std::reference_wrapper<PlaylistSong>> TableModelPlaylistSongs::getPlSong(const int plSongId)
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&plSongId] (const PlaylistSong& song) {
        return (song.id == plSongId);
    });
    if (it != m_songs.end())
        return *it;
    return std::nullopt;
}

std::optional<std::reference_wrapper<PlaylistSong>> TableModelPlaylistSongs::getPlSongByPosition(const int position) {
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&position] (const PlaylistSong& song) {
        return (song.position == position);
    });
    if (it == m_songs.end()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<std::reference_wrapper<PlaylistSong>> TableModelPlaylistSongs::getNextPlSong() {
    if (m_songs.empty())
        return std::nullopt;
    if (m_playingPlaylist != m_curPlaylistId)
        return getPlSongByPosition(0);
    auto curSong = getCurrentSong();
    if (!curSong.has_value())
        return getPlSongByPosition(0);
    if (curSong->get().position < m_songs.size() - 1)
        return getPlSongByPosition(curSong->get().position + 1);
    return getPlSongByPosition(0);
}

std::optional<std::reference_wrapper<PlaylistSong>> TableModelPlaylistSongs::getCurrentSong() {
    return getPlSong(m_playingPlSongId);
}

bool TableModelPlaylistSongs::isCurrentlyPlayingSong(int plSongId) const {
    return (plSongId == m_playingPlSongId && m_curPlaylistId == m_playingPlaylist);
}
