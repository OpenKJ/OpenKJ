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

#ifndef AUDIOFADER_H
#define AUDIOFADER_H

#include <QObject>
#include <QTimer>
#include <gst/gst.h>
#include <spdlog/async_logger.h>

class AudioFader : public QObject
{
    Q_OBJECT
private:
    GstElement *volumeElement;
    QTimer *timer;
    double targetVol;
    double volume();
    QString objName;
    std::shared_ptr<spdlog::logger> logger;

public:
    explicit AudioFader(QObject *parent = 0);
    enum FaderState{FadedIn=0,FadingIn,FadedOut,FadingOut};
    QString stateToStr(FaderState state);
    void setVolumeElement(GstElement *volumeElement);
    void setObjName(QString name);
    bool isFading();
    void setVolume(double volume);
    void immediateIn();
    void immediateOut();
    FaderState state();

private:
    FaderState curState;

signals:
    void volumeChanged(double volume);
    void volumeChanged(int volume);
    void fadeStarted();
    void fadeComplete();
    void faderStateChanged(AudioFader::FaderState);

public slots:
    void fadeOut(bool block = false);
    void fadeIn(bool block = false);


private slots:
    void timerTimeout();
};


#endif // AUDIOFADER_H
