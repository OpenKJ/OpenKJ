/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tablemodelqueuesongs.h"
#include <QSqlQuery>
#include <QDebug>
#include <QUrl>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "settings.h"

extern Settings settings;

TableModelQueueSongs::TableModelQueueSongs(QObject *parent, QSqlDatabase db) :
    QSqlRelationalTableModel(parent, db)
{
    setTable("queuesongs");
    setSinger(-1);
    setRelation(3, QSqlRelation("dbsongs", "songid", "artist"));
    setRelation(4, QSqlRelation("dbsongs", "songid", "title"));
    setRelation(5, QSqlRelation("dbsongs", "songid", "discid"));
    setRelation(6, QSqlRelation("dbsongs", "songid", "path"));
    setSort(9, Qt::AscendingOrder);
}

void TableModelQueueSongs::setSinger(int singerId)
{
    m_singerId = singerId;
    setFilter("singer=" + QString::number(singerId));
    select();
    if (singerId == -1)
        qInfo() << "Singer selection is none";
}

int TableModelQueueSongs::singer()
{
    return m_singerId;
}

int TableModelQueueSongs::getSongPosition(int songId)
{
    QSqlQuery query;
    query.exec("SELECT position FROM queuesongs WHERE qsongid == " + QString::number(songId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

bool TableModelQueueSongs::getSongPlayed(int songId)
{
    QSqlQuery query;
    query.exec("SELECT played FROM queuesongs WHERE qsongid == " + QString::number(songId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toBool();

    return false;
}

int TableModelQueueSongs::getSongKey(int songId)
{
    QSqlQuery query;
    query.exec("SELECT keychg FROM queuesongs WHERE qsongid == " + QString::number(songId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toInt();

    return 0;
}

void TableModelQueueSongs::songMove(int oldPosition, int newPosition)
{
    QSqlQuery query;
    int qSongId = index(oldPosition,0).data().toInt();
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
    {
        // Moving down
        QString sql = "UPDATE queuesongs SET position = position - 1 WHERE position > " + QString::number(oldPosition) + " AND position <= " + QString::number(newPosition) + " AND qsongid != " + QString::number(qSongId) + " AND singer ==" + QString::number(m_singerId);
        query.exec(sql);
        sql = "UPDATE queuesongs SET position = " + QString::number(newPosition) + " WHERE qsongid == " + QString::number(qSongId);
        query.exec(sql);
    }
    else if (newPosition < oldPosition)
    {
        // Moving up
        QString sql = "UPDATE queuesongs SET position = position + 1 WHERE position >= " + QString::number(newPosition) + " AND position < " + QString::number(oldPosition) + " AND qsongid != " + QString::number(qSongId) + " AND singer ==" + QString::number(m_singerId);
        query.exec(sql);
        sql = "UPDATE queuesongs SET position = " + QString::number(newPosition) + " WHERE qsongid == " + QString::number(qSongId);
        query.exec(sql);
    }
    query.exec("COMMIT TRANSACTION");
    emit queueModified(singer());
    select();
}

void TableModelQueueSongs::songMoveSongId(int songId, int newPosition)
{
    songMove(getSongPosition(songId), newPosition);
}

void TableModelQueueSongs::songAdd(int songId)
{
    QSqlQuery query;
    QString songIdStr = QString::number(songId);
    QString positionStr = QString::number(rowCount());
    query.exec("INSERT INTO queueSongs (singer,song,artist,title,discid,path,keychg,played,position) VALUES(" + QString::number(m_singerId) + "," + songIdStr + "," + songIdStr + "," + songIdStr + "," + songIdStr + "," + songIdStr + ",0,0," + positionStr + ")");
    select();
    emit queueModified(singer());
}

void TableModelQueueSongs::songInsert(int songId, int position)
{
    songAdd(songId);
    songMove(rowCount() - 1, position);
}

void TableModelQueueSongs::songDelete(int songId)
{
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    qDebug() << "UPDATE queuesongs SET position = position - 1 WHERE singer == " << QString::number(singer()) << " AND position > " << QString::number(getSongPosition(songId));
    query.exec("UPDATE queuesongs SET position = position - 1 WHERE singer == " + QString::number(singer()) + " AND position > " + QString::number(getSongPosition(songId)));
    query.exec("DELETE FROM queuesongs WHERE qsongid == " + QString::number(songId));
    query.exec("COMMIT TRANSACTION");
    select();
    emit queueModified(singer());
}

void TableModelQueueSongs::songSetKey(int songId, int semitones)
{
    QSqlQuery query;
    query.exec("UPDATE queuesongs SET keychg = " + QString::number(semitones) + " WHERE qsongid == " + QString::number(songId));
    select();
    emit queueModified(singer());
}

void TableModelQueueSongs::clearQueue()
{
    QSqlQuery query;
    query.exec("DELETE FROM queuesongs where singer == " + QString::number(singer()));
    select();
    emit queueModified(singer());
}

void TableModelQueueSongs::songSetPlayed(int qSongId, bool played)
{
    QSqlQuery query;
    query.exec("UPDATE queuesongs SET played = " + QString::number(played) + " WHERE qsongid == " + QString::number(qSongId));
    select();
    emit queueModified(singer());
}

QStringList TableModelQueueSongs::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/queuepos";
    types << "text/queueitems";
    return types;
}

bool TableModelQueueSongs::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);

    qInfo() <<"qdrop: action: " << action << " format: " << data->formats();

    if (singer() == -1)
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
        if (getSongPosition(ids.at(0).toInt()) > droprow)
            std::reverse(ids.begin(),ids.end());
        std::for_each(ids.begin(), ids.end(), [&] (auto val) {
            int oldPosition = getSongPosition(val.toInt());
            if (oldPosition < droprow && droprow != rowCount() - 1)
                songMoveSongId(val.toInt(), droprow - 1);
            else
                songMoveSongId(val.toInt(), droprow);
        });
        if (droprow == rowCount() - 1)
        {
            // moving to bottom
            emit qSongsMoved(droprow - ids.size() + 1, 0, rowCount() - 1, columnCount() - 1);
        }
        else if (getSongPosition(ids.at(0).toInt()) < droprow)
        {
            // moving down
            emit qSongsMoved(droprow - ids.size(), 0, droprow - 1, columnCount() - 1);
        }
        else
        {
            // moving up
            emit qSongsMoved(droprow, 0, droprow + ids.size() - 1, columnCount() - 1);
        }
        return true;
    }


    if (data->hasFormat("integer/queuepos"))
    {
        int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount() - 1;
        int oldPosition;
        QByteArray bytedata = data->data("integer/queuepos");
        oldPosition =  QString(bytedata.data()).toInt();
        if ((oldPosition < droprow) && (droprow != rowCount() - 1))
            songMove(oldPosition, droprow - 1);
        else
            songMove(oldPosition, droprow);
        sort(9, Qt::AscendingOrder);
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
        songInsert(songid, droprow);
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
            emit filesDroppedOnSinger(items, m_singerId, droprow);
        }
    }
    else
    {

        qInfo() << data->data("text/plain");
    }
    return false;
}

bool TableModelQueueSongs::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    if ((data->hasFormat("integer/songid")) || (data->hasFormat("integer/queuepos")) || data->hasFormat("text/plain") || data->hasFormat("text/uri-list"))
    {
        qInfo() << "QueueModel - Good data type - can drop: " << data->formats();
        return true;
    }
    qInfo() << "QueueModel - Unknown data type - can't drop: " << data->formats();
    return false;
}

Qt::DropActions TableModelQueueSongs::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData *TableModelQueueSongs::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/queuepos", indexes.at(0).sibling(indexes.at(0).row(), 9).data().toByteArray().data());
    if (indexes.size() > 1)
    {
        QJsonArray jArr;
        std::for_each(indexes.begin(), indexes.end(), [&] (QModelIndex index) {
            // Just using 3 here because it's the first column that's included in the index list
            if (index.column() != 3)
                return;
            jArr.append(index.sibling(index.row(), 0).data().toInt());
        });
        QJsonDocument jDoc(jArr);
        mimeData->setData("text/queueitems", jDoc.toJson());
    }
    return mimeData;
}

