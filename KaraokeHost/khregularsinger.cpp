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
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>

KhRegularSinger::KhRegularSinger(QString singerName, KhSongs *dbSongsPtr, QObject *parent)
{
    Q_UNUSED(parent);
    dbSongs = dbSongsPtr;
    QSqlQuery query;
    query.exec("INSERT INTO regularsingers (name) VALUES(\"" + singerName + "\")");
    regindex = query.lastInsertId().toInt();
    name = singerName;
    regSongs = new KhRegularSongs(regindex, dbSongs, this);
}

KhRegularSinger::KhRegularSinger(QString singerName, int singerID, KhSongs *dbSongsPtr, QObject *parent)
{
    Q_UNUSED(parent);
    dbSongs = dbSongsPtr;
    regindex = singerID;
    name = singerName;
    regSongs = new KhRegularSongs(regindex, dbSongs, this);
}

KhRegularSinger::~KhRegularSinger()
{
    delete regSongs;
}


KhRegularSingers::KhRegularSingers(KhSongs *dbSongsPtr, QObject *parent) :
    QObject(parent)
{
    dbSongs = dbSongsPtr;
    regularSingers = new QList<KhRegularSinger *>;
    loadFromDB();
}

KhRegularSingers::~KhRegularSingers()
{
    qDeleteAll(regularSingers->begin(),regularSingers->end());
    delete regularSingers;
}

QString KhRegularSinger::getName() const
{
    return name;
}

void KhRegularSinger::setName(const QString &value, bool skipDb)
{
    name = value;
    if (!skipDb)
    {
        QSqlQuery query;
        QString sql = "UPDATE regularSingers SET 'name'=\"" + value + "\" WHERE ROWID == " + QString::number(regindex);
        query.exec(sql);
    }
}

QList<KhRegularSinger *> *KhRegularSingers::getRegularSingers()
{
    return regularSingers;
}

KhRegularSinger *KhRegularSingers::getByRegularID(int regIndex)
{
    for (int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getIndex() == regIndex)
            return regularSingers->at(i);
    }
    return NULL;
}

KhRegularSinger *KhRegularSingers::getByName(QString regName)
{
    for (int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getName() == regName)
            return regularSingers->at(i);
    }
    return NULL;
}

bool KhRegularSingers::exists(QString searchName)
{
    for (int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getName().toLower() == searchName.toLower())
            return true;
    }
    return false;
}

int KhRegularSingers::add(QString name)
{
    if (!exists(name))
    {
        emit dataAboutToChange();
        KhRegularSinger *singer = new KhRegularSinger(name, dbSongs);
        regularSingers->push_back(singer);
        emit dataChanged();
        return singer->getIndex();
    }
    return -1;
}

int KhRegularSingers::size()
{
    return regularSingers->size();
}

KhRegularSinger *KhRegularSingers::at(int index)
{
    return regularSingers->at(index);
}

void KhRegularSingers::deleteSinger(int singerID)
{
    emit dataAboutToChange();
    int listIndex = getListIndexBySingerID(singerID);
    regularSingers->removeAt(listIndex);
    dbDeleteSinger(singerID);
    emit dataChanged();
    emit regularSingerDeleted(singerID);
}

void KhRegularSingers::deleteSinger(QString name)
{
    emit dataAboutToChange();
    int listIndex = getListIndexByName(name);
    emit regularSingerDeleted(regularSingers->at(listIndex)->getIndex());
    regularSingers->removeAt(listIndex);
    emit dataChanged();
}

void KhRegularSingers::exportSinger(int singerID, QString savePath)
{
    if (getByRegularID(singerID) != NULL)
    {
        KhRegularSinger *regSinger = getByRegularID(singerID);
        QFile *xmlFile = new QFile(savePath);
        xmlFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        QXmlStreamWriter xml(xmlFile);
        xml.setAutoFormatting(true);
        xml.writeStartDocument();
        xml.writeStartElement("regulars");
        xml.writeStartElement("singer");
        xml.writeAttribute("name", regSinger->getName());
        for (int i=0; i < regSinger->getRegSongs()->getRegSongs()->size(); i++)
        {
           KhRegularSong *regSong = regSinger->getRegSongs()->getRegSongs()->at(i);
           KhSong *dbSong;
           dbSong = NULL;
           for (int j=0; j < dbSongs->size(); j++)
           {
               if (dbSongs->at(j)->ID == regSong->getSongIndex())
               {
                   dbSong = dbSongs->at(j);
                   break;
               }
           }
           if (dbSong != NULL)
           {
               xml.writeStartElement("song");
               xml.writeAttribute("artist", dbSong->Artist);
               xml.writeAttribute("title", dbSong->Title);
               xml.writeAttribute("discid", dbSong->DiscID);
               xml.writeAttribute("key", QString::number(regSong->getKeyChange()));
               xml.writeEndElement();
           }

        }
        xml.writeEndElement();
        xml.writeEndElement();
        xml.writeEndDocument();
        xmlFile->close();
    }
}

