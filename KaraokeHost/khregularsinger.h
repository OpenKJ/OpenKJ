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

#ifndef KHREGULARSINGER_H
#define KHREGULARSINGER_H

#include <QObject>
#include "khregularsong.h"
#include <khsong.h>
#include <QStringList>

class KhRegImportSong
{
public:
    KhRegImportSong(QString discID, QString artist, QString title, int keyChange)
    {
        m_discId = discID;
        m_artist = artist;
        m_title = title;
        m_keyChange = keyChange;
    }
//    QString discID() { return m_discId; }
//    QString artist() { return m_artist; }
//    QString title() { return m_title; }
//    int keyChange() { return m_keyChange; }
    QString discId() const;
    void setDiscId(const QString &discId);

    QString artist() const;
    void setArtist(const QString &artist);

    QString title() const;
    void setTitle(const QString &title);

    int keyChange() const;
    void setKeyChange(int keyChange);

private:
    QString m_discId;
    QString m_artist;
    QString m_title;
    int m_keyChange;
};

class KhRegularSinger : public QObject
{
    Q_OBJECT
public:
    explicit KhRegularSinger(QString singerName, KhSongs *dbSongsPtr, QObject *parent = 0);
    explicit KhRegularSinger(QString singerName, int singerID, KhSongs *dbSongsPtr, QObject *parent = 0);
    ~KhRegularSinger();
    int getIndex() const;
    void setIndex(int value);
    QString getName() const;
    void setName(const QString &value, bool skipDb = false);
    KhRegularSongs *getRegSongs() const;
    int addSong(int songIndex, int keyChange, int position);
    KhRegularSong *getSongByIndex(int index);
    int songsSize();

signals:
    
public slots:

private:
    int regindex;
    QString name;
    KhRegularSongs *regSongs;
    KhSongs *dbSongs;
};

class KhRegularSingers : public QObject
{
    Q_OBJECT
public:
    explicit KhRegularSingers(KhSongs *dbSongsPtr, QObject *parent = 0);
    ~KhRegularSingers();
    QList<KhRegularSinger *> *getRegularSingers();
    KhRegularSinger *getByRegularID(int regIndex);
    KhRegularSinger *getByName(QString regName);
    bool exists(QString searchName);
    int add(QString name);
    int size();
    KhRegularSinger* at(int index);
    void deleteSinger(int singerID);
    void deleteSinger(QString name);
    void exportSinger(int singerID, QString savePath);
    void exportSingers(QList<int> singerIDs, QString savePath);
    QStringList importLoadSingerList(QString fileName);
    QList<KhRegImportSong> importLoadSongs(QString name, QString fileName);

signals:
    void dataAboutToChange();
    void dataChanged();
    void regularSingerDeleted(int regSingerIdx);

public slots:

private:
    void loadFromDB();
    QList<KhRegularSinger *> *regularSingers;
    int getListIndexBySingerID(int SingerID);
    int getListIndexByName(QString name);
    void dbDeleteSinger(int SingerID);
    KhSongs *dbSongs;

};



#endif // KHREGULARSINGER_H
