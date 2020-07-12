/*
 * Copyright (c) 2013-2020 Thomas Isaac Lightburn
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
#include <memory>
#include <array>
#include <vector>

/* playbin flags */
typedef enum {
    GST_PLAY_FLAG_VIDEO         = (1 << 0), /* We want video output */
    GST_PLAY_FLAG_AUDIO         = (1 << 1), /* We want audio output */
    GST_PLAY_FLAG_TEXT          = (1 << 2)  /* We want subtitle output */
} GstPlayFlags;

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
public:
    enum accel{OpenGL=0,XVideo};
private:
    gstTimerCallbackData *myObj;
    GstElement *aConvEnd;
    GstElement *audioPanorama;
    GstElement *fltrPostPanorama;
    GstElement *audioBin;
    GstElement *audioSink;
    GstElement *pitchShifterRubberBand;
    GstElement *pitchShifterSoundtouch;
    GstPad *videoQueue1SrcPad;
    GstPad *videoQueue2SrcPad;
    GstPad *videoTeePad1;
    GstPad *videoTeePad2;
//    GstElement *fltrEnd;
    GstElement *volumeElement;
    GstElement *faderVolumeElement;
    GstElement *equalizer;
    GstElement *videoTee;
    GstElement *videoBin;
    GstElement *videoSink1;
    GstElement *videoSink2;
    GstCaps *audioCapsStereo;
    GstCaps *audioCapsMono;
    QString m_filename;
    QString m_cdgFilename;
    QTimer fastTimer;
    QTimer slowTimer;
    int m_keyChange{0};
    int m_volume{0};
    bool m_cdgMode{false};
    bool m_fade{false};
    bool m_silenceDetect{false};
    bool m_canKeyChange{false};
    bool m_canChangeTempo{false};
    bool m_keyChangerRubberBand{false};
    bool m_keyChangerSoundtouch{false};
    bool m_muted{false};
    bool m_vidMuted{false};
    bool m_canRenderCdg{false};
    bool initDone{false};
    int m_silenceDuration{0};
    int m_outputChannels{0};
    bool m_previewEnabledLastBuild{true};
    double m_currentRmsLevel{0.0};
    std::array<int,10> eqLevels{0,0,0,0,0,0,0,0,0,0};
    bool bypass{false};
    bool loadPitchShift;
    int outputDeviceIdx{0};
    bool downmix{false};
    std::unique_ptr<GstBus> bus;
    std::unique_ptr<GstElement> pipeline;
    QStringList outputDeviceNames;
    int lastPosition{0};
    std::vector<GstDevice*> outputDevices;
    QPointer<AudioFader> fader;
    AbstractAudioBackend::State lastState{AbstractAudioBackend::StoppedState};
    WId videoWinId{0};
    WId videoWinId2{0};
    accel accelMode{XVideo};

    QStringList GstGetPlugins();
    QStringList GstGetElements(QString plugin);
    void buildPipeline();
    void destroyPipeline();
    void resetPipeline();
    static GstBusSyncReply busMessageDispatcher(GstBus *bus, GstMessage *message, gpointer userData);


public:
    GstElement *playBin;
    QString objName;
    int m_tempo{100};

    void setAccelType(accel type=accel::XVideo) { accelMode = type; }
    explicit AudioBackendGstreamer(bool loadPitchShift = true, QObject *parent = nullptr, QString objectName = "unknown");
    void setVideoWinId(WId winID) { videoWinId = winID; }
    void setVideoWinId2(WId winID) { videoWinId2 = winID; }
    void videoMute(bool mute);
    bool videoMuted();
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
    bool canRenderCdg() { return m_canRenderCdg; }
    int pitchShift();
    bool canChangeTempo();
    bool canDetectSilence();
    bool isSilent();
    bool canFade();
    bool canDownmix();
    bool downmixChangeRequiresRestart() { return false; }

    int tempo();
    QStringList getOutputDevices();
    void setOutputDevice(int deviceIndex);
    bool hasVideo();

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
    void faderStateChanged(AudioFader::FaderState state);


public slots:
    void play();
    void pause();
    void setMedia(QString filename);
    void setMediaCdg(QString cdgFilename, QString audioFilename);
    void setMuted(bool muted);
    void setPosition(qint64 position);
    void setVolume(int volume);
    void stop(bool skipFade = false);
    void rawStop();
    void setPitchShift(int pitchShift);
    void fadeOut(bool waitForFade = true);
    void fadeIn(bool waitForFade = true);
    void setUseFader(bool fade);
    void setUseSilenceDetection(bool enabled);
    void setDownmix(bool enabled);
    void setTempo(int percent);
    void setMplxMode(int mode);
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
    void fadeInImmediate();
    void fadeOutImmediate();

signals:


};

#endif // AUDIOBACKENDGSTREAMER_H
