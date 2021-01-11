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
#include "audiofader.h"
#include <QPointer>
#include <memory>
#include <array>
#include <vector>
//#include "libCDG/include/libCDG.h"
#include "libCDG/src/cdgfilereader.h"
#include "settings.h"

#define STUP 1.0594630943592952645618252949461
#define STDN 0.94387431268169349664191315666784
#define Multiplex_Normal 0
#define Multiplex_LeftChannel 1
#define Multiplex_RightChannel 2

/* playbin flags */


class MediaBackend : public QObject
{
    Q_OBJECT
public:
    enum State{
        PlayingState=0,
        PausedState,
        StoppedState,
        EndOfMediaState,
        UnknownState
    };
    explicit MediaBackend(bool m_loadPitchShift = true, QObject *parent = nullptr, QString objectName = "unknown");
    ~MediaBackend();
    enum accel{OpenGL=0,XVideo};
    bool canChangeTempo() { return true; }
    bool canDetectSilence() { return true; }
    bool canFade() { return true; }
    bool canPitchShift() { return true; }
    bool hasVideo();
    bool isSilent();
    void setAccelType(const accel &type=accel::XVideo) { m_accelMode = type; }
    void setOutputDevice(int deviceIndex);
    void setVideoWinId(WId winID) { m_videoWinId1 = winID; }
    void setVideoWinId2(WId winID) { m_videoWinId2 = winID; }
    void videoMute(const bool &mute);
//    int getCdgLastDraw() { return (m_cdgMode) ? m_cdg.lastCDGUpdate() : 0; } todo: andth
    bool isCdgMode() { return m_cdgMode; }
    bool videoMuted() { return m_vidMuted; }
    int getVolume() { return m_volume; }
    qint64 position();
    qint64 duration();
    State state();
    QStringList getOutputDevices() { return m_outputDeviceNames; }
    QString msToMMSS(const qint64 &msec)
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
    //void testCdgDecode();
    void newFrame();
    void newFrameCdg();

private:
    enum GstPlayFlags {
        GST_PLAY_FLAG_VIDEO         = (1 << 0),
        GST_PLAY_FLAG_AUDIO         = (1 << 1),
        GST_PLAY_FLAG_TEXT          = (1 << 2)
    };
    Settings m_settings;
    GstBus *m_bus;
    GstElement *m_cdgBin;
    GstElement *m_mediaBin;
    GstElement *m_cdgAppSrc;
    GstElement *m_scaleTempo;
    //GstElement *m_cdgPipeline;
    GstElement *m_queueMainVideo;
    GstElement *m_queuePostAppSrc;
    GstElement *m_fakeVideoSink;
    GstElement *m_playBin;
    GstElement *m_aConvEnd;
    GstElement *m_audioPanorama;
    GstElement *m_fltrPostPanorama;
    GstElement *m_audioBin;
    GstElement *m_audioSink;
    GstElement *m_pitchShifterRubberBand;
    GstElement *m_pitchShifterSoundtouch;
    GstElement *m_volumeElement;
    GstElement *m_faderVolumeElement;
    GstElement *m_equalizer;
    GstElement *m_videoSink1;
    GstElement *m_videoSink2;
    GstElement *m_videoSink1Cdg;
    GstElement *m_videoSink2Cdg;
    GstElement *m_videoScale1Cdg;
    GstElement *m_videoScale2Cdg;
    GstElement *m_videoScale1;
    GstElement *m_videoScale2;
    GstElement *m_videoBin;
    GstElement *m_videoAppSink;
    GstElement *m_videoAppSinkCdg;
    //GstElement *m_cdgPlaybin;

//    GstElement *cdgVidConv;
//    GstElement *videoQueue1;
//    GstElement *videoQueue2;
//    GstElement *videoConv1;
//    GstElement *videoConv2;
//    GstElement *videoScale1;
//    GstElement *videoScale2;
//    GstElement *videoTee;
//    GstPad *videoTeePad1;
//    GstPad *videoTeePad2;
//    GstPad *videoQueue1SrcPad;
//    GstPad *videoQueue2SrcPad;


