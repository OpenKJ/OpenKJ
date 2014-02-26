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

#ifndef KSPROTATIONTABLEMODEL_H
#define KSPROTATIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QMimeData>
#include <boost/shared_ptr.hpp>
#include "khsinger.h"

class RotationTableModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    KhRotationSingers *singers;

public:
    explicit RotationTableModel(KhRotationSingers *singersObject, QObject *parent = 0);
    enum {ICON=0,NAME,NEXTSONG};
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent);
    Qt::DropActions supportedDropActions() const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    bool canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) const;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    QStringList mimeTypes() const;

signals:
    void songDroppedOnSinger(int singerid, int songid, int rowid);
    void notify_user(QString);

public slots:
    
};

#endif // KSPROTATIONTABLEMODEL_H