void KhRegularSingers::exportSingers(QList<int> singerIDs, QString savePath)
{
    QFile *xmlFile = new QFile(savePath);
    xmlFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QXmlStreamWriter xml(xmlFile);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("regulars");
    for (int r=0; r < singerIDs.size(); r++)
    {
        int singerID = singerIDs.at(r);
        if (getByRegularID(singerID) != NULL)
        {
            KhRegularSinger *regSinger = getByRegularID(singerID);
            xml.writeStartElement("singer");
            xml.writeAttribute("name", regSinger->getName());
            for (int i=0; i < regSinger->getRegSongs()->getRegSongs()->size(); i++)
            {
                KhRegularSong *regSong = regSinger->getRegSongs()->getRegSongs()->at(i);
                KhSong *dbSong;
                dbSong = NULL;
                for (int j=0; j < dbSongs->size(); j++)
                {
                    if (dbSongs->at(j)->ID == regSong->getSongIndex())
                    {
                        dbSong = dbSongs->at(j);
                        break;
                    }
                }
                if (dbSong != NULL)
                {
                    xml.writeStartElement("song");
                    xml.writeAttribute("artist", dbSong->Artist);
                    xml.writeAttribute("title", dbSong->Title);
                    xml.writeAttribute("discid", dbSong->DiscID);
                    xml.writeAttribute("key", QString::number(regSong->getKeyChange()));
                    xml.writeEndElement();
                }

            }
            xml.writeEndElement();
        }
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    xmlFile->close();
}

QStringList KhRegularSingers::importLoadSingerList(QString fileName)
{
    QStringList singers;
    QFile *xmlFile = new QFile(fileName);
    xmlFile->open(QIODevice::ReadOnly);
    QXmlStreamReader xml(xmlFile);
    while (!xml.isEndDocument())
    {
        xml.readNext();
        if ((xml.isStartElement()) && (xml.name() == "singer"))
            singers << xml.attributes().value("name").toString();
    }
    xmlFile->close();
    singers.sort();
    return singers;
}

QList<KhRegImportSong> KhRegularSingers::importLoadSongs(QString name, QString fileName)
{
    QList<KhRegImportSong> songs;
    QFile *xmlFile = new QFile(fileName);
    xmlFile->open(QIODevice::ReadOnly);
    QXmlStreamReader xml(xmlFile);
    bool done = false;
    while ((!xml.isEndDocument()) && (!done))
    {
        xml.readNext();
        if ((xml.isStartElement()) && (xml.name() == "singer") && (xml.attributes().value("name") == name))
        {
            xml.readNext();
            while ((xml.name() != "singer") && (!xml.isEndDocument()))
            {
                if ((xml.isStartElement()) && (xml.name() == "song"))
                    songs.push_back(KhRegImportSong(xml.attributes().value("discid").toString(), xml.attributes().value("artist").toString(), xml.attributes().value("title").toString(), xml.attributes().value("keychange").toString().toInt()));
                xml.readNext();
            }
            done = true;
        }
    }
    xmlFile->close();
    return songs;
}

void KhRegularSingers::loadFromDB()
{
    emit dataAboutToChange();
    regularSingers->clear();
    QSqlQuery query("SELECT ROWID,name FROM regularSingers ORDER BY LOWER(name)");
    int regsingerid = query.record().indexOf("ROWID");
    int name = query.record().indexOf("name");
    while (query.next()) {
        regularSingers->push_back(new KhRegularSinger(query.value(name).toString(), query.value(regsingerid).toInt(), dbSongs));
    }
    emit dataChanged();
}

int KhRegularSingers::getListIndexBySingerID(int SingerID)
{
    for (int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getIndex() == SingerID)
            return i;
    }
    return -1;
}

int KhRegularSingers::getListIndexByName(QString name)
{
    for (int i=0; i < regularSingers->size(); i++)
    {
        if (regularSingers->at(i)->getName() == name)
            return i;
    }
    return -1;
}

void KhRegularSingers::dbDeleteSinger(int SingerID)
{
    QSqlQuery query;
    query.exec("DELETE FROM regularSingers WHERE ROWID=" + QString::number(SingerID));
    query.exec("DELETE FROM regularSongs WHERE SINGER=" + QString::number(SingerID));
}

int KhRegularSinger::getIndex() const
{
    return regindex;
}

void KhRegularSinger::setIndex(int value)
{
    regindex = value;
    regSongs = new KhRegularSongs(regindex, dbSongs);
}

KhRegularSongs *KhRegularSinger::getRegSongs() const
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
    KhRegularSong *song = new KhRegularSong;
    song->setRegSongIndex(query.lastInsertId().toInt());
    song->setRegSingerIndex(regindex);
    song->setSongIndex(songIndex);
    song->setKeyChange(keyChange);
    song->setPosition(position);
    regSongs->getRegSongs()->push_back(song);
    return query.lastInsertId().toInt();
}

KhRegularSong *KhRegularSinger::getSongByIndex(int index)
{
    for (int i=0; i < regSongs->getRegSongs()->size(); i++)
    {
        if (regSongs->getRegSongs()->at(i)->getRegSongIndex() == index)
            return regSongs->getRegSongs()->at(i);
    }
    return NULL;
}

int KhRegularSinger::songsSize()
{
    return regSongs->getRegSongs()->size();
}

QString KhRegImportSong::discId() const
{
    return m_discId;
}

void KhRegImportSong::setDiscId(const QString &discId)
{
    m_discId = discId;
}
QString KhRegImportSong::artist() const
{
    return m_artist;
}

void KhRegImportSong::setArtist(const QString &artist)
{
    m_artist = artist;
}
QString KhRegImportSong::title() const
{
    return m_title;
}

void KhRegImportSong::setTitle(const QString &title)
{
    m_title = title;
}
int KhRegImportSong::keyChange() const
{
    return m_keyChange;
}

void KhRegImportSong::setKeyChange(int keyChange)
{
    m_keyChange = keyChange;
}



