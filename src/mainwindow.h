/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
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
#include "dlgvideopreview.h"
#include "src/models/tablemodelhistorysongs.h"
#include "src/models/tablemodelplaylistsongs.h"
#include "src/models/tablemodelqueuesongs.h"
#include <spdlog/async_logger.h>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
Q_OBJECT

private:
    std::string m_loggingPrefix{"[MainWindow]"};
    Ui::MainWindow *ui;
    std::shared_ptr<spdlog::logger> logger;
    bool m_initialUiSetupDone{false};
    QSqlDatabase database;
    TableModelKaraokeSongs karaokeSongsModel;
    TableModelQueueSongs qModel{karaokeSongsModel, this};
    ItemDelegateQueueSongs qDelegate{this};
    TableModelRotation rotModel{this};
    ItemDelegateRotation rotDelegate{this};
    TableModelHistorySongs historySongsModel{karaokeSongsModel};
    DlgCdg *cdgWindow;
    DlgDatabase *dbDialog;
    DlgKeyChange *dlgKeyChange;
    DlgRequests *requestsDialog;
    DlgBookCreator *dlgBookCreator;
    DlgRegularSingers m_dlgRegularSingers{&rotModel, this};
    DlgEq *dlgEq;
    DlgAddSinger *dlgAddSinger;
    DlgSongShop *dlgSongShop;
    MediaBackend kMediaBackend{this, "KAR", MediaBackend::Karaoke};
    MediaBackend sfxMediaBackend{this, "SFX", MediaBackend::SFX};
    MediaBackend bmMediaBackend{this, "BM", MediaBackend::BackgroundMusic};
    AudioRecorder audioRecorder;
    QLabel labelSingerCount;
    QLabel labelRotationDuration;
    bool m_sliderPositionPressed{false};
    bool m_sliderBmPositionPressed{false};
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
    QShortcut scutAddSinger{this};
    QShortcut scutKSelectNextSinger{this};
    QShortcut scutKPlayNextUnsung{this};
    QShortcut scutBFfwd{this};
    QShortcut scutBPause{this};
    QShortcut scutBRestartSong{this};
    QShortcut scutBRwnd{this};
    QShortcut scutBStop{this};
    QShortcut scutBVolDn{this};
    QShortcut scutBVolMute{this};
    QShortcut scutBVolUp{this};
    QShortcut scutJumpToSearch{this};
    QShortcut scutKFfwd{this};
    QShortcut scutKPause{this};
    QShortcut scutKRestartSong{this};
    QShortcut scutKRwnd{this};
    QShortcut scutKStop{this};
    QShortcut scutKVolDn{this};
    QShortcut scutKVolMute{this};
    QShortcut scutKVolUp{this};
    QShortcut scutLoadRegularSinger{this};
    QShortcut scutRequests{this};
    QShortcut scutToggleSingerWindow{this};
    QShortcut scutDeleteSinger{nullptr};
    QShortcut scutDeleteSong{nullptr};
    QShortcut scutDeletePlSong{nullptr};
    QWidget *historyTabWidget;
    void addSfxButton(const QString &filename, const QString &label, bool reset = false);
    void refreshSfxButtons();
    SfxEntry lastRtClickedSfxBtn;
    LazyDurationUpdateController *lazyDurationUpdater;
    QTimer m_timerTest;
    bool m_testMode{false};
    void updateIcons();
    void setupShortcuts();
    void setupConnections();
    void resetBmLabels();

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void search();
    void databaseUpdated();
    void databaseCleared();
    void buttonStopClicked();
    void buttonPauseClicked();
    void tableViewDbDoubleClicked(const QModelIndex &index);
    void tableViewRotationDoubleClicked(const QModelIndex &index);
    void tableViewRotationClicked(const QModelIndex &index);
    void tableViewQueueDoubleClicked(const QModelIndex &index);
    void actionExportRegularsTriggered();
    void actionImportRegularsTriggered();
    void actionSettingsTriggered();
    void songDroppedOnSinger(const int &singerId, const int &songId, const int &dropRow);
    void tableViewQueueClicked(const QModelIndex &index);
    void clearRotation();
    void clearSingerQueue();
    void spinBoxKeyValueChanged(const int &arg1);
    void karaokeMediaBackend_positionChanged(const qint64 &position);
    void karaokeMediaBackend_durationChanged(const qint64 &duration);
    void karaokeMediaBackend_stateChanged(const MediaBackend::State &state);
    void sfxAudioBackend_positionChanged(const qint64 &position);
    void sfxAudioBackend_durationChanged(const qint64 &duration);
    void sfxAudioBackend_stateChanged(const MediaBackend::State &state);
    void hasActiveVideoChanged();
    void rotationDataChanged();
    void silenceDetectedKar();
    void silenceDetectedBm();
    void tableViewDBContextMenuRequested(const QPoint &pos);
    void tableViewRotationContextMenuRequested(const QPoint &pos);
    void tableViewQueueContextMenuRequested(const QPoint &pos);
    void sfxButtonContextMenuRequested(const QPoint &pos);
    void renameSinger();
    void sliderProgressPressed();
    void sliderProgressReleased();
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
    void tableViewBmPlaylistClicked(const QModelIndex &index);
    void tableViewBmPlaylistDoubleClicked(const QModelIndex &index);
    void tableViewBmDbDoubleClicked(const QModelIndex &index);
    void tableViewBmDbClicked(const QModelIndex &index);
    void comboBoxBmPlaylistsIndexChanged(const int &index);
    void checkBoxBmBreakToggled(const bool &checked);
    void buttonBmStopClicked();
    void searchBreakMusic();
    void buttonBmPauseClicked(const bool &checked);
    void actionDisplayMetadataToggled(const bool &arg1);
    void actionDisplayFilenamesToggled(const bool &arg1);
    void actionPlaylistNewTriggered();
    void actionPlaylistImportTriggered();
    void actionPlaylistExportTriggered();
    void actionPlaylistDeleteTriggered();
    void actionAboutTriggered();
    void pushButtonMplxLeftToggled(const bool &checked);
    void pushButtonMplxBothToggled(const bool &checked);
    void pushButtonMplxRightToggled(const bool &checked);
    void lineEditSearchTextChanged(const QString &arg1);
    void setMultiPlayed();
    void setMultiUnplayed();
    void spinBoxTempoValueChanged(const int &arg1);
    static void audioError(const QString &msg);
    void resizeRotation();
    static void songDropNoSingerSel();
    void newVersionAvailable(const QString &version);
    void filesDroppedOnQueue(const QList<QUrl> &urls, const int &singerId, const int &position);
    void appFontChanged(const QFont &font);
    void tabWidgetCurrentChanged(const int &index);
    void bmDatabaseAboutToUpdate();
    void bmSongMoved(const int &oldPos, const int &newPos);
    void sliderBmPositionPressed();
    void sliderBmPositionReleased();
    void sfxButtonPressed();
    void addSfxButtonPressed();
    void stopSfxPlayback();
    void removeSfxButton();
    static void showAlert(const QString &title, const QString &message);
    void tableViewRotationCurrentChanged(const QModelIndex &cur, const QModelIndex &prev);
    void updateRotationDuration();
    void cdgVisibilityChanged();
    void rotationSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void lineEditBmSearchChanged(const QString &arg1);
    void btnRotTopClicked();
    void btnRotUpClicked();
    void btnRotDownClicked();
    void btnRotBottomClicked();
    void btnQTopClicked();
    void btnQUpClicked();
    void btnQDownClicked();
    void btnQBottomClicked();
    void btnPlTopClicked();
    void btnPlUpClicked();
    void btnPlDownClicked();
    void btnPlBottomClicked();
    void btnBmPlRandomizeClicked();
    void actionSoundClipsTriggered(const bool &checked);
    void actionNowPlayingTriggered(const bool &checked);
    void actionVideoSmallTriggered();
    void actionVideoMediumTriggered();
    void actionVideoLargeTriggered();
    void actionKaraokeTorture();
    void actionKAndBTorture();
    void actionBurnIn();
    void actionCdgDecodeTorture();
    void writeGstPipelineDiagramToDisk();
    void comboBoxSearchTypeIndexChanged(int index);
    static void actionDocumentation();
    void shortcutsUpdated();
    void tableViewBmPlaylistContextMenu(const QPoint &pos);
    void treatAllSingersAsRegsChanged(bool enabled);
    void buttonHistoryPlayClicked();
    void buttonHistoryToQueueClicked();
    void tableViewHistoryDoubleClicked(const QModelIndex &index);
    void tableViewHistoryContextMenu(const QPoint &pos);
    void actionBreakMusicTorture();
    void actionBurnInEosJump();
    void sliderVolumeChanged(int value);
    void sliderBmVolumeChanged(int value);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dbInit(const QDir &okjDataDir);

};


#endif // MAINWINDOW_H
