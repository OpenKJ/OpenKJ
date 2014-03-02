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

#include "khqueuesong.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

QString KhQueueSong::getDiscID() const
{
    return discID;
}

void KhQueueSong::setDiscID(const QString &value)
{
    discID = value;
}

QString KhQueueSong::getSourceFile() const
{
    return sourceFile;
}

void KhQueueSong::setSourceFile(const QString &value)
{
    sourceFile = value;
}

KhQueueSong::KhQueueSong(KhRegularSingers *regSingers, QObject *parent) :
    QObject(parent)
{
    regularSingers = regSingers;
}


QString KhQueueSong::getTitle() const
{
    return title;
}

void KhQueueSong::setTitle(const QString &value)
{
    title = value;
}

QString KhQueueSong::getArtist() const
{
    return artist;
}

void KhQueueSong::setArtist(const QString &value)
{
    artist = value;
}

int KhQueueSong::getPosition() const
{
    return position;
}

void KhQueueSong::setPosition(int value, bool skipDB)
{
    position = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE queuesongs SET 'position'=" + QString::number(position) + " WHERE ROWID == " + QString::number(index));
        if (regSong)
           regularSingers->getByRegularID(regSingerIndex)->getSongByIndex(regSongIndex)->setPosition(position);
    }
}

bool KhQueueSong::getPlayed() const
{
    return played;
}

void KhQueueSong::setPlayed(bool value, bool skipDB)
{
    played = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE queuesongs SET 'played'=" + QString::number(played) + " WHERE ROWID == " + QString::number(index));
    }
}

int KhQueueSong::getKeyChange() const
{
    return keyChange;
}

void KhQueueSong::setKeyChange(int value, bool skipDB)
{
    keyChange = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE queuesongs SET 'keychg'=" + QString::number(keyChange) + " WHERE ROWID == " + QString::number(index));
        if (regSong)
            regularSingers->getByRegularID(regSingerIndex)->getSongByIndex(regSongIndex)->setKeyChange(keyChange);
    }
}

int KhQueueSong::getSongID() const
{
    return songID;
}

void KhQueueSong::setSongID(int value, bool skipDB)
{
    songID = value;
    QSqlQuery query;
    query.exec("SELECT artist,title,discid,path FROM dbsongs WHERE ROWID == " + QString::number(songID));
    int artist = query.record().indexOf("artist");
    int title = query.record().indexOf("title");
    int discid = query.record().indexOf("discid");
    int path = query.record().indexOf("path");
    while (query.next()) {
        setArtist(query.value(artist).toString());
        setTitle(query.value(title).toString());
        setDiscID(query.value(discid).toString());
        setSourceFile(query.value(path).toString());
    }
    if (!skipDB)
    {
        query.exec("UPDATE queuesongs SET 'songid'=" + QString::number(songID) + " WHERE ROWID == " + QString::number(index));
    }
}

int KhQueueSong::getSingerID() const
{
    return singerID;
}

void KhQueueSong::setSingerID(int value, bool skipDB)
{
    singerID = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE queuesongs SET 'singer'=" + QString::number(singerID) + " WHERE ROWID == " + QString::number(index));
    }
}

int KhQueueSong::getIndex() const
{
    return index;
}

void KhQueueSong::setIndex(int value)
{
    index = value;
}


KhQueueSongs::KhQueueSongs(int singerID, KhRegularSingers *regSingers, int regSingerID, QObject *parent) :
    QObject(parent)
{
    singerIndex = 0;
    singerIndex = singerID;
    regSingerIndex = regSingerID;
    songs = new QList<KhQueueSong *>;
    regularSingers = regSingers;
    loadFromDB();
}

KhQueueSongs::~KhQueueSongs()
{
    qDeleteAll(songs->begin(),songs->end());
    delete songs;
}

int KhQueueSong::getRegSongIndex() const
{
    return regSongIndex;
}

void KhQueueSong::setRegSongIndex(int value, bool skipDB)
{
    regSongIndex = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE queuesongs SET 'regsongid'=" + QString::number(regSongIndex) + " WHERE ROWID == " + QString::number(index));
    }
}

int KhQueueSong::getRegSingerIndex() const
{
    return regSingerIndex;
}

void KhQueueSong::setRegSingerIndex(int value, bool skipDB)
{
    regSingerIndex = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE queuesongs SET 'regsingerid'=" + QString::number(regSingerIndex) + " WHERE ROWID == " + QString::number(index));
        if (regSong)
            regularSingers->getByRegularID(regSingerIndex)->getSongByIndex(regSongIndex)->setRegSingerIndex(regSingerIndex);
    }
}

bool KhQueueSong::isRegSong() const
{
    return regSong;
}

void KhQueueSong::setRegSong(bool value, bool skipDB)
{
    regSong = value;
    if (!skipDB)
    {
        QSqlQuery query;
        query.exec("UPDATE queuesongs SET 'regsong'=" + QString::number(regSong) + " WHERE ROWID == " + QString::number(index));
    }
}

QList<KhQueueSong *> *KhQueueSongs::getSongs()
{
    return songs;
}

