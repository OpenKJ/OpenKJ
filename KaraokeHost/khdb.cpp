/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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

#include "khdb.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

KhDb::KhDb(QObject *parent) :
    QObject(parent)
{
}

void KhDb::beginTransaction()
{
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
}

void KhDb::endTransaction()
{
    QSqlQuery query;
    query.exec("COMMIT TRANSACTION");
}

bool KhDb::singerSetRegular(int singerId, bool value)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'regular'=" + QString::number(value) + " WHERE ROWID == " + QString::number(singerId);
    return query.exec(sql);
}

bool KhDb::singerSetPosition(int singerId, int position)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'position'=" + QString::number(position) + " WHERE ROWID == " + QString::number(singerId);
    return query.exec(sql);
}

bool KhDb::singerSetName(int singerId, QString name)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'name'=\"" + name + "\" WHERE ROWID == " + QString::number(singerId);
    return query.exec(sql);
}

bool KhDb::singerSetRegIndex(int singerId, int regId)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'regularid'=" + QString::number(regId) + " WHERE ROWID == " + QString::number(singerId);
    return query.exec(sql);
}

int KhDb::singerAdd(QString name, int position, bool regular)
{
    QSqlQuery query;
    query.exec("INSERT INTO rotationSingers (name, position, regular) VALUES(\"" + name + "\", " + QString::number(position) + "," + QString::number(regular) + ")");
    return query.lastInsertId().toInt();
}

bool KhDb::singerDelete(int singerId)
{
    QSqlQuery query;
    if (query.exec("DELETE FROM queueSongs WHERE singer == " + QString::number(singerId)))
        return query.exec("DELETE FROM rotationSingers WHERE ROWID == " + QString::number(singerId));
    return false;
}

QString KhDb::singerGetNextSong(int singerId)
{
    QSqlQuery query;
    query.exec("SELECT queueSongs.song,dbSongs.artist,dbSongs.title FROM queueSongs,dbSongs WHERE queueSongs.played == 0 AND queueSongs.singer == " + QString::number(singerId) + " AND dbSongs.ROWID == queueSongs.song ORDER BY position LIMIT 1");
    int idx = query.record().indexOf("song");
    int artist = query.record().indexOf("artist");
    int title = query.record().indexOf("title");
    int songid = -1;
    QString nextSong;
    while (query.next()) {
        songid = query.value(idx).toInt();
        nextSong = query.value(artist).toString() + " - " + query.value(title).toString();
    }
    if (songid == -1)
        nextSong = "--empty--";
    return nextSong;
}

bool KhDb::rotationClear()
{
    QSqlQuery query;
    if (query.exec("DELETE FROM rotationsingers"))
        return query.exec("DELETE FROM queuesongs");
    return false;
}

bool KhDb::songSetDuration(int songId, int duration)
{
    QSqlQuery query;
    QString sql = "UPDATE dbsongs SET 'duration'=" + QString::number(duration) + " WHERE ROWID == " + QString::number(songId);
    return query.exec(sql);
}
