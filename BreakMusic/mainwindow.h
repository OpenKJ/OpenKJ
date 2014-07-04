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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
//#include <QMediaPlayer>
#include <QSharedMemory>
#include "bmipcserver.h"
#include <QtSql>
#include <QDir>
#include "databasedialog.h"
#include "bmsong.h"
#include "songdbtablemodel.h"
#include "playlisttablemodel.h"
#include "bmplaylist.h"
//#include "fader.h"
#include "bmaudiobackendgstreamer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    //QMediaPlayer *mPlayer;
    QSharedMemory *sharedMemory;
    BmIPCServer *ipcServer;
    QSqlDatabase *database;
    QDir *khDir;
    DatabaseDialog *dbDialog;
    BmSongs *songs;
    SongdbTableModel *songdbmodel;
    PlaylistTableModel *playlistmodel;
    BmPlaylists *playlists;
    //Fader *fader;
    bool fading;
    void playCurrent(bool skipfade = false);
    QString msToMMSS(qint64 ms);
    BmAudioBackendGStreamer *mPlayer;

private slots:
//    void ipcMessageReceived(QString ipcMessage);
    void ipcMessageReceived(int ipcCommand);

    void on_treeViewDB_activated(const QModelIndex &index);

    void on_buttonStop_clicked();

    void on_lineEditSearch_returnPressed();

    void on_buttonAddPlaylist_clicked();

    void on_treeViewPlaylist_activated(const QModelIndex &index);

    void on_sliderVolume_valueChanged(int value);

    //void on_mediaStatusChanged(BmAbstractAudioBackend::MediaStatus status);

    void on_sliderPosition_sliderMoved(int position);

    void on_mediaPositionChanged(qint64 position);
    void on_mediaDurationChanged(qint64 duration);

    void on_buttonPause_clicked(bool checked);

    void on_comboBoxPlaylists_currentIndexChanged(const QString &arg1);
    void on_playlistChanged();
    void onActionShowMetadata(bool checked);
    void onActionShowFilenames(bool checked);
    void on_actionImport_Playlist_triggered();
    void mediaStateChanged(BmAbstractAudioBackend::State newState);


    void on_treeViewPlaylist_clicked(const QModelIndex &index);

public slots:
    void onActionManageDatabase();
    void on_playlistsChanged();

};

#endif // MAINWINDOW_H
