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

#ifndef KHREGULARSINGER_H
#define KHREGULARSINGER_H

#include <QObject>
#include <boost/shared_ptr.hpp>
#include <vector>
#include "khregularsong.h"

class KhRegularSinger : public QObject
{
    Q_OBJECT
public:
    explicit KhRegularSinger(QObject *parent = 0);
    explicit KhRegularSinger(QString singerName, QObject *parent = 0);
    explicit KhRegularSinger(QString singerName, int singerID, QObject *parent = 0);
    int getIndex() const;
    void setIndex(int value);
    QString getName() const;
    void setName(const QString &value);
    boost::shared_ptr<KhRegularSongs> getRegSongs() const;
    int addSong(int songIndex, int keyChange, int position);
    boost::shared_ptr<KhRegularSong> getSongByIndex(int index);
    int songsSize();

signals:
    
public slots:

private:
    int regindex;
    QString name;
    boost::shared_ptr<KhRegularSongs> regSongs;
};

//typedef std::vector<boost::shared_ptr<KhRegularSinger> > KhRegularSingerVector;

class KhRegularSingers : public QObject
{
    Q_OBJECT
public:
    explicit KhRegularSingers(QObject *parent = 0);
    QList<KhRegularSinger *> *getRegularSingers();
    KhRegularSinger *getByIndex(int regIndex);
    KhRegularSinger *getByName(QString regName);
    bool exists(QString searchName);
    int add(QString name);
    int size();
    KhRegularSinger* at(int index);

signals:

public slots:

private:
    void loadFromDB();
    QList<KhRegularSinger *> *regularSingers;

};

#endif // KHREGULARSINGER_H
