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
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>
#include <algorithm>
#include "khdb.h"

extern KhDb *db;

RotationTableModel::RotationTableModel(KhRegularSingers *regularSingersObject, QObject *parent) :
    QAbstractTableModel(parent)
{
    //singers = singersObject;
    regularSingers = regularSingersObject;
    m_singers = new QList<KhSinger *>;
    loadFromDB();
    setCurrentSingerPosition(-1);
    currentSingerPosition = -1;
    currentSingerIndex = -1;
    selectedSingerIndex = -1;
    selectedSingerPosition = -1;

}

RotationTableModel::~RotationTableModel()
{
    qDeleteAll(m_singers->begin(),m_singers->end());
    delete m_singers;
}

QVariant RotationTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= m_singers->size() || index.row() < 0)
        return QVariant();

//    if((role == Qt::BackgroundRole) && (singers->getSingers()->at(index.row())->getSingerPosition() == singers->getCurrentSingerPosition()))
    if((role == Qt::BackgroundRole) && (m_singers->at(index.row())->index() == getCurrentSingerIndex()))
        return QBrush(Qt::yellow);

    if (role == Qt::DecorationRole)
    {
        switch(index.column())
        {
        case 0:
            if (m_singers->at(index.row())->index() == getCurrentSingerIndex())
                return QPixmap(":/icons/microphone");
            else
                return QVariant();
        case 3:
            return QPixmap(":/icons/Icons/edit.png");
        case 4:
            return QPixmap(":/icons/Icons/edit-delete.png");
        case 5:
            if (m_singers->at(index.row())->regular())
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
            return m_singers->at(index.row())->name();
        case NEXTSONG:
            return getNextSongBySingerPosition(m_singers->at(index.row())->position());
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
    if (index.isValid() && role == Qt::EditRole && !(index.row() >= m_singers->size() || index.row() < 0))
    {
        if (getSingerByPosition(index.row() + 1)->name() == value.toString())
            return false;
        if (exists(value.toString()))
        {
            emit notify_user("Error: Duplicate singer name.  Edit cancelled.");
            return false;
        }
        getSingerByPosition(index.row() + 1)->setName(value.toString());
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
    return m_singers->size();
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
        droprow = m_singers->size();
    if (data->hasFormat("text/plain"))
    {
        int dragrow;
        dragrow = data->text().toInt();
        moveSinger(dragrow + 1,droprow + 1);
        return true;
    }
    else if (data->hasFormat("integer/songid"))
    {
        if (parent.row() >= 0)
        {
            QByteArray bytedata = data->data("integer/songid");
            int songid =  QString(bytedata.data()).toInt();
            if (getSingerByPosition(droprow + 1) != NULL)
            {
                int singerid = getSingerByPosition(droprow + 1)->index();
                songDroppedOnSinger(singerid,songid, parent.row());
            }
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

void RotationTableModel::loadFromDB()
{
    qDeleteAll(m_singers->begin(),m_singers->end());
    m_singers->clear();
    QSqlQuery query;
    query.exec("SELECT ROWID,name,position,regular,regularid FROM rotationSingers");
    int rotationsingerid = query.record().indexOf("ROWID");
    int name = query.record().indexOf("name");
    int position  = query.record().indexOf("position");
    int regular = query.record().indexOf("regular");
    int regularindex = query.record().indexOf("regularid");
    while (query.next()) {
        bool isReg = query.value(regular).toBool();
        int regIdx = query.value(regularindex).toInt();
        KhSinger *singer = new KhSinger(regularSingers);
        singer->setIndex(query.value(rotationsingerid).toInt());
        if ((isReg) && (regularSingers->getByRegularID(regIdx) != NULL))
        {
            singer->setRegular(query.value(regular).toBool(),true);
            singer->setRegularIndex(query.value(regularindex).toInt(),true);
        }
        else
        {
            singer->setRegular(false,true);
            singer->setRegularIndex(-1, true);
        }
        singer->setName(query.value(name).toString(),true);
        singer->setPosition(query.value(position).toInt(),true);
        m_singers->push_back(singer);
    }
    sortSingers();
}

bool RotationTableModel::moveSinger(int oldPosition, int newPosition)
{
    KhSinger *movingSinger = getSingerByPosition(oldPosition);
    db->beginTransaction();
    if (newPosition > oldPosition)
    {
        if (currentSingerPosition == oldPosition)
            currentSingerPosition = newPosition - 1;
        else if ((currentSingerPosition <= newPosition) && (currentSingerPosition > oldPosition))
            currentSingerPosition--;
        for (int i=0; i < m_singers->size(); i++)
        {
            if ((m_singers->at(i)->position() > oldPosition) && (m_singers->at(i)->position() <= newPosition - 1) && (m_singers->at(i)->index() != movingSinger->index()))
                m_singers->at(i)->setPosition(m_singers->at(i)->position() - 1);
        }
        movingSinger->setPosition(newPosition - 1);
    }
    else if (newPosition < oldPosition)
    {
        if (currentSingerPosition == oldPosition)
            currentSingerPosition = newPosition;
        else if ((currentSingerPosition >= newPosition) && (currentSingerPosition < oldPosition))
            currentSingerPosition++;
        for (int i=0; i < m_singers->size(); i++)
        {
            if ((m_singers->at(i)->position() >= newPosition) && (m_singers->at(i)->position() < oldPosition) && (m_singers->at(i)->index() != movingSinger->index()))
                m_singers->at(i)->setPosition(m_singers->at(i)->position() + 1);
        }
        movingSinger->setPosition(newPosition);
    }
    db->endTransaction();
    sortSingers();
    return true;
}

KhSinger *RotationTableModel::getSingerByPosition(int position) const
{
    if (position < 0) return NULL;
    for (int i=0; i < m_singers->size(); i++)
    {
        if (position == m_singers->at(i)->position())
        {
            return m_singers->at(i);
        }
    }
    return NULL;
}

KhSinger *RotationTableModel::getSingerByIndex(int singerid)
{
    for (int i=0; i < m_singers->size(); i++)
    {
        if (m_singers->at(i)->index() == singerid)
            return m_singers->at(i);
    }
    return NULL;
}

KhSinger *RotationTableModel::getSingerByName(QString name)
{
    for (int i=0; i < m_singers->size(); i++)
    {
        if (m_singers->at(i)->name() == name)
            return m_singers->at(i);
    }
    return NULL;
}

int RotationTableModel::getCurrentSingerPosition() const
{
    return currentSingerPosition;
}

void RotationTableModel::setCurrentSingerPosition(int value)
{
    emit layoutAboutToBeChanged();
    currentSingerPosition = value;
    if (getSingerByPosition(value) != NULL)
        currentSingerIndex = getSingerByPosition(value)->index();
    else
        currentSingerIndex = -1;
    emit layoutChanged();
}

bool RotationTableModel::exists(QString name)
{
    bool match = false;
    for (int i=0; i < m_singers->size(); i++)
    {
        if (name.toLower() == m_singers->at(i)->name().toLower())
        {
            match = true;
            break;
        }
    }
    return match;
}

QString RotationTableModel::getNextSongBySingerPosition(int position) const
{
    return db->singerGetNextSong(getSingerByPosition(position)->index());
}

void RotationTableModel::deleteSingerByIndex(int singerid)
{
    int delSingerPos = getSingerByIndex(singerid)->position();
    if (singerid == currentSingerIndex) setCurrentSingerPosition(-1);
    db->beginTransaction();
    db->singerDelete(singerid);
    m_singers->erase(m_singers->begin() + (delSingerPos - 1));
    for (int i=0; i < m_singers->size(); i++)
    {
        if (m_singers->at(i)->position() >  delSingerPos)
            m_singers->at(i)->setPosition(m_singers->at(i)->position() - 1);
    }
    db->endTransaction();
    sortSingers();
}

void RotationTableModel::deleteSingerByPosition(int position)
{
    KhSinger *singer = getSingerByPosition(position);
    deleteSingerByIndex(singer->index());
}

void RotationTableModel::clear()
{
    emit layoutAboutToBeChanged();
    db->rotationClear();
    currentSingerPosition = -1;
    qDeleteAll(m_singers->begin(),m_singers->end());
    m_singers->clear();
    emit layoutChanged();
}

KhSinger *RotationTableModel::getCurrent()
{
    return getSingerByPosition(currentSingerPosition);
}

KhSinger *RotationTableModel::getSelected()
{
    return getSingerByIndex(selectedSingerIndex);
}

bool singerPositionSort(KhSinger *singer1, KhSinger *singer2)
{
    if (singer1->position() >= singer2->position())
        return false;
    else
        return true;
}

void RotationTableModel::sortSingers()
{
    emit layoutAboutToBeChanged();
    std::sort(m_singers->begin(), m_singers->end(), singerPositionSort);
    emit layoutChanged();
}

int RotationTableModel::getCurrentSingerIndex() const
{
    return currentSingerIndex;
}

void RotationTableModel::setCurrentSingerIndex(int value)
{
    emit layoutAboutToBeChanged();
    currentSingerIndex = value;
    if (getSingerByIndex(value) != NULL)
        currentSingerPosition = getSingerByIndex(value)->position();
    else
        currentSingerPosition = -1;
    emit layoutChanged();
}

int RotationTableModel::getSelectedSingerPosition() const
{
    return selectedSingerPosition;
}

int RotationTableModel::getSelectedSingerIndex() const
{
    return selectedSingerIndex;
}

void RotationTableModel::setSelectedSingerIndex(int value)
{
    selectedSingerIndex = value;
    if (getSingerByIndex(value) != NULL)
        selectedSingerPosition = getSingerByIndex(value)->position();
    else
        selectedSingerPosition = -1;
}

void RotationTableModel::createRegularForSinger(int singerID)
{
    KhSinger *singer = getSingerByIndex(singerID);
    db->beginTransaction();
    int regularid = regularSingers->add(singer->name());
    singer->setRegular(true);
    singer->setRegularIndex(regularid);
    KhRegularSinger *regular = regularSingers->getByRegularID(regularid);
    for (int i=0; i < singer->queueSongs()->size(); i++)
    {
        int regsongindex = regular->addSong(singer->queueSongs()->at(i)->getSongID(),singer->queueSongs()->at(i)->getKeyChange(),singer->queueSongs()->at(i)->getPosition());
        singer->queueSongs()->at(i)->setRegSong(true);
        singer->queueSongs()->at(i)->setRegSongIndex(regsongindex);
    }
    db->endTransaction();
}

QStringList RotationTableModel::getSingerList()
{
    QStringList singerList;
    for (int i=0; i < m_singers->size(); i++)
        singerList << m_singers->at(i)->name();
    singerList.sort();
    return singerList;
}

KhSinger *RotationTableModel::at(int index)
{
    return m_singers->at(index);
}

int RotationTableModel::size()
{
    return m_singers->size();
}

bool RotationTableModel::add(QString name, int position, bool regular)
{
    if (exists(name))
    {
        return false;
    }
    emit layoutAboutToBeChanged();
    int nextPos = m_singers->size() + 1;
    int singerId = db->singerAdd(name, nextPos, regular);
    if (singerId == -1) return false;
    KhSinger *singer = new KhSinger(regularSingers, this);
    singer->setName(name,true);
    singer->setPosition(nextPos,true);
    singer->setIndex(singerId);
    singer->setRegular(regular,true);
    singer->setRegularIndex(-1,true);
    m_singers->push_back(singer);
    emit layoutChanged();
    if (position == -1)
        return true;
    else
    {
        if (moveSinger(nextPos,position))
            return true;
        else
            return false;
    }
    return false;
}

void RotationTableModel::regularSingerDeleted(int RegularID)
{
    emit layoutAboutToBeChanged();
    for (int i=0; i < m_singers->size(); i++)
    {
        if (m_singers->at(i)->regularIndex() == RegularID)
        {
            m_singers->at(i)->setRegular(false);
            m_singers->at(i)->setRegularIndex(-1);
        }
    }
    emit layoutChanged();
}
