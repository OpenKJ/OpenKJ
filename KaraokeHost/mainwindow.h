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
#include "libCDG/include/libCDG.h"
#include <QSortFilterProxyModel>
#include <QTemporaryDir>
#include <QDir>
#include <QSplashScreen>
#include <QLabel>
#include "dlgdatabase.h"
#include "dlgsettings.h"
#include "khipcclient.h"
#include "khabstractaudiobackend.h"
#include "dlgcdg.h"
#include "khsettings.h"
#include "dlgregularsingers.h"
#include "dlgregularexport.h"
#include "dlgregularimport.h"
#include "dlgrequests.h"
#include "dlgcdgpreview.h"
#include "dlgkeychange.h"
#include "khaudiorecorder.h"
#include "dbtablemodel.h"
#include "queuemodel.h"
#include "queueitemdelegate.h"
#include "rotationmodel.h"
#include "rotationitemdelegate.h"
#include "dbitemdelegate.h"


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
    void on_buttonPause_clicked();
    void on_lineEdit_returnPressed();
    void on_tableViewDB_activated(const QModelIndex &index);
    void on_buttonAddSinger_clicked();
    void on_editAddSinger_returnPressed();
    void on_tableViewRotation_activated(const QModelIndex &index);
    void on_tableViewRotation_clicked(const QModelIndex &index);
    void on_tableViewQueue_activated(const QModelIndex &index);
    void on_actionManage_DB_triggered();
    void on_actionExport_Regulars_triggered();
    void on_actionImport_Regulars_triggered();
    void on_actionSettings_triggered();
    void on_actionRegulars_triggered();
    void on_actionIncoming_Requests_triggered();
    void songDroppedOnSinger(int singerId, int songId, int dropRow);
    void on_pushButton_clicked();
    void on_tableViewQueue_clicked(const QModelIndex &index);
    void notify_user(QString message);
    void on_buttonClearRotation_clicked();
    void clearQueueSort();
    void on_buttonClearQueue_clicked();
    void on_spinBoxKey_valueChanged(int arg1);
    void on_sliderVolume_valueChanged(int value);
    void audioBackend_positionChanged(qint64 position);
    void audioBackend_durationChanged(qint64 duration);
    void audioBackend_stateChanged(KhAbstractAudioBackend::State state);
    void on_sliderProgress_sliderMoved(int position);
    void on_buttonRegulars_clicked();
    void rotationDataChanged();
    void silenceDetected();
    void audioBackendChanged(int index);
    void on_tableViewDB_customContextMenuRequested(const QPoint &pos);
    void on_tableViewRotation_customContextMenuRequested(const QPoint &pos);
    void renameSinger();
    void on_tableViewQueue_customContextMenuRequested(const QPoint &pos);
    void on_sliderProgress_sliderPressed();
    void on_sliderProgress_sliderReleased();
    void setKeyChange();
    void toggleQueuePlayed();
    void regularNameConflict(QString name);
    void regularAddError(QString errorText);

private:
    Ui::MainWindow *ui;
    QSqlDatabase *database;
    DbTableModel *dbModel;
    DbItemDelegate *dbDelegate;
    QueueModel *qModel;
    QueueItemDelegate *qDelegate;
    RotationModel *rotModel;
    RotationItemDelegate *rotDelegate;
    DlgCdg *cdgWindow;
    DlgDatabase *dbDialog;
    DlgSettings *settingsDialog;
    DlgRegularSingers *regularSingersDialog;
    DlgRegularExport *regularExportDialog;
    DlgRegularImport *regularImportDialog;
    DlgKeyChange *dlgKeyChange;
    DlgRequests *requestsDialog;
    DlgCdgPreview *cdgPreviewDialog;
    KhAbstractAudioBackend *activeAudioBackend;
    KhAudioBackends *audioBackends;
    KhAudioRecorder *audioRecorder;
    KhIPCClient *ipcClient;
    QLabel *labelSingerCount;
    bool sliderPositionPressed;
    void play(QString zipFilePath);
    int m_rtClickQueueSongId;
    int m_rtClickRotationSingerId;
    QTemporaryDir *khTmpDir;
    QDir *khDir;
    CDG *cdg;

    int sortColDB;
    int sortDirDB;

};


#endif // MAINWINDOW_H
