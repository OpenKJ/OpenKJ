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

#include "bmpltablemodel.h"
#include <QSqlQuery>
#include <QDebug>
#include <QDataStream>

BmPlTableModel::BmPlTableModel(QObject *parent, QSqlDatabase db) :
    QSqlRelationalTableModel(parent, db)
{
    m_playlistId = -1;
    setTable("bmplsongs");
    setRelation(1, QSqlRelation("bmplaylists", "playlistid", "title"));
    setRelation(3, QSqlRelation("bmsongs", "songid", "artist"));
    setRelation(4, QSqlRelation("bmsongs", "songid", "title"));
    setRelation(5, QSqlRelation("bmsongs", "songid", "filename"));
    setRelation(6, QSqlRelation("bmsongs", "songid", "duration"));
    setRelation(7, QSqlRelation("bmsongs", "songid", "path"));

    setHeaderData(3, Qt::Horizontal, tr("Artist"));
    setHeaderData(4, Qt::Horizontal, tr("Title"));
    setHeaderData(5, Qt::Horizontal, tr("Filename"));
    setHeaderData(6, Qt::Horizontal, tr("Duration"));
    setHeaderData(2, Qt::Horizontal, "");
    setHeaderData(7, Qt::Horizontal, "");
    setSort(2, Qt::AscendingOrder);
}

void BmPlTableModel::moveSong(int oldPosition, int newPosition)
{
    QSqlQuery query;
    int plSongId = index(oldPosition,0).data().toInt();
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
    {
        // Moving down
        QString sql = "UPDATE bmplsongs SET position = position - 1 WHERE playlist = " + QString::number(currentPlaylist()) + " AND position > " + QString::number(oldPosition) + " AND position <= " + QString::number(newPosition) + " AND plsongid != " + QString::number(plSongId);
        query.exec(sql);
        sql = "UPDATE bmplsongs SET position = " + QString::number(newPosition) + " WHERE playlist = " + QString::number(currentPlaylist()) + " AND plsongid == " + QString::number(plSongId);
        query.exec(sql);
    }
    else if (newPosition < oldPosition)
    {
        // Moving up
        QString sql = "UPDATE bmplsongs SET position = position + 1 WHERE playlist = " + QString::number(currentPlaylist()) + " AND position >= " + QString::number(newPosition) + " AND position < " + QString::number(oldPosition) + " AND plsongid != " + QString::number(plSongId);
        query.exec(sql);
        sql = "UPDATE bmplsongs SET position = " + QString::number(newPosition) + " WHERE playlist = " + QString::number(currentPlaylist()) + " AND plsongid == " + QString::number(plSongId);
        query.exec(sql);
    }
    query.exec("COMMIT TRANSACTION");
    select();
    emit bmSongMoved(oldPosition, newPosition);
}

void BmPlTableModel::addSong(int songId)
{
    QSqlQuery query;
    QString sIdStr = QString::number(songId);
    QString sql = "INSERT INTO bmplsongs (playlist,position,artist,title,filename,duration,path) VALUES(" + QString::number(m_playlistId) + "," + QString::number(rowCount()) + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + ")";
    qDebug() << sql;
    query.exec(sql);
    select();
}

void BmPlTableModel::insertSong(int songId, int position)
{
    addSong(songId);
    moveSong(rowCount() - 1, position);
}

void BmPlTableModel::deleteSong(int position)
{
    int plSongId = index(position,0).data().toInt();
    QSqlQuery query("DELETE FROM bmplsongs WHERE plsongid == " + QString::number(plSongId));
    query.exec("UPDATE bmplsongs SET position = position - 1 WHERE playlist = " + QString::number(currentPlaylist()) + " AND position > " + QString::number(position));
    select();
}

void BmPlTableModel::setCurrentPlaylist(int playlistId)
{
    m_playlistId = playlistId;
    setFilter("playlist=" + QString::number(m_playlistId));
    select();
}

int BmPlTableModel::currentPlaylist()
{
    return m_playlistId;
}

int BmPlTableModel::getSongIdByFilePath(QString filePath)
{
    QSqlQuery query("SELECT songid FROM bmsongs WHERE path == \"" + filePath + "\" LIMIT 1");
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

int BmPlTableModel::numSongs()
{
    return this->rowCount();
}

bool BmPlTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(column);
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
            moveSong(oldPosition, droprow - 1);
        else
            moveSong(oldPosition, droprow);
        return true;
    }
    if (data->hasFormat("application/vnd.bmsongid.list"))
    {
        QByteArray encodedData = data->data("application/vnd.bmsongid.list");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<int> songids;
        stream >> songids;
        qWarning() << songids;
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
    }
    return false;
}

QStringList BmPlTableModel::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/queuepos";
    types << "application/vnd.bmsongid.list";
    return types;
}

bool BmPlTableModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(data);
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    return true;
}

Qt::DropActions BmPlTableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData *BmPlTableModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/queuepos", indexes.at(0).sibling(indexes.at(0).row(), 2).data().toByteArray().data());
    return mimeData;
}

Qt::ItemFlags BmPlTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}


QVariant BmPlTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole && index.column() != 7)
        return QSqlRelationalTableModel::data(index, Qt::DisplayRole);
    else
        return QSqlRelationalTableModel::data(index, role);
}
