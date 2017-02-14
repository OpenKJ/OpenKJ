/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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

#include "rotationmodel.h"
#include <QSqlQuery>
#include <QDebug>

int RotationModel::currentSinger() const
{
    return m_currentSingerId;
}

void RotationModel::setCurrentSinger(int currentSingerId)
{
    emit layoutAboutToBeChanged();
    m_currentSingerId = currentSingerId;
    emit rotationModified();
    emit layoutChanged();
}

RotationModel::RotationModel(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
    m_currentSingerId = -1;
    setTable("rotationsingers");
    sort(2, Qt::AscendingOrder);
}

int RotationModel::singerAdd(QString name)
{
    QSqlQuery query;
    query.exec("INSERT INTO rotationsingers (name,position,regular,regularid) VALUES(\"" + name + "\"," + QString::number(rowCount()) + ",0,-1)");
    select();
    emit rotationModified();
    return query.lastInsertId().toInt();
}

void RotationModel::singerMove(int oldPosition, int newPosition)
{
    qDebug() << "moveSinger(" << oldPosition << "," << newPosition << ")";
    if (oldPosition == newPosition)
        return;
    QSqlQuery query;
    QString sql;
    int qSingerId = index(oldPosition,0).data().toInt();
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
        sql = "UPDATE rotationsingers SET position = position - 1 WHERE position > " + QString::number(oldPosition) + " AND position <= " + QString::number(newPosition) + " AND singerid != " + QString::number(qSingerId);
    else if (newPosition < oldPosition)
        sql = "UPDATE rotationsingers SET position = position + 1 WHERE position >= " + QString::number(newPosition) + " AND position < " + QString::number(oldPosition) + " AND singerid != " + QString::number(qSingerId);
    query.exec(sql);
    query.exec("UPDATE rotationsingers SET position = " + QString::number(newPosition) + " WHERE singerid == " + QString::number(qSingerId));
    query.exec("COMMIT TRANSACTION");
    select();
    emit rotationModified();
}

void RotationModel::singerSetName(int singerId, QString newName)
{
    QSqlQuery query;
    query.exec("UPDATE rotationsingers SET name = \"" + newName + "\" WHERE singerid == " + QString::number(singerId));
    emit rotationModified();
    select();
}

void RotationModel::singerDelete(int singerId)
{
    int position = getSingerPosition(singerId);
    QSqlQuery query;
    query.exec("DELETE FROM queuesongs WHERE singer == " + QString::number(singerId));
    query.exec("UPDATE rotationsingers SET position = position - 1 WHERE position > " + QString::number(position));
    query.exec("DELETE FROM rotationsingers WHERE singerid == " + QString::number(singerId));
    select();
    emit rotationModified();
}

bool RotationModel::singerExists(QString name)
{
    QStringList names = singers();
    for (int i=0; i < names.size(); i++)
    {
        if (names.at(i).toLower() == name.toLower())
            return true;
    }
    return false;
}

bool RotationModel::singerIsRegular(int singerId)
{
    QSqlQuery query;
    query.exec("SELECT regular FROM rotationsingers WHERE singerid == " + QString::number(singerId));
    if (query.first())
        return query.value(0).toBool();

    return false;
}

