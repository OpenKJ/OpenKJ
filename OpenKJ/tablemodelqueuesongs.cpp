#include "tablemodelqueuesongs.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QTime>
#include <QMimeData>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <QSvgRenderer>
#include "settings.h"

extern Settings settings;

TableModelQueueSongs::TableModelQueueSongs(TableModelKaraokeSongs &karaokeSongsModel, QObject *parent)
    : QAbstractTableModel(parent), m_karaokeSongsModel(karaokeSongsModel)
{
}

QVariant TableModelQueueSongs::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COL_ID:
            return "ID";
        case COL_DBSONGID:
            return "DBSongId";
        case COL_ARTIST:
            return "Artist";
        case COL_TITLE:
            return "Title";
        case COL_SONGID:
            return "SongID";
        case COL_KEY:
            return "Key";
        case COL_DURATION:
            return "Duration";
        case COL_PATH:
            return QVariant();
        }
    }
    return QVariant();
}

int TableModelQueueSongs::rowCount([[maybe_unused]]const QModelIndex &parent) const
{
    return m_songs.size();
}

int TableModelQueueSongs::columnCount([[maybe_unused]]const QModelIndex &parent) const
{
    return 8;
}

QVariant TableModelQueueSongs::data(const QModelIndex &index, int role) const
{

    if (role == Qt::FontRole)
    {
        if (m_songs.at(index.row()).played)
        {
           auto font = settings.applicationFont();
           font.setStrikeOut(true);
           return font;
        }
    }
    if (role == Qt::ForegroundRole)
    {
        if (m_songs.at(index.row()).played)
        {
            return QColor("darkGrey");
        }
    }
    if (role == Qt::TextAlignmentRole)
    {
        if (index.column() == COL_KEY)
            return Qt::AlignHCenter + Qt::AlignVCenter;
        if (index.column() == COL_DURATION)
            return Qt::AlignRight + Qt::AlignVCenter;
    }
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case COL_ID:
            return m_songs.at(index.row()).id;
        case COL_DBSONGID:
            return m_songs.at(index.row()).dbSongId;
        case COL_ARTIST:
            return m_songs.at(index.row()).artist;
        case COL_TITLE:
            return m_songs.at(index.row()).title;
        case COL_SONGID:
            return m_songs.at(index.row()).songId;
        case COL_KEY:
            if (m_songs.at(index.row()).keyChange == 0)
                return QVariant();
            else if (m_songs.at(index.row()).keyChange > 0)
                return "+" + QString::number(m_songs.at(index.row()).keyChange);
            else
                return m_songs.at(index.row()).keyChange;
        case COL_DURATION:
            return QTime(0,0,0,0).addMSecs(m_songs.at(index.row()).duration).toString("m:ss");
        case COL_PATH:
            return m_songs.at(index.row()).path;
            break;

        }
    }
    return QVariant();
}

void TableModelQueueSongs::loadSinger(const int singerId)
{
    qInfo() << "loadSinger( " << singerId << " ) fired";
    emit layoutAboutToBeChanged();
    m_songs.clear();
    m_songs.shrink_to_fit();
    m_curSingerId = singerId;
    QSqlQuery query;
    query.prepare("SELECT queuesongs.qsongid, queuesongs.singer, queuesongs.song, queuesongs.played, "
                  "queuesongs.keychg, queuesongs.position, rotationsingers.name, dbsongs.artist, "
                  "dbsongs.title, dbsongs.discid, dbsongs.duration, dbsongs.path FROM queuesongs "
                  "INNER JOIN rotationsingers ON rotationsingers.singerid = queuesongs.singer "
                  "INNER JOIN dbsongs ON dbsongs.songid = queuesongs.song WHERE queuesongs.singer = :singerId");
    query.bindValue(":singerId", singerId);
    query.exec();
    qInfo() << query.lastError();
    //qInfo() << query.lastQuery();
    qInfo() << "quey returned " << query.size() << " rows";
    while (query.next())
    {
        m_songs.emplace_back(QueueSong{
                                query.value(0).toInt(),
                                 query.value(1).toInt(),
                                 query.value(2).toInt(),
                                 query.value(3).toBool(),
                                 query.value(4).toInt(),
                                 query.value(5).toInt(),
                                 query.value(7).toString(),
                                 query.value(8).toString(),
                                 query.value(9).toString(),
                                 query.value(10).toInt(),
                                 query.value(11).toString()
                             });
    }
    emit layoutChanged();
}

