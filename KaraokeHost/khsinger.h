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
#include <boost/shared_ptr.hpp>
#include <vector>
#include "khqueuesong.h"
#include "khregularsinger.h"

class KhSinger : public QObject {
    Q_OBJECT
public:
    explicit KhSinger(boost::shared_ptr<KhRegularSingers> regSingers, QObject *parent = 0);

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

    boost::shared_ptr<KhQueueSongsVector> getQueueSongs();
    boost::shared_ptr<KhQueueSongs> getQueueObject();
    boost::shared_ptr<KhQueueSong> getSongByIndex(int queueSongID);
    boost::shared_ptr<KhQueueSong> getSongByPosition(int position);
    boost::shared_ptr<KhQueueSong> getNextSong();
    bool hasUnplayedSongs();
    int addSong(boost::shared_ptr<KhQueueSong> song);
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
    KhQueueSongsVector queue;
    boost::shared_ptr<KhQueueSongs> songs;
    boost::shared_ptr<KhRegularSingers> regularSingers;

signals:

public slots:

};

typedef std::vector<boost::shared_ptr<KhSinger> > KhRotationData;


class KhRotationSingers : public QObject {
    Q_OBJECT
public:
    explicit KhRotationSingers(QObject *parent = 0);
    void loadFromDB();
    boost::shared_ptr<KhRotationData> getSingers();
    bool moveSinger(int oldPosition, int newPosition);
    boost::shared_ptr<KhSinger> getSingerByPosition(int position) const;
    boost::shared_ptr<KhSinger> getSingerByIndex(int singerid);
    int getCurrentSingerPosition() const;
    void setCurrentSingerPosition(int value);
    bool singerAdd(QString name, int position = -1, bool regular = false);
    bool singerExists(QString name);
    QString getNextSongBySingerPosition(int position) const;
    void deleteSingerByIndex(int singerid);
    void deleteSingerByPosition(int position);
    void clear();
    boost::shared_ptr<KhSinger> getCurrent();
    boost::shared_ptr<KhSinger> getSelected();

    int getCurrentSingerIndex() const;
    void setCurrentSingerIndex(int value);
    int getSelectedSingerPosition() const;
    void setSelectedSingerPosition(int value);
    int getSelectedSingerIndex() const;
    void setSelectedSingerIndex(int value);
    void createRegularForSinger(int singerID);

private:
    boost::shared_ptr<KhRotationData> singers;
    boost::shared_ptr<KhRegularSingers> regularSingers;
    int currentSingerPosition;
    int currentSingerIndex;
    int selectedSingerPosition;
    int selectedSingerIndex;
    void sortSingers();

signals:
    void dataAboutToChange();
    void dataChanged();

public slots:

};


#endif // ROTATIONSINGER_H
