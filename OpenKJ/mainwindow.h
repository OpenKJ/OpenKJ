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
//#include "khipcclient.h"
#include "abstractaudiobackend.h"
#include "audiobackendgstreamer.h"
#include "dlgcdg.h"
#include "settings.h"
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
#include "bmdbtablemodel.h"
#include "bmdbitemdelegate.h"
#include "bmpltablemodel.h"
#include "bmplitemdelegate.h"
#include "bmdbdialog.h"
#include <QThread>
#include "audiorecorder.h"

using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
private:
    Ui::MainWindow *ui;
    QSqlDatabase database;
    DbTableModel *dbModel;
    DbItemDelegate *dbDelegate;
    QueueModel *qModel;
    QueueItemDelegate *qDelegate;
    RotationModel *rotModel;
    RotationItemDelegate *rotDelegate;
    DlgCdg *cdgWindow;
    //CdgWindow *cdgWindow2;
    DlgDatabase *dbDialog;
    DlgSettings *settingsDialog;
    DlgRegularSingers *regularSingersDialog;
    DlgRegularExport *regularExportDialog;
    DlgRegularImport *regularImportDialog;
    DlgKeyChange *dlgKeyChange;
    DlgRequests *requestsDialog;
    //DlgCdgPreview *cdgPreviewDialog;
    AbstractAudioBackend *kAudioBackend;
//    KhAudioBackends *audioBackends;
//    KhAudioRecorder *audioRecorder;
    AudioRecorder *audioRecorder;
    AudioBackendGstreamer *bmAudioBackend;
//    KhIPCClient *ipcClient;
    QLabel *labelSingerCount;
    bool sliderPositionPressed;
    void play(QString karaokeFilePath);
    int m_rtClickQueueSongId;
    int m_rtClickRotationSingerId;
    QTemporaryDir *khTmpDir;
    QDir *khDir;
    CDG *cdg;
    int sortColDB;
    int sortDirDB;
    QString dbRtClickFile;
    QString curSinger;
    QString curArtist;
    QString curTitle;
    int kAANextSinger;
    QString kAANextSongPath;
    bool kAASkip;
    int cdgOffset;


    BmDbDialog *bmDbDialog;
    BmDbTableModel *bmDbModel;
    BmDbItemDelegate *bmDbDelegate;
    BmPlTableModel *bmPlModel;
    BmPlItemDelegate *bmPlDelegate;
    QSqlTableModel *bmPlaylistsModel;
    int bmCurrentPosition;
    int bmCurrentPlaylist;
    void bmAddPlaylist(QString title);
    bool bmPlaylistExists(QString name);
    AbstractAudioBackend::State m_lastAudioState;
    void refreshSongDbCache();
    QTimer *karaokeAATimer;

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
    void audioBackend_stateChanged(AbstractAudioBackend::State state);
    void on_sliderProgress_sliderMoved(int position);
    void on_buttonRegulars_clicked();
    void rotationDataChanged();
    void silenceDetected();
    void silenceDetectedBm();
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
    void previewCdg();
    void editSong();
    void markSongBad();
    void setShowBgImage(bool show);
    void onBgImageChange();
    void karaokeAATimerTimeout();

    void bmDbUpdated();
    void bmDbCleared();
    void bmShowMetadata(bool checked);
    void bmShowFilenames(bool checked);
    void bmMediaStateChanged(AbstractAudioBackend::State newState);
    void bmMediaPositionChanged(qint64 position);
    void bmMediaDurationChanged(qint64 duration);
    void on_actionManage_Break_DB_triggered();
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
    void on_actionDisplay_Metadata_toggled(bool arg1);
    void on_actionDisplay_Filenames_toggled(bool arg1);
    void on_actionManage_Karaoke_DB_triggered();
    void on_actionPlaylistNew_triggered();
    void on_actionPlaylistImport_triggered();
    void on_actionPlaylistExport_triggered();
    void on_actionPlaylistDelete_triggered();
    void on_buttonBmSearch_clicked();
    void videoFrameReceived(QImage frame, QString backendName);
    void on_actionAbout_triggered();
    void on_pushButtonMplxLeft_toggled(bool checked);
    void on_pushButtonMplxBoth_toggled(bool checked);
    void on_pushButtonMplxRight_toggled(bool checked);
    void on_lineEdit_textChanged(const QString &arg1);
    void cdgOffsetChanged(int offset);
};


#endif // MAINWINDOW_H
