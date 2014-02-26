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

#include "rotationtablemodel.h"
#include <QMessageBox>
#include <QDebug>


RotationTableModel::RotationTableModel(KhRotationSingers *singersObject, QObject *parent) :
    QAbstractTableModel(parent)
{
    singers = singersObject;
    singers->setCurrentSingerPosition(-1);
    connect(singers,SIGNAL(dataAboutToChange()), this, SIGNAL(layoutAboutToBeChanged()));
    connect(singers, SIGNAL(dataChanged()), this, SIGNAL(layoutChanged()));
}

QVariant RotationTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if((unsigned)index.row() >= singers->getSingers()->size() || index.row() < 0)
        return QVariant();

//    if((role == Qt::BackgroundRole) && (singers->getSingers()->at(index.row())->getSingerPosition() == singers->getCurrentSingerPosition()))
    if((role == Qt::BackgroundRole) && (index.row() == (singers->getCurrentSingerPosition()) - 1))
        return QBrush(Qt::yellow);

    if (role == Qt::DecorationRole)
    {
        switch(index.column())
        {
        case 0:
            if (singers->getSingers()->at(index.row())->getSingerPosition() == singers->getCurrentSingerPosition())
                return QPixmap(":/icons/microphone");
            else
                return QVariant();
        case 3:
            return QPixmap(":/icons/Icons/edit.png");
        case 4:
            return QPixmap(":/icons/Icons/edit-delete.png");
        case 5:
            if (singers->getSingers()->at(index.row())->isRegular())
                return QPixmap(":/icons/Icons/emblem-favorite-16x16.png");
            else
                return QPixmap(":/icons/Icons/emblem-favorite-disabled-16x16.png");
        default:
            return QVariant();
        }
    }

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case NAME:
            return singers->getSingers()->at(index.row())->getSingerName();
        case NEXTSONG:
            return singers->getNextSongBySingerPosition(singers->getSingers()->at(index.row())->getSingerPosition());
        }
    }
    return QVariant();
}

QVariant RotationTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return tr("");
        case 1:
            return tr("Singer");
        case 2:
            return tr("Next Song");
        case 3:
            return tr("");
        case 4:
            return tr("");
        case 5:
            return tr("");
        }
    }
    return QVariant();
}

bool RotationTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);
    if (index.isValid() && role == Qt::EditRole && !((unsigned)index.row() >= singers->getSingers()->size() || index.row() < 0))
    {
        if (singers->singerExists(value.toString()))
        {
            emit notify_user("Error: Duplicate singer name.  Edit cancelled.");
            return false;
        }
        singers->getSingerByPosition(index.row() + 1)->setSingerName(value.toString());
        return true;
    }
    else
        return false;
}

Qt::ItemFlags RotationTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    if (index.column() == NAME)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    if (index.column() >= 3)
        return Qt::ItemIsEnabled;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

int RotationTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return singers->getSingers()->size();
}

int RotationTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}


bool RotationTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(column);
    int droprow;
    if (parent.row() >= 0)
        droprow = parent.row();
    else if (row >= 0)
        droprow = row;
    else
        droprow = singers->getSingers()->size();
    if (data->hasFormat("text/plain"))
    {
        int dragrow;
        dragrow = data->text().toInt();
        singers->moveSinger(dragrow + 1,droprow + 1);
        return true;
    }
    else if (data->hasFormat("integer/songid"))
    {
        if (parent.row() >= 0)
        {
            QByteArray bytedata = data->data("integer/songid");
            int songid =  QString(bytedata.data()).toInt();
            int singerid = singers->getSingerByPosition(droprow + 1)->getSingerIndex();
            emit songDroppedOnSinger(singerid,songid, parent.row());
        }
    }
    return false;
}

Qt::DropActions RotationTableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData* RotationTableModel::mimeData(const QModelIndexList &indexes) const
{
        QMimeData *mimeData = new QMimeData();
        foreach (const QModelIndex &index, indexes) {
            if (index.isValid()) {
                mimeData->setText(QString::number(index.row()));
            }
        }
        return mimeData;
}

bool RotationTableModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(data);
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    return true;
}

bool RotationTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(count);
    Q_UNUSED(parent);
    return false;
}

QStringList RotationTableModel::mimeTypes() const
{
    QStringList types;
    types << "text/plain";
    types << "integer/songid";
    return types;
}
