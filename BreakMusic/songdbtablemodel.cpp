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

#include "songdbtablemodel.h"
#include <QDebug>

SongdbTableModel::SongdbTableModel(BmSongs *songsObject, QObject *parent) :
    QAbstractTableModel(parent)
{
    songs = songsObject;
    connect(songs, SIGNAL(dataAboutToChange()), this, SIGNAL(layoutAboutToBeChanged()));
    connect(songs, SIGNAL(dataChanged()), this, SIGNAL(layoutChanged()));
}

int SongdbTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return songs->size();
}

int SongdbTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

QVariant SongdbTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if((unsigned)index.row() >= songs->size() || index.row() < 0)
        return QVariant();

    if(role == Qt::DisplayRole)
    {
            switch(index.column())
            {
            case 0:
                return songs->at(index.row())->artist();
            case 1:
                return songs->at(index.row())->title();
            case 2:
                return songs->at(index.row())->filename();
            case 3:
                return songs->at(index.row())->durationStr();
            }
    }
    return QVariant();
}

QVariant SongdbTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return tr("Artist");
        case 1:
            return tr("Title");
        case 2:
            return tr("Filename");
        case 3:
            return tr("Duration");
        }
    }
    return QVariant();
}

Qt::ItemFlags SongdbTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QMimeData *SongdbTableModel::mimeData(const QModelIndexList &indexes) const
{
    Q_UNUSED(indexes);
    QMimeData *mimeData = new QMimeData();
    QByteArray output;
    QBuffer outputBuffer(&output);
    BmSong *dragSong = songs->at(indexes.at(0).row());
    qDebug() << "Dragged song: " << dragSong->index() << " - " << dragSong->artist() << " - " << dragSong->title();
    int songid = songs->at(indexes.at(0).row())->index();
    outputBuffer.open(QIODevice::WriteOnly);
    outputBuffer.write(QString::number(songid).toLocal8Bit());
    mimeData->setData("integer/songid", output);
    return mimeData;
}

void SongdbTableModel::showMetadata(bool value)
{
    layoutAboutToBeChanged();
    m_showMetadata = value;
    layoutChanged();
}

void SongdbTableModel::showFilenames(bool value)
{
    layoutAboutToBeChanged();
    m_showFilenames = value;
    layoutChanged();
}
