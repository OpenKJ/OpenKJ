/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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

#ifndef AUDIOBACKENDGSTREAMER_H
#define AUDIOBACKENDGSTREAMER_H

#include "abstractaudiobackend.h"
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/gstdevicemonitor.h>
#include <gst/gstdevice.h>
#include <gst/gstplugin.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gst/controller/gstdirectcontrolbinding.h>

#include <QTimer>
#include <QThread>
#include <QImage>
#include <QAudioOutput>
#include "audiofader.h"
#include <QPointer>

class gstTimerCallbackData
{
public:
    gstTimerCallbackData() {}
    QObject *qObj;
    gpointer gObject;
};

class AudioBackendGstreamer : public AbstractAudioBackend
{
    Q_OBJECT

private:
    gstTimerCallbackData *myObj;
    GstElement *tee;
    GstElement *queueS;
    GstElement *queueM;
    GstElement *queueL;
    GstElement *queueR;
    GstElement *aConvR;
    GstElement *aConvL;
    GstElement *audioMixer;
    GstElement *fltrPostMixer;
    GstPad *teeSrcPadN;
    GstPad *teeSrcPadM;
    GstPad *queueSinkPadN;
    GstPad *queueSinkPadM;
    GstPad *queueSrcPadN;
    GstPad *queueSrcPadM;
    GstPad *queueSrcPadL;
    GstPad *queueSinkPadL;
    GstPad *queueSrcPadR;
    GstPad *queueSinkPadR;
    GstPad *mixerSinkPadL;
    GstPad *mixerSinkPadR;
    GstPad *mixerSinkPadN;
    GstPad *aConvSrcPadR;
    GstPad *aConvSrcPadL;
    GstElement *aConvPostMixer;
    GstElement *customBin;
    GstElement *videoAppSink;
    GstElement *aConvInput;
    GstElement *aConvPreSplit;
    GstElement *aConvPrePitchShift;
    GstElement *aConvPostPitchShift;
    GstElement *aConvEnd;
    GstElement *audioSink;
    GstElement *rgVolume;
    GstElement *pitchShifterRubberBand;
    GstElement *pitchShifterSoundtouch;
    GstElement *deInterleave;
    GstElement *level;
    GstElement *fltrMplxInput;
    GstElement *fltrEnd;
    GstElement *audioResample;
    GstElement *volumeElement;
    GstElement *equalizer;
    GstCaps *audioCapsStereo;
    GstCaps *audioCapsMono;
    GstCaps *videoCaps;
    GstPad *pad;
    GstPad *ghostPad;
 //   GstBus *bus;
    GstDeviceMonitor *monitor;
    GstControlSource *csource;
    GstTimedValueControlSource *tv_csource;
    QString m_filename;
    QTimer *fastTimer;
    QTimer *slowTimer;
    int m_keyChange;
    int m_volume;
    bool m_fade;
    bool m_silenceDetect;
    bool m_canKeyChange;
    bool m_canChangeTempo;
    bool m_keyChangerRubberBand;
    bool m_keyChangerSoundtouch;
    bool m_muted;
    bool isFading;
    int m_silenceDuration;
    void processGstMessages();
    int m_outputChannels;
    double m_currentRmsLevel;
    double m_preFadeVolume;
    int m_preFadeVolumeInt;
    int eq1, eq2, eq3, eq4, eq5, eq6, eq7, eq8, eq9, eq10;
    bool bypass;
    bool loadPitchShift;
    int outputDeviceIdx;
    bool downmix;
    static gboolean gstTimerDispatcher(QObject *qObj);
    static void EndOfStreamCallback(GstAppSink *appsink, gpointer user_data);
    static GstFlowReturn NewPrerollCallback(GstAppSink *appsink, gpointer user_data);
    static GstFlowReturn NewSampleCallback(GstAppSink *appsink, gpointer user_data);
    static GstFlowReturn NewAudioSampleCallback(GstAppSink *appsink, gpointer user_data);
    static void cb_new_pad (GstElement *element, GstPad *pad, gpointer data);

    QStringList GstGetPlugins();
    QStringList GstGetElements(QString plugin);
    QStringList outputDeviceNames;
    QList<GstDevice*> outputDevices;
    QPointer<AudioFader> fader;
    void buildPipeline();
    void destroyPipeline();
    void resetPipeline();
    bool faderRunning;
    std::shared_ptr<GstBus> bus;
    std::shared_ptr<GstElement> pipeline;
    static void DestroyCallback(gpointer user_data);
    static GstBusSyncReply busMessageDispatcher(GstBus *bus, GstMessage *message, gpointer userData);
    AbstractAudioBackend::State lastState;
public:
    GstElement *playBin;
    explicit AudioBackendGstreamer(bool loadPitchShift = true, QObject *parent = 0, QString objectName = "unknown");
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
    bool canChangeTempo();
    bool canDetectSilence();
    bool isSilent();
    bool canFade();
    bool canDownmix();
    bool downmixChangeRequiresRestart() { return false; }
    void newFrame();
    QString objName;
    int m_tempo;
    int tempo();
    bool m_hasVideo;

private slots:
    void fastTimer_timeout();
    void slowTimer_timeout();
    void faderChangedVolume(int volume);
    void faderStarted();
    void faderFinished();
    void busMessage(std::shared_ptr<GstMessage> message);
    void gstPositionChanged(qint64 position);
    void gstDurationChanged(qint64 duration);
    void gstFastTimerFired();
    void faderChangedVolume(double volume);


public slots:
    void play();
    void pause();
    void setMedia(QString filename);
    void setMuted(bool muted);
    void setPosition(qint64 position);
    void setVolume(int volume);
    void stop(bool skipFade = false);
    void setPitchShift(int pitchShift);
    void fadeOut(bool waitForFade = true);
    void fadeIn(bool waitForFade = true);
    void setUseFader(bool fade);
    void setUseSilenceDetection(bool enabled);
    void setDownmix(bool enabled);
    void setTempo(int percent);

signals:



    // AbstractAudioBackend interface
public:
    QStringList getOutputDevices();

    // AbstractAudioBackend interface
public:
    void setOutputDevice(int deviceIndex);

    // AbstractAudioBackend interface
public slots:
    void setMplxMode(int mode);

    // AbstractAudioBackend interface
public slots:
    void setEqBypass(bool bypass);
    void setEqLevel1(int level);
    void setEqLevel2(int level);
    void setEqLevel3(int level);
    void setEqLevel4(int level);
    void setEqLevel5(int level);
    void setEqLevel6(int level);
    void setEqLevel7(int level);
    void setEqLevel8(int level);
    void setEqLevel9(int level);
    void setEqLevel10(int level);

    // AbstractAudioBackend interface
public:
    bool hasVideo();
};

#endif // AUDIOBACKENDGSTREAMER_H