KhQueueSong *KhQueueSongs::getSongByIndex(int index)
{
    for (int i=0; i < songs->size(); i++)
    {
        if (songs->at(i)->getIndex() == index)
            return songs->at(i);
    }
    return NULL;
}

KhQueueSong *KhQueueSongs::getSongByPosition(int position)
{
    for (int i=0; i < songs->size(); i++)
    {
        if (songs->at(i)->getPosition() == position)
            return songs->at(i);
    }
    return NULL;
}

KhQueueSong *KhQueueSongs::getNextSong()
{
    for (int i=0; i < songs->size(); i++)
    {
        if (!songs->at(i)->getPlayed())
        {
            return songs->at(i);
        }
    }
    return NULL;
}

int KhQueueSongs::getSingerIndex() const
{
    return singerIndex;
}

void KhQueueSongs::setSingerIndex(int value)
{
    singerIndex = value;
    loadFromDB();
}

void KhQueueSongs::loadFromDB()
{
    qDeleteAll(songs->begin(),songs->end());
    songs->clear();
    QSqlQuery query;
    QString singerIdx = QString::number(singerIndex);
    query.exec("SELECT ROWID,singer,song,keychg,played,position,regsong,regsongid FROM queuesongs WHERE singer == " + singerIdx);
    int queuesongid = query.record().indexOf("ROWID");
    int singer = query.record().indexOf("singer");
    int songid = query.record().indexOf("song");
    int keychg = query.record().indexOf("keychg");
    int played = query.record().indexOf("played");
    int position = query.record().indexOf("position");
    int regsong = query.record().indexOf("regsong");
    int regsongid = query.record().indexOf("regsongid");
    while (query.next()) {
        KhQueueSong *song = new KhQueueSong(regularSingers);
        song->setIndex(query.value(queuesongid).toInt());
        song->setSingerID(query.value(singer).toInt(), true);
        song->setSongID(query.value(songid).toInt(), true);
        song->setKeyChange(query.value(keychg).toInt(), true);
        song->setPlayed(query.value(played).toBool(), true);
        song->setPosition(query.value(position).toInt(), true);
        song->setRegSong(query.value(regsong).toBool(), true);
        song->setRegSongIndex(query.value(regsongid).toInt(), true);
        songs->push_back(song);
    }
    sort();
}


int KhQueueSongs::addSongAtEnd(int songid, bool regularSong, int regSongID)
{
    KhQueueSong *song = new KhQueueSong(regularSingers);
    song->setSongID(songid,true);
    song->setSingerID(singerIndex,true);
    song->setPlayed(false,true);
    song->setKeyChange(0,true);
    song->setRegSong(regularSong,true);
    song->setRegSongIndex(regSongID,true);
    song->setPosition(-1);
    return addSong(song);
}

int KhQueueSongs::addSongAtPosition(int songid, int position, bool regularSong, int regSongID)
{
    KhQueueSong *song = new KhQueueSong(regularSingers);
    song->setSongID(songid,true);
    song->setSingerID(singerIndex,true);
    song->setPlayed(false,true);
    song->setKeyChange(0,true);
    song->setPosition(position,true);
    song->setRegSong(regularSong,true);
    song->setRegSongIndex(regSongID,true);
    return addSong(song);
}

void KhQueueSongs::deleteSongByIndex(int index)
{
    KhQueueSong *song = getSongByIndex(index);
    qDebug() << "KhQueueSongs::deleteSongByIndex(int " << index << ")";
    qDebug() << "Deleting song at position: " << song->getPosition();
    QSqlQuery query;
    query.exec("DELETE FROM queuesongs WHERE ROWID == " + QString::number(index));
    for (int i=0; i < songs->size(); i++)
        if (songs->at(i)->getPosition() > song->getPosition())
            songs->at(i)->setPosition(songs->at(i)->getPosition() - 1);
    if (song->isRegSong())
        regularSingers->getByRegularID(song->getRegSingerIndex())->getRegSongs()->deleteSongByIndex(song->getRegSongIndex());
        songs->erase(songs->begin() + (song->getPosition()));

    emit queueUpdated();
}

void KhQueueSongs::deleteSongByPosition(int position)
{
    deleteSongByIndex(getSongByPosition(position)->getIndex());
}

bool sortByPositionCallback(KhQueueSong *song1, KhQueueSong *song2)
{
    if (song1->getPosition() < song2->getPosition())
        return true;
    return false;
}

bool sortByArtistCallback(KhQueueSong *song1, KhQueueSong *song2)
{
    if (song1->getArtist().toLower() < song2->getArtist().toLower())
        return true;
    return false;
}

bool sortByArtistReverseCallback(KhQueueSong *song1, KhQueueSong *song2)
{
    if (song1->getArtist().toLower() > song2->getArtist().toLower())
        return true;
    return false;
}

bool sortByTitleCallback(KhQueueSong *song1, KhQueueSong *song2)
{
    if (song1->getTitle().toLower() < song2->getTitle().toLower())
        return true;
    return false;
}