int TableModelQueueSongs::getPosition(const int songId)
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&songId] (QueueSong &song)
    {
        return (song.id == songId);
    });
    return it->position;
}

bool TableModelQueueSongs::getPlayed(const int songId)
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&songId] (QueueSong &song)
    {
        return (song.id == songId);
    });
    return it->played;
}

int TableModelQueueSongs::getKey(const int songId)
{
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&songId] (QueueSong &song)
    {
        return (song.id == songId);
    });
    return it->keyChange;
}

void TableModelQueueSongs::move(const int oldPosition, const int newPosition)
{
    if (oldPosition == newPosition)
        return;
    emit layoutAboutToBeChanged();
    if (oldPosition > newPosition)
    {
        // moving up
        std::for_each(m_songs.begin(), m_songs.end(), [&oldPosition, &newPosition] (QueueSong &song) {
           if (song.position == oldPosition)
               song.position = newPosition;
           else if(song.position >= newPosition && song.position < oldPosition)
               song.position++;
        });
    }
    else
    {
        // moving down
        std::for_each(m_songs.begin(), m_songs.end(), [&oldPosition, &newPosition] (QueueSong &song) {
            if (song.position == oldPosition)
                song.position = newPosition;
            else if (song.position > oldPosition && song.position <= newPosition)
                song.position--;
        });
    }
    std::sort(m_songs.begin(), m_songs.end(), [] (QueueSong &a, QueueSong &b) {
        return (a.position < b.position);
    });
    emit layoutChanged();
    commitChanges();
    emit queueModified(m_curSingerId);
}

void TableModelQueueSongs::moveSongId(const int songId, const int newPosition)
{
    move(getPosition(songId), newPosition);
}

int TableModelQueueSongs::add(const int songId)
{
    KaraokeSong ksong = m_karaokeSongsModel.getSong(songId);
    QSqlQuery query;
    query.prepare("INSERT INTO queuesongs (singer,song,artist,title,discid,path,keychg,played,position) "
                  "VALUES (:singerId,:songId,:songId,:songId,:songId,:songId,:key,:played,:position)");
    query.bindValue(":singerId", m_curSingerId);
    query.bindValue(":songId", songId);
    query.bindValue(":key", 0);
    query.bindValue(":played", false);
    query.bindValue(":position", (int)m_songs.size());
    query.exec();
    auto queueSongId = query.lastInsertId().toInt();
    emit layoutAboutToBeChanged();
    m_songs.emplace_back(QueueSong{
                             queueSongId,
                             m_curSingerId,
                             songId,
                             false,
                             0,
                             (int)m_songs.size(),
                             ksong.artist,
                             ksong.title,
                             ksong.songid,
                             ksong.duration,
                             ksong.path
                         });
    emit layoutChanged();
    emit queueModified(m_curSingerId);
    return queueSongId;
}

void TableModelQueueSongs::insert(const int songId, const int position)
{
    add(songId);
    move(m_songs.size() - 1, position);
}

void TableModelQueueSongs::remove(const int songId)
{
    qInfo() << "songs before delete: " << m_songs.size();
    emit layoutAboutToBeChanged();
    auto it = std::remove_if(m_songs.begin(), m_songs.end(), [&songId] (QueueSong &song) {
       return (song.id == songId);
    });
    m_songs.erase(it, m_songs.end());
    int pos{0};
    std::for_each(m_songs.begin(), m_songs.end(), [&pos] (QueueSong &song) {
       song.position = pos++;
    });
    emit layoutChanged();
    qInfo() << "songs after delete" << m_songs.size();
    commitChanges();
    emit queueModified(m_curSingerId);
}

void TableModelQueueSongs::setKey(const int songId, const int semitones)
{
    QSqlQuery query;
    query.prepare("UPDATE queuesongs SET keychg = :key WHERE qsongid = :id");
    query.bindValue(":id", songId);
    query.bindValue(":key", semitones);
    query.exec();
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&songId] (QueueSong &song)
    {
        return (song.id == songId);
    });
    it->keyChange = semitones;
    emit dataChanged(this->index(it->position, COL_KEY),this->index(it->position, COL_KEY),QVector<int>{Qt::DisplayRole});
}

