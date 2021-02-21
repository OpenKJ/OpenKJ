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

#include "tablemodelrotationsingers.h"
#include <QSqlQuery>
#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include "settings.h"

extern Settings settings;
extern int remainSecs;

int TableModelRotationSingers::currentSinger() const
{
    return m_currentSingerId;
}

void TableModelRotationSingers::setCurrentSinger(int currentSingerId)
{
    emit layoutAboutToBeChanged();
    m_currentSingerId = currentSingerId;
    emit rotationModified();
    emit layoutChanged();
    settings.setCurrentRotationPosition(currentSingerId);
}

bool TableModelRotationSingers::rotationIsValid()
{
    for (int i = 0; i < singers().size(); i++)
    {
        int id = singerIdAtPosition(i);
        if (id == -1)
        {
            qCritical() << "Rotation position corruption detected!";
            return false;
        }
    }
    return true;
}

int TableModelRotationSingers::numSongs(int singerId)
{
    QSqlQuery query("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = " + QString::number(singerId));
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotationSingers::numSongsSung(int singerId) const
{
    QSqlQuery query("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = " + QString::number(singerId) + " AND played = 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotationSingers::numSongsUnsung(int singerId) const
{
    QSqlQuery query("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = " + QString::number(singerId) + " AND played = 0");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotationSingers::timeAdded(int singerId) const
{
    QSqlQuery query("SELECT strftime('%s',addts) FROM rotationSingers WHERE singerid == " + QString::number(singerId));
    if (query.first())
        return query.value(0).toInt();
    return 0;
}

void TableModelRotationSingers::outputRotationDebug()
{
    qInfo() << " -- Rotation debug output -- ";
    qInfo() << "singerid,position,regular,regularid,added,name";
    QSqlQuery query;
    query.exec("SELECT singerid,position,regular,regularid,addts,name FROM rotationSingers ORDER BY position");
    int expectedPosition = 0;
    bool needsRepair = false;
    while (query.next())
    {
        qInfo() << query.value(0).toInt() << "," << query.value(1).toInt() << "," << query.value(2).toInt() << "," << query.value(3).toInt() << "," << query.value(4).toString() << "," << query.value(5).toString();
        if (query.value(1).toInt() != expectedPosition)
        {
            needsRepair = true;
            qInfo() << "ERROR DETECTED!!! - Singer position does not match expected position";
        }
        expectedPosition++;
    }
    if (needsRepair)
        fixSingerPositions();
    qInfo() << " -- Rotation debug output end -- ";
}

void TableModelRotationSingers::fixSingerPositions()
{
    qInfo() << "Attempting to fix corrupted rotation position data...";
    QSqlQuery query;
    query.exec("SELECT singerId FROM rotationSingers ORDER BY position");
    QList<int> singers;
    while (query.next())
    {
        singers << query.value(0).toInt();
    }
    for (int i=0; i < singers.size();i++)
    {
        QString sql = "UPDATE rotationSingers SET position=" + QString::number(i) + " WHERE singerid = " + QString::number(singers.at(i));
        qInfo() << sql;
        query.exec(sql);
    }
    outputRotationDebug();
}

TableModelRotationSingers::TableModelRotationSingers(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
    m_currentSingerId = -1;
    setTable("rotationsingers");
    sort(2, Qt::AscendingOrder);
    singerCount = singers().size();
}

int TableModelRotationSingers::singerAdd(const QString& name, int positionHint)
{
    QSqlQuery query;
    query.exec("INSERT INTO rotationsingers (name,position,regular,regularid, addts) VALUES(\"" + name + "\"," + QString::number(rowCount()) + ",0,-1, CURRENT_TIMESTAMP)");
    select();
    singerCount = singers().size();
    int curSingerPos = getSingerPosition(m_currentSingerId);
    switch (positionHint) {
    case ADD_FAIR:
    {
        if (curSingerPos != 0 && !settings.rotationAltSortOrder())
            singerMove(rowCount() - 1, getSingerPosition(m_currentSingerId));
        break;
    }
    case ADD_NEXT:
        if (curSingerPos != rowCount() - 2)
            singerMove(rowCount() - 1, getSingerPosition(m_currentSingerId) + 1);
        break;
    }
    emit rotationModified();
    outputRotationDebug();
    return query.lastInsertId().toInt();
}

void TableModelRotationSingers::singerMove(int oldPosition, int newPosition)
{
    if (newPosition == -1)
        newPosition = 0;
    int movingSinger = singerIdAtPosition(oldPosition);
    qInfo() << "Moving singer " << getSingerName(movingSinger) << " from position: " << oldPosition << " to: " << newPosition;
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
    rotationIsValid();
    select();
    emit rotationModified();
    outputRotationDebug();
}

void TableModelRotationSingers::singerSetName(int singerId, QString newName)
{
    QSqlQuery query;
    query.exec("UPDATE rotationsingers SET name = \"" + newName + "\" WHERE singerid == " + QString::number(singerId));
    emit rotationModified();
    select();
    outputRotationDebug();
}

void TableModelRotationSingers::singerDelete(int singerId)
{
    int position = getSingerPosition(singerId);
    QSqlQuery query;
    query.exec("DELETE FROM queuesongs WHERE singer == " + QString::number(singerId));
    query.exec("UPDATE rotationsingers SET position = position - 1 WHERE position > " + QString::number(position));
    query.exec("DELETE FROM rotationsingers WHERE singerid == " + QString::number(singerId));
    select();
    singerCount = singers().size();
    emit rotationModified();
    outputRotationDebug();
}

bool TableModelRotationSingers::singerExists(QString name)
{
    QStringList names = singers();
    for (int i=0; i < names.size(); i++)
    {
        if (names.at(i).toLower() == name.toLower())
            return true;
    }
    return false;
}

bool TableModelRotationSingers::singerIsRegular(int singerId)
{
    if (singerId == -1)
        return false;
    QSqlQuery query;
    query.exec("SELECT regular FROM rotationsingers WHERE singerid == " + QString::number(singerId));
    if (query.first())
        return query.value(0).toBool();

    return false;
}

int TableModelRotationSingers::singerRegSingerId(int singerId)
{
    QSqlQuery query;
    query.exec("SELECT regularid FROM rotationsingers WHERE singerid == " + QString::number(singerId));
    if (query.first())
        return query.value(0).toInt();

    return -1;
}

void TableModelRotationSingers::singerMakeRegular(int singerId)
{
    QSqlQuery query;
    query.exec("UPDATE rotationsingers SET regular = 1 WHERE singerid = " + QString::number(singerId));
    select();
}

void TableModelRotationSingers::singerDisableRegularTracking(int singerId)
{
    QSqlQuery query;
    query.exec("UPDATE rotationsingers SET regular = 0 WHERE singerid = " + QString::number(singerId));
    select();
}

int TableModelRotationSingers::regularAdd(QString name)
{
    if (historySingerExists(name))
    {
        emit regularAddNameConflict(name);
        return -1;
    }
    QSqlQuery query;
    query.exec("INSERT INTO regularsingers (name) VALUES(\"" + name + "\")");
    emit regularsModified();
    return query.lastInsertId().toInt();
}

bool TableModelRotationSingers::historySingerExists(QString name)
{
    QStringList names = historySingers();
    for (int i=0; i < names.size(); i++)
    {
        if (names.at(i).toLower() == name.toLower())
            return true;
    }
    return false;
}

QString TableModelRotationSingers::getSingerName(int singerId)
{
    QSqlQuery query("SELECT name FROM rotationsingers WHERE singerid = " + QString::number(singerId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString TableModelRotationSingers::getRegularName(int regSingerId)
{
    QSqlQuery query("SELECT name FROM regularsingers WHERE regsingerid = " + QString::number(regSingerId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

int TableModelRotationSingers::getSingerId(QString name)
{
    QSqlQuery query("SELECT singerid FROM rotationsingers WHERE name == \"" + name + "\" LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotationSingers::getSingerPosition(int singerId)
{
    QSqlQuery query("SELECT position FROM rotationsingers WHERE singerid = " + QString::number(singerId) + " LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotationSingers::singerIdAtPosition(int position) const
{
    QSqlQuery query("SELECT singerId FROM rotationsingers WHERE position == " + QString::number(position) + " LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

QStringList TableModelRotationSingers::singers()
{
    QStringList singers;
    for (int i=0; i < rowCount(); i++)
        singers << index(i,1).data().toString();
    return singers;
}

QStringList TableModelRotationSingers::historySingers()
{
    QStringList names;
    QSqlQuery query;
    query.exec("SELECT name FROM historySingers");
    while (query.next())
        names << query.value(0).toString();
    return names;
}

QString TableModelRotationSingers::nextSongPath(int singerId)
{
    QSqlQuery query("SELECT dbsongs.path FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString TableModelRotationSingers::nextSongArtist(int singerId)
{
    QSqlQuery query("SELECT dbsongs.artist FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString TableModelRotationSingers::nextSongTitle(int singerId)
{
    QSqlQuery query("SELECT dbsongs.title FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

QString TableModelRotationSingers::nextSongSongId(const int singerId) const
{
    QSqlQuery query("SELECT dbsongs.discid FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toString();
    return QString();
}

int TableModelRotationSingers::nextSongDurationSecs(int singerId) const
{
    QSqlQuery query("SELECT dbsongs.duration FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return (query.value(0).toInt() / 1000) + settings.estimationSingerPad();
    else if (!settings.estimationSkipEmptySingers())
        return settings.estimationEmptySongLength() + settings.estimationSingerPad();
    else
        return 0;
}

int TableModelRotationSingers::rotationDuration()
{
    int secs = 0;
    for (int i=0; i < rowCount(); i++)
    {
        secs = secs + nextSongDurationSecs(index(i,0).data().toInt());
    }
    return secs;
}

int TableModelRotationSingers::nextSongKeyChg(int singerId)
{
    QSqlQuery query("SELECT keychg FROM queuesongs WHERE singer = " + QString::number(singerId) + " AND queuesongs.played = 0 ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return 0;
}

int TableModelRotationSingers::nextSongId(int singerId)
{
    QSqlQuery query("SELECT dbsongs.songid FROM dbsongs,queuesongs WHERE queuesongs.singer = " + QString::number(singerId) + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotationSingers::nextSongQueueId(int singerId)
{
    QSqlQuery query("SELECT qsongid FROM queuesongs WHERE singer = " + QString::number(singerId) + " AND played = 0 ORDER BY position LIMIT 1");
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

void TableModelRotationSingers::clearRotation()
{
    QSqlQuery query;
    query.exec("DELETE from queuesongs");
    query.exec("DELETE FROM rotationsingers");
    select();
    singerCount = singers().size();
    settings.setCurrentRotationPosition(-1);
    m_currentSingerId = -1;
    emit rotationModified();
}

QStringList TableModelRotationSingers::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/rotationpos";
    types << "application/rotsingers";
    return types;
}

QMimeData *TableModelRotationSingers::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/rotationpos", indexes.at(0).sibling(indexes.at(0).row(), 2).data().toByteArray().data());
    if (indexes.size() > 1)
    {
        QJsonArray jArr;
        std::for_each(indexes.begin(), indexes.end(), [&] (QModelIndex index) {
            // Just using 1 here because it's the first column that's included in the index list
            if (index.column() != 1)
                return;
            jArr.append(index.sibling(index.row(), 0).data().toInt());
        });
        QJsonDocument jDoc(jArr);
        mimeData->setData("application/rotsingers", jDoc.toJson());
        qInfo() << "Rotation singers mime data: " << jDoc.toJson();
    }
    return mimeData;
}

bool TableModelRotationSingers::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(column);
    Q_UNUSED(row);
    if (parent.row() == -1  && !data->hasFormat("integer/rotationpos"))
        return false;
    if ((data->hasFormat("integer/songid")) || (data->hasFormat("integer/rotationpos")))
        return true;
    return false;
}

bool TableModelRotationSingers::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(column);

    if (action == Qt::MoveAction && data->hasFormat("application/rotsingers"))
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(data->data("application/rotsingers"));
        QJsonArray jArr = jDoc.array();
        auto ids = jArr.toVariantList();
        qInfo() << "mime data dropped: " << jDoc.toJson();
        int droprow{0};
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = rowCount() - 1;
        if (getSingerPosition(ids.at(0).toInt()) > droprow)
            std::reverse(ids.begin(),ids.end());
        std::for_each(ids.begin(), ids.end(), [&] (auto val) {
            singerMove(getSingerPosition(val.toInt()), droprow);
        });
        qInfo() << "droprow: " << droprow;
        emit rotationModified();
        if (droprow == rowCount() - 1)
        {
            qInfo() << "moving to bottom";
            // moving to bottom
            emit singersMoved(rowCount() - ids.size(), 0, rowCount() - 1, columnCount() - 1);
        }
        else if (getSingerPosition(ids.at(0).toInt()) < droprow)
        {
            // moving down
            emit singersMoved(droprow - ids.size() + 1, 0, droprow, columnCount() - 1);
        }
        else
        {
            // moving up
            emit singersMoved(droprow, 0, droprow + ids.size() - 1, columnCount() - 1);
        }
        return true;
    }

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
        if (droprow == oldPosition)
        {
            // Singer dropped, but would result in same position, ignore to prevent rotation corruption.
            return false;
        }
        if ((oldPosition < droprow) && (droprow != rowCount() - 1))
            singerMove(oldPosition, droprow);
        else
            singerMove(oldPosition, droprow);
        emit singersMoved(droprow, 0, droprow, columnCount() - 1);
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

Qt::ItemFlags TableModelRotationSingers::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}

Qt::DropActions TableModelRotationSingers::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QVariant TableModelRotationSingers::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole)
    {
        int curSingerPos = 0;
        int hoverSingerPos = index.row();
        QSqlQuery query;
        query.exec("SELECT position FROM rotationsingers WHERE singerId == " + QString::number(m_currentSingerId) + " LIMIT 1");
        if (query.first())
        {
            curSingerPos = query.value("position").toInt();
        }
        qInfo() << "Cur singer pos: " << curSingerPos;
        qInfo() << "Hover singer pos: " << hoverSingerPos;
        QString toolTipText;
        int totalWaitDuration = 0;
        int singerId = index.sibling(index.row(), 0).data().toInt();
        int qSongsSung = numSongsSung(singerId);
        int qSongsUnsung = numSongsUnsung(singerId);
        if (curSingerPos == hoverSingerPos)
        {
            toolTipText = "Current singer - Sung: " + QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);
        }
        else if (curSingerPos < hoverSingerPos)
        {
            toolTipText = "Wait: " + QString::number(hoverSingerPos - curSingerPos) + " - Sung: " + QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);           
            for (int i=curSingerPos; i < hoverSingerPos; i++)
            {
                int sId = singerIdAtPosition(i);
                if (i == curSingerPos)
                {
                    totalWaitDuration = totalWaitDuration + remainSecs;
                }
                else if (sId != singerId)
                {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration = totalWaitDuration + nextDuration;
                }
            }
        }
        else if (curSingerPos > hoverSingerPos)
        {
            toolTipText = "Wait: " + QString::number(hoverSingerPos + (singerCount - curSingerPos)) + " - Sung: " + QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);
            for (int i=0; i < hoverSingerPos; i++)
            {
                int sId = singerIdAtPosition(i);
                if (sId != singerId)
                {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration = totalWaitDuration + nextDuration;
                }
            }
            for (int i=curSingerPos; i < singerCount; i++)
            {
                int sId = singerIdAtPosition(i);
                if (i == curSingerPos)
                    totalWaitDuration = totalWaitDuration + 240;
                else if (sId != singerId)
                {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration = totalWaitDuration + nextDuration;
                }
            }
        }
        QDateTime time;
        time.setTime_t(timeAdded(singerId));
        toolTipText += "\nTime Added: " + time.toString("h:mm a");

        if (totalWaitDuration > 0)
        {
            int hours = 0;
            int minutes = totalWaitDuration / 60;
            int seconds = totalWaitDuration % 60;
            if (seconds > 0)
                minutes++;
            if (minutes > 60)
            {
                hours = minutes / 60;
                minutes = minutes % 60;
                if (hours > 1)
                    toolTipText += "\nEst wait time: " + QString::number(hours) + " hours " + QString::number(minutes) + " min";
                else
                    toolTipText += "\nEst wait time: " + QString::number(hours) + " hour " + QString::number(minutes) + " min";
            }
            else
                toolTipText += "\nEst wait time: " + QString::number(minutes) + " min";
        }
        return QString(toolTipText);
    }
    else
        return QSqlTableModel::data(index, role);
}


QVariant TableModelRotationSingers::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0)
        return "Wait";
    if (section == 2)
        return "Next Song";
    return QSqlTableModel::headerData(section, orientation, role);
}

ItemDelegateRotationSingers::ItemDelegateRotationSingers(QObject *parent) :
    QItemDelegate(parent)
{
    m_currentSingerId = -1;
    singerCount = 0;
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    favorite16Off = QIcon(thm + "actions/16/im-user.svg");
    favorite22Off = QIcon(thm + "actions/22/im-user.svg");
    favorite16On = QIcon(thm + "actions/16/im-user-online.svg");
    favorite22On = QIcon(thm + "actions/22/im-user-online.svg");
    mic16 = QIcon(thm + "status/16/mic-on");
    mic22 = QIcon(thm + "status/22/mic-on");
    delete16 = QIcon(thm + "actions/16/edit-delete.svg");
    delete22 = QIcon(thm + "actions/22/edit-delete.svg");

}

int ItemDelegateRotationSingers::currentSinger()
{
    return m_currentSingerId;
}

void ItemDelegateRotationSingers::setCurrentSinger(int currentSingerId)
{
    m_currentSingerId = currentSingerId;
}

void ItemDelegateRotationSingers::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sbSize(QFontMetrics(settings.applicationFont()).height(), QFontMetrics(settings.applicationFont()).height());
    int topPad = (option.rect.height() - sbSize.height()) / 2;
    int leftPad = (option.rect.width() - sbSize.width()) / 2;

    QString nextSong;
    bool hasSong{false};
    QString sql = "select dbsongs.artist,dbsongs.title from dbsongs,queuesongs WHERE queuesongs.singer = " + index.sibling(index.row(), 0).data().toString() + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
    {
        nextSong = " " + query.value(0).toString() + " - " + query.value(1).toString();
        hasSong = true;
    }
    else
        nextSong = "  -- Empty -- ";

    if (option.state & QStyle::State_Selected)
    {
        if (index.column() == 1)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (index.row() % 2) ? option.palette.alternateBase() : option.palette.base());
    }
    if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId && index.column() < 2 && index.column() > 0)
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (settings.theme() == 1) ? QColor(180,180,0) : QColor("yellow"));
    }
    if (index.column() == 3)
    {
            if (sbSize.height() > 18)
            {
                if (index.data().toBool())
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite22On.pixmap(sbSize));
                else
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite22Off.pixmap(sbSize));
            }
            else
            {
                if (index.data().toBool())
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite16On.pixmap(sbSize));
                else
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite16Off.pixmap(sbSize));
            }
        return;
    }
    if (index.column() == 2)
    {
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        else if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
        {
            painter->setPen(QColor("black"));
        }
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, nextSong);
        painter->restore();
        return;
    }
    if (index.column() == 0)
    {
        if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
        {
            if (sbSize.height() > 18)
                painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), mic22.pixmap(sbSize));
            else
                painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), mic16.pixmap(sbSize));

        }
        else if (settings.rotationDisplayPosition())
        {
            int curSingerPos = 0;
            int drawSingerPos = index.row();
            QSqlQuery query;
            int wait = 0;
            query.exec("SELECT position FROM rotationsingers WHERE singerId == " + QString::number(m_currentSingerId) + " LIMIT 1");
            if (query.first())
            {
                curSingerPos = query.value("position").toInt();
            }
            if (curSingerPos < drawSingerPos)
                wait = drawSingerPos - curSingerPos;
            else if (curSingerPos > drawSingerPos)
                wait = drawSingerPos + (singerCount - curSingerPos);
            if (wait > 0)
            {
                painter->save();
                if (option.state & QStyle::State_Selected)
                    painter->setPen(option.palette.highlightedText().color());
                else if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
                {
                    painter->setPen(QColor("black"));
                }
                painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignHCenter, QString::number(wait));
                painter->restore();
            }
            return;
        }
        return;
    }
    if (index.column() == 4)
    {
        if (sbSize.height() > 18)
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete22.pixmap(sbSize));
        else
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete16.pixmap(sbSize));
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    else if ((index.sibling(index.row(), 0).data().toInt() == m_currentSingerId) && (settings.theme() == 1))
    {
        painter->setPen(QColor("black"));
    }
    if (hasSong && index.column() == 1)
    {
        if (index.sibling(index.row(), 0).data().toInt() != m_currentSingerId && !option.state.testFlag(QStyle::State_Selected))
            painter->setPen(QColor(128,255,128));
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString() + " ∙");
    }
    else
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}
