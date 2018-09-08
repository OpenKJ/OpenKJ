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

#ifndef BMPLTABLEMODEL_H
#define BMPLTABLEMODEL_H

#include <QSqlRelationalTableModel>
#include <QMimeData>
#include <QStringList>

class BmPlTableModel : public QSqlRelationalTableModel
{
    Q_OBJECT
private:
    int m_playlistId;

public:
    explicit BmPlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    enum {
	dbBmPl_Plsongid = 0,
	dbBmPl_Playlist = 1,
	dbBmPl_Position = 2,
	dbBmPl_Artist = 3,
	dbBmPl_Title = 4,
	dbBmPl_Filename = 5,
	dbBmPl_Duration = 6,
	dbBmPl_Path = 7
    };
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

signals:
    void bmSongMoved(int oldPos, int newPos);

public slots:


    // QAbstractItemModel interface
public:
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    // QAbstractItemModel interface
public:
    QStringList mimeTypes() const;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    Qt::DropActions supportedDropActions() const;

    // QAbstractItemModel interface
public:
    QMimeData *mimeData(const QModelIndexList &indexes) const;


    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // PLAYLISTMODEL_H
