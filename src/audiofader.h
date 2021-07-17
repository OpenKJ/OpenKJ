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
public:
    explicit AudioFader(QObject *parent = nullptr);
    enum FaderState{FadedIn=0,FadingIn,FadedOut,FadingOut};
    [[nodiscard]] static std::string stateToStr(FaderState state);
    void setVolumeElement(GstElement *volumeElement);
    void setObjName(const QString &name);
    [[nodiscard]] bool isFading();
    void setVolume(double volume);
    void immediateIn();
    void immediateOut();
    [[nodiscard]] FaderState state();

private:
    [[nodiscard]] double volume();

    GstElement *m_volumeElement{nullptr};
    QTimer m_timer;
    double m_targetVol{0.0};
    QString m_objName;
    std::shared_ptr<spdlog::logger> m_logger;
    FaderState m_curState{FadedIn};

signals:
    void volumeChanged(double volume);
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
