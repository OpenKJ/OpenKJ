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

#ifndef KHDB_H
#define KHDB_H

#include <QObject>

class KhDb : public QObject
{
    Q_OBJECT

public:
    explicit KhDb(QObject *parent = 0);
    void beginTransaction();
    void endTransaction();
    bool singerSetRegular(int singerId, bool value);
    bool singerSetPosition(int singerId, int position);
    bool singerSetName(int singerId, QString name);
    bool singerSetRegIndex(int singerId, int regId);
    bool singerMove(int singerId, int newPosition);
    int  singerAdd(QString name, int position = -1, bool regular = false);
    bool singerDelete(int singerId);
    QString singerGetNextSong(int singerId);
    bool rotationClear();
    bool songSetDuration(int sondId, int duration);
    void songMarkBad(QString filename);

};

#endif // KHDB_H
