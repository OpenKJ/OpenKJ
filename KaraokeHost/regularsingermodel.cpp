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

#include "regularsingermodel.h"
#include <QPixmap>

RegularSingerModel::RegularSingerModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    regularSingers = new KhRegularSingers(this);
}


int RegularSingerModel::rowCount(const QModelIndex &parent) const
{
    return regularSingers->size();
}

int RegularSingerModel::columnCount(const QModelIndex &parent) const
{
    return 6;
}

QVariant RegularSingerModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if((unsigned)index.row() >= regularSingers->size() || index.row() < 0)
        return QVariant();

    if ((index.column() == LOAD) && (role == Qt::DecorationRole))
    {
        QPixmap icon(":/icons/Icons/list-add-user-small.png");
        return icon;
    }

    if ((index.column() == RENAME) && (role == Qt::DecorationRole))
    {
        QPixmap icon(":/icons/Icons/edit.png");
        return icon;
    }

    if ((index.column() == DEL) && (role == Qt::DecorationRole))
    {
        QPixmap icon(":/icons/Icons/edit-delete.png");
        return icon;
    }
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case REGID:
            return regularSingers->at(index.row())->getIndex();
        case SINGER:
            return regularSingers->at(index.row())->getName();
        case SONGCOUNT:
            return regularSingers->at(index.row())->songsSize();
        }
    }

    if(role == Qt::TextAlignmentRole)
    {
        switch(index.column())
        {
        case SINGER:
            return Qt::AlignLeft;
        case SONGCOUNT:
            return Qt::AlignCenter;
        }
    }
    return QVariant();
}

bool RegularSingerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
}

QVariant RegularSingerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole))
    {
        switch (section)
        {
        case REGID:
            return tr("ID");
        case SINGER:
            return tr("Regular Singer");
        case SONGCOUNT:
            return tr(" Songs ");
        }
    }
    if(role == Qt::TextAlignmentRole)
    {
        switch(section)
        {
        case SINGER:
            return Qt::AlignLeft;
        case SONGCOUNT:
            return Qt::AlignCenter;
        }
    }
    return QVariant();
}

Qt::ItemFlags RegularSingerModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
