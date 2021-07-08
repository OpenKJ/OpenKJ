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

#include "audiofader.h"
#include <gst/audio/streamvolume.h>
#include <QApplication>
#include <spdlog/spdlog.h>


void AudioFader::setVolume(double volume)
{
    //qInfo() << objName << "setVolume(" << volume << ")";
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
    emit volumeChanged(volume);
}

void AudioFader::immediateIn()
{
    logger->debug("[{}] Immediate IN requested", objName.toStdString());
    timer->stop();
    //g_object_set(volumeElement, "mute", false, nullptr);
    if (volume() == 1.0 && curState == FadedIn)
        return;
    setVolume(1.0);
    curState = FadedIn;
    emit faderStateChanged(curState);
}

void AudioFader::immediateOut()
{
    logger->debug("[{}] Immediate OUT requested", objName.toStdString());

    timer->stop();
    setVolume(0);
    //g_object_set(volumeElement, "mute", true, nullptr);
    curState = FadedOut;
    emit faderStateChanged(curState);
}

AudioFader::FaderState AudioFader::state()
{
    return curState;
}

void AudioFader::setVolumeElement(GstElement *volumeElement)
{
    logger->trace("[{}] setVolumeElement called", objName.toStdString());
    this->volumeElement = volumeElement;
}

void AudioFader::setObjName(QString name)
{
    objName = name;
}

bool AudioFader::isFading() {
    if (curState == FadingIn || curState == FadingOut)
        return true;
    return false;
}

double AudioFader::volume()
{
    return gst_stream_volume_get_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC);
}

AudioFader::AudioFader(QObject *parent) : QObject(parent)
{
    logger = spdlog::get("logger");
    curState = FadedIn;
    emit faderStateChanged(curState);
    targetVol = 0;
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

QString AudioFader::stateToStr(AudioFader::FaderState state)
{
    switch (state)
    {
    case AudioFader::FadedIn:
        return "AudioFader::FadedIn";
    case AudioFader::FadingIn:
        return "AudioFader::FadingIn";
    case AudioFader::FadedOut:
        return "AudioFader::FadedOut";
    case AudioFader::FadingOut:
        return "AudioFader::FadingOut";
    }
    return "Unknown";
}

void AudioFader::fadeOut(bool block) {
    logger->debug("[{}] Fade OUT requested - blocking: {}", objName.toStdString(), block);
    emit fadeStarted();
    targetVol = 0;
    curState = FadingOut;
    emit faderStateChanged(curState);
    timer->start();
    if (!block)
        return;
    logger->debug("[{}] Waiting for fade to complete", objName.toStdString());
    while (volume() != targetVol && curState == FadingOut) {
        QApplication::processEvents();
    }
    logger->debug("[{}] Fade completed", objName.toStdString());
}

void AudioFader::fadeIn(bool block)
{
    logger->debug("[{}] Fade IN requested - blocking: {}", objName.toStdString(), block);
    emit fadeStarted();
    targetVol = 1.0;
    curState = FadingIn;
    emit faderStateChanged(curState);
    //g_object_set(volumeElement, "mute", false, nullptr);
    timer->start();
    if (!block)
        return;
        while (volume() != targetVol && curState == FadingIn)
        {
            QApplication::processEvents();
        }
    logger->debug("[{}] Fade completed", objName.toStdString());
}

void AudioFader::timerTimeout()
{
    //qInfo() << objName << " - Timer - Current: " << volume() << " Target: " << targetVol;
    double increment = .05;
    if (isFading())
    {
        if (volume() == targetVol)
        {
            logger->debug("[{}] Target volume reached", objName.toStdString());
            timer->stop();
            if (curState == FadingOut)
                curState = FadedOut;
            if (curState == FadingIn)
                curState = FadedIn;
            emit faderStateChanged(curState);
            emit fadeComplete();
            return;
        }
        if (volume() > targetVol)
        {
            if ((volume() - increment) < targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                curState = FadedOut;
                //g_object_set(volumeElement, "mute", true, nullptr);
                emit faderStateChanged(curState);
                emit fadeComplete();
                return;
            }
            setVolume(volume() - increment);
        }
        if (volume() < targetVol)
        {
            if ((volume() + increment) > targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                curState = FadedIn;
                emit faderStateChanged(curState);
                emit fadeComplete();
                return;
            }
            setVolume(volume() + increment);
        }
    }
}
