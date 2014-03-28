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
#include "songdbloadthread.h"
#include <QApplication>

#define UNUSED(x) (void)x

int sortorder = SONGSORT_ATS;
bool sortasc = true;

bool songsort(KhSong *song1, KhSong *song2)
{
    QString term1a;
    QString term1b;
    QString term2a;
    QString term2b;
    QString term3a;
    QString term3b;

    if (sortorder == SONGSORT_ATS)
    {
        term1a = song1->Artist.toLower();
        term1b = song2->Artist.toLower();
        term2a = song1->Title.toLower();
        term2b = song2->Title.toLower();
        term3a = song1->DiscID.toLower();
        term3b = song2->DiscID.toLower();
    } else if (sortorder == SONGSORT_TAS)
    {
        term1a = song1->Title.toLower();
        term1b = song2->Title.toLower();
        term2a = song1->Artist.toLower();
        term2b = song2->Artist.toLower();
        term3a = song1->DiscID.toLower();
        term3b = song2->DiscID.toLower();
    } else if (sortorder == SONGSORT_SAT)
    {
        term1a = song1->DiscID.toLower();
        term1b = song2->DiscID.toLower();
        term2a = song1->Artist.toLower();
        term2b = song2->Artist.toLower();
        term3a = song1->Title.toLower();
        term3b = song2->Title.toLower();
    }

        if (term1a > term1b)
            return !sortasc;
        else if (term1a < term1b)
            return sortasc;
        else if (term1a == term1b)
        {
            if (term2a > term2b)
                return !sortasc;
            else if (term2a < term2b)
                return sortasc;
            else if (term2a == term2b)
            {
                if (term3a > term3b)
                    return !sortasc;
                else if (term3a < term3b)
                    return sortasc;
                else return song1->ID < song2->ID;
            }
        }
    return false;
}

void DBSortThread::run()
{
    std::sort(mydata->begin(),mydata->end(),songsort);
}

SongDBTableModel::SongDBTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    fulldata = new KhSongs;
    filteredData = new KhSongs;
    lastSortCol = 0;
    lastSortOrder = Qt::AscendingOrder;
}

SongDBTableModel::~SongDBTableModel()
{
    qDeleteAll(fulldata->begin(),fulldata->end());
    delete fulldata;
    delete filteredData;
}

void SongDBTableModel::applyFilter(QString filterstr)
{
    filteredData->clear();
    filter = filterstr;
    QStringList terms;
    terms = filterstr.split(" ",QString::SkipEmptyParts);
    for (int i=0; i < fulldata->size(); i++)
    {
        bool match = true;
        for (int j=0; j < terms.size(); j++)
        {
            if (!fulldata->at(i)->filename.contains(terms.at(j), Qt::CaseInsensitive))
            {
                match = false;
                break;
            }
        }

        if (match)
            filteredData->push_back(fulldata->at(i));
    }
    sort(lastSortCol, lastSortOrder);
}

QVariant SongDBTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= filteredData->size() || index.row() < 0)
        return QVariant();

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case ARTIST:
            return filteredData->at(index.row())->Artist;
        case TITLE:
            return filteredData->at(index.row())->Title;
        case DISCID:
            return filteredData->at(index.row())->DiscID;
        case DURATION:
            return filteredData->at(index.row())->Duration;
        }
    }
    return QVariant();
}

QVariant SongDBTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            return tr("DiscID");
        case 3:
            return tr("Duration");
        }
    }
    return QVariant();
}

bool SongDBTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole && !(index.row() >= fulldata->size() || index.row() < 0))
    {
        int row = index.row();

        switch(index.column())
        {
        case 0:
            fulldata->at(row)->Artist = value.toString();
            break;
        case 1:
            fulldata->at(row)->Title = value.toString();
            break;
        case 2:
            fulldata->at(row)->DiscID = value.toString();
            break;
        case 3:
            fulldata->at(row)->Duration = value.toString();
        default:
            return false;
        }
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags SongDBTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
}

void SongDBTableModel::addSong(KhSong *song)
{
    if(std::find(fulldata->begin(),fulldata->end(),song) != fulldata->end())
        return;
    beginInsertRows(QModelIndex(),fulldata->size(),fulldata->size());
    fulldata->push_back(song);
    endInsertRows();
}

void SongDBTableModel::removeSong(int row)
{
    UNUSED(row);
}

int SongDBTableModel::rowCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return filteredData->size();
}

int SongDBTableModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return 4;
}


void SongDBTableModel::sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
{
    lastSortCol = column;
    lastSortOrder = order;
    if (order == Qt::AscendingOrder) sortasc = true;
    else sortasc = false;
    if (column == 1) sortorder = SONGSORT_TAS;
    else if (column == 2) sortorder = SONGSORT_SAT;
    else sortorder = SONGSORT_ATS;
    qDebug() << "Table tried to call sort. Column:" << column << " Order:" << order;
    this->layoutAboutToBeChanged();
//    std::sort(mydata->begin(),mydata->end(),songsort);
    DBSortThread *sortThread = new DBSortThread();
    sortThread->mydata = filteredData;
    sortThread->start();
    while(!sortThread->isFinished()) QCoreApplication::processEvents();
    this->layoutChanged();
    qDebug() << "Sort completed";
    // std::sort(begin(),end(),songsort);
    delete sortThread;
}

QMimeData *SongDBTableModel::mimeData(const QModelIndexList &indexes) const
{
    Q_UNUSED(indexes);
    QMimeData *mimeData = new QMimeData();
    QByteArray output;
    QBuffer outputBuffer(&output);
    int songid = filteredData->at(indexes.at(0).row())->ID;
    outputBuffer.open(QIODevice::WriteOnly);
    outputBuffer.write(QString::number(songid).toLocal8Bit());
//    outputBuffer.write("1");
    mimeData->setData("integer/songid", output);
    return mimeData;
}

void SongDBTableModel::loadFromDB()
{
    qDebug() << "Loading songDB";
    layoutAboutToBeChanged();
    filteredData->clear();
    SongDBLoadThread *thread = new SongDBLoadThread(fulldata, this);
    thread->start();
    while (!thread->isFinished())
    {
        QApplication::processEvents();
        //usleep(100);
    }
    layoutChanged();
    qDebug() << "Songdb Loaded";
    delete thread;
}

KhSong *SongDBTableModel::getRowSong(int row)
{
    return filteredData->at(row);
}

KhSong *SongDBTableModel::getSongByID(int songid)
{
    for (int i=0; i < fulldata->size(); i++)
    {
        if (fulldata->at(i)->ID == songid)
            return fulldata->at(i);
    }
    return new KhSong();
}

KhSongs *SongDBTableModel::getDbSongs()
{
    return fulldata;
}
