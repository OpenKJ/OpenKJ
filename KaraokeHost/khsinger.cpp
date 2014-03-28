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

#include "khsinger.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtCore>
#include <algorithm>
#include <QDebug>


KhSinger::KhSinger(KhRegularSingers *regSingers, QObject *parent) :
    QObject(parent)
{
    songs = new KhQueueSongs(singerIndex,regSingers, regularIndex, this);
    regularSingers = regSingers;
}

bool KhSinger::isRegular() const
{
    return regular;
}

void KhSinger::setRegular(bool value, bool skipDB)
{
    regular = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'regular'=" + QString::number(regular) + " WHERE ROWID == " + QString::number(singerIndex);
        query.exec(sql);
    }
}

int KhSinger::getSingerPosition() const
{
    return singerPosition;
}

void KhSinger::setSingerPosition(int value, bool skipDB)
{
    singerPosition = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'position'=" + QString::number(singerPosition) + " WHERE ROWID == " + QString::number(singerIndex);
        query.exec(sql);
    }
}

QString KhSinger::getSingerName() const
{
    return singerName;
}

void KhSinger::setSingerName(const QString &value, bool skipDB)
{
    singerName = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'name'=\"" + singerName + "\" WHERE ROWID == " + QString::number(singerIndex);
        query.exec(sql);
    }
}

int KhSinger::getSingerIndex() const
{
    return singerIndex;
}

void KhSinger::setSingerIndex(int value)
{
    songs->setSingerIndex(value);
    singerIndex = value;
}

int KhSinger::getRegularIndex() const
{
    return regularIndex;
}

void KhSinger::setRegularIndex(int value, bool skipDB)
{
    regularIndex = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'regularid'=" + QString::number(regularIndex) + " WHERE ROWID == " + QString::number(singerIndex);
        query.exec(sql);
    }
    songs->setRegSingerIndex(regularIndex,skipDB);
}

QList<KhQueueSong *> *KhSinger::getQueueSongs()
{
    return songs->getSongs();
}

KhQueueSongs *KhSinger::getQueueObject()
{
    return songs;
}

KhQueueSong *KhSinger::getSongByIndex(int queueSongID)
{
    return songs->getSongByIndex(queueSongID);
}

KhQueueSong *KhSinger::getSongByPosition(int position)
{
    return songs->getSongByPosition(position);
}

KhQueueSong *KhSinger::getNextSong()
{
    return songs->getNextSong();
}

KhRotationSingers::KhRotationSingers(KhRegularSingers *regSingersPtr, QObject *parent) :
    QObject(parent)
{
    regularSingers = regSingersPtr;
    singers = new QList<KhSinger *>;
    loadFromDB();
    selectedSingerIndex = -1;
    selectedSingerPosition = -1;
    currentSingerIndex = -1;
    currentSingerPosition = -1;
    connect(regularSingers, SIGNAL(dataAboutToChange()), this, SIGNAL(dataAboutToChange()));
    connect(regularSingers, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()));
    connect(regularSingers, SIGNAL(regularSingerDeleted(int)), this, SLOT(regularSingerDeleted(int)));
}

KhRotationSingers::~KhRotationSingers()
{
    qDeleteAll(singers->begin(),singers->end());
    delete singers;
    //delete regularSingers;
}

void KhRotationSingers::loadFromDB()
{
    qDeleteAll(singers->begin(),singers->end());
    singers->clear();
    QSqlQuery query;
    query.exec("SELECT ROWID,name,position,regular,regularid FROM rotationSingers");
    int rotationsingerid = query.record().indexOf("ROWID");
    int name = query.record().indexOf("name");
    int position  = query.record().indexOf("position");
    int regular = query.record().indexOf("regular");
    int regularindex = query.record().indexOf("regularid");
    while (query.next()) {
        bool isReg = query.value(regular).toBool();
        int regIdx = query.value(regularindex).toInt();
        KhSinger *singer = new KhSinger(regularSingers);
        singer->setSingerIndex(query.value(rotationsingerid).toInt());
        if ((isReg) && (regularSingers->getByRegularID(regIdx) != NULL))
        {
            singer->setRegular(query.value(regular).toBool(),true);
            singer->setRegularIndex(query.value(regularindex).toInt(),true);
        }
        else
        {
            singer->setRegular(false,true);
            singer->setRegularIndex(-1, true);
        }
        singer->setSingerName(query.value(name).toString(),true);
        singer->setSingerPosition(query.value(position).toInt(),true);
        singers->push_back(singer);
    }
    sortSingers();
}

