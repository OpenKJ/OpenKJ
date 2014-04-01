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

#ifndef KSPSONGDBTABLEMODEL_H
#define KSPSONGDBTABLEMODEL_H

#include <QAbstractTableModel>
#include <QtSql>
#include <algorithm>
#include <QThread>
#include "khsong.h"



#define SONGSORT_ATS 0
#define SONGSORT_TAS 1
#define SONGSORT_SAT 2
#define SORTDIR_ASC 0
#define SORTDIR_DEC 0

bool songsort(KhSong *song1, KhSong *song2);

class SongDBTableModel : public QAbstractTableModel
{
    Q_OBJECT
    KhSongs *fulldata;
    KhSongs *filteredData;
public:
    explicit SongDBTableModel(QObject *parent = 0);
    ~SongDBTableModel();
    enum {ARTIST=0,TITLE,DISCID,DURATION,MAX_COLS};
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    void sort(int column, Qt::SortOrder order);
    QMimeData* mimeData(const QModelIndexList &indexes) const;

    void addSong(KhSong *song);
    void removeSong(int row);
    void loadFromDB();
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QString filter;
    int lastSortCol;
    Qt::SortOrder lastSortOrder;
    void applyFilter(QString filterstr);
    KhSong *getRowSong(int row);
    KhSong *getSongByID(int songid);
    KhSongs *getDbSongs();

signals:
    
public slots:
    
};

class DBSortThread : public QThread
{
    Q_OBJECT
    void run();


signals:
    void resultReady(const QString &s);

public:
    KhSongs *mydata;
    int sortOrder;

};

#endif // KSPSONGDBTABLEMODEL_H
