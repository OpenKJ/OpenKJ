/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#include "songdbloadthread.h"
#include <QSql>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

SongDBLoadThread::SongDBLoadThread(KhSongs songsVectorPointer, QObject *parent):
    QThread(parent)
{
    songs.swap(songsVectorPointer);
}

void SongDBLoadThread::run()
{
    songs->clear();
    QSqlQuery query("SELECT ROWID,discid,artist,title,filename,path,length FROM dbSongs");
    int dbsongid = query.record().indexOf("ROWID");
    int discid = query.record().indexOf("discid");
    int artist = query.record().indexOf("artist");
    int title  = query.record().indexOf("title");
    int filename = query.record().indexOf("filename");
    int path = query.record().indexOf("path");
    int length = query.record().indexOf("length");
    qDebug() << "Loading songdb into cache";
    while (query.next()) {
        boost::shared_ptr<KhSong> song(new KhSong());
        song->ID = query.value(dbsongid).toInt();
        song->DiscID = query.value(discid).toString();
        song->Artist = query.value(artist).toString();
        song->Title = query.value(title).toString();
        song->filename = query.value(filename).toString();
        song->path = query.value(path).toString();
        song->Duration = query.value(length).toString();
        songs->push_back(song);
    }
}