Qt::ItemFlags TableModelQueueSongs::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}

void TableModelQueueSongs::songAddSlot(int songId, int singerId, int keyChg)
{
    QSqlQuery query;
    QString songIdStr = QString::number(songId);
    query.exec("SELECT COUNT(*) FROM queuesongs WHERE singer == " + QString::number(singerId));
    int newPos = 0;
    if (query.first())
        newPos = query.value(0).toInt();
    QString positionStr = QString::number(newPos);
    query.exec("INSERT INTO queueSongs (singer,song,artist,title,discid,path,keychg,played,position) VALUES(" + QString::number(singerId) + "," + songIdStr + "," + songIdStr + "," + songIdStr + "," + songIdStr + "," + songIdStr + "," + QString::number(keyChg) + ",0," + positionStr + ")");
    select();
    emit queueModified(singer());
}


void TableModelQueueSongs::sort(int column, Qt::SortOrder order)
{
    QSqlQuery query;
    QString orderByClause;
    QString artistOrder = "ASC";
    QString titleOrder = "ASC";
    QString songIdOrder = "ASC";
    QString sortField = "artist";
    switch (column) {
    case 3:
        if (order == Qt::AscendingOrder)
            artistOrder = "ASC";
        else
            artistOrder = "DESC";
        orderByClause = " ORDER BY dbsongs.artist " + artistOrder + ", dbsongs.title " + titleOrder + ", dbsongs.discid " + songIdOrder;
        break;
    case 4:
        sortField = "title";
        if (order == Qt::AscendingOrder)
            titleOrder = "ASC";
        else
            titleOrder = "DESC";
        orderByClause = " ORDER BY dbsongs.title " + titleOrder + ", dbsongs.artist " + artistOrder + ", dbsongs.discid " + songIdOrder;
        break;
    case 5:
        sortField = "discid";
        if (order == Qt::AscendingOrder)
            songIdOrder = "ASC";
        else
            songIdOrder = "DESC";
        orderByClause = " ORDER BY dbsongs.discid " + songIdOrder + ", dbsongs.artist " + artistOrder + ", dbsongs.title " + titleOrder;
        break;
    default:
        return;
    }

    QList<int> qSongIds;
    QString sql = "SELECT queuesongs.qsongid FROM queuesongs,dbsongs WHERE queuesongs.singer == " + QString::number(singer()) + " AND dbsongs.songid == queuesongs.song " + orderByClause;
    query.exec(sql);
    while (query.next())
        qSongIds << query.value(0).toInt();
    query.exec("BEGIN TRANSACTION");
    for (int i=0; i < qSongIds.size(); i++)
    {
        sql = "UPDATE queuesongs SET position = " + QString::number(i) + " WHERE qsongid == " + QString::number(qSongIds.at(i));
        query.exec(sql);
    }
    query.exec("COMMIT TRANSACTION");
    select();
    emit queueModified(singer());
}

