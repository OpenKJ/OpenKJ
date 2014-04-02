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
#include <QtSql>
#include "queuetablemodel.h"
#include "songdbtablemodel.h"
#include "rotationtablemodel.h"
#include "khsinger.h"
#include "khregularsinger.h"
#include "libCDG/include/libCDG.h"
#include <QSortFilterProxyModel>
#include <QTemporaryDir>
#include <QDir>
#include <QSplashScreen>
#include "databasedialog.h"
#include "settingsdialog.h"
#include "khipcclient.h"
#include "khabstractaudiobackend.h"
#include <cdgwindow.h>
#include <khsettings.h>
#include <regularsingersdialog.h>


using namespace std;

namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void search();
    void songdbUpdated();
    void databaseCleared();
    void on_buttonStop_clicked();
    void on_buttonPlay_clicked();
    void on_buttonPause_clicked();
    void on_lineEdit_returnPressed();
    void on_treeViewDB_activated(const QModelIndex &index);
    void on_buttonAddSinger_clicked();
    void on_editAddSinger_returnPressed();
    void on_treeViewRotation_activated(const QModelIndex &index);
    void on_treeViewRotation_clicked(const QModelIndex &index);
    void on_treeViewQueue_activated(const QModelIndex &index);
    void on_actionManage_DB_triggered();
    void on_actionSettings_triggered();
    void on_actionRegulars_triggered();
    void songDroppedOnSinger(int singer, int song, int row);
    void on_pushButton_clicked();
    void on_treeViewQueue_clicked(const QModelIndex &index);
    void notify_user(QString message);
    void on_buttonClearRotation_clicked();
    void clearQueueSort();
    void on_buttonClearQueue_clicked();
    void on_spinBoxKey_valueChanged(int arg1);
    void on_sliderVolume_valueChanged(int value);
    void audioBackend_positionChanged(qint64 position);
    void audioBackend_durationChanged(qint64 duration);
    void audioBackend_stateChanged(QMediaPlayer::State state);
    void on_sliderProgress_sliderMoved(int position);
    void on_buttonRegulars_clicked();
    void rotationDataChanged();

private:
    Ui::MainWindow *ui;
    QSqlDatabase *database;
    SongDBTableModel *songdbmodel;
    RotationTableModel *rotationmodel;
    QueueTableModel *queuemodel;
    QSortFilterProxyModel *dbproxymodel;
    QSqlQueryModel *queryModelDB;
    QSqlTableModel *tableModelDB;
    QSqlQueryModel *queryModelRotation;
    CdgWindow *cdgWindow;
    DatabaseDialog *dbDialog;
    SettingsDialog *settingsDialog;
    RegularSingersDialog *regularSingersDialog;
    KhAbstractAudioBackend *audioBackend;
    KhRotationSingers *singers;
    KhRegularSingers *regularSingers;
    KhIPCClient *ipcClient;

    void play(QString zipFilePath);

    QTemporaryDir *khTmpDir;
    QDir *khDir;
    CDG *cdg;
    KhSong *songCurrent;
//    KhSettings *settings;

    int sortColDB;
    int sortDirDB;

};


#endif // MAINWINDOW_H
