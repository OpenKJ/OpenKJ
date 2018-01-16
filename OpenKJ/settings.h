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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QHeaderView>
#include <QObject>
#include <QSettings>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>
#include <QWidget>

class Settings : public QObject
{
    Q_OBJECT

private:
    QSettings *settings;

public:
    enum BgMode { BG_MODE_IMAGE = 0, BG_MODE_SLIDESHOW };
    explicit Settings(QObject *parent = 0);
    bool cdgWindowFullscreen();
    bool showCdgWindow();
    void setCdgWindowFullscreenMonitor(int monitor);
    int  cdgWindowFullScreenMonitor();
    bool controlBreakMusic();
    void setControlBreakMusic(bool control);
    bool fadeBreakMusic();
    void setFadeBreakMusic(bool fade);
    bool pauseBreakMusic();
    void setPauseBreakMusic(bool pause);
    bool fadeToStop();
    void setFadeToStop(bool fade);
    bool fadedPause();
    void setFadedPause(bool faded);
    bool silenceDetection();
    bool setSilenceDetection(bool detect);
    void saveWindowState(QWidget *window);
    void restoreWindowState(QWidget *window);
    void saveColumnWidths(QTreeView *treeView);
    void saveColumnWidths(QTableView *tableView);
    void restoreColumnWidths(QTreeView *treeView);
    void restoreColumnWidths(QTableView *tableView);
    void saveSplitterState(QSplitter *splitter);
    void restoreSplitterState(QSplitter *splitter);
    void setTickerFont(QFont font);
    QFont tickerFont();
    int tickerHeight();
    void setTickerHeight(int height);
    int tickerSpeed();
    void setTickerSpeed(int speed);
    QColor tickerTextColor();
    void setTickerTextColor(QColor color);
    QColor tickerBgColor();
    void setTickerBgColor(QColor color);
    bool tickerFullRotation();
    void setTickerFullRotation(bool full);
    int tickerShowNumSingers();
    void setTickerShowNumSingers(int limit);
    void setTickerEnabled(bool enable);
    bool tickerEnabled();
    QString tickerCustomString();
    void setTickerCustomString(QString value);
    bool tickerShowRotationInfo();
    bool requestServerEnabled();
    void setRequestServerEnabled(bool enable);
    QString requestServerUrl();
    void setRequestServerUrl(QString url);
    int requestServerVenue();
    void setRequestServerVenue(int venueId);
    QString requestServerApiKey();
    void setRequestServerApiKey(QString apiKey);
    bool requestServerIgnoreCertErrors();
    void setRequestServerIgnoreCertErrors(bool ignore);
    bool audioUseFader();
    bool audioUseFaderBm();
    void setAudioUseFader(bool fader);
    void setAudioUseFaderBm(bool fader);
    int audioVolume();
    void setAudioVolume(int volume);
    QString cdgDisplayBackgroundImage();
    void setCdgDisplayBackgroundImage(QString imageFile);
    BgMode bgMode();
    void setBgMode(BgMode mode);
    QString bgSlideShowDir();
    void setBgSlideShowDir(QString dir);
    bool audioDownmix();
    void setAudioDownmix(bool downmix);
    bool audioDownmixBm();
    void setAudioDownmixBm(bool downmix);
    bool audioDetectSilence();
    bool audioDetectSilenceBm();
    void setAudioDetectSilence(bool enabled);
    void setAudioDetectSilenceBm(bool enabled);
    QString audioOutputDevice();
    QString audioOutputDeviceBm();
    void setAudioOutputDevice(QString device);
    void setAudioOutputDeviceBm(QString device);
    int audioBackend();
    void setAudioBackend(int index);
    QString recordingContainer();
    void setRecordingContainer(QString container);
    QString recordingCodec();
    void setRecordingCodec(QString codec);
    QString recordingInput();
    void setRecordingInput(QString input);
    QString recordingOutputDir();
    void setRecordingOutputDir(QString path);
    bool recordingEnabled();
    void setRecordingEnabled(bool enabled);
    QString recordingRawExtension();
    void setRecordingRawExtension(QString extension);
    int cdgVOffset();
    int cdgHOffset();
    int cdgVSizeAdjustment();
    int cdgHSizeAdjustment();
    bool ignoreAposInSearch();

