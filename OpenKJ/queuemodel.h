/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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

class QueueModel : public QSqlRelationalTableModel
{
    Q_OBJECT

private:
    int m_singerId;

public:
    explicit QueueModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    enum {
	 dbQueue_Qsongid = 0,
	 dbQueue_Singer = 1,
	 dbQueue_Song = 2,
	 dbQueue_Artist = 3,
	 dbQueue_Title = 4,
	 dbQueue_Discid = 5,
	 dbQueue_Path = 6,
	 dbQueue_Keychg = 7,
	 dbQueue_Played = 8,
	 dbQueue_Position = 9
	};
    void setSinger(int singerId);
    int singer();
    int getSongPosition(int songId);
    bool getSongPlayed(int songId);
    int getSongKey(int songId);
    void songMove(int oldPosition, int newPosition);
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

public slots:
    void songAdd(int songId, int singerId);


    // QAbstractItemModel interface
public:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // QUEUEMODEL_H