QList<KhSinger *> *KhRotationSingers::getSingers()
{
    return singers;
}

bool KhRotationSingers::moveSinger(int oldPosition, int newPosition)
{
    QSqlQuery query;
    KhSinger *movingSinger = getSingerByPosition(oldPosition);
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
    {
        if (currentSingerPosition == oldPosition)
            currentSingerPosition = newPosition - 1;
        else if ((currentSingerPosition <= newPosition) && (currentSingerPosition > oldPosition))
            currentSingerPosition--;
        for (int i=0; i < singers->size(); i++)
        {
            if ((singers->at(i)->getSingerPosition() > oldPosition) && (singers->at(i)->getSingerPosition() <= newPosition - 1) && (singers->at(i)->getSingerIndex() != movingSinger->getSingerIndex()))
                singers->at(i)->setSingerPosition(singers->at(i)->getSingerPosition() - 1);
        }
        movingSinger->setSingerPosition(newPosition - 1);
    }
    else if (newPosition < oldPosition)
    {
        if (currentSingerPosition == oldPosition)
            currentSingerPosition = newPosition;
        else if ((currentSingerPosition >= newPosition) && (currentSingerPosition < oldPosition))
            currentSingerPosition++;
        for (int i=0; i < singers->size(); i++)
        {
            if ((singers->at(i)->getSingerPosition() >= newPosition) && (singers->at(i)->getSingerPosition() < oldPosition) && (singers->at(i)->getSingerIndex() != movingSinger->getSingerIndex()))
                singers->at(i)->setSingerPosition(singers->at(i)->getSingerPosition() + 1);
        }
        movingSinger->setSingerPosition(newPosition);
    }
    query.exec("COMMIT TRANSACTION");
    sortSingers();
    return true;
}

KhSinger *KhRotationSingers::getSingerByPosition(int position) const
{
    for (int i=0; i < singers->size(); i++)
    {
        if (position == singers->at(i)->getSingerPosition())
        {
            return singers->at(i);
        }
    }
    return NULL;
}

KhSinger *KhRotationSingers::getSingerByIndex(int singerid)
{
        for (int i=0; i < singers->size(); i++)
        {
            if (singers->at(i)->getSingerIndex() == singerid)
                return singers->at(i);
        }
        return NULL;
}

int KhRotationSingers::getCurrentSingerPosition() const
{
    return currentSingerPosition;
}

void KhRotationSingers::setCurrentSingerPosition(int value)
{
    currentSingerPosition = value;
    if (getSingerByPosition(value) != NULL)
        currentSingerIndex = getSingerByPosition(value)->getSingerIndex();
    else
        currentSingerIndex = -1;
}

bool KhRotationSingers::add(QString name, int position, bool regular)
{
    if (exists(name))
    {
        return false;
    }
    emit dataAboutToChange();
    QSqlQuery query;
    int nextPos = singers->size() + 1;
    //        singerPos = singers->size() + 1;
    bool result = query.exec("INSERT INTO rotationSingers (name, position, regular) VALUES(\"" + name + "\", " + QString::number(nextPos) + "," + QString::number(regular) + ")");
    if (!result) return false;
    KhSinger *singer = new KhSinger(regularSingers, this);
    singer->setSingerName(name,true);
    singer->setSingerPosition(nextPos,true);
    singer->setSingerIndex(query.lastInsertId().toInt());
    singer->setRegular(regular,true);
    singer->setRegularIndex(-1,true);
    singers->push_back(singer);
    emit dataChanged();
    if (position == -1)
        return true;
    else
    {
        if (moveSinger(nextPos,position))
            return true;
        else
            return false;
    }
    return false;
}

