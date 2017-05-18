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
    bool requestServerEnabled();
    void setRequestServerEnabled(bool enable);
    QString requestServerUrl();
    void setRequestServerUrl(QString url);
    QString requestServerUsername();
    void setRequestServerUsername(QString username);
    QString requestServerPassword();
    void setRequestServerPassword(QString password);
    bool requestServerIgnoreCertErrors();
    void setRequestServerIgnoreCertErrors(bool ignore);
    bool audioUseFader();
    void setAudioUseFader(bool fader);
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
    bool audioDetectSilence();
    void setAudioDetectSilence(bool enabled);
    QString audioOutputDevice();
    void setAudioOutputDevice(QString device);
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

    bool bmShowFilenames();
    void bmSetShowFilenames(bool show);
    bool bmShowMetadata();
    void bmSetShowMetadata(bool show);
    int bmVolume();
    void bmSetVolume(int bmVolume);
    int bmPlaylistIndex();
    void bmSetPlaylistIndex(int index);

signals:
    void tickerFontChanged();
    void tickerHeightChanged(int height);
    void tickerSpeedChanged();
    void tickerTextColorChanged();
    void tickerBgColorChanged();
    void tickerOutputModeChanged();
    void tickerEnableChanged();
    void audioBackendChanged(int index);
    void recordingSetupChanged();
    void cdgBgImageChanged();
    void cdgShowCdgWindowChanged(bool show);
    void cdgWindowFullscreenChanged(bool fullscreen);
    void cdgHSizeAdjustmentChanged(int pixels);
    void cdgVSizeAdjustmentChanged(int pixels);
    void cdgHOffsetChanged(int pixels);
    void cdgVOffsetChanged(int pixels);
    void bgModeChanged(BgMode mode);
    void bgSlideShowDirChanged(QString dir);

public slots:
    void setShowCdgWindow(bool show);
    void setCdgWindowFullscreen(bool fullScreen);
    void setCdgHSizeAdjustment(int pixels);
    void setCdgVSizeAdjustment(int pixels);
    void setCdgHOffset(int pixels);
    void setCdgVOffset(int pixels);
};

#endif // KHSETTINGS_H
