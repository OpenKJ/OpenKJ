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

#ifndef QUEUETABLEMODEL_H
#define QUEUETABLEMODEL_H

#define UNUSED(x) (void)x

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QMimeData>
#include <boost/shared_ptr.hpp>
#include "khqueuesong.h"
#include "khsinger.h"




class QueueTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit QueueTableModel(KhRotationSingers *singersObject, QObject *parent = 0);
    enum {ARTIST=0,TITLE,DISCID,KEYCHANGE};
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent);
    Qt::DropActions supportedDropActions() const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    bool canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) const;
    QStringList mimeTypes() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void sort(int column, Qt::SortOrder order);

    void clear();

signals:
    void itemMoved();

public slots:
    
private:
    KhRotationSingers *singers;
};

#endif // QUEUETABLEMODEL_H