void TableModelQueueSongs::setPlayed(const int songId, const bool played)
{
    qInfo() << "Setting songId " << songId << " to played = " << played;
    QSqlQuery query;
    query.prepare("UPDATE queuesongs SET played = :played WHERE qsongid = :id");
    query.bindValue(":id", songId);
    query.bindValue(":played", played);
    query.exec();
    auto it = std::find_if(m_songs.begin(), m_songs.end(), [&songId] (QueueSong &song)
    {
        return (song.id == songId);
    });
    it->played = played;
    emit dataChanged(this->index(it->position, 0),this->index(it->position, columnCount() - 1),QVector<int>{Qt::FontRole,Qt::BackgroundRole,Qt::ForegroundRole});
    emit queueModified(m_curSingerId);
}

void TableModelQueueSongs::removeAll()
{
    emit layoutAboutToBeChanged();
    QSqlQuery query;
    query.prepare("DELETE FROM queuesongs WHERE singer = :singerId");
    query.bindValue(":singerId", m_curSingerId);
    query.exec();
    m_songs.clear();
    m_songs.shrink_to_fit();
    emit layoutChanged();
    emit queueModified(m_curSingerId);
}

void TableModelQueueSongs::commitChanges()
{
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.prepare("DELETE FROM queuesongs WHERE singer = :singerId");
    query.bindValue(":singerId", m_curSingerId);
    query.exec();
    query.prepare("INSERT INTO queuesongs (qsongid,singer,song,artist,title,discid,path,keychg,played,position) "
                  "VALUES(:id,:singerId,:songId,:songId,:songId,:songId,:songId,:key,:played,:position)");
    std::for_each(m_songs.begin(), m_songs.end(), [&] (QueueSong &song)
    {
        query.bindValue(":id", song.id);
        query.bindValue(":singerId", song.singerId);
        query.bindValue(":songId", song.dbSongId);
        query.bindValue(":key", song.keyChange);
        query.bindValue(":played", song.played);
        query.bindValue(":position", song.position);
        query.exec();
    });
    query.exec("COMMIT");
}

void TableModelQueueSongs::songAddSlot(int songId, int singerId, int keyChg)
{
    qInfo() << "TableModelQueueSongs::songAddSlot(" << songId << ", " << singerId << ", " << keyChg << ") called";
    if (singerId == m_curSingerId)
    {
        int queueSongId = add(songId);
        setKey(queueSongId, keyChg);
    }
    else
    {
        int newPos{0};
        KaraokeSong ksong = m_karaokeSongsModel.getSong(songId);
        QSqlQuery query;
        query.prepare("SELECT COUNT(id) FROM queuesongs WHERE singer = :singerId");
        query.bindValue(":singerId", singerId);
        query.exec();
        if (query.first())
            newPos = query.value(0).toInt();
        query.prepare("INSERT INTO queuesongs (singer,song,artist,title,discid,path,keychg,played,position) "
                      "VALUES (:singerId,:songId,:songId,:songId,:songId,:songId,:key,:played,:position)");
        query.bindValue(":singerId", singerId);
        query.bindValue(":songId", songId);
        query.bindValue(":key", keyChg);
        query.bindValue(":played", false);
        query.bindValue(":position", newPos);
        query.exec();
        qInfo() << query.lastError();
    }
}


QStringList TableModelQueueSongs::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "text/queueitems";
    return types;
}

QMimeData *TableModelQueueSongs::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    if (indexes.size() > 1)
    {
        QJsonArray jArr;
        std::for_each(indexes.begin(), indexes.end(), [&] (QModelIndex index) {
            // Just using COL_ARTIST here because it's the first column that's included in the index list
            if (index.column() != COL_ARTIST)
                return;
            jArr.append(index.sibling(index.row(), 0).data().toInt());
        });
        QJsonDocument jDoc(jArr);
        mimeData->setData("text/queueitems", jDoc.toJson());
    }
    return mimeData;
}

bool TableModelQueueSongs::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    if ((data->hasFormat("integer/songid")) || (data->hasFormat("text/queueitems")) || data->hasFormat("text/uri-list"))
    {
        qInfo() << "QueueModel - Good data type - can drop: " << data->formats();
        return true;
    }
    qInfo() << "QueueModel - Unknown data type - can't drop: " << data->formats();
    return false;
}