QString TableModelQueueSongs::orderByClause() const
{
    return "ORDER BY position";
}


QVariant TableModelQueueSongs::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 5 && role == Qt::DisplayRole)
        return "SongID";
    return QSqlRelationalTableModel::headerData(section, orientation, role);
}


QVariant TableModelQueueSongs::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole)
        return data(index);
    else
        return QSqlRelationalTableModel::data(index, role);
}


ItemDelegateQueueSongs::ItemDelegateQueueSongs(QObject *parent) :
    QItemDelegate(parent)
{
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    delete16 = QIcon(thm + "actions/16/edit-delete.svg");
    delete22 = QIcon(thm + "actions/22/edit-delete.svg");
}


void ItemDelegateQueueSongs::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sbSize(QFontMetrics(settings.applicationFont()).height(), QFontMetrics(settings.applicationFont()).height());
    int topPad = (option.rect.height() - sbSize.height()) / 2;
    int leftPad = (option.rect.width() - sbSize.width()) / 2;
    if ((index.sibling(index.row(), 8).data().toBool()) && (index.column() != 7) && (index.column() != 8))
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
        {
            if (settings.theme() != 1)
                painter->fillRect(option.rect, QColor("darkGrey"));
        }
    }
    if (option.state & QStyle::State_Selected)
    {
        if (index.column() < 8)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (index.row() % 2) ? option.palette.alternateBase() : option.palette.base());
        //painter->fillRect(option.rect, option.palette.highlight());
    }
    if (index.column() == 7)
    {
        QString displayText = index.data().toString();
        if (index.data().toInt() > 0)
            displayText.prepend("+");
        if (index.data().toInt() == 0)
            displayText = "";
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, displayText);
        painter->restore();
        return;
    }
    if (index.column() == 8)
    {
        if (sbSize.height() > 18)
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete22.pixmap(sbSize));
        else
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete16.pixmap(sbSize));
        return;
    }
    if ((index.column() == 5) && (index.data().toString() == "!!DROPPED!!"))
    {
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    if (index.sibling(index.row(), 8).data().toBool() && settings.theme() == 1)
    {
        painter->setPen("darkGrey");
        QFont font = painter->font();
        font.setStrikeOut(true);
        painter->setFont(font);
    }
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}
