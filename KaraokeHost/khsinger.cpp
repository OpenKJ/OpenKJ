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
    m_songs = new KhQueueSongs(m_index,regSingers, m_regularIndex, this);
    m_regularSingers = regSingers;
    m_index = -1;
    m_position = -1;
    m_regular = false;
    m_regularIndex = -1;
}

bool KhSinger::regular() const
{
    return m_regular;
}

void KhSinger::setRegular(bool value, bool skipDB)
{
    m_regular = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'regular'=" + QString::number(m_regular) + " WHERE ROWID == " + QString::number(m_index);
        query.exec(sql);
    }
}

int KhSinger::position() const
{
    return m_position;
}

void KhSinger::setPosition(int value, bool skipDB)
{
    m_position = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'position'=" + QString::number(m_position) + " WHERE ROWID == " + QString::number(m_index);
        query.exec(sql);
    }
}

QString KhSinger::name() const
{
    return m_name;
}

void KhSinger::setName(const QString &value, bool skipDB)
{
    m_name = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'name'=\"" + m_name + "\" WHERE ROWID == " + QString::number(m_index);
        query.exec(sql);
    }
}

int KhSinger::index() const
{
    return m_index;
}

void KhSinger::setIndex(int value)
{
    m_songs->setSingerIndex(value);
    m_index = value;
}

int KhSinger::regularIndex() const
{
    return m_regularIndex;
}

void KhSinger::setRegularIndex(int value, bool skipDB)
{
    m_regularIndex = value;
    if (!skipDB)
    {
        QSqlQuery query;
        QString sql = "UPDATE rotationsingers SET 'regularid'=" + QString::number(m_regularIndex) + " WHERE ROWID == " + QString::number(m_index);
        query.exec(sql);
    }
    m_songs->setRegSingerIndex(m_regularIndex,skipDB);
}

QList<KhQueueSong *> *KhSinger::queueSongs()
{
    return m_songs->getSongs();
}

KhQueueSongs *KhSinger::queueObject()
{
    return m_songs;
}

KhQueueSong *KhSinger::getSongByIndex(int queueSongID)
{
    return m_songs->getSongByIndex(queueSongID);
}

KhQueueSong *KhSinger::getSongByPosition(int position)
{
    return m_songs->getSongByPosition(position);
}

KhQueueSong *KhSinger::getNextSong()
{
    return m_songs->getNextSong();
}

KhSingers::KhSingers(KhRegularSingers *regSingersPtr, QObject *parent) :
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

KhSingers::~KhSingers()
{
    qDeleteAll(singers->begin(),singers->end());
    delete singers;
    //delete regularSingers;
}

void KhSingers::loadFromDB()
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
        singer->setIndex(query.value(rotationsingerid).toInt());
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
        singer->setName(query.value(name).toString(),true);
        singer->setPosition(query.value(position).toInt(),true);
        singers->push_back(singer);
    }
    sortSingers();
}

QList<KhSinger *> *KhSingers::getSingers()
{
    return singers;
}

bool KhSingers::moveSinger(int oldPosition, int newPosition)
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
            if ((singers->at(i)->position() > oldPosition) && (singers->at(i)->position() <= newPosition - 1) && (singers->at(i)->index() != movingSinger->index()))
                singers->at(i)->setPosition(singers->at(i)->position() - 1);
        }
        movingSinger->setPosition(newPosition - 1);
    }
    else if (newPosition < oldPosition)
    {
        if (currentSingerPosition == oldPosition)
            currentSingerPosition = newPosition;
        else if ((currentSingerPosition >= newPosition) && (currentSingerPosition < oldPosition))
            currentSingerPosition++;
        for (int i=0; i < singers->size(); i++)
        {
            if ((singers->at(i)->position() >= newPosition) && (singers->at(i)->position() < oldPosition) && (singers->at(i)->index() != movingSinger->index()))
                singers->at(i)->setPosition(singers->at(i)->position() + 1);
        }
        movingSinger->setPosition(newPosition);
    }
    query.exec("COMMIT TRANSACTION");
    sortSingers();
    return true;
}

