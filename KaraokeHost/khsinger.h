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

    int getSingerIndex() const;
    void setSingerIndex(int value);

    QString getSingerName() const;
    void setSingerName(const QString &value, bool skipDB = false);

    int getSingerPosition() const;
    void setSingerPosition(int value, bool skipDB = false);

    bool isRegular() const;
    void setRegular(bool value, bool skipDB = false);

    int getRegularIndex() const;
    void setRegularIndex(int value, bool skipDB = false);

    QList<KhQueueSong *> *getQueueSongs();
    KhQueueSongs *getQueueObject();
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
    int singerIndex;
    QString singerName;
    int singerPosition;
    bool regular;
    int regularIndex;
    KhQueueSongs *songs;
    KhRegularSingers *regularSingers;

signals:

public slots:

};

class KhRotationSingers : public QObject {
    Q_OBJECT
public:
    explicit KhRotationSingers(KhRegularSingers *regSingersPtr, QObject *parent = 0);
    ~KhRotationSingers();
    void loadFromDB();
    QList<KhSinger *> *getSingers();
    bool moveSinger(int oldPosition, int newPosition);
    KhSinger *getSingerByPosition(int position) const;
    KhSinger *getSingerByIndex(int singerid);
    int getCurrentSingerPosition() const;
    void setCurrentSingerPosition(int value);
    bool add(QString name, int position = -1, bool regular = false);
    bool exists(QString name);
    QString getNextSongBySingerPosition(int position) const;
    void deleteSingerByIndex(int singerid);
    void deleteSingerByPosition(int position);
    void clear();
    KhSinger *getCurrent();
    KhSinger *getSelected();

    int getCurrentSingerIndex() const;
    void setCurrentSingerIndex(int value);
    int getSelectedSingerPosition() const;
    void setSelectedSingerPosition(int value);
    int getSelectedSingerIndex() const;
    void setSelectedSingerIndex(int value);
    void createRegularForSinger(int singerID);

private:
    QList<KhSinger *> *singers;
    KhRegularSingers *regularSingers;
    int currentSingerPosition;
    int currentSingerIndex;
    int selectedSingerPosition;
    int selectedSingerIndex;
    void sortSingers();

signals:
    void dataAboutToChange();
    void dataChanged();

public slots:
    void regularSingerDeleted(int RegularID);
};


#endif // ROTATIONSINGER_H
