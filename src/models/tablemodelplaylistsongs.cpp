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
#include "settings.h"

extern Settings settings;


TableModelPlaylistSongs::TableModelPlaylistSongs(TableModelBreakSongs &breakSongsModel, QObject *parent)
    : QAbstractTableModel(parent), m_breakSongsModel(breakSongsModel)
{
}

QVariant TableModelPlaylistSongs::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COL_ID:
            return QVariant();
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
        case COL_PATH:
            return QVariant();
        }
    }
    return QVariant();
}

int TableModelPlaylistSongs::rowCount([[maybe_unused]]const QModelIndex &parent) const
{
    return m_songs.size();
}

int TableModelPlaylistSongs::columnCount([[maybe_unused]]const QModelIndex &parent) const
{
    return 7;
}

QVariant TableModelPlaylistSongs::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::ForegroundRole)
    {
        if (index.row() == m_currentPosition)
            return QColor("black");
        return QVariant();
    }
    if (role == Qt::BackgroundRole)
    {
        if (index.row() == m_currentPosition)
            return (settings.theme() == 1) ? QColor(180,180,0) : QColor("yellow");
        return QVariant();
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
    return QVariant();
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
    qInfo() << "Loaded " << m_songs.size() << "songs";
}

void TableModelPlaylistSongs::setCurrentPosition(const int currentPos)
{
    emit layoutAboutToBeChanged();
    m_currentPosition = currentPos;
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
    qInfo() << "Break playlist - Saved playlist changes to database";
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
    moveSong(m_songs.size() - 1, position);
}

void TableModelPlaylistSongs::deleteSong(const int position)
{
    qInfo() << "songs before delete: " << m_songs.size();
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
    qInfo() << "songs after delete" << m_songs.size();
}

int TableModelPlaylistSongs::currentPlaylist() const
{
    return m_curPlaylistId;
}

int TableModelPlaylistSongs::getSongIdByFilePath(const QString &filePath) const
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&filePath] (PlaylistSong song) {
            return (song.path == filePath);
    });
    return it->id;
}

int TableModelPlaylistSongs::numSongs() const
{
    return m_songs.size();
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
    savePlaylistChanges();
    emit layoutChanged();
    m_currentPosition = newCurPos;
    return newCurPos;
}

int TableModelPlaylistSongs::getPlSongIdAtPos(const int position) const
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&position] (PlaylistSong song) {
            return (song.position == position);
    });
    return it->position;
}

int TableModelPlaylistSongs::getSongPositionById(const int plSongId) const
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&plSongId] (PlaylistSong song) {
            return (song.id == plSongId);
    });
    return it->position;
}

int ItemDelegatePlaylistSongs::currentSong() const
{
    return m_currentSong;
}

void ItemDelegatePlaylistSongs::setCurrentPosition(int value)
{
    m_currentSong = value;
}

void ItemDelegatePlaylistSongs::resizeIconsForFont(const QFont &font)
{
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
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
    m_currentSong = -1;
    resizeIconsForFont(settings.applicationFont());
}

void ItemDelegatePlaylistSongs::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == TableModelPlaylistSongs::COL_ID)
    {
        if (index.row() == m_currentSong)
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
    QMimeData *mimeData = new QMimeData();
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
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
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
            droprow = rowCount() - 1;
        if (getSongPositionById(ids.at(0).toInt()) > droprow)
            std::reverse(ids.begin(),ids.end());
        std::for_each(ids.begin(), ids.end(), [&] (auto val) {
            int oldPosition = getSongPositionById(val.toInt());
            if (oldPosition < droprow && droprow != rowCount() - 1)
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
        qInfo() << songids;
        unsigned int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount();
        for (int i = songids.size() -1; i >= 0; i--)
        {
            insertSong(songids.at(i), droprow);
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

PlaylistSong &TableModelPlaylistSongs::getPlSong(const int plSongId)
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&plSongId] (PlaylistSong song) {
        return (song.id == plSongId);
    });
    return *it;
}

PlaylistSong &TableModelPlaylistSongs::getPlSongByPosition(const int position) {
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&position] (PlaylistSong song) {
        return (song.position == position);
    });
    if (it == m_songs.end()) {
        qWarning() << "TableModelPlaylistSongs - Something went wrong getting song by position, returning first song to avoid crash";
        qWarning() << "TableModelPlaylistSongs - The position that was requested was " << position;
        return *m_songs.begin();
    }
    return *it;
}

PlaylistSong &TableModelPlaylistSongs::getNextPlSong() {
    if (m_currentPosition < m_songs.size() - 1)
        return getPlSongByPosition(m_currentPosition + 1);
    return getPlSongByPosition(0);
}

PlaylistSong &TableModelPlaylistSongs::getCurrentSong() {
    return getPlSongByPosition(m_currentPosition);
}
