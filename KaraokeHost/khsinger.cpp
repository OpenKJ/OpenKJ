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


KhSinger::KhSinger(boost::shared_ptr<KhRegularSingers> regSingers, QObject *parent) :
    QObject(parent)
{
    boost::shared_ptr<KhQueueSongs> tmp_ptr(new KhQueueSongs(singerIndex,regSingers, regularIndex, this));
    songs.swap(tmp_ptr);

//    boost::shared_ptr<KhRegularSingers> tmp_reg(new KhRegularSingers);
//    regularSingers.swap(tmp_reg);
    regularSingers = regSingers;

}

bool KhSinger::isRegular() const
{
    return regular;
}

void KhSinger::setRegular(bool value, bool skipDB)
{
    qDebug() << "setRegular fired. Singer: " << singerIndex << " Regular: " << regular;
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
        songs->setRegSingerIndex(regularIndex);
    }
}

boost::shared_ptr<KhQueueSongsVector> KhSinger::getQueueSongs()
{
    return songs->getSongs();
}

boost::shared_ptr<KhQueueSongs> KhSinger::getQueueObject()
{
    return songs;
}

boost::shared_ptr<KhQueueSong> KhSinger::getSongByIndex(int queueSongID)
{
    return songs->getSongByIndex(queueSongID);
}

boost::shared_ptr<KhQueueSong> KhSinger::getSongByPosition(int position)
{
    return songs->getSongByPosition(position);
}

boost::shared_ptr<KhQueueSong> KhSinger::getNextSong()
{
    return songs->getNextSong();
}


//KhRotationSingers::KhRotationSingers()
//{
//    boost::shared_ptr<KhRotationData> tmp_ptr(new KhRotationData);
//    singers.swap(tmp_ptr);
//    loadFromDB();
//}

KhRotationSingers::KhRotationSingers(QObject *parent) :
    QObject(parent)
{
    boost::shared_ptr<KhRegularSingers> tmp_reg(new KhRegularSingers);
    regularSingers.swap(tmp_reg);
    boost::shared_ptr<KhRotationData> tmp_ptr(new KhRotationData);
    singers.swap(tmp_ptr);
    loadFromDB();
    selectedSingerIndex = -1;
    selectedSingerPosition = -1;
    currentSingerIndex = -1;
    currentSingerPosition = -1;
}

void KhRotationSingers::loadFromDB()
{
    singers->clear();
    QSqlQuery query("SELECT ROWID,name,position,regular,regularid FROM rotationSingers");
    int rotationsingerid = query.record().indexOf("ROWID");
    int name = query.record().indexOf("name");
    int position  = query.record().indexOf("position");
    int regular = query.record().indexOf("regular");
    int regularindex = query.record().indexOf("regularid");
    while (query.next()) {
        boost::shared_ptr<KhSinger> singer(new KhSinger(regularSingers,this));
        singer->setSingerIndex(query.value(rotationsingerid).toInt());
        singer->setSingerName(query.value(name).toString(),true);
        singer->setSingerPosition(query.value(position).toInt(),true);
        singer->setRegular(query.value(regular).toBool(),true);
        singer->setRegularIndex(query.value(regularindex).toInt());
        singers->push_back(singer);
    }
    sortSingers();
}

boost::shared_ptr<KhRotationData> KhRotationSingers::getSingers()
{
    return singers;
}

