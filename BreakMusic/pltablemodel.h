/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QSqlRelationalTableModel>
#include <QMimeData>
#include <QStringList>

class PlTableModel : public QSqlRelationalTableModel
{
    Q_OBJECT
private:
    int m_playlistId;

public:
    explicit PlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void moveSong(int oldPosition, int newPosition);
    void addSong(int songId);
    void insertSong(int songId, int position);
    void deleteSong(int position);
    void setCurrentPlaylist(int playlistId);
    int currentPlaylist();
    int getSongIdByFilePath(QString filePath);
    QString currentSongString();
    QString nextSongString();

signals:

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
