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

#include "playlisttablemodel.h"
#include <QDebug>
#include <QBrush>


PlaylistTableModel::PlaylistTableModel(BmPlaylists *playlistsObject, QObject *parent) :
    QAbstractTableModel(parent)
{
    playlists = playlistsObject;
    connect(playlists, SIGNAL(dataAboutToChange()), this, SIGNAL(layoutAboutToBeChanged()));
    connect(playlists, SIGNAL(dataChanged()), this, SIGNAL(layoutChanged()));
    m_showFilenames = false;
    m_showMetadata = true;
}

int PlaylistTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return playlists->getCurrent()->size();
}

int PlaylistTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QVariant PlaylistTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if((unsigned)index.row() >= playlists->getCurrent()->size() || index.row() < 0)
        return QVariant();

    if((role == Qt::BackgroundRole) && (playlists->getCurrent()->getCurrentSong()->position() == (unsigned)index.row()))
    {
        return QBrush(Qt::yellow);
    }
    if ((index.column() == 0) && (role == Qt::DecorationRole) && (playlists->getCurrent()->getCurrentSong()->position() == (unsigned)index.row()))
    {
        QPixmap icon(":/icons/play-small.png");
        return icon;
    }

    if ((index.column() == 5) && (role == Qt::DecorationRole))
    {
        QPixmap icon(":/icons/edit-delete.png");
        return icon;
    }

    if(role == Qt::DisplayRole)
    {

        switch(index.column())
        {
        case 1:
            return playlists->getCurrent()->at(index.row())->song()->artist();
        case 2:
            return playlists->getCurrent()->at(index.row())->song()->title();
        case 3:
            return playlists->getCurrent()->at(index.row())->song()->filename();
        case 4:
            return playlists->getCurrent()->at(index.row())->song()->durationStr();
        }
    }
    return QVariant();
}

QVariant PlaylistTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 1:
            return tr("Artist");
        case 2:
            return tr("Title");
        case 3:
            return tr("Filename");
        case 4:
            return tr("Duration");
        }
    }
    return QVariant();
}

bool PlaylistTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(column);


    if (data->hasFormat("integer/queuepos"))
    {
        int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = playlists->getCurrent()->size() - 1;
        int queuepos;
        QByteArray bytedata = data->data("integer/queuepos");
        queuepos =  QString(bytedata.data()).toInt();
        int newpos = playlists->getCurrent()->at(droprow)->position();
        playlists->getCurrent()->moveSong(queuepos,newpos);
        return true;
    }
    if (data->hasFormat("integer/songid"))
    {
        unsigned int droprow;
        if (parent.row() >= 0)
            droprow = parent.row();
        else if (row >= 0)
            droprow = row;
        else
            droprow = playlists->getCurrent()->size();
        int songid;
        QByteArray bytedata = data->data("integer/songid");
        songid = QString(bytedata.data()).toInt();
        int position;
        if (droprow >= playlists->getCurrent()->size())
            position = playlists->getCurrent()->size();
        else
            position = playlists->getCurrent()->at(droprow)->position();
        playlists->getCurrent()->insertSong(songid,position);
        return true;
    }
    return false;
}

Qt::DropActions PlaylistTableModel::supportedDropActions() const
{
        return Qt::CopyAction | Qt::MoveAction;
}

int PlaylistTableModel::getColumnCount()
{
        return 6;
}

void PlaylistTableModel::showMetadata(bool value)
{
    layoutAboutToBeChanged();
    m_showMetadata = value;
    layoutChanged();
}

void PlaylistTableModel::showFilenames(bool value)
{
    layoutAboutToBeChanged();
    m_showFilenames = value;
    layoutChanged();
}


QMimeData *PlaylistTableModel::mimeData(const QModelIndexList &indexes) const
{
    Q_UNUSED(indexes);
    QMimeData *mimeData = new QMimeData();
    QByteArray output;
    QBuffer outputBuffer(&output);
    int queuepos = playlists->getCurrent()->at(indexes.at(0).row())->position();
    outputBuffer.open(QIODevice::WriteOnly);
    outputBuffer.write(QString::number(queuepos).toLocal8Bit());
    mimeData->setData("integer/queuepos", output);
    return mimeData;
}

bool PlaylistTableModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(data);
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    return true;
}


QStringList PlaylistTableModel::mimeTypes() const
{
    QStringList types;
    types << "integer/songid";
    types << "integer/queuepos";
    return types;
}

Qt::ItemFlags PlaylistTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}






