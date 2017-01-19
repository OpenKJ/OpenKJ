/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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

#ifndef AUDIOBACKENDHYBRID_H
#define AUDIOBACKENDHYBRID_H

#include "abstractaudiobackend.h"
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <gst/gst.h>
#include <QTimer>
#include <QThread>

class FaderGStreamer : public QThread
{
    Q_OBJECT

private:
    double m_targetVolume;
    double m_preOutVolume;
    GstElement *volumeElement;
    bool fading;
    void setVolume(double targetVolume);
    double volume();

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

};

class AudioBackendGstreamer : public AbstractAudioBackend
{
    Q_OBJECT

private:
    GstElement *sinkBin;
    GstElement *playBin;
    GstElement *audioConvert;
    GstElement *audioConvert2;
    GstElement *autoAudioSink;
    GstElement *rgVolume;
    GstElement *pitch;
    GstElement *pitch2;
    GstElement *volumeElement;
    GstElement *level;
    GstElement *filter;
    GstCaps *audioCapsStereo;
    GstCaps *audioCapsMono;
    GstPad *pad;
    GstPad *ghostPad;
    GstBus *bus;
    QString m_filename;
    QTimer *fastTimer;
    QTimer *slowTimer;
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

public:
    explicit AudioBackendGstreamer(QObject *parent = 0);
    ~AudioBackendGstreamer();
    int volume();
    qint64 position();
    bool isMuted();
    qint64 duration();
    AbstractAudioBackend::State state();
    QString backendName();
    bool stopping();
    void keyChangerOn();
    void keyChangerOff();
    bool canPitchShift();
    int pitchShift();
    bool canDetectSilence();
    bool isSilent();
    bool canFade();
    bool canDownmix();
    bool downmixChangeRequiresRestart() { return false; }

private slots:
    void fastTimer_timeout();
    void slowTimer_timeout();
    void faderChangedVolume(int volume);

public slots:
    void play();
    void pause();
    void setMedia(QString filename);
    void setMuted(bool muted);
    void setPosition(qint64 position);
    void setVolume(int volume);
    void stop(bool skipFade = false);
    void setPitchShift(int pitchShift);
    void fadeOut();
    void fadeIn();
    void setUseFader(bool fade);
    void setUseSilenceDetection(bool enabled);
    void setDownmix(bool enabled);

};

#endif // AUDIOBACKENDHYBRID_H
