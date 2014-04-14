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

#include "bmsong.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

BmSong::BmSong(QObject *parent) :
    QObject(parent)
{
}

BmSong::BmSong(int songIndex, QObject *parent) :
    QObject(parent)
{
    QSqlQuery query("SELECT ROWID,artist,title,path,filename,duration FROM songs WHERE ROWID == " + QString::number(songIndex));
    int index = query.record().indexOf("ROWID");
    int artist = query.record().indexOf("artist");
    int title = query.record().indexOf("title");
    int path = query.record().indexOf("path");
    int filename = query.record().indexOf("filename");
    int duration = query.record().indexOf("duration");
    while (query.next()) {
        setIndex(query.value(index).toInt());
        setArtist(query.value(artist).toString());
        setTitle(query.value(title).toString());
        setPath(query.value(path).toString());
        setFilename(query.value(filename).toString());
        setDuration(query.value(duration).toInt());
    }
}

int BmSong::index() const
{
    return m_index;
}

void BmSong::setIndex(int index)
{
    m_index = index;
}

int BmSong::duration() const
{
    return m_duration;
}

void BmSong::setDuration(int duration)
{
    m_duration = duration;
}

QString BmSong::durationStr()
{
    int min;
    int sec;
    QString minStr;
    QString secStr;
    min = (int) (duration() / 60);
    sec = duration() % 60;
    minStr = QString::number(min);
    if (sec < 10)
        secStr = "0" + QString::number(sec);
    else
        secStr = QString::number(sec);
    return QString(minStr + ":" + secStr);
}

QString BmSong::filename() const
{
    return m_filename;
}

void BmSong::setFilename(const QString &filename)
{
    m_filename = filename;
}

QString BmSong::getSearchableString() const
{
    return QString(m_filename + " " + m_artist + " " + m_title);
}

QString BmSong::path() const
{
    return m_path;
}

void BmSong::setPath(const QString &path)
{
    m_path = path;
}

QString BmSong::title() const
{
    return m_title;
}

void BmSong::setTitle(const QString &title)
{
    m_title = title;
}

QString BmSong::artist() const
{
    return m_artist;
}

void BmSong::setArtist(const QString &artist)
{
    m_artist = artist;
}


BmSongs::BmSongs(QObject *parent) :
    QObject(parent)
{
    filteredSongs = new QList<BmSong *>;
    allSongs = new QList<BmSong *>;
}

BmSongs::~BmSongs()
{
    qDeleteAll(allSongs->begin(),allSongs->end());
    // Don't need to qDeleteAll from filteredSongs as it contains pointers to objects already being deleted
    delete filteredSongs;
    delete allSongs;
}

void BmSongs::loadFromDB()
{
    emit dataAboutToChange();
    qDebug() << "Loading songs from database";
    qDeleteAll(allSongs->begin(),allSongs->end());
//  qDeleteAll(filteredSongs->begin(),filteredSongs->end());
    allSongs->clear();
    filteredSongs->clear();
    filterTerms.clear();
    QSqlQuery query("SELECT ROWID,artist,title,path,filename,duration FROM songs ORDER BY filename");
    int index = query.record().indexOf("ROWID");
    int artist = query.record().indexOf("artist");
    int title = query.record().indexOf("title");
    int path = query.record().indexOf("path");
    int filename = query.record().indexOf("filename");
    int duration = query.record().indexOf("duration");
    while (query.next()) {
        BmSong *song = new BmSong();
        song->setIndex(query.value(index).toInt());
        song->setArtist(query.value(artist).toString());
        song->setTitle(query.value(title).toString());
        song->setPath(query.value(path).toString());
        song->setFilename(query.value(filename).toString());
        song->setDuration(query.value(duration).toInt());
        allSongs->push_back(song);
        filteredSongs->push_back(song);
    }
    emit dataChanged();
    qDebug() << "Loading songs from database - complete";
}

void BmSongs::setFilterTerms(QStringList terms)
{
    // if no search terms return full data set
    if (terms.size() == 0)
    {
        emit dataAboutToChange();
        filteredSongs->clear();
        for (int i=0; i < allSongs->size(); i++)
            filteredSongs->push_back(allSongs->at(i));
        emit dataChanged();
        return;
    }

    emit dataAboutToChange();
    filteredSongs->clear();
    for (int i=0; i < allSongs->size(); i++)
    {
        bool match = true;
        for (int j=0; j < terms.size(); j++)
        {
            if (!allSongs->at(i)->getSearchableString().contains(terms.at(j), Qt::CaseInsensitive))
            {
                match = false;
                break;
            }
        }

        if (match)
            filteredSongs->push_back(allSongs->at(i));
    }
    emit dataChanged();
}


unsigned int BmSongs::size()
{
    return filteredSongs->size();
}


BmSong *BmSongs::at(int vectorIndex)
{
    return filteredSongs->at(vectorIndex);
}

BmSong *BmSongs::getSongByPath(QString path)
{
    for (int i=0; i < allSongs->size(); i++)
    {
        if (allSongs->at(i)->path() == path)
            return allSongs->at(i);
    }
    return NULL;
}
