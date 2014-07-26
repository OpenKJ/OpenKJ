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

#ifndef BMAUDIOBACKENDGSTREAMER_H
#define BMAUDIOBACKENDGSTREAMER_H

#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <gst/gst.h>
#include <QTimer>
#include <QThread>
#include "bmabstractaudiobackend.h"



class FaderGStreamer : public QThread
{
    Q_OBJECT
public:
    explicit FaderGStreamer(GstElement *GstVolumeElement, QObject *parent = 0);
    void run();
    void fadeIn();
    void fadeOut();
    bool isFading();
    void restoreVolume();
    void setVolumeElement(GstElement *GstVolumeElement);

signals:
    void volumeChanged(int);

public slots:
    void setBaseVolume(int volume);

private:
    double m_targetVolume;
    double m_preOutVolume;
    GstElement *volumeElement;
    bool fading;
    void setVolume(double targetVolume);
    double volume();
};

class BmAudioBackendGStreamer : public BmAbstractAudioBackend
{
    Q_OBJECT
public:
    explicit BmAudioBackendGStreamer(QObject *parent = 0);
    ~BmAudioBackendGStreamer();
signals:

public slots:

private:
    GstElement *sinkBin;
    GstElement *playBin;
    GstElement *audioConvert;
    GstElement *audioConvert2;
    GstElement *autoAudioSink;
    GstElement *rgVolume;
    GstElement *volumeElement;
    GstElement *level;
    GstElement *filter;
    GstCaps *audioCapsStereo;
    GstCaps *audioCapsMono;
    GstPad *pad;
    GstPad *ghostPad;
    GstBus *bus;
    QString m_filename;
    QTimer *signalTimer;
//    QTimer *silenceDetectTimer;
    bool m_keyChangerOn;
    int m_keyChange;
    int m_volume;
    FaderGStreamer *fader;
    bool m_fade;
    bool m_silenceDetect;
    bool m_canKeyChange;
    void processGstMessages();
    int m_outputChannels;
    double m_currentRmsLevel;


    // KhAbstractAudioBackend interface
public:
    int volume();
    qint64 position();
    bool isMuted();
    qint64 duration();
    BmAbstractAudioBackend::State state();
    QString backendName();
    bool stopping();
    void keyChangerOn();
    void keyChangerOff();

public slots:
    void play();
    void pause();
    void setMedia(QString filename);
    void setMuted(bool muted);
    void setPosition(qint64 position);
    void setVolume(int volume);
    void stop(bool skipFade = false);

private slots:
    void signalTimer_timeout();
//    void silenceDetectTimer_timeout();
    void faderChangedVolume(int volume);

    // KhAbstractAudioBackend interface
public:
    bool canFade();

public slots:
    void fadeOut();
    void fadeIn();
    void setUseFader(bool fade);

    // KhAbstractAudioBackend interface
public:
    bool canDetectSilence();
    bool isSilent();

public slots:
    void setUseSilenceDetection(bool enabled);

    // KhAbstractAudioBackend interface
public:
    bool canDownmix();
    bool downmixChangeRequiresRestart() { return false; }

public slots:
    void setDownmix(bool enabled);
};

#endif // KHAUDIOBACKENDGSTREAMER_H
