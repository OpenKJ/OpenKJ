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
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "khsong.h"



#define SONGSORT_ATS 0
#define SONGSORT_TAS 1
#define SONGSORT_SAT 2
#define SORTDIR_ASC 0
#define SORTDIR_DEC 0

bool songsort(boost::shared_ptr<KhSong> song1, boost::shared_ptr<KhSong> song2);

class SongDBTableModel : public QAbstractTableModel
{
    Q_OBJECT
    boost::shared_ptr<std::vector<boost::shared_ptr<KhSong> > > fulldata;
    boost::shared_ptr<std::vector<boost::shared_ptr<KhSong> > > filteredData;
public:
    typedef std::vector<boost::shared_ptr<KhSong> >::const_iterator const_iterator;
    explicit SongDBTableModel(QObject *parent = 0);
    enum {ARTIST=0,TITLE,DISCID,DURATION,MAX_COLS};
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    void sort(int column, Qt::SortOrder order);
    QMimeData* mimeData(const QModelIndexList &indexes) const;

    void addSong(boost::shared_ptr<KhSong> song);
    void removeSong(int row);
    void loadFromDB(boost::shared_ptr<QSqlDatabase> db);
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
//    KSPDBSong& getSong(size_t index);
    const_iterator begin()const{return filteredData->begin();}
    const_iterator end()const{return filteredData->end();}
    QString filter;
    int lastSortCol;
    Qt::SortOrder lastSortOrder;
    void applyFilter(QString filterstr);
    boost::shared_ptr<KhSong> getRowSong(int row);
    boost::shared_ptr<KhSong> getSongByID(int songid);

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
    boost::shared_ptr<std::vector<boost::shared_ptr<KhSong> > > mydata;
    int sortOrder;

};

#endif // KSPSONGDBTABLEMODEL_H
