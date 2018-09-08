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

#ifndef ROTATIONMODEL_H
#define ROTATIONMODEL_H

#include <QSqlTableModel>
#include <QMimeData>
#include <QStringList>

class RotationModel : public QSqlTableModel
{
    Q_OBJECT

private:
    int m_currentSingerId;

public:
    explicit RotationModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    enum {ADD_FAIR=0,ADD_BOTTOM,ADD_NEXT};
    enum {
	 dbRotation_Singerid =0,
	 dbRotation_Name=1,
	 dbRotation_Position=2,
	 dbRotation_Regular=3,
	 dbRotation_Regularid=4
	};

    int singerAdd(QString name);
    int singerCount;
    void singerMove(int oldPosition, int newPosition);
    void singerSetName(int singerId, QString newName);
    void singerDelete(int singerId);
    bool singerExists(QString name);
    bool singerIsRegular(int singerId);
    int singerRegSingerId(int singerId);
    void singerMakeRegular(int singerId);
    void singerDisableRegularTracking(int singerId);
    int regularAdd(QString name);
    void regularDelete(int regSingerId);
    bool regularExists(QString name);
    void regularUpdate(int singerId);
    void regularLoad(int regSingerId, int positionHint);
    void regularSetName(int regSingerId, QString newName);
    QString getSingerName(int singerId);
    QString getRegularName(int regSingerId);
    int getSingerId(QString name);
    QString getRegSingerId(QString name);
    int getSingerPosition(int singerId);
    int singerIdAtPosition(int position);
    QStringList singers();
    QStringList regulars();
    QString nextSongPath(int singerId);
    QString nextSongArtist(int singerId);
    QString nextSongTitle(int singerId);
    int nextSongKeyChg(int singerId);
    int nextSongId(int singerId);
    int nextSongQueueId(int singerId);
    void clearRotation();
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Qt::DropActions supportedDropActions() const;
    int currentSinger() const;
    void setCurrentSinger(int currentSingerId);
    bool rotationIsValid();
    int numSongs(int singerId);
    int numSongsSung(int singerId) const;
    int numSongsUnsung(int singerId) const;

signals:
    void songDroppedOnSinger(int singerId, int songId, int dropRow);
    void regularAddNameConflict(QString name);
    void regularLoadNameConflict(QString name);
    void regularAddError(QString errorText);
    void rotationModified();
    void regularsModified();

public slots:
    void queueModified(int singerId);


    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role) const;
};

#endif // ROTATIONMODEL_H
