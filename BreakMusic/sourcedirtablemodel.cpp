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

#include "sourcedirtablemodel.h"

SourceDirTableModel::SourceDirTableModel(BmSourceDirs *srcDirsObject, QObject *parent) :
    QAbstractTableModel(parent)
{
    srcDirs = srcDirsObject;
}

int SourceDirTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return srcDirs->size();
}

int SourceDirTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant SourceDirTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= srcDirs->size() || index.row() < 0)
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0:
            return srcDirs->at(index.row())->getPath();
        }
    }
    return QVariant();
}

QVariant SourceDirTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return tr("Path");
        }
    }
    return QVariant();
}

Qt::ItemFlags SourceDirTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
