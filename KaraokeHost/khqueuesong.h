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

#ifndef QUEUESONG_H
#define QUEUESONG_H

#include <QString>
#include <QObject>
#include "khregularsinger.h"

class KhQueueSong : public QObject {
    Q_OBJECT
private:
    int index;
    int singerID;
    int songID;
    int keyChange;
    bool played;
    int position;
    QString artist;
    QString title;
    QString sourceFile;
    QString discID;
    bool regSong;
    int regSongIndex;
    int regSingerIndex;
    void setArtist(const QString &value);
    void setTitle(const QString &value);
    void setDiscID(const QString &value);
    void setSourceFile(const QString &value);
    KhRegularSingers *regularSingers;

public:
    explicit KhQueueSong(KhRegularSingers *regSingers, QObject *parent=0);
    int getIndex() const;
    void setIndex(int value);
    int getSingerID() const;
    void setSingerID(int value, bool skipDB = false);
    int getSongID() const;
    void setSongID(int value, bool skipDB = false);
    int getKeyChange() const;
    void setKeyChange(int value, bool skipDB = false);
    bool getPlayed() const;
    void setPlayed(bool value, bool skipDB = false);
    int getPosition() const;
    void setPosition(int value, bool skipDB = false);
    QString getArtist() const;
    QString getTitle() const;
    QString getSourceFile() const;
    QString getDiscID() const;
    bool isRegSong() const;
    void setRegSong(bool value, bool skipDB = false);
    int getRegSongIndex() const;
    void setRegSongIndex(int value, bool skipDB = false);
    int getRegSingerIndex() const;
    void setRegSingerIndex(int value, bool skipDB = false);
};

class KhQueueSongs : public QObject {
    Q_OBJECT
public:
    explicit KhQueueSongs(int singerID, KhRegularSingers *regSingers, int regSingerID = -1 , QObject *parent = 0);
    ~KhQueueSongs();
    QList<KhQueueSong *> *getSongs();
    KhQueueSong *getSongByIndex(int index);
    KhQueueSong *getSongByPosition(int position);
    KhQueueSong *getNextSong();
    bool songExists(int songIndex);
    int addSong(KhQueueSong *song);
    int addSongAtEnd(int songid, bool regularSong = false, int regSongID = -1);
    int addSongAtPosition(int songid, int position, bool regularSong = false, int regSongID = -1, int regSingerID = -1);
    void deleteSongByIndex(int index);
    void deleteSongByPosition(int position);
    void sort();
    void sortByArtist(bool reverse=false);
    void sortByTitle(bool reverse=false);
    void sortByDiscID(bool reverse=false);
    void clear();
    bool moveSong(int oldPosition, int newPosition);
    int getSingerIndex() const;
    void setSingerIndex(int value);

    int getRegSingerIndex() const;
    void setRegSingerIndex(int value, bool skipDB = false);

private:
    QList<KhQueueSong *> *songs;
    KhRegularSingers *regularSingers;
    void loadFromDB();
    int singerIndex;
    int regSingerIndex;

signals:
    void queueUpdated();

public slots:

};

#endif // QUEUESONG_H
