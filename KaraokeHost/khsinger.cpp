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
#include <QtCore>
#include "khdb.h"

extern KhDb *db;

KhSinger::KhSinger(KhRegularSingers *regSingers, QObject *parent) :
    QObject(parent)
{
    m_songs = new KhQueueSongs(m_index,regSingers, m_regularIndex, this);
    m_regularSingers = regSingers;
    m_index = -1;
    m_position = -1;
    m_regular = false;
    m_regularIndex = -1;
    db = new KhDb(this);
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
        db->singerSetRegular(m_index, value);
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
        db->singerSetPosition(m_index, value);
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
        db->singerSetName(m_index, value);
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
        db->singerSetRegIndex(m_index, value);
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
    if (this != NULL)
        return m_songs->getNextSong();
    return NULL;
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
            db->beginTransaction();
            song->setRegSong(true);
            song->setRegSingerIndex(m_regularIndex);
            KhRegularSinger *regsinger = m_regularSingers->getByRegularID(m_regularIndex);
            KhRegularSong *regSong = new KhRegularSong();
            regSong->setRegSingerIndex(regsinger->getIndex());
            regSong->setSongIndex(song->getSongID());
            int regsongid = regsinger->getRegSongs()->addSong(regSong);
            song->setRegSongIndex(regsongid);
            db->endTransaction();
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