KhSinger *KhSingers::getSingerByPosition(int position) const
{
    if (position < 0) return NULL;
    for (int i=0; i < singers->size(); i++)
    {
        if (position == singers->at(i)->position())
        {
            return singers->at(i);
        }
    }
    return NULL;
}

KhSinger *KhSingers::getSingerByIndex(int singerid)
{
        for (int i=0; i < singers->size(); i++)
        {
            if (singers->at(i)->index() == singerid)
                return singers->at(i);
        }
        return NULL;
}

KhSinger *KhSingers::getSingerByName(QString name)
{
    for (int i=0; i < singers->size(); i++)
    {
        if (singers->at(i)->name() == name)
            return singers->at(i);
    }
    return NULL;
}

int KhSingers::getCurrentSingerPosition() const
{
    return currentSingerPosition;
}

void KhSingers::setCurrentSingerPosition(int value)
{
    emit dataAboutToChange();
    currentSingerPosition = value;
    if (getSingerByPosition(value) != NULL)
        currentSingerIndex = getSingerByPosition(value)->index();
    else
        currentSingerIndex = -1;
    emit dataChanged();
}

bool KhSingers::add(QString name, int position, bool regular)
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
    singer->setName(name,true);
    singer->setPosition(nextPos,true);
    singer->setIndex(query.lastInsertId().toInt());
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

bool KhSingers::exists(QString name)
{
    bool match = false;
    for (int i=0; i < singers->size(); i++)
    {
        if (name.toLower() == singers->at(i)->name().toLower())
        {
            match = true;
            break;
        }
    }
    return match;
}

