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


void AudioFader::setVolume(double volume) {
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
    emit volumeChanged(volume);
}

void AudioFader::immediateIn() {
    m_logger->debug("[{}] Immediate IN requested", m_objName.toStdString());
    m_timer.stop();
    if (volume() == 1.0 && m_curState == FadedIn)
        return;
    setVolume(1.0);
    m_curState = FadedIn;
    emit faderStateChanged(m_curState);
}

void AudioFader::immediateOut() {
    m_logger->debug("[{}] Immediate OUT requested", m_objName.toStdString());
    m_timer.stop();
    setVolume(0);
    m_curState = FadedOut;
    emit faderStateChanged(m_curState);
}

AudioFader::FaderState AudioFader::state() {
    return m_curState;
}

void AudioFader::setVolumeElement(GstElement *volumeElement) {
    m_logger->trace("[{}] setVolumeElement called", m_objName.toStdString());
    this->m_volumeElement = volumeElement;
}

void AudioFader::setObjName(const QString &name) {
    m_objName = name;
}

bool AudioFader::isFading() {
    if (m_curState == FadingIn || m_curState == FadingOut)
        return true;
    return false;
}

double AudioFader::volume() {
    return gst_stream_volume_get_volume(GST_STREAM_VOLUME(m_volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC);
}

AudioFader::AudioFader(QObject *parent) : QObject(parent) {
    m_logger = spdlog::get("logger");
    m_timer.setInterval(100);
    connect(&m_timer, &QTimer::timeout, this, &AudioFader::timerTimeout);
}

std::string AudioFader::stateToStr(AudioFader::FaderState state) {
    switch (state) {
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
    m_logger->debug("[{}] Fade OUT requested - blocking: {}", m_objName.toStdString(), block);
    emit fadeStarted();
    m_targetVol = 0;
    m_curState = FadingOut;
    emit faderStateChanged(FadingOut);
    m_timer.start();
    if (!block)
        return;
    m_logger->debug("[{}] Waiting for fade to complete", m_objName.toStdString());
    while (volume() != m_targetVol && m_curState == FadingOut) {
        QApplication::processEvents();
    }
    m_logger->debug("[{}] Fade completed", m_objName.toStdString());
}

void AudioFader::fadeIn(bool block) {
    m_logger->debug("[{}] Fade IN requested - blocking: {}", m_objName.toStdString(), block);
    emit fadeStarted();
    m_targetVol = 1.0;
    m_curState = FadingIn;
    emit faderStateChanged(FadingIn);
    m_timer.start();
    if (!block)
        return;
    while (volume() != m_targetVol && m_curState == FadingIn) {
        QApplication::processEvents();
    }
    m_logger->debug("[{}] Fade completed", m_objName.toStdString());
}

void AudioFader::timerTimeout() {
    double increment = .05;
    if (isFading()) {
        if (volume() == m_targetVol) {
            m_logger->debug("[{}] Target volume reached", m_objName.toStdString());
            m_timer.stop();
            if (m_curState == FadingOut)
                m_curState = FadedOut;
            else
                m_curState = FadedIn;
            emit faderStateChanged(m_curState);
            emit fadeComplete();
            return;
        }
        else if (volume() > m_targetVol) {
            if ((volume() - increment) < m_targetVol) {
                setVolume(m_targetVol);
                m_timer.stop();
                m_curState = FadedOut;
                emit faderStateChanged(FadedOut);
                emit fadeComplete();
                return;
            }
            setVolume(volume() - increment);
        }
        else if (volume() < m_targetVol) {
            if ((volume() + increment) > m_targetVol) {
                setVolume(m_targetVol);
                m_timer.stop();
                m_curState = FadedIn;
                emit faderStateChanged(FadedIn);
                emit fadeComplete();
                return;
            }
            setVolume(volume() + increment);
        }
    }
}