bool KhRotationSingers::exists(QString name)
{
    bool match = false;
    for (int i=0; i < singers->size(); i++)
    {
        if (name.toLower() == singers->at(i)->getSingerName().toLower())
        {
            match = true;
            break;
        }
    }
    return match;
}

QString KhRotationSingers::getNextSongBySingerPosition(int position) const
{
    QString nextSong;
    int singerid = getSingerByPosition(position)->getSingerIndex();
    QSqlQuery query("SELECT queueSongs.song,dbSongs.artist,dbSongs.title FROM queueSongs,dbSongs WHERE queueSongs.played == 0 AND queueSongs.singer == " + QString::number(singerid) + " AND dbSongs.ROWID == queueSongs.song ORDER BY position LIMIT 1");
    int idx = query.record().indexOf("song");
    int artist = query.record().indexOf("artist");
    int title = query.record().indexOf("title");
    int songid = -1;
    while (query.next()) {
        songid = query.value(idx).toInt();
        nextSong = query.value(artist).toString() + " - " + query.value(title).toString();
    }
    if (songid == -1)
        nextSong = "--empty--";
    return nextSong;
}

void KhRotationSingers::deleteSingerByIndex(int singerid)
{
    QSqlQuery query;
    int delSingerPos = getSingerByIndex(singerid)->getSingerPosition();
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM queueSongs WHERE singer == " + QString::number(singerid));
    query.exec("DELETE FROM rotationSingers WHERE ROWID == " + QString::number(singerid));
    singers->erase(singers->begin() + (delSingerPos - 1));
    for (int i=0; i < singers->size(); i++)
    {
        if (singers->at(i)->getSingerPosition() >  delSingerPos)
            singers->at(i)->setSingerPosition(singers->at(i)->getSingerPosition() - 1);
    }
    query.exec("COMMIT TRANSACTION");
    sortSingers();
}

void KhRotationSingers::deleteSingerByPosition(int position)
{
    KhSinger *singer = getSingerByPosition(position);
    deleteSingerByIndex(singer->getSingerIndex());
}

void KhRotationSingers::clear()
{
    emit dataAboutToChange();
    QSqlQuery query;
    query.exec("DELETE FROM rotationsingers");
    query.exec("DELETE FROM queuesongs");
    currentSingerPosition = -1;
    qDeleteAll(singers->begin(),singers->end());
    singers->clear();
    emit dataChanged();
}

KhSinger *KhRotationSingers::getCurrent()
{
    return getSingerByPosition(currentSingerPosition);
}

KhSinger *KhRotationSingers::getSelected()
{
    return getSingerByIndex(selectedSingerIndex);
}

bool positionSort(KhSinger *singer1, KhSinger *singer2)
{
    if (singer1->getSingerPosition() >= singer2->getSingerPosition())
        return false;
    else
        return true;
}

void KhRotationSingers::sortSingers()
{
    emit dataAboutToChange();
    std::sort(singers->begin(), singers->end(), positionSort);
    emit dataChanged();
}

void KhRotationSingers::regularSingerDeleted(int RegularID)
{
    for (int i=0; i < singers->size(); i++)
    {
        if (singers->at(i)->getRegularIndex() == RegularID)
        {
            emit dataAboutToChange();
            singers->at(i)->setRegular(false);
            singers->at(i)->setRegularIndex(-1);
            emit dataChanged();
        }
    }
}




int KhRotationSingers::getCurrentSingerIndex() const
{
    return currentSingerIndex;
}

void KhRotationSingers::setCurrentSingerIndex(int value)
{
    currentSingerIndex = value;
    if (getSingerByIndex(value) != NULL)
        currentSingerPosition = getSingerByIndex(value)->getSingerPosition();
    else
        currentSingerPosition = -1;
}

int KhRotationSingers::getSelectedSingerIndex() const
{
    return selectedSingerIndex;
}

