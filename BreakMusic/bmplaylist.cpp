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

#include "bmplaylist.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>


BmPlaylistSong::BmPlaylistSong(QObject *parent) :
    QObject(parent)
{
    setValid(false);
}

BmPlaylist::BmPlaylist(QObject *parent) :
    QObject(parent)
{
    songs = new QList<BmPlaylistSong *>;
    m_currentSong = getSongByPosition(0);
}

BmPlaylist::~BmPlaylist()
{
    qDeleteAll(songs->begin(),songs->end());
    delete songs;
}

void BmPlaylist::loadSongs()
{
    QSqlQuery query;
    query.exec("SELECT ROWID,song,position FROM plsongs WHERE playlist == " + QString::number(plIndex()) + " ORDER BY position");
    int index = query.record().indexOf("ROWID");
    int song = query.record().indexOf("song");
    int position = query.record().indexOf("position");
    while (query.next()) {
        BmPlaylistSong *plSong = new BmPlaylistSong(this);
        plSong->setIndex(query.value(index).toInt());
        plSong->setPosition(query.value(position).toInt(),true);
        plSong->setSong(new BmSong(query.value(song).toInt(),this));
        songs->push_back(plSong);
    }
}

unsigned int BmPlaylist::size()
{
    return songs->size();
}

BmPlaylists::BmPlaylists(QObject *parent) :
    QObject(parent)
{
    playlists = new QList<BmPlaylist *>;
    loadFromDB();
    if (playlists->size() < 1)
    {
        setCurrent(addPlaylist("Default"));
    }
    setCurrent(1);
}

BmPlaylists::~BmPlaylists()
{
    qDeleteAll(playlists->begin(),playlists->end());
    delete playlists;
}

unsigned int BmPlaylists::addPlaylist(QString title)
{
    if (!exists(title))
    {
        QSqlQuery query;
        query.exec("INSERT INTO playlists (title) VALUES(\"" + title + "\")");
        BmPlaylist *playlist = new BmPlaylist;
        playlist->setTitle(title);
        playlist->setPlIndex(query.lastInsertId().toInt());
        playlists->push_back(playlist);
        emit playlistsChanged();
        return playlist->plIndex();
    }
    return 0;
}

void BmPlaylist::addSong(BmSong *song)
{
    emit dataAboutToChange();
    BmPlaylistSong *plSong = new BmPlaylistSong(this);
    QSqlQuery query;
    query.exec("INSERT INTO plsongs (playlist,song,position) VALUES(" + QString::number(m_plIndex) + "," + QString::number(song->index()) + "," + QString::number(songs->size()) + ")");
    plSong->setSong(song);
    plSong->setPosition(songs->size(),true);
    plSong->setIndex(query.lastInsertId().toInt());
    songs->push_back(plSong);
    emit dataChanged();
}

void BmPlaylist::moveSong(unsigned int oldPosition, unsigned int newPosition)
{
    QSqlQuery query;
    BmPlaylistSong *movingSong = getSongByPosition(oldPosition);
    query.exec("BEGIN TRANSACTION");
    if (newPosition > oldPosition)
    {
        //Moving down
        for (int i=0; i < songs->size(); i++)
        {
            if ((songs->at(i)->position() > oldPosition) && (songs->at(i)->position() <= newPosition) && (songs->at(i)->index() != movingSong->index()))
                songs->at(i)->setPosition(songs->at(i)->position() - 1);
        }
        movingSong->setPosition(newPosition);
    }
    else if (newPosition < oldPosition)
    {
        //Moving up
        for (int i=0; i < songs->size(); i++)
        {
            if ((songs->at(i)->position() >= newPosition) && (songs->at(i)->position() < oldPosition) && (songs->at(i)->index() != movingSong->index()))
                songs->at(i)->setPosition(songs->at(i)->position() + 1);
        }
        movingSong->setPosition(newPosition);
    }
    query.exec("COMMIT TRANSACTION");
    sort();
}

unsigned int BmPlaylistSong::index() const
{
    return m_index;
}

void BmPlaylistSong::setIndex(unsigned int index)
{
    m_index = index;
}

BmSong *BmPlaylistSong::song() const
{
    return m_song;
}

void BmPlaylistSong::setSong(BmSong *song)
{
    m_song = song;
    setValid(true);
}

unsigned int BmPlaylistSong::position() const
{
    return m_position;
}

void BmPlaylistSong::setPosition(unsigned int position, bool skipDb)
{
    m_position = position;
    QSqlQuery query;
    if (!skipDb)
        query.exec("UPDATE plsongs SET position = " + QString::number(position) + " WHERE ROWID == " + QString::number(m_index));
}

QString BmPlaylist::title() const
{
    return m_title;
}

void BmPlaylist::setTitle(const QString &title)
{
    m_title = title;
}

void BmPlaylist::next()
{
    emit dataAboutToChange();
    m_currentSong = getNextSong();
    emit dataChanged();
}

bool positionSort(BmPlaylistSong *song1, BmPlaylistSong *song2)
{
    if (song1->position() >= song2->position())
        return false;
    else
        return true;
}

void BmPlaylist::sort()
{
    emit dataAboutToChange();
    std::sort(songs->begin(), songs->end(), positionSort);
    emit dataChanged();
}

unsigned int BmPlaylist::plIndex() const
{
    return m_plIndex;
}

void BmPlaylist::setPlIndex(unsigned int plIndex)
{
    m_plIndex = plIndex;
}

