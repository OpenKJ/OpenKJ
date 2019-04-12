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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "customlineedit.h"
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
#include <QShortcut>
#include <QThread>
#include "audiorecorder.h"
#include "dlgbookcreator.h"
#include "dlgeq.h"
#include "updatechecker.h"
#include "dlgaddsinger.h"
#include "dlgsongshop.h"
#include "songshop.h"
#include "khdb.h"
#include "durationlazyupdater.h"

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
    DlgBookCreator *dlgBookCreator;
    DlgEq *dlgEq;
    DlgAddSinger *dlgAddSinger;
    DlgSongShop *dlgSongShop;
    //DlgCdgPreview *cdgPreviewDialog;
    AbstractAudioBackend *kAudioBackend;
    AbstractAudioBackend *sfxAudioBackend;
//    KhAudioBackends *audioBackends;
//    KhAudioRecorder *audioRecorder;
    AudioRecorder *audioRecorder;
    AudioBackendGstreamer *bmAudioBackend;
//    KhIPCClient *ipcClient;
    QLabel *labelSingerCount;
    QLabel *labelRotationDuration;
    bool sliderPositionPressed;
    bool sliderBmPositionPressed;
    void play(QString karaokeFilePath, bool k2k = false);
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
    SongShop *shop;
    bool k2kTransition;

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
    QTimer *startupOneShot;
    UpdateChecker *checker;
    QTimer *slowUiUpdateTimer;
    QTimer *timerButtonFlash;
    bool blinkRequestsBtn;
    QString GetRandomString() const;
    QSharedMemory *_singular;
    bool kNeedAutoSize;
    bool bNeedAutoSize;
    KhDb *db;
    QShortcut *scutAddSinger;
    QShortcut *scutSearch;
    QShortcut *scutRegulars;
    QShortcut *scutRequests;
    void addSfxButton(QString filename, QString label, bool reset = false);
    void refreshSfxButtons();
    SfxEntry lastRtClickedSfxBtn;
    QString findMatchingAudioFile(QString cdgFilePath);
    LazyDurationUpdateController *lazyDurationUpdater;

public:
    explicit MainWindow(QWidget *parent = 0);
    bool isSingleInstance();

    ~MainWindow();
    
private slots:
    void search();
    void databaseUpdated();
    void databaseCleared();
    void on_buttonStop_clicked();
    void on_buttonPause_clicked();
    void on_lineEdit_returnPressed();
    void on_tableViewDB_doubleClicked(const QModelIndex &index);
    void on_buttonAddSinger_clicked();
    void on_tableViewRotation_doubleClicked(const QModelIndex &index);
    void on_tableViewRotation_clicked(const QModelIndex &index);
    void on_tableViewQueue_doubleClicked(const QModelIndex &index);
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
    void audioBackend_positionChanged(qint64 position);
    void audioBackend_durationChanged(qint64 duration);
    void audioBackend_stateChanged(AbstractAudioBackend::State state);
    void sfxAudioBackend_positionChanged(qint64 position);
    void sfxAudioBackend_durationChanged(qint64 duration);
    void sfxAudioBackend_stateChanged(AbstractAudioBackend::State state);
    void on_sliderProgress_sliderMoved(int position);
    void on_buttonRegulars_clicked();
    void rotationDataChanged();
    void silenceDetected();
    void silenceDetectedBm();
    void audioBackendChanged(int index);
    void on_tableViewDB_customContextMenuRequested(const QPoint &pos);
    void on_tableViewRotation_customContextMenuRequested(const QPoint &pos);
    void sfxButton_customContextMenuRequested(const QPoint &pos);
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
    void timerButtonFlashTimeout();
    void autosizeViews();
    void autosizeBmViews();
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
    void on_tableViewBmDb_doubleClicked(const QModelIndex &index);
    void on_buttonBmStop_clicked();
    void on_lineEditBmSearch_returnPressed();
    void on_tableViewBmPlaylist_doubleClicked(const QModelIndex &index);
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
    void setMultiPlayed();
    void setMultiUnplayed();
    void on_spinBoxTempo_valueChanged(int arg1);
    void on_actionSongbook_Generator_triggered();
    void on_actionEqualizer_triggered();
    void audioError(QString msg);
    void resizeRotation();

    // QWidget interface
    void on_sliderVolume_sliderMoved(int position);

    void on_sliderBmVolume_sliderMoved(int position);
    void songDropNoSingerSel();
    void newVersionAvailable(QString version);

    void on_pushButtonIncomingRequests_clicked();

    void on_pushButtonShop_clicked();
    void filesDroppedOnQueue(QList<QUrl> urls, int singerId, int position);
    void appFontChanged(QFont font);

    void on_tabWidget_currentChanged(int index);
    void databaseAboutToUpdate();
    void bmDatabaseAboutToUpdate();
    void scutSearchActivated();
    void bmSongMoved(int oldPos, int newPos);

    void on_sliderBmPosition_sliderPressed();

    void on_sliderBmPosition_sliderReleased();
    void sfxButtonPressed();

    void on_btnAddSfx_clicked();

    void on_btnSfxStop_clicked();
    void removeSfxButton();
    void showAlert(QString title, QString message);
    void tableViewRotationCurrentChanged(QModelIndex cur, QModelIndex prev);
    void updateRotationDuration();

    void on_btnToggleCdgWindow_clicked();
    void cdgVisibilityChanged();

protected:
    void closeEvent(QCloseEvent *event);

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event);
};


#endif // MAINWINDOW_H
