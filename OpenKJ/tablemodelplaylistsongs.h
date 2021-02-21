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

#ifndef BMPLTABLEMODEL_H
#define BMPLTABLEMODEL_H

#include <QSqlRelationalTableModel>
#include <QMimeData>
#include <QStringList>
#include <QItemSelection>
#include <QIcon>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QItemDelegate>

class ItemDelegatePlaylistSongs : public QItemDelegate
{
    Q_OBJECT
private:
    int m_currentSong;
    QIcon m_iconDelete;
    QIcon m_iconPlaying;
public:
    explicit ItemDelegatePlaylistSongs(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int currentSong() const;
    void setCurrentSong(int value);
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

signals:

public slots:


};

class TableModelPlaylistSongs : public QSqlRelationalTableModel
{
    Q_OBJECT
private:
    int m_playlistId;

public:
    explicit TableModelPlaylistSongs(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void moveSong(int oldPosition, int newPosition);
    void addSong(int songId);
    void insertSong(int songId, int position);
    void deleteSong(int position);
    void setCurrentPlaylist(int playlistId);
    int currentPlaylist();
    int getSongIdByFilePath(QString filePath);
    QString currentSongString();
    QString nextSongString();
    int numSongs();
    qint32 randomizePlaylist(qint32 currentpos);
    qint32 getPlSongIdAtPos(qint32 position);
    int getSongPositionById(const int plSongId);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QStringList mimeTypes() const;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    Qt::DropActions supportedDropActions() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void bmSongMoved(int oldPos, int newPos);
    void bmPlSongsMoved(const int startRow, const int startCol, const int endRow, const int endCol);

public slots:

};

#endif // PLAYLISTMODEL_H
