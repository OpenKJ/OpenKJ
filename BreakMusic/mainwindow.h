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
#include "bmdbdialog.h"
//#include "audiobackendqtmultimedia.h"
//#include "bmabstractaudiobackend.h"
#include "abstractaudiobackend.h"
#include "audiobackendgstreamer.h"
#include "bmdbtablemodel.h"
#include "bmpltablemodel.h"
#include "bmplitemdelegate.h"
#include "bmdbitemdelegate.h"
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
    BmDbDialog *bmDbDialog;
    BmDbTableModel *bmDbModel;
    BmDbItemDelegate *bmDbDelegate;
    BmPlTableModel *bmPlModel;
    BmPlItemDelegate *bmPlDelegate;
    QSqlTableModel *bmPlaylistsModel;
    AudioBackendGstreamer *bmAudioBackend;
    //AudioBackendQtMultimedia *mPlayer;

    int bmCurrentPosition;
    int bmCurrentPlaylist;
    bool bmPlaylistExists(QString name);
    void bmAddPlaylist(QString title);

private slots:
    void ipcMessageReceived(int ipcCommand);

    void on_actionImport_Playlist_triggered();
    void on_actionExport_Playlist_triggered();
    void on_actionNew_Playlist_triggered();
    void on_actionRemove_Playlist_triggered();


    //Migrated
    void bmMediaStateChanged(AbstractAudioBackend::State newState);
    void bmMediaPositionChanged(qint64 position);
    void bmMediaDurationChanged(qint64 duration);
    void bmDbUpdated();
    void bmDbCleared();
    void bmShowMetadata(bool checked);
    void bmShowFilenames(bool checked);
    void on_tableViewBmPlaylist_clicked(const QModelIndex &index);
    void on_comboBoxBmPlaylists_currentIndexChanged(int index);
    void on_checkBoxBmBreak_toggled(bool checked);
    void on_tableViewBmDb_activated(const QModelIndex &index);
    void on_buttonBmStop_clicked();
    void on_lineEditBmSearch_returnPressed();
    void on_tableViewBmPlaylist_activated(const QModelIndex &index);
    void on_sliderBmVolume_valueChanged(int value);
    void on_sliderBmPosition_sliderMoved(int position);
    void on_buttonBmPause_clicked(bool checked);

    //Ruled out
    void on_actionAbout_triggered();


public slots:

    //Migrated
    void onActionManageDatabase();

};

#endif // MAINWINDOW_H
