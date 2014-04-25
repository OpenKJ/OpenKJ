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

#ifndef KHSETTINGS_H
#define KHSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QWidget>


class KhSettings : public QObject
{
    Q_OBJECT
public:
    explicit KhSettings(QObject *parent = 0);
    bool cdgWindowFullscreen();
    void setCdgWindowFullscreen(bool fullScreen);
    bool showCdgWindow();
    void setShowCdgWindow(bool show);
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
    bool audioDownmix();
    void setAudioDownmix(bool downmix);
    bool audioDetectSilence();
    void setAudioDetectSilence(bool enabled);

signals:
    void tickerFontChanged();
    void tickerHeightChanged();
    void tickerSpeedChanged();
    void tickerTextColorChanged();
    void tickerBgColorChanged();
    void tickerOutputModeChanged();
    void tickerEnableChanged();

public slots:

private:
    QSettings *settings;
};

#endif // KHSETTINGS_H
