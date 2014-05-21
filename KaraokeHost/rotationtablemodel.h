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
#include "khsinger.h"
#include "khregularsinger.h"

class RotationTableModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    //KhSingers *singers;
    QList<KhSinger *> *m_singers;
    KhRegularSingers *regularSingers;
    int currentSingerPosition;
    int currentSingerIndex;
    int selectedSingerIndex;
    int selectedSingerPosition;
    void sortSingers();

public:
    explicit RotationTableModel(KhRegularSingers *regularSingersObject, QObject *parent = 0);
    ~RotationTableModel();
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

    // Brought over from KhSingers
    void loadFromDB();
    bool moveSinger(int oldPosition, int newPosition);
    KhSinger *getSingerByPosition(int position) const;
    KhSinger *getSingerByIndex(int singerid);
    KhSinger *getSingerByName(QString name);
    int getCurrentSingerPosition() const;
    void setCurrentSingerPosition(int value);
    bool exists(QString name);
    QString getNextSongBySingerPosition(int position) const;
    void deleteSingerByIndex(int singerid);
    void deleteSingerByPosition(int position);
    void clear();
    KhSinger *getCurrent();
    KhSinger *getSelected();

    int getCurrentSingerIndex() const;
    void setCurrentSingerIndex(int value);
    int getSelectedSingerPosition() const;
    void setSelectedSingerPosition(int value);
    int getSelectedSingerIndex() const;
    void setSelectedSingerIndex(int value);
    void createRegularForSinger(int singerID);
    QStringList getSingerList();
    KhSinger *at(int index);
    int size();

    bool add(QString name, int position = -1, bool regular = false);


signals:
    void songDroppedOnSinger(int singerid, int songid, int rowid);
    void notify_user(QString);

public slots:
    void regularSingerDeleted(int RegularID);

};

#endif // KSPROTATIONTABLEMODEL_H
