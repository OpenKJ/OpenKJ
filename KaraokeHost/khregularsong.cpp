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
#include <QDebug>

KhRegularSong::KhRegularSong(QObject *parent) :
    QObject(parent)
{
}

KhRegularSong::~KhRegularSong()
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

KhRegularSongs::~KhRegularSongs()
{
    qDeleteAll(regSongs->begin(),regSongs->end());
    delete regSongs;
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
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM regularsongs WHERE ROWID == " + QString::number(index));
    for (int i=0; i < regSongs->size(); i++)
        if (regSongs->at(i)->getPosition() > song->getPosition())
            regSongs->at(i)->setPosition(regSongs->at(i)->getPosition() - 1);
    regSongs->erase(regSongs->begin() + (song->getPosition()));
    query.exec("COMMIT TRANSACTION");
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

bool sortKhRegularSongCompare(const KhRegularSong *s1, const KhRegularSong *s2)
{
    return s1->getPosition() < s2->getPosition();
}

void KhRegularSongs::sort()
{
    qSort(regSongs->begin(),regSongs->end(),sortKhRegularSongCompare);
}

void KhRegularSongs::clear()
{
    QSqlQuery query;
    qDebug() << QString("DELETE FROM regularsongs WHERE singer == " + QString::number(regSingerIndex));
    if (query.exec("DELETE FROM regularsongs WHERE singer == " + QString::number(regSingerIndex)))
        qDebug() << "SQL returned okay";
    else
        qDebug() << "SQL returned error";
    qDeleteAll(regSongs->begin(),regSongs->end());
    regSongs->clear();
}

QList<KhRegularSong *> *KhRegularSongs::getRegSongs()
{
    return regSongs;
}

int KhRegularSongs::addSong(KhRegularSong *regSong)
{
    regSong->setPosition(regSongs->size());
//    qDebug() << QString("INSERT INTO regularSongs (singer,song,keychg,position) VALUES(" + QString::number(regSingerIndex) + "," + QString::number(regSong->getSongIndex()) + "," + QString::number(regSong->getKeyChange()) + "," + QString::number(regSong->getPosition()) + ")");
    QSqlQuery query("INSERT INTO regularSongs (singer,song,keychg,position) VALUES(" + QString::number(regSingerIndex) + "," + QString::number(regSong->getSongIndex()) + "," + QString::number(regSong->getKeyChange()) + "," + QString::number(regSong->getPosition()) + ")");
    int regSongID = query.lastInsertId().toInt();
//    qDebug() << "Added regular song regid: " << regSongID;
    regSong->setRegSongIndex(regSongID);
    regSongs->push_back(regSong);
    return regSongID;
}

void KhRegularSongs::moveSong(int regSongID, int newPos)
{
    QSqlQuery query;
    KhRegularSong *regSong = getSongByIndex(regSongID);
    int oldPos = regSong->getPosition();
    query.exec("BEGIN TRANSACTION");
    if (newPos > oldPos)
    {
        for (int i=0; i < regSongs->size(); i++)
        {
            if ((regSongs->at(i)->getPosition() > oldPos) && (regSongs->at(i)->getPosition() <= newPos - 1) && (regSongs->at(i)->getRegSongIndex() != regSong->getRegSongIndex()))
                regSongs->at(i)->setPosition(regSongs->at(i)->getPosition() - 1);
        }
        regSong->setPosition(newPos - 1);
    }
    else if (newPos < oldPos)
    {
        for (int i=0; i < regSongs->size(); i++)
        {
            if ((regSongs->at(i)->getPosition() >= newPos) && (regSongs->at(i)->getPosition() < oldPos) && (regSongs->at(i)->getRegSongIndex() != regSong->getRegSongIndex()))
                regSongs->at(i)->setPosition(regSongs->at(i)->getPosition() + 1);
        }
        regSong->setPosition(newPos);
    }
    query.exec("COMMIT TRANSACTION");
}
