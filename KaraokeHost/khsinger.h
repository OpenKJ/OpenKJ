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

#ifndef ROTATIONSINGER_H
#define ROTATIONSINGER_H

#include <QString>
#include <QObject>
#include "khqueuesong.h"
#include "khregularsinger.h"

class KhSinger : public QObject {
    Q_OBJECT
public:
    explicit KhSinger(KhRegularSingers *regSingers, QObject *parent = 0);

    int index() const;
    void setIndex(int value);

    QString name() const;
    void setName(const QString &value, bool skipDB = false);

    int position() const;
    void setPosition(int value, bool skipDB = false);

    bool regular() const;
    void setRegular(bool value, bool skipDB = false);

    int regularIndex() const;
    void setRegularIndex(int value, bool skipDB = false);

    QList<KhQueueSong *> *queueSongs();
    KhQueueSongs *queueObject();
    KhQueueSong *getSongByIndex(int queueSongID);
    KhQueueSong *getSongByPosition(int position);
    KhQueueSong *getNextSong();
    bool hasUnplayedSongs();
    int addSong(KhQueueSong *song);
    int addSongAtEnd(int songid, bool regularSong = false, int regSongID = -1);
    int addSongAtPosition(int songid, int position, bool regularSong = false, int regSongID = -1);
    void clearQueue();
    void moveSong(int oldPosition, int newPosition);


private:
    int m_index;
    QString m_name;
    int m_position;
    bool m_regular;
    int m_regularIndex;
    KhQueueSongs *m_songs;
    KhRegularSingers *m_regularSingers;
signals:

public slots:

};




#endif // ROTATIONSINGER_H
