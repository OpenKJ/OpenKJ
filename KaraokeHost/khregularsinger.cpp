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

#include "khregularsinger.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

KhRegularSinger::KhRegularSinger(QObject *parent) :
    QObject(parent)
{
    regindex = -1;
    name = "Empty KhRegularSinger";
    boost::shared_ptr<KhRegularSongs> sng_ptr(new KhRegularSongs(-1));
    regSongs.swap(sng_ptr);
}

KhRegularSinger::KhRegularSinger(QString singerName, QObject *parent)
{
    Q_UNUSED(parent);
    QSqlQuery query;
    query.exec("INSERT INTO regularsingers (name) VALUES(\"" + singerName + "\")");
    regindex = query.lastInsertId().toInt();
    name = singerName;
    boost::shared_ptr<KhRegularSongs> sng_ptr(new KhRegularSongs(regindex));
    regSongs.swap(sng_ptr);
}

KhRegularSinger::KhRegularSinger(QString singerName, int singerID, QObject *parent)
{
    Q_UNUSED(parent);
    regindex = singerID;
    name = singerName;
    boost::shared_ptr<KhRegularSongs> sng_ptr(new KhRegularSongs(regindex));
    regSongs.swap(sng_ptr);
}


KhRegularSingers::KhRegularSingers(QObject *parent) :
    QObject(parent)
{
    boost::shared_ptr<KhRegularSingerVector> tmp_ptr(new KhRegularSingerVector);
    regularSingers.swap(tmp_ptr);

    loadFromDB();
}

QString KhRegularSinger::getName() const
{
    return name;
}

void KhRegularSinger::setName(const QString &value)
{
    name = value;
}

boost::shared_ptr<KhRegularSingerVector> KhRegularSingers::getRegularSingers() const
{
    return regularSingers;
}

boost::shared_ptr<KhRegularSinger> KhRegularSingers::getByIndex(int regIndex)
{
    for (unsigned int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getIndex() == regIndex)
            return regularSingers->at(i);
    }
    return boost::shared_ptr<KhRegularSinger>(new KhRegularSinger);
}

boost::shared_ptr<KhRegularSinger> KhRegularSingers::getByName(QString regName)
{
    for (unsigned int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getName() == regName)
            return regularSingers->at(i);
    }
    return boost::shared_ptr<KhRegularSinger>(new KhRegularSinger);
}

bool KhRegularSingers::exists(QString searchName)
{
    for (unsigned int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getName() == searchName)
            return true;
    }
    return false;
}

int KhRegularSingers::add(QString name)
{
    if (!exists(name))
    {
        boost::shared_ptr<KhRegularSinger> singer(new KhRegularSinger(name));
        regularSingers->push_back(singer);
        qDebug() << "Created regular singer. ID:" << singer->getIndex();
        return singer->getIndex();
    }
    return -1;
}

int KhRegularSingers::size()
{
    return regularSingers->size();
}

boost::shared_ptr<KhRegularSinger> KhRegularSingers::at(int index)
{
    return regularSingers->at(index);
}

void KhRegularSingers::loadFromDB()
{
    regularSingers->clear();
    QSqlQuery query("SELECT ROWID,name FROM regularSingers");
    int regsingerid = query.record().indexOf("ROWID");
    int name = query.record().indexOf("name");
    while (query.next()) {
        boost::shared_ptr<KhRegularSinger> singer(new KhRegularSinger());
        singer->setIndex(query.value(regsingerid).toInt());
        singer->setName(query.value(name).toString());
        regularSingers->push_back(singer);
    }
}

int KhRegularSinger::getIndex() const
{
    return regindex;
}

void KhRegularSinger::setIndex(int value)
{
    regindex = value;
    boost::shared_ptr<KhRegularSongs> tmp_ptr(new KhRegularSongs(regindex));
    regSongs.swap(tmp_ptr);
}

boost::shared_ptr<KhRegularSongs> KhRegularSinger::getRegSongs() const
{
    return regSongs;
}

int KhRegularSinger::addSong(int songIndex, int keyChange, int position)
{
    if (regindex == -1)
    {
        qDebug() << "KhRegularSinger::addSong() - Tried to add data to an uninitialized regular!";
                    return -1;
    }
    qDebug() << "KhRegularSinger::addSong(" << songIndex << "," << keyChange << "," << position << ") call on regular singer: " << regindex;
    QSqlQuery query;
    QString sql = "INSERT INTO regularsongs (singer, song, keychg, position) VALUES(" + QString::number(regindex) + "," + QString::number(songIndex) + "," + QString::number(keyChange) + "," + QString::number(position) + ")";
    qDebug() << "Doing sql: " << sql;
    query.exec(sql);
    boost::shared_ptr<KhRegularSong> song(new KhRegularSong);
    song->setRegSongIndex(query.lastInsertId().toInt());
    song->setRegSingerIndex(regindex);
    song->setSongIndex(songIndex);
    song->setKeyChange(keyChange);
    song->setPosition(position);
    regSongs->getRegSongs()->push_back(song);
    return query.lastInsertId().toInt();
}

boost::shared_ptr<KhRegularSong> KhRegularSinger::getSongByIndex(int index)
{
    for (unsigned int i=0; i < regSongs->getRegSongs()->size(); i++)
    {
        if (regSongs->getRegSongs()->at(i)->getRegSongIndex() == index)
            return regSongs->getRegSongs()->at(i);
    }
    return boost::shared_ptr<KhRegularSong>(new KhRegularSong());
}

int KhRegularSinger::songsSize()
{
    return regSongs->getRegSongs()->size();
}