int RotationModel::singerRegSingerId(int singerId)
{
    QSqlQuery query;
    query.exec("SELECT regularid FROM rotationsingers WHERE singerid == " + QString::number(singerId));
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

void RotationModel::singerMakeRegular(int singerId)
{
    if (regularExists(getSingerName(singerId)))
    {
        emit regularAddNameConflict(getSingerName(singerId));
        return;
    }
    int regSingerId = regularAdd(getSingerName(singerId));
    if (regSingerId == -1)
    {
        emit regularAddError("Error adding regular singer.  Reason: Failure while writing new regular singer to database");
        return;
    }
    QSqlQuery query;
    query.exec("UPDATE rotationsingers SET regular=1,regularid=" + QString::number(regSingerId) + " WHERE singerid == " + QString::number(singerId));
    query.exec("INSERT INTO regularsongs (regsingerid, songid, keychg, position) SELECT " + QString::number(regSingerId) + ", queuesongs.song, queuesongs.keychg, queuesongs.position FROM queuesongs WHERE queuesongs.singer == " + QString::number(singerId));
    select();
}

void RotationModel::singerDisableRegularTracking(int singerId)
{
    QSqlQuery query;
    query.exec("UPDATE rotationsingers SET regular=0,regularid=-1 WHERE singerid == " + QString::number(singerId));
    select();
}

int RotationModel::regularAdd(QString name)
{
    if (regularExists(name))
    {
        emit regularAddNameConflict(name);
        return -1;
    }
    QSqlQuery query;
    query.exec("INSERT INTO regularsingers (name) VALUES(\"" + name + "\")");
    emit regularsModified();
    return query.lastInsertId().toInt();
}

void RotationModel::regularDelete(int regSingerId)
{
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM regularsingers WHERE regsingerid == " + QString::number(regSingerId));
    query.exec("DELETE FROM regularsongs WHERE regsingerid == " + QString::number(regSingerId));
    query.exec("UPDATE rotationsingers SET regular=0,regularid=-1 WHERE regularid == " + QString::number(regSingerId));
    query.exec("COMMIT TRANSACTION");
    select();
    emit regularsModified();
}

bool RotationModel::regularExists(QString name)
{
    QStringList names = regulars();
    for (int i=0; i < names.size(); i++)
    {
        if (names.at(i).toLower() == name.toLower())
            return true;
    }
    return false;
}

void RotationModel::regularUpdate(int singerId)
{
    int regSingerId = singerRegSingerId(singerId);
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM regularsongs WHERE regsingerid == " + QString::number(regSingerId));
    query.exec("INSERT INTO regularsongs (regsingerid, songid, keychg, position) SELECT " + QString::number(regSingerId) + ", queuesongs.song, queuesongs.keychg, queuesongs.position FROM queuesongs WHERE queuesongs.singer == " + QString::number(singerId));
    query.exec("COMMIT TRANSACTION");
}

QString RotationModel::getSingerName(int singerId)
{
    QSqlQuery query("SELECT name FROM rotationsingers WHERE singerid = " + QString::number(singerId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString RotationModel::getRegularName(int regSingerId)
{
    QSqlQuery query("SELECT name FROM regularsingers WHERE regsingerid = " + QString::number(regSingerId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

int RotationModel::getSingerId(QString name)
{
    QSqlQuery query("SELECT singerid FROM rotationsingers WHERE name == \"" + name + "\" LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int RotationModel::getSingerPosition(int singerId)
{
    QSqlQuery query("SELECT position FROM rotationsingers WHERE singerid = " + QString::number(singerId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int RotationModel::singerIdAtPosition(int position)
{
    QSqlQuery query("SELECT singerId FROM rotationsingers WHERE position == " + QString::number(position) + " LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

QStringList RotationModel::singers()
{
    QStringList singers;
    for (int i=0; i < rowCount(); i++)
        singers << index(i,1).data().toString();
    return singers;
}

QStringList RotationModel::regulars()
{
    QStringList names;
    QSqlQuery query;
    query.exec("SELECT name FROM regularsingers");
    while (query.next())
        names << query.value(0).toString();
    return names;
}

QString RotationModel::nextSongPath(int singerId)
{
    QSqlQuery query("SELECT dbsongs.path FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString RotationModel::nextSongArtist(int singerId)
{
    QSqlQuery query("SELECT dbsongs.artist FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString RotationModel::nextSongTitle(int singerId)
{
    QSqlQuery query("SELECT dbsongs.title FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

int RotationModel::nextSongKeyChg(int singerId)
{
    QSqlQuery query("SELECT keychg FROM queuesongs WHERE singer = " + QString::number(singerId) + " AND queuesongs.played = 0 ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return 0;
}

int RotationModel::nextSongId(int singerId)
{
    QSqlQuery query("SELECT dbsongs.songid FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int RotationModel::nextSongQueueId(int singerId)
{
    QSqlQuery query("SELECT qsongid FROM queuesongs WHERE singer = " + QString::number(singerId) + " AND played = 0 ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

void RotationModel::clearRotation()
{
    QSqlQuery query;
    query.exec("DELETE from queuesongs");
    query.exec("DELETE FROM rotationsingers");
    select();
    emit rotationModified();
}

void RotationModel::queueModified(int singerId)
{
    emit layoutAboutToBeChanged();
    emit layoutChanged();
    if (singerIsRegular(singerId))
        regularUpdate(singerId);
}

void RotationModel::regularLoad(int regSingerId, int positionHint)
{
    if (singerExists(getRegularName(regSingerId)))
    {
        emit regularLoadNameConflict(getRegularName(regSingerId));
        return;
    }
    int singerId = singerAdd(getRegularName(regSingerId));
    QSqlQuery query;
    query.exec("UPDATE rotationsingers SET regular=1,regularid=" + QString::number(regSingerId) + " WHERE singerid == " + QString::number(singerId));
    query.exec("INSERT INTO queuesongs (singer, song, artist, title, discid, path, keychg, played, position) SELECT " + QString::number(singerId) + ", regularsongs.songid, regularsongs.songid, regularsongs.songid, regularsongs.songid, regularsongs.songid, regularsongs.keychg, 0, regularsongs.position FROM regularsongs WHERE regsingerid == " + QString::number(regSingerId));
    switch (positionHint) {
    case ADD_FAIR:
        singerMove(rowCount() - 1, getSingerPosition(m_currentSingerId));
        break;
    case ADD_NEXT:
        singerMove(rowCount() - 1, getSingerPosition(m_currentSingerId) + 1);
        break;
    }
    select();
}

void RotationModel::regularSetName(int regSingerId, QString newName)
{
    QSqlQuery query;
    query.exec("UPDATE regularsingers SET name = \"" + newName + "\" WHERE regsingerid == " + QString::number(regSingerId));
    emit rotationModified();
    select();
}


QStringList RotationModel::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/rotationpos";
    return types;
}

QMimeData *RotationModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/rotationpos", indexes.at(0).sibling(indexes.at(0).row(), 2).data().toByteArray().data());
    return mimeData;
}

bool RotationModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    if ((data->hasFormat("integer/songid")) || (data->hasFormat("integer/rotationpos")))
        return true;
    return false;
}

bool RotationModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(column);

    if (data->hasFormat("integer/rotationpos"))
    {
        int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount() - 1;
        int oldPosition;
        QByteArray bytedata = data->data("integer/rotationpos");
        oldPosition =  QString(bytedata.data()).toInt();
        if ((oldPosition < droprow) && (droprow != rowCount() - 1))
            singerMove(oldPosition, droprow - 1);
        else
            singerMove(oldPosition, droprow);
        return true;
    }
    if (data->hasFormat("integer/songid"))
    {
        unsigned int dropRow;
        if (parent.row() >= 0)
            dropRow = parent.row();
        else if (row >= 0)
            dropRow = row;
        else
            dropRow = rowCount();
        int songId = data->data("integer/songid").toInt();
        int singerId = index(dropRow,0).data().toInt();
        emit songDroppedOnSinger(singerId, songId, dropRow);
    }
    return false;
}

Qt::ItemFlags RotationModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}

Qt::DropActions RotationModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}
