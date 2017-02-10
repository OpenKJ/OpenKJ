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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bmipcserver.h"
#include <QtSql>
#include <QDir>
#include "databasedialog.h"
//#include "audiobackendqtmultimedia.h"
//#include "bmabstractaudiobackend.h"
#include "abstractaudiobackend.h"
#include "audiobackendgstreamer.h"
#include "dbtablemodel.h"
#include "pltablemodel.h"
#include "plitemdelegate.h"
#include "dbitemdelegate.h"
#include <QSqlTableModel>


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
    BmIPCServer *ipcServer;
    QSqlDatabase *database;
    QDir *khDir;
    DatabaseDialog *dbDialog;
    BmDbTableModel *bmDbModel;
    BmDbItemDelegate *bmDbDelegate;
    BmPlTableModel *bmPlModel;
    BmPlItemDelegate *bmPlDelegate;
    QSqlTableModel *bmPlaylistsModel;
    AudioBackendGstreamer *bmAudioBackend;
    //AudioBackendQtMultimedia *mPlayer;

    int bmCurrentPosition;
    int bmCurrentPlaylist;
    bool playlistExists(QString name);
    void bmAddPlaylist(QString title);

private slots:
    void ipcMessageReceived(int ipcCommand);
    void on_tableViewDB_activated(const QModelIndex &index);
    void on_buttonStop_clicked();
    void on_lineEditSearch_returnPressed();
    void on_tableViewPlaylist_activated(const QModelIndex &index);
    void on_sliderVolume_valueChanged(int value);
    void on_sliderPosition_sliderMoved(int position);
    void on_mediaPositionChanged(qint64 position);
    void on_mediaDurationChanged(qint64 duration);
    void on_buttonPause_clicked(bool checked);
    void showMetadata(bool checked);
    void showFilenames(bool checked);
    void on_actionImport_Playlist_triggered();
    void on_actionExport_Playlist_triggered();
    void on_actionNew_Playlist_triggered();
    void on_actionRemove_Playlist_triggered();
    void on_actionAbout_triggered();
    void mediaStateChanged(AbstractAudioBackend::State newState);
    void dbUpdated();
    void dbCleared();
    void on_tableViewPlaylist_clicked(const QModelIndex &index);
    void on_comboBoxPlaylists_currentIndexChanged(int index);
    void on_checkBoxBreak_toggled(bool checked);

public slots:
    void onActionManageDatabase();

};

#endif // MAINWINDOW_H