bool TableModelQueueSongs::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);

    qInfo() <<"qdrop: action: " << action << " format: " << data->formats();

    if (getSingerId() == -1)
    {
        qInfo() << "Song dropped into queue w/ no singer selected";
        emit songDroppedWithoutSinger();
        return false;
    }

    if (action == Qt::MoveAction && data->hasFormat("text/queueitems"))
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(data->data("text/queueitems"));
        QJsonArray jArr = jDoc.array();
        auto ids = jArr.toVariantList();
        int droprow{0};
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount() - 1;
        if (getPosition(ids.at(0).toInt()) > droprow)
            std::reverse(ids.begin(),ids.end());
        std::for_each(ids.begin(), ids.end(), [&] (auto val) {
            int oldPosition = getPosition(val.toInt());
            if (oldPosition < droprow && droprow != rowCount() - 1)
                moveSongId(val.toInt(), droprow - 1);
            else
                moveSongId(val.toInt(), droprow);
        });
        if (droprow == rowCount() - 1)
        {
            // moving to bottom
            emit qSongsMoved(droprow - ids.size() + 1, 0, rowCount() - 1, columnCount() - 1);
        }
        else if (getPosition(ids.at(0).toInt()) < droprow)
        {
            // moving down
            emit qSongsMoved(droprow - ids.size(), 0, droprow - 1, columnCount() - 1);
        }
        else
        {
            // moving up
            emit qSongsMoved(droprow, 0, droprow + ids.size() - 1, columnCount() - 1);
        }
        commitChanges();
        return true;
    }
    if (data->hasFormat("integer/songid"))
    {
        unsigned int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount();
        int songid;
        QByteArray bytedata = data->data("integer/songid");
        songid = QString(bytedata.data()).toInt();
        insert(songid, droprow);
        commitChanges();
        return true;
    }
    else if (data->hasFormat("text/uri-list"))
    {
        QString dataIn = data->data("text/urilist");
        QList<QUrl> items = data->urls();
        if (items.size() > 0)
        {
            unsigned int droprow;
            if (parent.row() >= 0)
                droprow = parent.row();
            else if (row >= 0)
                droprow = row;
            else
                droprow = rowCount();
            emit filesDroppedOnSinger(items, m_curSingerId, droprow);
        }
        commitChanges();
        return true;
    }
    return false;
}

Qt::DropActions TableModelQueueSongs::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags TableModelQueueSongs::flags([[maybe_unused]]const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

}

void TableModelQueueSongs::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    if (order == Qt::AscendingOrder)
    {
        std::sort(m_songs.begin(), m_songs.end(), [&column] (QueueSong &a, QueueSong &b)
        {
            switch (column) {
            case COL_ARTIST:
                return (a.artist < b.artist);
            case COL_TITLE:
                return (a.title < b.title);
            case COL_SONGID:
                return (a.songId < b.songId);
            case COL_DURATION:
                return (a.duration < b.duration);
            case COL_KEY:
                return (a.keyChange < b.keyChange);
            default:
                return (a.position < b.position);
            }
        });
    }
    else
    {
        std::sort(m_songs.rbegin(), m_songs.rend(), [&column] (QueueSong &a, QueueSong &b)
        {
            switch (column) {
            case COL_ARTIST:
                return (a.artist < b.artist);
            case COL_TITLE:
                return (a.title < b.title);
            case COL_SONGID:
                return (a.songId < b.songId);
            case COL_DURATION:
                return (a.duration < b.duration);
            case COL_KEY:
                return (a.keyChange < b.keyChange);
            default:
                return (a.position < b.position);
            }
        });
    }
    int pos{0};
    std::for_each(m_songs.begin(), m_songs.end(), [&pos] (QueueSong &song)
    {
        song.position = pos++;
    });
    emit layoutChanged();
    commitChanges();
}

void ItemDelegateQueueSongs::resizeIconsForFont(const QFont &font)
{
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_curFontHeight = QFontMetrics(font).height();
    m_iconDelete = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconDelete.fill(Qt::transparent);
    QPainter painterDelete(&m_iconDelete);
    QSvgRenderer svgrndrDelete(thm + "actions/16/edit-delete.svg");
    svgrndrDelete.render(&painterDelete);
}

ItemDelegateQueueSongs::ItemDelegateQueueSongs(QObject *parent) :
    QItemDelegate(parent)
{
    m_currentSong = -1;
    resizeIconsForFont(settings.applicationFont());
}

void ItemDelegateQueueSongs::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == TableModelQueueSongs::COL_PATH)
    {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),m_iconDelete);
        return;
    }
    QItemDelegate::paint(painter, option, index);
}
