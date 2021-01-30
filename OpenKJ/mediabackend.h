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
#include <QMutex>
#include <QImage>
#include "audiofader.h"
#include <QPointer>
#include <memory>
#include <array>
#include <vector>
#include "libCDG/src/cdgfilereader.h"
#include "settings.h"
#include "gstreamer/gstreamerhelper.h"

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

    enum MediaType {
        Karaoke,
        BackgroundMusic,
        SFX,
        VideoPreview
    };

    enum State {
        PlayingState=0,
        PausedState,
        StoppedState,
        EndOfMediaState,
        UnknownState
    };

    enum accel {
        OpenGL=0,
        XVideo
    };

    explicit MediaBackend(QObject *parent, QString objectName, MediaType type);
    ~MediaBackend();

    bool canChangeTempo() { return true; }
    bool canDetectSilence() { return true; }
    bool canFade() { return true; }
    bool canPitchShift() { return true; }
    bool hasVideo();
    bool isSilent();
    void setAccelType(const accel &type=accel::XVideo) { m_accelMode = type; }
    void setAudioOutputDevice(int deviceIndex);
    void setHWVideoOutputDevices(std::vector<WId> videoWinIds);
    void setVideoEnabled(const bool &enabled);
    bool isVideoEnabled() { return m_videoEnabled; }
    bool isCdgMode() { return m_cdgMode; }
    int getVolume() { return m_volume; }

    void writePipelinesGraphToFile(const QString filePath);

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

private:
    enum GstPlayFlags {
        GST_PLAY_FLAG_VIDEO         = (1 << 0),
        GST_PLAY_FLAG_AUDIO         = (1 << 1),
        GST_PLAY_FLAG_TEXT          = (1 << 2)
    };

    struct VideoSinkData {
        WId windowId { 0 };
        GstElement *videoSink { nullptr };
    };

    QString m_objName;
    MediaType m_type;
    Settings m_settings;
    GstBus *m_bus;

    GstElement *m_cdgAppSrc { nullptr };
    GstElement *m_scaleTempo { nullptr };
    GstElement *m_queueMainVideo { nullptr };
    GstElement *m_decoder { nullptr };
    GstElement *m_playBin { nullptr };  // Pipeline
    GstBin *m_playBinAsBin { nullptr };
    GstElement *m_aConvEnd { nullptr };
    GstElement *m_audioPanorama { nullptr };
    GstElement *m_fltrPostPanorama { nullptr };
    GstElement *m_audioBin { nullptr }; // GstBin
    GstElement *m_audioSink { nullptr };
    GstElement *m_pitchShifterRubberBand { nullptr };
    GstElement *m_pitchShifterSoundtouch { nullptr };
    GstElement *m_volumeElement { nullptr };
    GstElement *m_faderVolumeElement { nullptr };
    GstElement *m_equalizer { nullptr };

    GstElement *m_videoBin { nullptr }; // GstBin

    GstElement *m_videoTee { nullptr };
    //GstElement *m_videoTeeCdg { nullptr };
    GstElement *m_videoAppSink { nullptr };

    PadInfo *m_audioSrcPad { nullptr };
    PadInfo *m_videoSrcPad { nullptr };

    GstCaps *m_audioCapsStereo { nullptr };
    GstCaps *m_audioCapsMono { nullptr };

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
    bool m_videoEnabled{true};
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

    State m_lastState{StoppedState};
    std::vector<VideoSinkData> m_videoSinks;
    accel m_accelMode{XVideo};
    int m_videoOffsetMs{0};


    void buildPipeline();
    void resetVideoSinks();
    void buildCdgBin();
    const char* getVideoSinkElementNameForFactory();
    void getGstDevices();
    void writePipelineGraphToFile(GstBin *bin, QString filePath, QString fileName);
    double getPitchForSemitone(const int &semitone);

    CdgFileReader *m_cdgFileReader {nullptr};
    std::atomic<bool> g_appSrcNeedData {false};
    QMutex m_cdgFileReaderLock;

    // AppSrc callbacks
    static void cb_need_data(GstElement *appsrc, guint unused_size, gpointer user_data);
    static void cb_enough_data(GstElement *appsrc, gpointer user_data);
    static gboolean cb_seek_data(GstElement *appsrc, guint64 position, gpointer user_data);

    // AppSink for software rendering (no HW accel)
    static GstFlowReturn NewSampleCallback(GstAppSink *appsink, gpointer user_data);
    bool pullFromSinkAndEmitNewVideoFrame(GstAppSink *appSink);

    static gboolean gstBusFunc(GstBus *bus, GstMessage *message, gpointer user_data);


    static void NoMorePadsCallback(GstElement *gstelement, gpointer data);
    static void padAddedToDecoder_cb(GstElement *element,  GstPad *pad, gpointer caller);

    void patchPipelineSinks();
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