    GstCaps *m_audioCapsStereo;
    GstCaps *m_audioCapsMono;
    QString m_objName;
    QString m_filename;
    QString m_cdgFilename;
    QStringList m_outputDeviceNames;
    QTimer m_gstMsgBusHandlerTimer;
    QTimer m_timerFast;
    QTimer m_timerSlow;
    int m_silenceDuration{0};
    int m_tempo{100};
    int m_volume{0};
    int m_lastPosition{0};
    int m_outputDeviceIdx{0};
    double m_currentRmsLevel{0.0};
    bool m_cdgMode{false};
    bool m_fade{false};
    bool m_currentlyFadedOut{false};
    bool m_silenceDetect{false};
    bool m_canKeyChange{false};
    bool m_canChangeTempo{false};
    bool m_keyChangerRubberBand{false};
    bool m_keyChangerSoundtouch{false};
    bool m_vidMuted{false};
    bool m_previewEnabledLastBuild{true};
    bool m_bypass{false};
    bool m_loadPitchShift;
    bool m_downmix{false};
    bool m_noaccelHasVideo{false};
    bool m_enforceAspectRatio{true};
    bool m_videoAccelEnabled{false};
    std::array<int,10> m_eqLevels{0,0,0,0,0,0,0,0,0,0};
    std::vector<GstDevice*> m_outputDevices;
    QPointer<AudioFader> m_fader;

    CdgFileReader *m_cdgFileReader {nullptr};
    State m_lastState{StoppedState};
    WId m_videoWinId1{0};
    WId m_videoWinId2{0};
    accel m_accelMode{XVideo};
    int m_videoOffsetMs{0};


    void buildPipeline();
    void resetVideoSinks();
    void buildCdgBin();
    void getGstDevices();
    double getPitchForSemitone(const int &semitone);
    qint64 getCdgPosition();
    State cdgState();

    static void cb_need_data(GstElement *appsrc, guint unused_size, gpointer user_data);
    static void cb_enough_data(GstElement *appsrc, gpointer user_data);

    static gboolean cb_seek_data(GstElement *appsrc, guint64 position, gpointer user_data);


    static void EndOfStreamCallback(GstAppSink *appsink, gpointer user_data);
    static GstFlowReturn NewPrerollCallback(GstAppSink *appsink, gpointer user_data);
    static GstFlowReturn NewSampleCallback(GstAppSink *appsink, gpointer user_data);
    static GstFlowReturn NewSampleCallbackCdg(GstAppSink *appsink, gpointer user_data);
    //static GstFlowReturn NewAudioSampleCallback(GstAppSink *appsink, gpointer user_data);
    static void DestroyCallback(gpointer user_data);
    static gboolean gstBusFunc(GstBus *bus, GstMessage *message, gpointer user_data);


private slots:
    void timerFast_timeout();
    void timerSlow_timeout();


public slots:
    void setVideoOffset(const int offsetMs);
    void play();
    void pause();
    void setMedia(const QString &filename);
    void setMediaCdg(const QString &cdgFilename, const QString &audioFilename);
    void setMuted(const bool &muted);
    bool isMuted();
    void setPosition(const qint64 &position);
    void setVolume(const int &volume);
    void stop(const bool &skipFade = false);
    void rawStop();
    void setPitchShift(const int &pitchShift);
    void fadeOut(const bool &waitForFade = true);
    void fadeIn(const bool &waitForFade = true);
    void setUseFader(const bool &fade) {m_fade = fade;}
    void setUseSilenceDetection(const bool &enabled);
    void setDownmix(const bool &enabled);
    void setTempo(const int &percent);
    void setMplxMode(const int &mode);
    void setEqBypass(const bool &m_bypass);
    void setEqLevel1(const int &level);
    void setEqLevel2(const int &level);
    void setEqLevel3(const int &level);
    void setEqLevel4(const int &level);
    void setEqLevel5(const int &level);
    void setEqLevel6(const int &level);
    void setEqLevel7(const int &level);
    void setEqLevel8(const int &level);
    void setEqLevel9(const int &level);
    void setEqLevel10(const int &level);
    void fadeInImmediate();
    void fadeOutImmediate();
    void setEnforceAspectRatio(const bool &enforce);

signals:
    void audioAvailableChanged(const bool);
    void bufferStatusChanged(const int);
    void durationChanged(const qint64);
    void mutedChanged(const bool);
    void positionChanged(const qint64);
    void stateChanged(const State);
    void videoAvailableChanged(const bool);
    void volumeChanged(const int);
    void silenceDetected();
    void pitchChanged(const int);
    void newVideoFrame(const QImage &frame, const QString &backendName);
    void audioError(const QString &msg);

};

#endif // AUDIOBACKENDGSTREAMER_H
