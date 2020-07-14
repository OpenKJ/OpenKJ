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
#include "libCDG/include/libCDG.h"

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

class MediaBackend : public AbstractAudioBackend
{
    Q_OBJECT
public:
    explicit MediaBackend(bool loadPitchShift = true, QObject *parent = nullptr, QString objectName = "unknown");
    ~MediaBackend();
    enum accel{OpenGL=0,XVideo};
    bool canChangeTempo() { return m_canChangeTempo; }
    bool canDetectSilence() { return true; }
    bool canDownmix() { return true; }
    bool canFade() { return true; }
    bool canPitchShift() { return m_canKeyChange; }
    bool canRenderCdg() { return true; }
    bool hasVideo();
    bool isMuted() { return m_muted; }
    bool isSilent();
    void setAccelType(const accel &type=accel::XVideo) { accelMode = type; }
    void setOutputDevice(int deviceIndex);
    void setVideoWinId(WId winID) { videoWinId = winID; }
    void setVideoWinId2(WId winID) { videoWinId2 = winID; }
    void videoMute(const bool &mute);
    int getCdgLastDraw() { return cdg.lastCDGUpdate(); }
    bool isCdgMode() { return m_cdgMode; }
    const bool& videoMuted() { return m_vidMuted; }
    int volume() { return m_volume; }
    qint64 position();
    qint64 getCdgPosition();
    qint64 duration();
    State state();
    State cdgState();
    QString backendName() { return "GStreamer"; }
    int pitchShift() { return m_keyChange; }
    bool downmixChangeRequiresRestart() { return false; }
    int tempo() { return m_tempo; }
    QStringList getOutputDevices() { return outputDeviceNames; }
    QString msToMMSS(qint64 msec)
    {
        QString sec;
        QString min;
        int seconds = (int) (msec / 1000) % 60 ;
        int minutes = (int) ((msec / (1000*60)) % 60);

        if (seconds < 10)
            sec = "0" + QString::number(seconds);
        else
        {
            sec = QString::number(seconds);
        }
        min = QString::number(minutes);
        return QString(min + ":" + sec);
    }
private:
    GstPad *videoQueue1SrcPad;
    GstPad *videoQueue2SrcPad;
    GstPad *videoTeePad1;
    GstPad *videoTeePad2;
    GstElement *cdgPipeline;
    GstElement *playBin;
    GstElement *aConvEnd;
    GstElement *audioPanorama;
    GstElement *fltrPostPanorama;
    GstElement *audioBin;
    GstElement *audioSink;
    GstElement *pitchShifterRubberBand;
    GstElement *pitchShifterSoundtouch;
    GstElement *volumeElement;
    GstElement *faderVolumeElement;
    GstElement *equalizer;
    GstElement *videoTee;
    GstElement *videoBin;
    GstElement *videoSink1;
    GstElement *videoSink2;
    gstTimerCallbackData *myObj;
    GstCaps *audioCapsStereo;
    GstCaps *audioCapsMono;
    QString objName;
    QString m_filename;
    QString m_cdgFilename;
    QStringList outputDeviceNames;
    QStringList GstGetPlugins();
    QStringList GstGetElements(QString plugin);
    QTimer fastTimer;
    QTimer slowTimer;
    int m_silenceDuration{0};
    int m_outputChannels{0};    int m_tempo{100};
    int m_keyChange{0};
    int m_volume{0};
    int lastPosition{0};
    int outputDeviceIdx{0};
    double m_currentRmsLevel{0.0};
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
    bool m_previewEnabledLastBuild{true};
    bool bypass{false};
    bool loadPitchShift{false};
    bool downmix{false};
    std::array<int,10> eqLevels{0,0,0,0,0,0,0,0,0,0};
    std::vector<GstDevice*> outputDevices;
    QPointer<AudioFader> fader;
    CdgParser cdg;
    AbstractAudioBackend::State lastState{AbstractAudioBackend::StoppedState};
    WId videoWinId{0};
    WId videoWinId2{0};
    accel accelMode{XVideo};

