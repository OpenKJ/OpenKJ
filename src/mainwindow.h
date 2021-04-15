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
#include <QSortFilterProxyModel>
#include <QTemporaryDir>
#include <QDir>
#include <QSplashScreen>
#include <QLabel>
#include "dlgdatabase.h"
#include "dlgsettings.h"
#include "mediabackend.h"
#include "dlgcdg.h"
#include "settings.h"
#include "dlgregularsingers.h"
#include "dlgregularexport.h"
#include "dlgregularimport.h"
#include "dlgrequests.h"
#include "dlgkeychange.h"
#include "src/models/tablemodelkaraokesongs.h"
#include "src/models/tablemodelqueuesongs.h"
#include "src/models/tablemodelrotation.h"
#include "src/models/tablemodelbreaksongs.h"
#include "src/models/tablemodelplaylistsongs.h"
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
#include "src/models/tablemodelhistorysongs.h"
#include "src/models/tablemodelplaylistsongs.h"
#include "src/models/tablemodelqueuesongs.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
Q_OBJECT

private:
    Ui::MainWindow *ui;
    bool m_initialUiSetupDone{false};
    QSqlDatabase database;
    TableModelKaraokeSongs karaokeSongsModel;
    TableModelQueueSongs qModel{karaokeSongsModel, this};
    ItemDelegateQueueSongs qDelegate{this};
    TableModelRotation rotModel{this};
    ItemDelegateRotation rotDelegate{this};
    TableModelHistorySongs historySongsModel;
    DlgCdg *cdgWindow;
    DlgDebugOutput *debugDialog;
    DlgDatabase *dbDialog;
    DlgKeyChange *dlgKeyChange;
    DlgRequests *requestsDialog;
    DlgBookCreator *dlgBookCreator;
    DlgEq *dlgEq;
    DlgAddSinger *dlgAddSinger;
    DlgSongShop *dlgSongShop;
    MediaBackend kMediaBackend{this, "KAR", MediaBackend::Karaoke};
    MediaBackend sfxMediaBackend{this, "SFX", MediaBackend::SFX};
    MediaBackend bmMediaBackend{this, "BM", MediaBackend::BackgroundMusic};
    AudioRecorder audioRecorder;
    QLabel labelSingerCount;
    QLabel labelRotationDuration;
    bool sliderPositionPressed{false};
    bool sliderBmPositionPressed{false};
    bool m_shuttingDown{false};
    void play(const QString &karaokeFilePath, const bool &k2k = false);
    int m_rtClickQueueSongId{-1};
    int m_rtClickRotationSingerId{-1};
    int m_curSingerOriginalPosition{0};
    QTemporaryDir *khTmpDir;
    QString dbRtClickFile;
    QString curSinger;
    QString curArtist;
    QString curTitle;
    int kAANextSinger{-1};
    QString kAANextSongPath;
    bool kAASkip{false};
    SongShop *shop;
    bool k2kTransition{false};
    BmDbDialog *bmDbDialog;
    TableModelBreakSongs bmDbModel{this};
    TableModelPlaylistSongs playlistSongsModel{bmDbModel, this};
    ItemDelegatePlaylistSongs bmPlDelegate;
    QSqlTableModel *bmPlaylistsModel;
    int bmCurrentPlaylist;
    void bmAddPlaylist(const QString& title);
    bool bmPlaylistExists(const QString& name);
    MediaBackend::State m_lastAudioState{MediaBackend::StoppedState};
    bool m_kHasActiveVideo{false};
    bool m_bmHasActiveVideo{false};
    QTimer m_timerKaraokeAA;
    UpdateChecker *checker;
    QTimer m_timerSlowUiUpdate;
    QTimer m_timerButtonFlash;
    bool kNeedAutoSize{false};
    bool bNeedAutoSize{true};
    QShortcut *scutAddSinger{nullptr};
    QShortcut *scutKSelectNextSinger{nullptr};
    QShortcut *scutKPlayNextUnsung{nullptr};
    QShortcut *scutBFfwd{nullptr};
    QShortcut *scutBPause{nullptr};
    QShortcut *scutBRestartSong{nullptr};
    QShortcut *scutBRwnd{nullptr};
    QShortcut *scutBStop{nullptr};
    QShortcut *scutBVolDn{nullptr};
    QShortcut *scutBVolMute{nullptr};
    QShortcut *scutBVolUp{nullptr};
    QShortcut *scutJumpToSearch{nullptr};
    QShortcut *scutKFfwd{nullptr};
    QShortcut *scutKPause{nullptr};
    QShortcut *scutKRestartSong{nullptr};
    QShortcut *scutKRwnd{nullptr};
    QShortcut *scutKStop{nullptr};
    QShortcut *scutKVolDn{nullptr};
    QShortcut *scutKVolMute{nullptr};
    QShortcut *scutKVolUp{nullptr};
    QShortcut *scutLoadRegularSinger{nullptr};
    QShortcut *scutRequests{nullptr};
    QShortcut *scutToggleSingerWindow{nullptr};
    QShortcut *scutDeleteSinger{nullptr};
    QShortcut *scutDeleteSong{nullptr};
    QShortcut *scutDeletePlSong{nullptr};
    QWidget *historyTabWidget;
    void addSfxButton(const QString &filename, const QString &label, const bool &reset = false);
    void refreshSfxButtons();
    SfxEntry lastRtClickedSfxBtn;
    LazyDurationUpdateController *lazyDurationUpdater;
    QTimer m_timerTest;
    bool m_testMode{false};
    void updateIcons();
    void setupShortcuts();
    void resetBmLabels();

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

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
    void on_buttonClearRotation_clicked();
    void on_buttonClearQueue_clicked();
    void on_spinBoxKey_valueChanged(const int &arg1);
    void karaokeMediaBackend_positionChanged(const qint64 &position);
    void karaokeMediaBackend_durationChanged(const qint64 &duration);
    void karaokeMediaBackend_stateChanged(const MediaBackend::State &state);
    void sfxAudioBackend_positionChanged(const qint64 &position);
    void sfxAudioBackend_durationChanged(const qint64 &duration);
    void sfxAudioBackend_stateChanged(const MediaBackend::State &state);
    void hasActiveVideoChanged();
    void on_buttonRegulars_clicked();
    void rotationDataChanged();
    void silenceDetectedKar();
    void silenceDetectedBm();
    void on_tableViewDB_customContextMenuRequested(const QPoint &pos);
    void on_tableViewRotation_customContextMenuRequested(const QPoint &pos);
    void sfxButton_customContextMenuRequested(const QPoint &pos);
    void renameSinger();
    void on_tableViewQueue_customContextMenuRequested(const QPoint &pos);
    void on_sliderProgress_sliderPressed();
    void on_sliderProgress_sliderReleased();
    void setKeyChange();
    void toggleQueuePlayed();
    void previewCdg();
    void editSong();
    void markSongBad();
    void karaokeAATimerTimeout();
    void timerButtonFlashTimeout();
    void autosizeViews();
    void autosizeQueue();
    void autosizeBmViews();
    void bmDbUpdated();
    void bmDbCleared();
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
    void setMultiPlayed();
    void setMultiUnplayed();
    void on_spinBoxTempo_valueChanged(const int &arg1);
    void on_actionSongbook_Generator_triggered();
    void on_actionEqualizer_triggered();
    static void audioError(const QString &msg);
    void resizeRotation();
    static void songDropNoSingerSel();
    void newVersionAvailable(const QString &version);
    void on_pushButtonIncomingRequests_clicked();
    void on_pushButtonShop_clicked();
    void filesDroppedOnQueue(const QList<QUrl> &urls, const int &singerId, const int &position);
    void appFontChanged(const QFont &font);
    void on_tabWidget_currentChanged(const int &index);
    void bmDatabaseAboutToUpdate();
    void bmSongMoved(const int &oldPos, const int &newPos);
    void on_sliderBmPosition_sliderPressed();
    void on_sliderBmPosition_sliderReleased();
    void sfxButtonPressed();
    void on_btnAddSfx_clicked();
    void on_btnSfxStop_clicked();
    void removeSfxButton();
    static void showAlert(const QString &title, const QString &message);
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

    void on_actionBurn_in_EOS_Jump_triggered();

    void on_actionSong_Shop_triggered();

    void on_sliderVolume_valueChanged(int value);

    void on_sliderBmVolume_valueChanged(int value);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dbInit(const QDir &okjDataDir);


    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
};


#endif // MAINWINDOW_H