void BmPlaylist::setCurrentSongByPosition(int position)
{
    emit dataAboutToChange();
    m_currentSong = getSongByPosition(position);
    emit dataChanged();
}


void BmPlaylists::loadFromDB()
{
    emit dataAboutToChange();
    qDebug() << "Loading playlists from database";
    qDeleteAll(playlists->begin(),playlists->end());
    playlists->clear();
    QSqlQuery query("SELECT ROWID,title FROM playlists ORDER BY title");
    int index = query.record().indexOf("ROWID");
    int title = query.record().indexOf("title");
    while (query.next()) {
        BmPlaylist *playlist = new BmPlaylist();
        playlist->setPlIndex(query.value(index).toInt());
        playlist->setTitle(query.value(title).toString());
        playlist->loadSongs();
        playlists->push_back(playlist);
    }
    qDebug() << "Loading playlists from database - complete";
    emit dataChanged();
}


bool BmPlaylists::exists(QString title)
{
    for (int i=0; i < playlists->size(); i++)
    {
        if (playlists->at(i)->title() == title)
            return true;
    }
    return false;
}

QStringList BmPlaylists::getTitleList()
{
    QStringList titles;
    for (int i=0; i < playlists->size(); i++)
        titles << playlists->at(i)->title();
    return titles;
}


BmPlaylist *BmPlaylists::getCurrent()
{
    return currentPlaylist;
}

BmPlaylist *BmPlaylists::getByIndex(unsigned int plIndex)
{
    for (int i=0; i < playlists->size(); i++)
    {
        if (playlists->at(i)->plIndex() == plIndex)
            return playlists->at(i);
    }
    return new BmPlaylist(this);
}

BmPlaylist *BmPlaylists::getByTitle(QString plTitle)
{
    for (int i=0; i < playlists->size(); i++)
    {
        if (playlists->at(i)->title() == plTitle)
            return playlists->at(i);
    }
    return new BmPlaylist(this);
}


void BmPlaylists::setCurrent(int plIndex)
{
    emit dataAboutToChange();
    currentPlaylist = getByIndex(plIndex);
    emit dataChanged();
    emit currentPlaylistChanged(currentPlaylist->title());
    connect(currentPlaylist, SIGNAL(dataAboutToChange()), this, SIGNAL(dataAboutToChange()));
    connect(currentPlaylist, SIGNAL(dataChanged()),this, SIGNAL(dataChanged()));
}

void BmPlaylists::setCurrent(QString plTitle)
{
    emit dataAboutToChange();
    currentPlaylist = getByTitle(plTitle);
    emit dataChanged();
    emit currentPlaylistChanged(currentPlaylist->title());
    connect(currentPlaylist, SIGNAL(dataAboutToChange()), this, SIGNAL(dataAboutToChange()));
    connect(currentPlaylist, SIGNAL(dataChanged()),this, SIGNAL(dataChanged()));
}


BmPlaylistSong *BmPlaylist::at(int vectorPos)
{
    return songs->at(vectorPos);
}

bool BmPlaylistSong::valid() const
{
    return m_valid;
}

void BmPlaylistSong::setValid(bool valid)
{
    m_valid = valid;
}

BmPlaylistSong *BmPlaylist::getCurrentSong()
{
    return m_currentSong;
}

BmPlaylistSong *BmPlaylist::getNextSong()
{
    if (songs->size() == 0)
        return new BmPlaylistSong();
    int nextpos;
    if (m_currentSong->position() < (unsigned)songs->size() - 1)
        nextpos = m_currentSong->position() + 1;
    else
        nextpos = 0;
    return getSongByPosition(nextpos);
}

BmPlaylistSong *BmPlaylist::getSongByPosition(unsigned int position)
{
    for (int i=0; i < songs->size(); i++)
    {
        if (songs->at(i)->position() == position)
            return songs->at(i);
    }
    return new BmPlaylistSong(this);
}


void BmPlaylist::insertSong(int songid, unsigned int position)
{
    qDebug() << "doing insertSong(" << songid << "," << position << ")";
    QSqlQuery query;
    query.exec("INSERT INTO plsongs (playlist,song,position) VALUES(" + QString::number(m_plIndex) + "," + QString::number(songid) + "," + QString::number(position) + ")");
    BmPlaylistSong *plSong = new BmPlaylistSong(this);
    plSong->setIndex(query.lastInsertId().toInt());
    plSong->setPosition(position, true);
    plSong->setSong(new BmSong(songid,this));
    query.exec("BEGIN TRANSACTION");
    for (int i=0; i < songs->size(); i++)
    {
        if (songs->at(i)->position() >= position)
            songs->at(i)->setPosition(songs->at(i)->position() + 1);
    }
    query.exec("COMMIT TRANSACTION");
    songs->push_back(plSong);
    sort();
}


void BmPlaylist::removeSong(unsigned int position)
{
    emit dataAboutToChange();
    BmPlaylistSong *delSong = getSongByPosition(position);
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM plsongs WHERE ROWID == " + QString::number(delSong->index()));
    for (int i=0; i < songs->size(); i++)
    {
        if (songs->at(i)->position() > delSong->position())
            songs->at(i)->setPosition(songs->at(i)->position() - 1);
    }
    query.exec("COMMIT TRANSACTION");
    songs->erase(songs->begin() + (delSong->position()));
    emit dataChanged();
}