void KhRotationSingers::setSelectedSingerIndex(int value)
{
    selectedSingerIndex = value;
    if (getSingerByIndex(value) != NULL)
        selectedSingerPosition = getSingerByIndex(value)->getSingerPosition();
    else
        selectedSingerPosition = -1;
}

void KhRotationSingers::createRegularForSinger(int singerID)
{
    KhSinger *singer = getSingerByIndex(singerID);
    QSqlQuery query("BEGIN TRANSACTION");
    int regularid = regularSingers->add(singer->getSingerName());
    singer->setRegular(true);
    singer->setRegularIndex(regularid);
    KhRegularSinger *regular = regularSingers->getByRegularID(regularid);
    for (int i=0; i < singer->getQueueSongs()->size(); i++)
    {
        int regsongindex = regular->addSong(singer->getQueueSongs()->at(i)->getSongID(),singer->getQueueSongs()->at(i)->getKeyChange(),singer->getQueueSongs()->at(i)->getPosition());
        singer->getQueueSongs()->at(i)->setRegSong(true);
        singer->getQueueSongs()->at(i)->setRegSongIndex(regsongindex);
    }
    query.exec("COMMIT TRANSACTION");
}

int KhRotationSingers::getSelectedSingerPosition() const
{
    return selectedSingerPosition;
}

void KhRotationSingers::setSelectedSingerPosition(int value)
{
    selectedSingerPosition = value;
    selectedSingerIndex = getSingerByPosition(value)->getSingerIndex();
}


int KhSinger::addSong(KhQueueSong *song)
{
    if (singerIndex != -1)
    {
        if (regular)
        {
            int regsongid = regularSingers->getByRegularID(regularIndex)->addSong(song->getSongID(), song->getKeyChange(), song->getPosition());
            song->setRegSong(true);
            song->setRegSongIndex(regsongid);
        }
        int qsongid = songs->addSong(song);
        return qsongid;
    }
    return -1;
}

int KhSinger::addSongAtEnd(int songid, bool regularSong, int regSongID)
{
    if (singerIndex != -1)
    {
        int qsongid = songs->addSongAtEnd(songid, regularSong,regSongID);
        KhQueueSong *song = getSongByIndex(qsongid);
        if (regular)
        {
            int regsongid = regularSingers->getByRegularID(regularIndex)->addSong(song->getSongID(), song->getKeyChange(), song->getPosition());
            song->setRegSong(true);
            song->setRegSongIndex(regsongid);
            song->setRegSingerIndex(regularIndex);
        }
        return qsongid;
    }
    return -1;
}

int KhSinger::addSongAtPosition(int songid, int position, bool regularSong, int regSongID)
{
    if (singerIndex != 0)
    {
        int qsongid = songs->addSongAtPosition(songid,position,regularSong,regSongID,regularIndex);
        KhQueueSong *song = getSongByIndex(qsongid);
        if (regular)
        {
            QSqlQuery query("BEGIN TRANSACTION");
            song->setRegSong(true);
            song->setRegSingerIndex(regularIndex);
            KhRegularSinger *regsinger = regularSingers->getByRegularID(regularIndex);
            KhRegularSong *regSong = new KhRegularSong();
            regSong->setRegSingerIndex(regsinger->getIndex());
            regSong->setSongIndex(song->getSongID());
            int regsongid = regsinger->getRegSongs()->addSong(regSong);
            song->setRegSongIndex(regsongid);
            query.exec("COMMIT TRANSACTION");
            regsinger->getRegSongs()->moveSong(regsongid, position);

        }
        return qsongid;
    }
    return -1;
}

void KhSinger::clearQueue()
{
    if ((isRegular()) && (regularSingers->getByRegularID(getRegularIndex()) != NULL))
       regularSingers->getByRegularID(getRegularIndex())->getRegSongs()->clear();
    songs->clear();
}

void KhSinger::moveSong(int oldPosition, int newPosition)
{
    songs->moveSong(oldPosition, newPosition);
}
