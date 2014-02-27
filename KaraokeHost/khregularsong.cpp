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

#include "khregularsong.h"
#include <QString>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

KhRegularSong::KhRegularSong(QObject *parent) :
    QObject(parent)
{
}

int KhRegularSong::getPosition() const
{
    return position;
}

void KhRegularSong::setPosition(int value, bool skipDB)
{
    position = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE regularsongs SET 'position'=" + QString::number(position) + " WHERE ROWID == " + QString::number(regSongIndex));
    }
}

int KhRegularSong::getKeyChange() const
{
    return keyChange;
}

void KhRegularSong::setKeyChange(int value, bool skipDB)
{
    keyChange = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE regularsongs SET 'keychg'=" + QString::number(keyChange) + " WHERE ROWID == " + QString::number(regSongIndex));
    }
}

int KhRegularSong::getSongIndex() const
{
    return songIndex;
}

void KhRegularSong::setSongIndex(int value)
{
    songIndex = value;
}

int KhRegularSong::getRegSingerIndex() const
{
    return regSingerIndex;
}

void KhRegularSong::setRegSingerIndex(int value)
{
    regSingerIndex = value;
}

int KhRegularSong::getRegSongIndex() const
{
    return regSongIndex;
}

void KhRegularSong::setRegSongIndex(int value)
{
    regSongIndex = value;
}


KhRegularSongs::KhRegularSongs(int regSingerID, QObject *parent)
{
    Q_UNUSED(parent);
    regSongs = new QList<KhRegularSong *>;
    regSingerIndex = regSingerID;
    loadFromDB();
}

void KhRegularSongs::loadFromDB()
{
    regSongs->clear();
    QSqlQuery query("SELECT ROWID,singer,song,keychg,position FROM regularSongs WHERE singer == " + QString::number(regSingerIndex));
    int regsongid = query.record().indexOf("ROWID");
    int regsingerid = query.record().indexOf("singer");
    int songid = query.record().indexOf("song");
    int keychg = query.record().indexOf("keychg");
    int position = query.record().indexOf("position");
    while (query.next()) {
        KhRegularSong *song = new KhRegularSong();
        song->setRegSongIndex(query.value(regsongid).toInt());
        song->setRegSingerIndex(query.value(regsingerid).toInt());
        song->setSongIndex(query.value(songid).toInt());
        song->setKeyChange(query.value(keychg).toInt(),true);
        song->setPosition(query.value(position).toInt(),true);
        regSongs->push_back(song);
    }
}

void KhRegularSongs::deleteSongByIndex(int index)
{
    KhRegularSong *song = getSongByIndex(index);
    QSqlQuery query;
    query.exec("DELETE FROM regularsongs WHERE ROWID == " + QString::number(index));
    for (int i=0; i < regSongs->size(); i++)
        if (regSongs->at(i)->getPosition() > song->getPosition())
            regSongs->at(i)->setPosition(regSongs->at(i)->getPosition() - 1);
    regSongs->erase(regSongs->begin() + (song->getPosition()));
}

KhRegularSong *KhRegularSongs::getSongByIndex(int index)
{
    for (int i=0; i < regSongs->size(); i++)
    {
        if (regSongs->at(i)->getRegSongIndex() == index)
            return regSongs->at(i);
    }
    return new KhRegularSong();
}

QList<KhRegularSong *> *KhRegularSongs::getRegSongs()
{
    return regSongs;
}