    void buildPipeline();
    void destroyPipeline();
    void resetPipeline();
    static GstBusSyncReply busMessageDispatcher(GstBus *bus, GstMessage *message, gpointer userData);
    static GstBusSyncReply busMessageDispatcherCdg(GstBus *bus, GstMessage *message, gpointer userData);
    guint64 cdgPosition{0};
    unsigned int curFrame{0};
    static void cb_need_data(GstElement *appsrc, guint unused_size, gpointer user_data);
    static void cb_seek_data(GstElement *appsrc, guint64 position, gpointer user_data);

private slots:
    void fastTimer_timeout();
    void slowTimer_timeout();
    void faderChangedVolume(int volume);
    void faderStarted();
    void faderFinished();
    void busMessage(std::shared_ptr<GstMessage> message);
    void busMessageCdg(std::shared_ptr<GstMessage> message);
    void gstPositionChanged(qint64 position);
    void gstDurationChanged(qint64 duration);
    void gstFastTimerFired();
    void faderChangedVolume(double volume);
    void faderStateChanged(AudioFader::FaderState state);

public slots:
    void play();
    void cdgPlay();
    void pause();
    void cdgPause();
    void setMedia(QString filename);
    void setMediaCdg(QString cdgFilename, QString audioFilename);
    void setMuted(bool muted);
    void setPosition(qint64 position);
    void cdgSetPosition(qint64 position);
    void setVolume(int volume);
    void stop(bool skipFade = false);
    void cdgStop();
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


//class MediaBackend : public QThread {
//    Q_OBJECT
//private:
//    AudioBackendGstreamer *gst;
//    void run() override {
//        gst = new AudioBackendGstreamer(true, this, "karaoke");
//        connect(gst, &AudioBackendGstreamer::audioAvailableChanged, [&] (auto val) {
//            emit audioAvailableChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::bufferStatusChanged, [&] (auto val) {
//            emit bufferStatusChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::durationChanged, [&] (auto val) {
//            emit durationChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::mutedChanged, [&] (auto val) {
//            emit mutedChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::positionChanged, [&] (auto val) {
//            emit positionChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::stateChanged, [&] (auto val) {
//            emit stateChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::videoAvailableChanged, [&] (auto val) {
//            emit videoAvailableChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::volumeChanged, [&] (auto val) {
//            emit volumeChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::silenceDetected, [&] () {
//            emit silenceDetected();
//        });
//        connect(gst, &AudioBackendGstreamer::pitchChanged, [&] (auto val) {
//            emit pitchChanged(val);
//        });
//        connect(gst, &AudioBackendGstreamer::newVideoFrame, [&] (auto val1, auto val2) {
//            emit newVideoFrame(val1, val2);
//        });
//        connect(gst, &AudioBackendGstreamer::audioError, [&] (auto val) {
//            emit audioError(val);
//        });
//        while (true)
//        {
//            this->yieldCurrentThread();
//        }
//    }

//public:
//    //enum accel{OpenGL=0,XVideo};
//    bool canChangeTempo() { return gst->canChangeTempo(); }
//    bool canDetectSilence() { return gst->canDetectSilence(); }
//    bool canDownmix() { return gst->canDownmix(); }
//    bool canFade() { return gst->canFade(); }
//    bool canPitchShift() { return gst->canPitchShift(); }
//    bool canRenderCdg() { return gst->canRenderCdg(); }
//    bool hasVideo() {return gst->hasVideo();}
//    bool isMuted() { return gst->isMuted(); }
//    bool isSilent() {return gst->isSilent();}
//    //void setAccelType(const accel &type=accel::XVideo) { gst->setAccelType(type); }
//    void setOutputDevice(int deviceIndex) {gst->setOutputDevice(deviceIndex);}
//    void setVideoWinId(WId winID) { gst->setVideoWinId(winID); }
//    void setVideoWinId2(WId winID) { gst->setVideoWinId2(winID); }
//    void videoMute(const bool &mute) {gst->videoMute(mute);}
//    int getCdgLastDraw() { return gst->getCdgLastDraw(); }
//    bool isCdgMode() { return gst->isCdgMode(); }
//    const bool& videoMuted() { return gst->videoMuted(); }
//    int volume() { return gst->volume(); }
//    qint64 position() { return gst->position();}
//    qint64 duration() { return gst->duration();}
//    AbstractAudioBackend::State state() {return gst->state();}
//    QString backendName() { return gst->backendName(); }
//    int pitchShift() { return gst->pitchShift(); }
//    bool downmixChangeRequiresRestart() { return gst->downmixChangeRequiresRestart(); }
//    int tempo() { return gst->tempo(); }
//    QStringList getOutputDevices() { return gst->getOutputDevices(); }
//    QString msToMMSS(qint64 msec)
//    {
//        QString sec;
//        QString min;
//        int seconds = (int) (msec / 1000) % 60 ;
//        int minutes = (int) ((msec / (1000*60)) % 60);

//        if (seconds < 10)
//            sec = "0" + QString::number(seconds);
//        else
//        {
//            sec = QString::number(seconds);
//        }
//        min = QString::number(minutes);
//        return QString(min + ":" + sec);
//    }

//public slots:
//    void play() {gst->play();}
//    void pause() {gst->pause();}
//    void setMedia(QString filename) { gst->setMedia(filename);}
//    void setMediaCdg(QString cdgFilename, QString audioFilename) {gst->setMediaCdg(cdgFilename, audioFilename);}
//    void setMuted(bool muted) {gst->setMuted(muted);}
//    void setPosition(qint64 position) {gst->setPosition(position);}
//    void setVolume(int volume) {gst->setVolume(volume);}
//    void stop(bool skipFade = false) {gst->stop(skipFade);}
//    void rawStop() {gst->rawStop();}
//    void setPitchShift(int pitchShift) {gst->setPitchShift(pitchShift);}
//    void fadeOut(bool waitForFade = true) {gst->fadeOut(waitForFade);}
//    void fadeIn(bool waitForFade = true) {gst->fadeIn(waitForFade);}
//    void setUseFader(bool fade) {gst->setUseFader(fade);}
//    void setUseSilenceDetection(bool enabled) {gst->setUseSilenceDetection(enabled);}
//    void setDownmix(bool enabled) {gst->setDownmix(enabled);}
//    void setTempo(int percent) {gst->setTempo(percent);}
//    void setMplxMode(int mode) {gst->setMplxMode(mode);}
//    void setEqBypass(bool bypass) {gst->setEqBypass(bypass);}
//    void setEqLevel1(int level) {gst->setEqLevel1(level);}
//    void setEqLevel2(int level) {gst->setEqLevel1(level);}
//    void setEqLevel3(int level) {gst->setEqLevel1(level);}
//    void setEqLevel4(int level) {gst->setEqLevel1(level);}
//    void setEqLevel5(int level) {gst->setEqLevel1(level);}
//    void setEqLevel6(int level) {gst->setEqLevel1(level);}
//    void setEqLevel7(int level) {gst->setEqLevel1(level);}
//    void setEqLevel8(int level) {gst->setEqLevel1(level);}
//    void setEqLevel9(int level) {gst->setEqLevel1(level);}
//    void setEqLevel10(int level) {gst->setEqLevel1(level);}
//    void fadeInImmediate() {gst->fadeInImmediate();}
//    void fadeOutImmediate() {gst->fadeOutImmediate();}

//signals:
//    void audioAvailableChanged(bool);
//    void bufferStatusChanged(int);
//    void durationChanged(qint64);
//    void mutedChanged(bool);
//    void positionChanged(qint64);
//    void stateChanged(AbstractAudioBackend::State);
//    void videoAvailableChanged(bool);
//    void volumeChanged(int);
//    void silenceDetected();
//    void pitchChanged(int);
//    void newVideoFrame(QImage frame, QString backendName);
//    void audioError(QString msg);
//};


#endif // AUDIOBACKENDGSTREAMER_H