bool KhRotationSingers::moveSinger(int oldPosition, int newPosition)
{
    QSqlQuery query;
    boost::shared_ptr<KhSinger> movingSinger = getSingerByPosition(oldPosition);
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
    {
        if (currentSingerPosition == oldPosition)
            currentSingerPosition = newPosition - 1;
        else if ((currentSingerPosition <= newPosition) && (currentSingerPosition > oldPosition))
            currentSingerPosition--;
        for (unsigned int i=0; i < singers->size(); i++)
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
        for (unsigned int i=0; i < singers->size(); i++)
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

boost::shared_ptr<KhSinger> KhRotationSingers::getSingerByPosition(int position) const
{
    for (unsigned int i=0; i < singers->size(); i++)
    {
        if (position == singers->at(i)->getSingerPosition())
        {
            return singers->at(i);
        }
    }
    boost::shared_ptr<KhSinger> singer(new KhSinger(regularSingers));
    return singer;
}

boost::shared_ptr<KhSinger> KhRotationSingers::getSingerByIndex(int singerid)
{
        for (unsigned int i=0; i < singers->size(); i++)
        {
            if (singers->at(i)->getSingerIndex() == singerid)
                return singers->at(i);
        }
        return boost::shared_ptr<KhSinger>(new KhSinger(regularSingers));
}

int KhRotationSingers::getCurrentSingerPosition() const
{
    return currentSingerPosition;
}

void KhRotationSingers::setCurrentSingerPosition(int value)
{
    currentSingerPosition = value;
    currentSingerIndex = getSingerByPosition(value)->getSingerIndex();
}

bool KhRotationSingers::singerAdd(QString name, int position, bool regular)
{
    if (singerExists(name))
    {
        return false;
    }
    emit dataAboutToChange();
    QSqlQuery query;
    if (position == -1)
    {
        position = singers->size() + 1;
        bool result = query.exec("INSERT INTO rotationSingers (name, position, regular) VALUES(\"" + name + "\", " + QString::number(position) + "," + QString::number(regular) + ")");
        if (!result) return false;
        boost::shared_ptr<KhSinger> singer(new KhSinger(regularSingers,this));
        singer->setSingerName(name,true);
        singer->setSingerPosition(position,true);
        singer->setSingerIndex(query.lastInsertId().toInt());
        singer->setRegular(regular,true);
        singer->setRegularIndex(-1,true);
        singers->push_back(singer);
        emit dataChanged();
        return true;
    }
    // still need to implement other positional adds
    return false;
}

bool KhRotationSingers::singerExists(QString name)
{
    bool match = false;
    for (unsigned int i=0; i < singers->size(); i++)
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
    for (unsigned int i=0; i < singers->size(); i++)
    {
        if (singers->at(i)->getSingerPosition() >  delSingerPos)
            singers->at(i)->setSingerPosition(singers->at(i)->getSingerPosition() - 1);
    }
    query.exec("COMMIT TRANSACTION");
    sortSingers();
}

void KhRotationSingers::deleteSingerByPosition(int position)
{
    boost::shared_ptr<KhSinger> singer = getSingerByPosition(position);
    deleteSingerByIndex(singer->getSingerIndex());
}

void KhRotationSingers::clear()
{
    emit dataAboutToChange();
    QSqlQuery query;
    query.exec("DELETE FROM rotationsingers");
    query.exec("DELETE FROM queuesongs");
    currentSingerPosition = -1;
    singers->clear();
    emit dataChanged();
}

boost::shared_ptr<KhSinger> KhRotationSingers::getCurrent()
{
    return getSingerByPosition(currentSingerPosition);
}

boost::shared_ptr<KhSinger> KhRotationSingers::getSelected()
{
    return getSingerByIndex(selectedSingerIndex);
}

bool positionSort(boost::shared_ptr<KhSinger> singer1, boost::shared_ptr<KhSinger> singer2)
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




int KhRotationSingers::getCurrentSingerIndex() const
{
    return currentSingerIndex;
}

void KhRotationSingers::setCurrentSingerIndex(int value)
{
    currentSingerIndex = value;
    currentSingerPosition = getSingerByIndex(value)->getSingerPosition();
}

int KhRotationSingers::getSelectedSingerIndex() const
{
    return selectedSingerIndex;
}

void KhRotationSingers::setSelectedSingerIndex(int value)
{
    selectedSingerIndex = value;
    selectedSingerPosition = getSingerByIndex(value)->getSingerPosition();
}

void KhRotationSingers::createRegularForSinger(int singerID)
{
    boost::shared_ptr<KhSinger> singer = getSingerByIndex(singerID);
    int regularid = regularSingers->add(singer->getSingerName());
    singer->setRegular(true);
    singer->setRegularIndex(regularid);
    boost::shared_ptr<KhRegularSinger> regular = regularSingers->getByIndex(regularid);
    for (unsigned int i=0; i < singer->getQueueSongs()->size(); i++)
    {
        int regsongindex = regular->addSong(singer->getQueueSongs()->at(i)->getSongID(),singer->getQueueSongs()->at(i)->getKeyChange(),singer->getQueueSongs()->at(i)->getPosition());
        singer->getQueueSongs()->at(i)->setRegSong(true);
        singer->getQueueSongs()->at(i)->setRegSongIndex(regsongindex);
    }
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


int KhSinger::addSong(boost::shared_ptr<KhQueueSong> song)
{
    if (singerIndex != -1)
    {
        if (regular)
        {
            int regsongid = regularSingers->getByIndex(regularIndex)->addSong(song->getSongID(), song->getKeyChange(), song->getPosition());
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
        boost::shared_ptr<KhQueueSong> song = getSongByIndex(qsongid);
        if (regular)
        {
            int regsongid = regularSingers->getByIndex(regularIndex)->addSong(song->getSongID(), song->getKeyChange(), song->getPosition());
            song->setRegSong(true);
            song->setRegSongIndex(regsongid);
        }
        return qsongid;
    }
    return -1;
}

int KhSinger::addSongAtPosition(int songid, int position, bool regularSong, int regSongID)
{
    if (singerIndex != 0)
    {
        int qsongid = songs->addSongAtPosition(songid,position,regularSong,regSongID);
        boost::shared_ptr<KhQueueSong> song = getSongByIndex(qsongid);
        if (regular)
        {
            boost::shared_ptr<KhRegularSinger> regsinger = regularSingers->getByIndex(regularIndex);
            int regsongid = regsinger->addSong(song->getSongID(), song->getKeyChange(), song->getPosition());
            song->setRegSong(true);
            song->setRegSongIndex(regsongid);
        }
        return qsongid;
    }
    return -1;
}

void KhSinger::clearQueue()
{
    songs->clear();
}

void KhSinger::moveSong(int oldPosition, int newPosition)
{
    songs->moveSong(oldPosition, newPosition);
}
