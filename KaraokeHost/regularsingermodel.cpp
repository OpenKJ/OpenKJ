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

RegularSingerModel::RegularSingerModel(KhRegularSingers *regulars, QObject *parent) :
    QAbstractTableModel(parent)
{
    regularSingers = regulars;
    connect(regularSingers, SIGNAL(dataAboutToChange()), this, SIGNAL(layoutAboutToBeChanged()));
    connect(regularSingers, SIGNAL(dataChanged()), this, SIGNAL(layoutChanged()));
}


int RegularSingerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return regularSingers->size();
}

int RegularSingerModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QVariant RegularSingerModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= regularSingers->size() || index.row() < 0)
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
    if(role == Qt::DisplayRole || role == Qt::EditRole)
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
    Q_UNUSED(role);
    if (index.isValid() && role == Qt::EditRole && !(index.row() >= regularSingers->size() || index.row() < 0))
    {
        if (regularSingers->exists(value.toString()))
        {
            //QMessageBox::warning(this, tr("Duplicate Name"), tr("A regular singer by that name already exists, edit cancelled."),QMessageBox::Close);
            emit editSingerDuplicateError();
            return false;
        }
        regularSingers->at(index.row())->setName(value.toString());
        //singers->getSingerByPosition(index.row() + 1)->setSingerName(value.toString());
        return true;
    }
    else
        return false;
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
    if (index.column() == SINGER)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void RegularSingerModel::removeByName(QString name)
{
    layoutAboutToBeChanged();
    regularSingers->deleteSinger(name);
    layoutChanged();

}

void RegularSingerModel::removeBySingerID(int singerID)
{
    layoutAboutToBeChanged();
    regularSingers->deleteSinger(singerID);
    layoutChanged();
}

void RegularSingerModel::removeByListIndex(int listIndex)
{
    layoutAboutToBeChanged();
    regularSingers->deleteSinger(regularSingers->at(listIndex)->getIndex());
    layoutChanged();
}

KhRegularSinger *RegularSingerModel::getRegularSingerByListIndex(int listIndex)
{
    return regularSingers->at(listIndex);
}
