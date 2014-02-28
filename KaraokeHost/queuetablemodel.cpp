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

#include "queuetablemodel.h"
#include <QSqlRecord>
#include <QSqlQuery>
#include <QStringList>
#include <QBrush>
#include <QDebug>
#include "khsinger.h"

QueueTableModel::QueueTableModel(KhRotationSingers *singersObject, QObject *parent) :
    QAbstractTableModel(parent)
{
    singers = singersObject;
    singers->setCurrentSingerIndex(-1);
}

int QueueTableModel::rowCount(const QModelIndex &parent) const
{
    UNUSED(parent);
//    return mydata->size();
    if (singers->getSelected() != NULL)
    return singers->getSelected()->getQueueSongs()->size();
    else
    return 0;
}

int QueueTableModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return 5;
}

QVariant QueueTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= singers->getSelected()->getQueueSongs()->size() || index.row() < 0)
        return QVariant();

    if((role == Qt::BackgroundRole) && (singers->getSelected()->getQueueSongs()->at(index.row())->getPlayed()))
    {
        return QBrush(Qt::gray);
    }
    if ((index.column() == 4) && (role == Qt::DecorationRole))
    {
        QPixmap icon(":/icons/Icons/edit-delete.png");
        return icon;
    }
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case ARTIST:
            return singers->getSelected()->getQueueSongs()->at(index.row())->getArtist();
        case TITLE:
            return singers->getSelected()->getQueueSongs()->at(index.row())->getTitle();
        case DISCID:
            return singers->getSelected()->getQueueSongs()->at(index.row())->getDiscID();
        case KEYCHANGE:
            int keychange = singers->getSelected()->getQueueSongs()->at(index.row())->getKeyChange();
            if (keychange == 0)
                return QVariant();
            else if (keychange > 0)
                return QString("+") + QString::number(keychange);
            else
                return QString::number(keychange);
        }
    }
    return QVariant();
}

QVariant QueueTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case ARTIST:
            return tr("Artist");
        case TITLE:
            return tr("Title");
        case DISCID:
            return tr("DiscID");
        case KEYCHANGE:
            return tr("Key");
        }
    }
    return QVariant();
}

bool QueueTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    UNUSED(action);
    UNUSED(column);
    int droprow;
    if (parent.row() >= 0)
        droprow = parent.row();
    else if (row >= 0)
        droprow = row;
    else
        droprow = singers->getSelected()->getQueueSongs()->size();

    if (data->hasFormat("text/plain"))
    {
        int dragrow;
        dragrow = data->text().toInt();
        singers->getSelected()->moveSong(dragrow,droprow);
        emit itemMoved();
        return true;
    }
    else if (data->hasFormat("integer/songid"))
    {
        if (singers->getSelectedSingerIndex() >= 0)
        {
            int songid;
            QByteArray bytedata = data->data("integer/songid");
            songid =  QString(bytedata.data()).toInt();
            layoutAboutToBeChanged();
            singers->getSelected()->addSongAtPosition(songid, droprow);
            layoutChanged();
            return true;
        }
    }
    return false;
}

Qt::DropActions QueueTableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData *QueueTableModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    foreach (const QModelIndex &index, indexes) {
        if (index.isValid()) {
            mimeData->setText(QString::number(index.row()));
        }
    }
    return mimeData;
}

bool QueueTableModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    UNUSED(data);
    UNUSED(action);
    UNUSED(row);
    UNUSED(column);
    UNUSED(parent);
    return true;
}

QStringList QueueTableModel::mimeTypes() const
{
    QStringList types;
    types << "text/plain";
    types << "integer/songid";
    return types;
}

Qt::ItemFlags QueueTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

void QueueTableModel::clear()
{
    layoutAboutToBeChanged();
    singers->getSelected()->clearQueue();
    layoutChanged();
}

void QueueTableModel::sort(int column, Qt::SortOrder order)
{
    layoutAboutToBeChanged();
    if (column == ARTIST)
    {
        if (order == Qt::AscendingOrder)
            singers->getSelected()->getQueueObject()->sortByArtist();
        else
            singers->getSelected()->getQueueObject()->sortByArtist(true);
    }
    else if (column == TITLE)
    {
        if (order == Qt::AscendingOrder)
            singers->getSelected()->getQueueObject()->sortByTitle();
        else
            singers->getSelected()->getQueueObject()->sortByTitle(true);
    }
    else if (column == DISCID)
    {
        if (order == Qt::AscendingOrder)
            singers->getSelected()->getQueueObject()->sortByDiscID();
        else
            singers->getSelected()->getQueueObject()->sortByDiscID(true);
    }
    else
    {
        if (singers->getSelected() != NULL)
            singers->getSelected()->getQueueObject()->sort();
    }
    layoutChanged();
}


