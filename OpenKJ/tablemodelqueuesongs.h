/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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

#ifndef QUEUEMODEL_H
#define QUEUEMODEL_H

#include <QSqlRelationalTableModel>
#include <QMimeData>
#include <QStringList>
#include <QUrl>
#include <QIcon>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QItemDelegate>

class ItemDelegateQueueSongs : public QItemDelegate
{
    Q_OBJECT
private:
    QIcon delete16;
    QIcon delete22;
public:
    explicit ItemDelegateQueueSongs(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

class TableModelQueueSongs : public QSqlRelationalTableModel
{
    Q_OBJECT

private:
    int m_singerId;

public:
    explicit TableModelQueueSongs(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void setSinger(int singerId);
    int singer();
    int getSongPosition(int songId);
    bool getSongPlayed(int songId);
    int getSongKey(int songId);
    void songMove(int oldPosition, int newPosition);
    void songMoveSongId(int songId, int newPosition);
    void songAdd(int songId);
    void songInsert(int songId, int position);
    void songDelete(int songId);
    void songSetKey(int songId, int semitones);
    void songSetPlayed(int qSongId, bool played = true);
    void clearQueue();
    QStringList mimeTypes() const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    Qt::DropActions supportedDropActions() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void sort(int column, Qt::SortOrder order);

protected:
    QString orderByClause() const;

signals:
    void queueModified(int singerId);
    void songDroppedWithoutSinger();
    void filesDroppedOnSinger(QList<QUrl> urls, int singerId, int position);
    void qSongsMoved(const int startRow, const int startCol, const int endRow, const int endCol);


public slots:
    void songAddSlot(int songId, int singerId, int keyChg = 0);


    // QAbstractItemModel interface
public:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // QUEUEMODEL_H