    bool bmShowFilenames();
    void bmSetShowFilenames(bool show);
    bool bmShowMetadata();
    void bmSetShowMetadata(bool show);
    int bmVolume();
    void bmSetVolume(int bmVolume);
    int bmPlaylistIndex();
    void bmSetPlaylistIndex(int index);
    int mplxMode();
    void setMplxMode(int mode);
    bool karaokeAutoAdvance();
    int karaokeAATimeout();
    void setKaraokeAATimeout(int secs);
    bool karaokeAAAlertEnabled();
    void setKaraokeAAAlertEnabled(bool enabled);
    QFont karaokeAAAlertFont();
    void setKaraokeAAAlertFont(QFont font);
    bool showQueueRemovalWarning();
    bool showSingerRemovalWarning();
    bool showSongInterruptionWarning();
    bool showSongPauseStopWarning();
    QColor alertTxtColor();
    QColor alertBgColor();
    bool bmAutoStart();
    void setBmAutoStart(bool enabled);
    int cdgDisplayOffset();
    QFont bookCreatorItemFont();
    QFont bookCreatorHeaderFont();
    int bookCreatorSortCol();
    double bookCreatorMarginRt();
    double bookCreatorMarginLft();
    double bookCreatorMarginTop();
    double bookCreatorMarginBtm();
    bool eqKBypass();
    bool eqBBypass();
    int eqKLevel1();
    int eqKLevel2();
    int eqKLevel3();
    int eqKLevel4();
    int eqKLevel5();
    int eqKLevel6();
    int eqKLevel7();
    int eqKLevel8();
    int eqKLevel9();
    int eqKLevel10();
    int eqBLevel1();
    int eqBLevel2();
    int eqBLevel3();
    int eqBLevel4();
    int eqBLevel5();
    int eqBLevel6();
    int eqBLevel7();
    int eqBLevel8();
    int eqBLevel9();
    int eqBLevel10();
    int requestServerInterval();
    bool bmKCrossFade();
    bool requestRemoveOnRotAdd();

signals:
    void tickerFontChanged();
    void tickerHeightChanged(int height);
    void tickerSpeedChanged();
    void tickerTextColorChanged();
    void tickerBgColorChanged();
    void tickerOutputModeChanged();
    void tickerEnableChanged();
    void tickerShowRotationInfoChanged(bool show);
    void audioBackendChanged(int index);
    void recordingSetupChanged();
    void cdgBgImageChanged();
    void cdgShowCdgWindowChanged(bool show);
    void cdgWindowFullscreenChanged(bool fullscreen);
    void cdgWindowFullscreenMonitorChanged(int monitor);
    void cdgHSizeAdjustmentChanged(int pixels);
    void cdgVSizeAdjustmentChanged(int pixels);
    void cdgHOffsetChanged(int pixels);
    void cdgVOffsetChanged(int pixels);
    void bgModeChanged(BgMode mode);
    void bgSlideShowDirChanged(QString dir);
    void requestServerVenueChanged(int venueId);
    void tickerCustomStringChanged();
    void mplxModeChanged(int mode);
    void karaokeAAAlertFontChanged(QFont font);
    void karaokeAutoAdvanceChanged(bool enabled);
    void showQueueRemovalWarningChanged(bool enabled);
    void showSingerRemovalWarningChanged(bool enabled);
    void showSongInterruptionWarningChanged(bool enabled);
    void alertTxtColorChanged(QColor color);
    void alertBgColorChanged(QColor color);
    void cdgDisplayOffsetChanged(int offset);
    void showSongStopPauseWarningChanged(bool enabled);
    void eqKBypassChanged(bool bypass);
    void eqBBypassChanged(bool bypass);
    void eqKLevel1Changed(int level);
    void eqKLevel2Changed(int level);
    void eqKLevel3Changed(int level);
    void eqKLevel4Changed(int level);
    void eqKLevel5Changed(int level);
    void eqKLevel6Changed(int level);
    void eqKLevel7Changed(int level);
    void eqKLevel8Changed(int level);
    void eqKLevel9Changed(int level);
    void eqKLevel10Changed(int level);
    void eqBLevel1Changed(int level);
    void eqBLevel2Changed(int level);
    void eqBLevel3Changed(int level);
    void eqBLevel4Changed(int level);
    void eqBLevel5Changed(int level);
    void eqBLevel6Changed(int level);
    void eqBLevel7Changed(int level);
    void eqBLevel8Changed(int level);
    void eqBLevel9Changed(int level);
    void eqBLevel10Changed(int level);
    void requestServerIntervalChanged(int interval);

public slots:
    void setBmKCrossfade(bool enabled);
    void setShowCdgWindow(bool show);
    void setCdgWindowFullscreen(bool fullScreen);
    void setCdgHSizeAdjustment(int pixels);
    void setCdgVSizeAdjustment(int pixels);
    void setCdgHOffset(int pixels);
    void setCdgVOffset(int pixels);
    void setShowQueueRemovalWarning(bool show);
    void setShowSingerRemovalWarning(bool show);
    void setKaraokeAutoAdvance(bool enabled);
    void setShowSongInterruptionWarning(bool enabled);
    void setAlertBgColor(QColor color);
    void setAlertTxtColor(QColor color);
    void setIgnoreAposInSearch(bool ignore);
    void setCdgDisplayOffset(int offset);
    void setShowSongPauseStopWarning(bool enabled);
    void setBookCreatorHeaderFont(QFont font);
    void setBookCreatorItemFont(QFont font);
    void setBookCreatorSortCol(int col);
    void setBookCreatorMarginRt(double margin);
    void setBookCreatorMarginLft(double margin);
    void setBookCreatorMarginTop(double margin);
    void setBookCreatorMarginBtm(double margin);
    void setEqKBypass(bool bypass);
    void setEqBBypass(bool bypass);
    void setEqKLevel1(int level);
    void setEqKLevel2(int level);
    void setEqKLevel3(int level);
    void setEqKLevel4(int level);
    void setEqKLevel5(int level);
    void setEqKLevel6(int level);
    void setEqKLevel7(int level);
    void setEqKLevel8(int level);
    void setEqKLevel9(int level);
    void setEqKLevel10(int level);
    void setEqBLevel1(int level);
    void setEqBLevel2(int level);
    void setEqBLevel3(int level);
    void setEqBLevel4(int level);
    void setEqBLevel5(int level);
    void setEqBLevel6(int level);
    void setEqBLevel7(int level);
    void setEqBLevel8(int level);
    void setEqBLevel9(int level);
    void setEqBLevel10(int level);
    void setRequestServerInterval(int interval);
    void setTickerShowRotationInfo(bool show);
    void setRequestRemoveOnRotAdd(bool remove);


};

#endif // KHSETTINGS_H
