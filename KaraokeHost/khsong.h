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

#ifndef DBSONG_H
#define DBSONG_H

#include <QObject>
#include <QString>

class KhSong : public QObject
{
    Q_OBJECT
public:
    explicit KhSong(QObject *parent=0);
    bool operator==(KhSong cmpsong) {
        if (ID == cmpsong.ID) return true;
        else return false;
    }

    int ID;
    QString Artist;
    QString Title;
    QString DiscID;
    QString filename;
    long Duration;
    QString path;

};

typedef QList<KhSong *> KhSongs;

#endif // DBSONG_H