QString KhSingers::getNextSongBySingerPosition(int position) const
{
    QString nextSong;
    int singerid = getSingerByPosition(position)->index();
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

void KhSingers::deleteSingerByIndex(int singerid)
{
    QSqlQuery query;
    int delSingerPos = getSingerByIndex(singerid)->position();
    if (singerid == currentSingerIndex) setCurrentSingerPosition(-1);
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM queueSongs WHERE singer == " + QString::number(singerid));
    query.exec("DELETE FROM rotationSingers WHERE ROWID == " + QString::number(singerid));
    singers->erase(singers->begin() + (delSingerPos - 1));
    for (int i=0; i < singers->size(); i++)
    {
        if (singers->at(i)->position() >  delSingerPos)
            singers->at(i)->setPosition(singers->at(i)->position() - 1);
    }
    query.exec("COMMIT TRANSACTION");
    sortSingers();
}

void KhSingers::deleteSingerByPosition(int position)
{
    KhSinger *singer = getSingerByPosition(position);
    deleteSingerByIndex(singer->index());
}

void KhSingers::clear()
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

KhSinger *KhSingers::getCurrent()
{
    return getSingerByPosition(currentSingerPosition);
}

KhSinger *KhSingers::getSelected()
{
    return getSingerByIndex(selectedSingerIndex);
}

bool positionSort(KhSinger *singer1, KhSinger *singer2)
{
    if (singer1->position() >= singer2->position())
        return false;
    else
        return true;
}

void KhSingers::sortSingers()
{
    emit dataAboutToChange();
    std::sort(singers->begin(), singers->end(), positionSort);
    emit dataChanged();
}

void KhSingers::regularSingerDeleted(int RegularID)
{
    for (int i=0; i < singers->size(); i++)
    {
        if (singers->at(i)->regularIndex() == RegularID)
        {
            emit dataAboutToChange();
            singers->at(i)->setRegular(false);
            singers->at(i)->setRegularIndex(-1);
            emit dataChanged();
        }
    }
}




int KhSingers::getCurrentSingerIndex() const
{
    return currentSingerIndex;
}

void KhSingers::setCurrentSingerIndex(int value)
{
    emit dataAboutToChange();
    currentSingerIndex = value;
    if (getSingerByIndex(value) != NULL)
        currentSingerPosition = getSingerByIndex(value)->position();
    else
        currentSingerPosition = -1;
    emit dataChanged();
}

int KhSingers::getSelectedSingerIndex() const
{
    return selectedSingerIndex;
}

void KhSingers::setSelectedSingerIndex(int value)
{
    selectedSingerIndex = value;
    if (getSingerByIndex(value) != NULL)
        selectedSingerPosition = getSingerByIndex(value)->position();
    else
        selectedSingerPosition = -1;
}

void KhSingers::createRegularForSinger(int singerID)
{
    KhSinger *singer = getSingerByIndex(singerID);
    QSqlQuery query("BEGIN TRANSACTION");
    int regularid = regularSingers->add(singer->name());
    singer->setRegular(true);
    singer->setRegularIndex(regularid);
    KhRegularSinger *regular = regularSingers->getByRegularID(regularid);
    for (int i=0; i < singer->queueSongs()->size(); i++)
    {
        int regsongindex = regular->addSong(singer->queueSongs()->at(i)->getSongID(),singer->queueSongs()->at(i)->getKeyChange(),singer->queueSongs()->at(i)->getPosition());
        singer->queueSongs()->at(i)->setRegSong(true);
        singer->queueSongs()->at(i)->setRegSongIndex(regsongindex);
    }
    query.exec("COMMIT TRANSACTION");
}

QStringList KhSingers::getSingerList()
{
    QStringList singerList;
    for (int i=0; i < singers->size(); i++)
        singerList << singers->at(i)->name();
    singerList.sort();
    return singerList;
}

int KhSingers::getSelectedSingerPosition() const
{
    return selectedSingerPosition;
}

void KhSingers::setSelectedSingerPosition(int value)
{
    selectedSingerPosition = value;
    selectedSingerIndex = getSingerByPosition(value)->index();
}


int KhSinger::addSong(KhQueueSong *song)
{
    if (m_index != -1)
    {
        if (m_regular)
        {
            int regsongid = m_regularSingers->getByRegularID(m_regularIndex)->addSong(song->getSongID(), song->getKeyChange(), song->getPosition());
            song->setRegSong(true);
            song->setRegSongIndex(regsongid);
        }
        int qsongid = m_songs->addSong(song);
        return qsongid;
    }
    return -1;
}

int KhSinger::addSongAtEnd(int songid, bool regularSong, int regSongID)
{
    if (m_index != -1)
    {
        int qsongid = m_songs->addSongAtEnd(songid, regularSong,regSongID);
        if (m_regular)
        {
            KhQueueSong *song = getSongByIndex(qsongid);
            int regsongid = m_regularSingers->getByRegularID(m_regularIndex)->addSong(song->getSongID(), song->getKeyChange(), song->getPosition());
            song->setRegSong(true);
            song->setRegSongIndex(regsongid);
            song->setRegSingerIndex(m_regularIndex);
        }
        return qsongid;
    }
    return -1;
}

int KhSinger::addSongAtPosition(int songid, int position, bool regularSong, int regSongID)
{
    if (m_index != 0)
    {
        int qsongid = m_songs->addSongAtPosition(songid,position,regularSong,regSongID,m_regularIndex);
        KhQueueSong *song = getSongByIndex(qsongid);
        if (m_regular)
        {
            QSqlQuery query("BEGIN TRANSACTION");
            song->setRegSong(true);
            song->setRegSingerIndex(m_regularIndex);
            KhRegularSinger *regsinger = m_regularSingers->getByRegularID(m_regularIndex);
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
    if ((regular()) && (m_regularSingers->getByRegularID(regularIndex()) != NULL))
       m_regularSingers->getByRegularID(regularIndex())->getRegSongs()->clear();
    m_songs->clear();
}

void KhSinger::moveSong(int oldPosition, int newPosition)
{
    m_songs->moveSong(oldPosition, newPosition);
}
