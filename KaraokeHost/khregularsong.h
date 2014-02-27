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

#ifndef KHREGULARSONG_H
#define KHREGULARSONG_H

#include <QObject>

class KhRegularSong : public QObject {
    Q_OBJECT
public:
    explicit KhRegularSong(QObject *parent = 0);
    int getRegSongIndex() const;
    void setRegSongIndex(int value);
    int getRegSingerIndex() const;
    void setRegSingerIndex(int value);
    int getSongIndex() const;
    void setSongIndex(int value);
    int getKeyChange() const;
    void setKeyChange(int value, bool skipDB=false);
    int getPosition() const;
    void setPosition(int value, bool skipDB=false);

private:
    int regSongIndex;
    int regSingerIndex;
    int songIndex;
    int keyChange;
    int position;
};

class KhRegularSongs : public QObject {
    Q_OBJECT

public:
    explicit KhRegularSongs(int regSingerID, QObject *parent = 0);
    void deleteSongByIndex(int index);
    QList<KhRegularSong*> *getRegSongs();

private:
    QList<KhRegularSong*> *regSongs;
    int regSingerIndex;
    void loadFromDB();
    KhRegularSong *getSongByIndex(int index);
};

#endif // KHREGULARSONG_H
