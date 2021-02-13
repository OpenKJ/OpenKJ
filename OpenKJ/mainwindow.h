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
//#include "libCDG/include/libCDG.h"
#include <QSortFilterProxyModel>
#include <QTemporaryDir>
#include <QDir>
#include <QSplashScreen>
#include <QLabel>
#include "dlgdatabase.h"
#include "dlgsettings.h"
//#include "khipcclient.h"
#include "mediabackend.h"
#include "dlgcdg.h"
#include "settings.h"
#include "dlgregularsingers.h"
#include "dlgregularexport.h"
#include "dlgregularimport.h"
#include "dlgrequests.h"
#include "dlgkeychange.h"
#include "tablemodelkaraokesongs.h"
#include "tablemodelqueuesongs.h"
#include "tablemodelrotationsingers.h"
#include "tablemodelbreaksongs.h"
#include "tablemodelplaylistsongs.h"
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
#include "durationlazyupdater.h"
#include "dlgdebugoutput.h"
#include "dlgvideopreview.h"
#include "tablemodelhistorysongs.h"
#include "tablemodelplaylistsongs.h"
#include "tablemodelqueuesongs.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
private:
    Ui::MainWindow *ui;
    QSqlDatabase database;
    TableModelKaraokeSongs karaokeSongsModel;
    TableModelQueueSongs qModel{karaokeSongsModel, this};
    ItemDelegateQueueSongs qDelegate{this};
    TableModelRotationSingers *rotModel;
    TableModelHistorySongs historySongsModel;
    ItemDelegateRotationSingers *rotDelegate;
    DlgCdg *cdgWindow;
    DlgDebugOutput *debugDialog;
    DlgDatabase *dbDialog;
    DlgKeyChange *dlgKeyChange;
    DlgRequests *requestsDialog;
    DlgBookCreator *dlgBookCreator;
    DlgEq *dlgEq;
    DlgAddSinger *dlgAddSinger;
    DlgSongShop *dlgSongShop;
    MediaBackend kMediaBackend { this, "KAR", MediaBackend::Karaoke };
    MediaBackend sfxMediaBackend { this, "SFX", MediaBackend::SFX };
    MediaBackend bmMediaBackend { this, "BM", MediaBackend::BackgroundMusic };
    AudioRecorder audioRecorder;
    QLabel labelSingerCount;
    QLabel labelRotationDuration;
    bool sliderPositionPressed{false};
    bool sliderBmPositionPressed{false};
    bool m_shuttingDown{false};
    void play(const QString &karaokeFilePath, const bool &k2k = false);
    int m_rtClickQueueSongId{-1};
    int m_rtClickRotationSingerId{-1};
    QTemporaryDir *khTmpDir;
    int sortColDB{1};
    int sortDirDB{0};
    QString dbRtClickFile;
    QString curSinger;
    QString curArtist;
    QString curTitle;
    int kAANextSinger{-1};
    QString kAANextSongPath;
    bool kAASkip{false};
    int cdgOffset;
    SongShop *shop;
    bool k2kTransition{false};
    bool previewEnabled;
    BmDbDialog *bmDbDialog;
    TableModelBreakSongs bmDbModel{this};
    //ItemDelegateBreakSongs *bmDbDelegate;
    //TableModelPlaylistSongs *playlistSongsModel;
    TableModelPlaylistSongs playlistSongsModel{bmDbModel,this};
    ItemDelegatePlaylistSongs bmPlDelegate;
    QSqlTableModel *bmPlaylistsModel;
    int bmCurrentPosition{0};
    int bmCurrentPlaylist;
    void bmAddPlaylist(QString title);
    bool bmPlaylistExists(QString name);
    MediaBackend::State m_lastAudioState{MediaBackend::StoppedState};
    void refreshSongDbCache();
    QTimer m_timerKaraokeAA;
    UpdateChecker *checker;
    QTimer m_timerSlowUiUpdate;
    QTimer m_timerButtonFlash;
    bool blinkRequestsBtn{false};
    bool kNeedAutoSize{false};
    bool bNeedAutoSize{true};
    QShortcut *scutAddSinger;
    QShortcut *scutBFfwd;
    QShortcut *scutBPause;
    QShortcut *scutBRestartSong;
    QShortcut *scutBRwnd;
    QShortcut *scutBStop;
    QShortcut *scutBVolDn;
    QShortcut *scutBVolMute;
    QShortcut *scutBVolUp;
    QShortcut *scutJumpToSearch;
    QShortcut *scutKFfwd;
    QShortcut *scutKPause;
    QShortcut *scutKRestartSong;
    QShortcut *scutKRwnd;
    QShortcut *scutKStop;
    QShortcut *scutKVolDn;
    QShortcut *scutKVolMute;
    QShortcut *scutKVolUp;
    QShortcut *scutLoadRegularSinger;
    QShortcut *scutRequests;
    QShortcut *scutToggleSingerWindow;
    QShortcut *scutDeleteSinger;
    QShortcut *scutDeleteSong;
    QShortcut *scutDeletePlSong;
    QWidget *historyTabWidget;
    void addSfxButton(const QString &filename, const QString &label, const bool &reset = false);
    void refreshSfxButtons();
    SfxEntry lastRtClickedSfxBtn;
    LazyDurationUpdateController *lazyDurationUpdater;
    QTimer m_timerTest;
    bool m_testMode{false};
    void updateIcons();
    void setupShortcuts();

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
    void songDroppedOnSinger(const int &singerId, const int &songId, const int &dropRow);
    void on_pushButton_clicked();
    void on_tableViewQueue_clicked(const QModelIndex &index);
    void notify_user(const QString &message);
    void on_buttonClearRotation_clicked();
    void clearQueueSort();
    void on_buttonClearQueue_clicked();
    void on_spinBoxKey_valueChanged(const int &arg1);
    void karaokeMediaBackend_positionChanged(const qint64 &position);
    void karaokeMediaBackend_durationChanged(const qint64 &duration);
    void karaokeMediaBackend_stateChanged(const MediaBackend::State &state);
    void sfxAudioBackend_positionChanged(const qint64 &position);
    void sfxAudioBackend_durationChanged(const qint64 &duration);
    void sfxAudioBackend_stateChanged(const MediaBackend::State &state);
    void on_sliderProgress_sliderMoved(const int &position);
    void on_buttonRegulars_clicked();
    void rotationDataChanged();
    void silenceDetectedKar();
    void silenceDetectedBm();
    void audioBackendChanged(const int &index);
    void on_tableViewDB_customContextMenuRequested(const QPoint &pos);
    void on_tableViewRotation_customContextMenuRequested(const QPoint &pos);
    void sfxButton_customContextMenuRequested(const QPoint &pos);
    void renameSinger();
    void on_tableViewQueue_customContextMenuRequested(const QPoint &pos);
    void on_sliderProgress_sliderPressed();
    void on_sliderProgress_sliderReleased();
    void setKeyChange();
    void toggleQueuePlayed();
    void regularNameConflict(const QString &name);
    void regularAddError(const QString &errorText);
    void previewCdg();
    void editSong();
    void markSongBad();
    void setShowBgImage(const bool &show);
    void onBgImageChange();
    void karaokeAATimerTimeout();
    void timerButtonFlashTimeout();
    void autosizeViews();
    void autosizeQueue();
    void autosizeBmViews();
    void bmDbUpdated();
    void bmDbCleared();
    void bmShowMetadata(const bool &checked);
    void bmShowFilenames(const bool &checked);
    void bmMediaStateChanged(const MediaBackend::State &newState);
    void bmMediaPositionChanged(const qint64 &position);
    void bmMediaDurationChanged(const qint64 &duration);
    void on_actionManage_Break_DB_triggered();
    void on_tableViewBmPlaylist_clicked(const QModelIndex &index);
    void on_comboBoxBmPlaylists_currentIndexChanged(const int &index);
    void on_checkBoxBmBreak_toggled(const bool &checked);
    void on_tableViewBmDb_doubleClicked(const QModelIndex &index);
    void on_buttonBmStop_clicked();
    void on_lineEditBmSearch_returnPressed();
    void on_tableViewBmPlaylist_doubleClicked(const QModelIndex &index);
    void on_buttonBmPause_clicked(const bool &checked);
    void on_actionDisplay_Metadata_toggled(const bool &arg1);
    void on_actionDisplay_Filenames_toggled(const bool &arg1);
    void on_actionShow_Debug_Log_toggled(const bool &arg1);
    void on_actionManage_Karaoke_DB_triggered();
    void on_actionPlaylistNew_triggered();
    void on_actionPlaylistImport_triggered();
    void on_actionPlaylistExport_triggered();
    void on_actionPlaylistDelete_triggered();
    void on_buttonBmSearch_clicked();
    void on_actionAbout_triggered();
    void on_pushButtonMplxLeft_toggled(const bool &checked);
    void on_pushButtonMplxBoth_toggled(const bool &checked);
    void on_pushButtonMplxRight_toggled(const bool &checked);
    void on_lineEdit_textChanged(const QString &arg1);
    void cdgOffsetChanged(const int &offset);
    void setMultiPlayed();
    void setMultiUnplayed();
    void on_spinBoxTempo_valueChanged(const int &arg1);
    void on_actionSongbook_Generator_triggered();
    void on_actionEqualizer_triggered();
    void audioError(const QString &msg);
    void resizeRotation();
    void previewEnabledChanged(const bool &enabled) { previewEnabled = enabled; }
    void on_sliderVolume_sliderMoved(const int &position);
    void on_sliderBmVolume_sliderMoved(const int &position);
    void songDropNoSingerSel();
    void newVersionAvailable(const QString &version);
    void on_pushButtonIncomingRequests_clicked();
    void on_pushButtonShop_clicked();
    void filesDroppedOnQueue(const QList<QUrl> &urls, const int &singerId, const int &position);
    void appFontChanged(const QFont &font);
    void on_tabWidget_currentChanged(const int &index);
    void databaseAboutToUpdate();
    void bmDatabaseAboutToUpdate();
    void bmSongMoved(const int &oldPos, const int &newPos);
    void on_sliderBmPosition_sliderPressed();
    void on_sliderBmPosition_sliderReleased();
    void sfxButtonPressed();
    void on_btnAddSfx_clicked();
    void on_btnSfxStop_clicked();
    void removeSfxButton();
    void showAlert(const QString &title, const QString &message);
    void tableViewRotationCurrentChanged(const QModelIndex &cur, const QModelIndex &prev);
    void updateRotationDuration();
    void cdgVisibilityChanged();
    void rotationSelectionChanged(const QItemSelection &sel, const QItemSelection &desel);
    void on_lineEditBmSearch_textChanged(const QString &arg1);
    void on_btnRotTop_clicked();
    void on_btnRotUp_clicked();
    void on_btnRotDown_clicked();
    void on_btnRotBottom_clicked();
    void on_btnQTop_clicked();
    void on_btnQUp_clicked();
    void on_btnQDown_clicked();
    void on_btnQBottom_clicked();
    void on_btnBmPlRandomize_clicked();
    void on_btnPlTop_clicked();
    void on_btnPlUp_clicked();
    void on_btnPlDown_clicked();
    void on_btnPlBottom_clicked();
    void on_actionSound_Clips_triggered(const bool &checked);
    void on_actionNow_Playing_triggered(const bool &checked);
    void on_actionVideoSmall_triggered();
    void on_actionVideoMedium_triggered();
    void on_actionVideoLarge_triggered();
    void on_actionVideo_Output_2_triggered(const bool &checked);

    void on_actionKaraoke_torture_triggered();

    void on_actionK_B_torture_triggered();

    void on_actionBurn_in_triggered();

    void on_actionMultiplex_Controls_triggered(bool checked);

    void on_actionCDG_Decode_Torture_triggered();
    void on_actionWrite_Gstreamer_pipeline_dot_files_triggered();

    void videoFrameReceived(QImage frame, QString backendName);

    void on_comboBoxSearchType_currentIndexChanged(int index);

    void on_actionDocumentation_triggered();

    void on_btnToggleCdgWindow_clicked(bool checked);
    void shortcutsUpdated();

    void on_tableViewBmPlaylist_customContextMenuRequested(const QPoint &pos);
    void treatAllSingersAsRegsChanged(bool enabled);

    void on_pushButtonHistoryPlay_clicked();

    void on_pushButtonHistoryToQueue_clicked();

    void on_tableViewHistory_doubleClicked(const QModelIndex &index);

    void on_tableViewHistory_customContextMenuRequested(const QPoint &pos);

    void on_actionBreak_music_torture_triggered();

    void on_tableViewBmDb_clicked(const QModelIndex &index);

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
};


#endif // MAINWINDOW_H
