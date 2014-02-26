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

#ifndef SONGDBTABLEMODEL_H
#define SONGDBTABLEMODEL_H

#include <QAbstractTableModel>
#include <QMimeData>
#include <QBuffer>
#include "bmsong.h"

class SongdbTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SongdbTableModel(BmSongs *songsObject, QObject *parent = 0);
    enum{ARTIST=0,TITLE,FILENAME,DURATION};
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;


signals:
    
public slots:
    void showMetadata(bool value);
    void showFilenames(bool value);
    
private:
    BmSongs *songs;
    bool m_showMetadata;
    bool m_showFilenames;
};

#endif // SONGDBTABLEMODEL_H
