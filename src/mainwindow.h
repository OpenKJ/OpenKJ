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
#include <memory>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
Q_OBJECT

private:
    Settings m_settings;
    std::unique_ptr<Ui::MainWindow> ui;
    std::string m_loggingPrefix{"[MainWindow]"};
    std::shared_ptr<spdlog::logger> m_logger;
    bool m_initialUiSetupDone{false};
    bool m_sliderPositionPressed{false};
    bool m_sliderBmPositionPressed{false};
    bool m_shuttingDown{false};
    bool m_kAASkip{false};
    bool m_k2kTransition{false};
    bool m_kHasActiveVideo{false};
    bool m_bmHasActiveVideo{false};
    bool m_kNeedAutoSize{false};
    bool m_bNeedAutoSize{true};
    bool m_testMode{false};
    int m_rtClickQueueSongId{-1};
    int m_rtClickRotationSingerId{-1};
    int m_curSingerOriginalPosition{0};
    int m_kAANextSinger{-1};
    int m_bmCurrentPlaylist{0};
    QString m_dbRtClickFile;
    QString m_curSinger;
    QString m_curArtist;
    QString m_curTitle;
    QString m_kAANextSongPath;
    MediaBackend::State m_lastAudioState{MediaBackend::StoppedState};
    SfxEntry m_lastRtClickedSfxBtn;
    QSqlDatabase m_database;
    TableModelKaraokeSongs m_karaokeSongsModel;
    TableModelQueueSongs m_qModel{m_karaokeSongsModel, this};
    ItemDelegateQueueSongs m_qDelegate{this};
    TableModelRotation m_rotModel{this};
    ItemDelegateRotation m_rotDelegate{this};
    TableModelHistorySongs m_historySongsModel{m_karaokeSongsModel};
    TableModelBreakSongs m_tableModelBreakSongs{this};
    TableModelPlaylistSongs m_tableModelPlaylistSongs{m_tableModelBreakSongs, this};
    std::unique_ptr<QSqlTableModel> m_tableModelPlaylists;
    ItemDelegatePlaylistSongs m_itemDelegatePlSongs{this};
    std::unique_ptr<DlgCdg> cdgWindow;
    std::unique_ptr<DlgDatabase> dbDialog;
    std::unique_ptr<DlgKeyChange> dlgKeyChange;
    std::unique_ptr<DlgRequests> requestsDialog;
    std::unique_ptr<DlgSongShop> dlgSongShop;
    std::unique_ptr<BmDbDialog> bmDbDialog;
    DlgRegularSingers m_dlgRegularSingers{&m_rotModel, this};
    MediaBackend m_mediaBackendKar{this, "KAR", MediaBackend::Karaoke};
    MediaBackend m_mediaBackendSfx{this, "SFX", MediaBackend::SFX};
    MediaBackend m_mediaBackendBm{this, "BM", MediaBackend::BackgroundMusic};
    AudioRecorder audioRecorder;
    QLabel m_labelSingerCount;
    QLabel m_labelRotationDuration;
    QTimer m_timerKaraokeAA;
    QTimer m_timerSlowUiUpdate;
    QTimer m_timerTest;
    QTimer m_timerButtonFlash;
    QShortcut m_scutAddSinger{this};
    QShortcut m_scutKSelectNextSinger{this};
    QShortcut m_scutKPlayNextUnsung{this};
    QShortcut m_scutBFfwd{this};
    QShortcut m_scutBPause{this};
    QShortcut m_scutBRestartSong{this};
    QShortcut m_scutBRwnd{this};
    QShortcut m_scutBStop{this};
    QShortcut m_scutBVolDn{this};
    QShortcut m_scutBVolMute{this};
    QShortcut m_scutBVolUp{this};
    QShortcut m_scutJumpToSearch{this};
    QShortcut m_scutKFfwd{this};
    QShortcut m_scutKPause{this};
    QShortcut m_scutKRestartSong{this};
    QShortcut m_scutKRwnd{this};
    QShortcut m_scutKStop{this};
    QShortcut m_scutKVolDn{this};
    QShortcut m_scutKVolMute{this};
    QShortcut m_scutKVolUp{this};
    QShortcut m_scutLoadRegularSinger{this};
    QShortcut m_scutRequests{this};
    QShortcut m_scutToggleSingerWindow{this};
    QShortcut m_scutDeleteSinger{nullptr};
    QShortcut m_scutDeleteSong{nullptr};
    QShortcut m_scutDeletePlSong{nullptr};
    std::unique_ptr<LazyDurationUpdateController> m_lazyDurationUpdater;
    std::unique_ptr<QTemporaryDir> m_mediaTempDir;
    std::shared_ptr<SongShop> m_songShop;
    std::unique_ptr<UpdateChecker> m_updateChecker;
    OKJSongbookAPI m_songbookApi;
    QWidget *m_historyTabWidget;

    void updateIcons();
    void setupShortcuts();
    void setupConnections();
    void loadSettings();
    void resetBmLabels();
    void play(const QString &karaokeFilePath, const bool &k2k = false);
    void bmAddPlaylist(const QString& title);
    bool bmPlaylistExists(const QString& name);
    void addSfxButton(const QString &filename, const QString &label, bool reset = false);
    void refreshSfxButtons();

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
    void showAddSingerDialog();


protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dbInit(const QDir &okjDataDir);
    void tableViewQueueSelChanged();
    void tableViewPlaylistSelectionChanged();
    void tableViewRotationSelChanged();
};


#endif // MAINWINDOW_H