bool sortByTitleReverseCallback(KhQueueSong *song1, KhQueueSong *song2)
{
    if (song1->getTitle().toLower() > song2->getTitle().toLower())
        return true;
    return false;
}

bool sortByDiscIDCallback(KhQueueSong *song1, KhQueueSong *song2)
{
    if (song1->getDiscID().toLower() < song2->getDiscID().toLower())
        return true;
    return false;
}

bool sortByDiscIDReverseCallback(KhQueueSong *song1, KhQueueSong *song2)
{
    if (song1->getDiscID().toLower() > song2->getDiscID().toLower())
        return true;
    return false;
}

void KhQueueSongs::sort()
{
    std::sort(songs->begin(),songs->end(),sortByPositionCallback);
}

void KhQueueSongs::sortByArtist(bool reverse)
{
    if (!reverse)
        std::sort(songs->begin(),songs->end(),sortByArtistCallback);
    else
        std::sort(songs->begin(),songs->end(),sortByArtistReverseCallback);
    QSqlQuery query("BEGIN TRANSACTION");
    for (int i=0; i < songs->size(); i++)
        songs->at(i)->setPosition(i);
    query.exec("COMMIT TRANSACTION");
}

void KhQueueSongs::sortByTitle(bool reverse)
{
    if (!reverse)
        std::sort(songs->begin(),songs->end(),sortByTitleCallback);
    else
        std::sort(songs->begin(),songs->end(),sortByTitleReverseCallback);
    QSqlQuery query("BEGIN TRANSACTION");
    for (int i=0; i < songs->size(); i++)
        songs->at(i)->setPosition(i);
    query.exec("COMMIT TRANSACTION");
}

void KhQueueSongs::sortByDiscID(bool reverse)
{
    if (!reverse)
        std::sort(songs->begin(),songs->end(),sortByDiscIDCallback);
    else
        std::sort(songs->begin(),songs->end(),sortByDiscIDReverseCallback);
    QSqlQuery query("BEGIN TRANSACTION");
    for (int i=0; i < songs->size(); i++)
        songs->at(i)->setPosition(i);
    query.exec("COMMIT TRANSACTION");
}


int KhQueueSongs::addSong(KhQueueSong *song)
{
    QSqlQuery query;
    QString positionStr;
    if (song->getPosition() <= -1)
    {
        positionStr = QString::number(songs->size());
        song->setPosition(songs->size(),true);
    }
    else if (song->getPosition() != songs->size())
    {
        positionStr = QString::number(song->getPosition());
        query.exec("BEGIN TRANSACTION");
        for (int i=0; i < songs->size(); i++)
            if (songs->at(i)->getPosition() >= song->getPosition()) songs->at(i)->setPosition(songs->at(i)->getPosition() + 1);
        query.exec("COMMIT TRANSACTION");
    }
    else
    {
        positionStr = QString::number(song->getPosition());
    }
    query.exec("INSERT INTO queueSongs (singer,song,keychg,played,position,regsong,regsongid) VALUES(" + QString::number(song->getSingerID()) + "," + QString::number(song->getSongID()) + "," + QString::number(song->getKeyChange()) + "," + QString::number(song->getPlayed()) + "," + positionStr + "," + QString::number(song->isRegSong()) + "," + QString::number(song->getRegSongIndex()) + ")");
    song->setIndex(query.lastInsertId().toInt());
    songs->push_back(song);
    sort();
    emit queueUpdated();
    return query.lastInsertId().toInt();
}

bool KhQueueSongs::songExists(int songIndex)
{
    for (int i=0; i < songs->size(); i++)
    {
        if (songs->at(i)->getSongID() == songIndex)
            return true;
    }
    return false;
}


void KhQueueSongs::clear()
{
    songs->clear();
}

bool KhQueueSongs::moveSong(int oldPosition, int newPosition)
{
    QSqlQuery query;
    KhQueueSong *movingSong = getSongByPosition(oldPosition);
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
    {
        for (int i=0; i < songs->size(); i++)
        {
            if ((songs->at(i)->getPosition() > oldPosition) && (songs->at(i)->getPosition() <= newPosition - 1) && (songs->at(i)->getIndex() != movingSong->getIndex()))
                songs->at(i)->setPosition(songs->at(i)->getPosition() - 1);
        }
        movingSong->setPosition(newPosition - 1);
    }
    else if (newPosition < oldPosition)
    {
        for (int i=0; i < songs->size(); i++)
        {
            if ((songs->at(i)->getPosition() >= newPosition) && (songs->at(i)->getPosition() < oldPosition) && (songs->at(i)->getIndex() != movingSong->getIndex()))
                songs->at(i)->setPosition(songs->at(i)->getPosition() + 1);
        }
        movingSong->setPosition(newPosition);
    }
    query.exec("COMMIT TRANSACTION");
    sort();
    return true;
}

int KhQueueSongs::getRegSingerIndex() const
{
    return regSingerIndex;
}

void KhQueueSongs::setRegSingerIndex(int value)
{
    regSingerIndex = value;
    for (int i=0; i < songs->size(); i++)
        songs->at(i)->setRegSingerIndex(regSingerIndex);
}
