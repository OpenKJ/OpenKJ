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

#ifndef MEDIABACKEND_H
#define MEDIABACKEND_H

#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <gst/gst.h>
#include <gst/gstdevicemonitor.h>
#include <gst/gstdevice.h>
#include <gst/gstplugin.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gst/controller/gstdirectcontrolbinding.h>
#include <cdg/cdgappsrc.h>

#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QImage>
#include "audiofader.h"
#include "softwarerendervideosink.h"
#include <QPointer>
#include <memory>
#include <array>
#include <vector>
#include "cdg/cdgfilereader.h"
#include "settings.h"
#include "gstreamer/gstreamerhelper.h"

#define STUP 1.0594630943592952645618252949461
#define STDN 0.94387431268169349664191315666784
#define Multiplex_Normal 0
#define Multiplex_LeftChannel 1
#define Multiplex_RightChannel 2

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
    ~MediaBackend() override;

    static bool canChangeTempo() { return true; }
    static bool canFade() { return true; }
    static bool canPitchShift() { return true; }
    bool hasVideo() { return m_hasVideo; }
    bool isSilent();
    void setAccelType(const accel &type=accel::XVideo) { m_accelMode = type; }
    void setAudioOutputDevice(int deviceIndex);
    void setVideoOutputWidgets(const std::vector<QWidget*>& surfaces);
    void setVideoEnabled(const bool &enabled);
    [[nodiscard]] bool isVideoEnabled() const { return m_videoEnabled; }
    bool hasActiveVideo();
    [[nodiscard]] int getVolume() const { return m_volume; }

    void writePipelinesGraphToFile(const QString& filePath);

    qint64 position();
    qint64 duration();
    State state();
    QStringList getOutputDevices() { return m_outputDeviceNames; }
    static QString msToMMSS(const qint64 &msec)
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

    struct VideoSinkData {
        QWidget *surface { nullptr };
        GstElement *videoSink { nullptr };
        GstElement *videoScale { nullptr };
        SoftwareRenderVideoSink *softwareRenderVideoSink { nullptr };
    };

    QString m_objName;
    MediaType m_type;
    Settings m_settings;
    GstBus *m_bus{nullptr};

    /* PIPELINE */
    GstElement *m_pipeline { nullptr };  // Pipeline
    GstBin     *m_pipelineAsBin { nullptr };
    GstElement *m_decoder { nullptr };
    CdgAppSrc  *m_cdgSrc { nullptr };

    PadInfo *m_audioSrcPad { nullptr };
    PadInfo *m_videoSrcPad { nullptr };


    /* AUDIO SINK */
    GstElement *m_audioBin { nullptr }; // GstBin
    GstElement *m_scaleTempo { nullptr };
    GstElement *m_aConvEnd { nullptr };
    GstElement *m_audioPanorama { nullptr };
    GstElement *m_fltrPostPanorama { nullptr };
    GstElement *m_pitchShifterRubberBand { nullptr };
    GstElement *m_pitchShifterSoundtouch { nullptr };
    GstElement *m_volumeElement { nullptr };
    GstElement *m_faderVolumeElement { nullptr };
    GstElement *m_equalizer { nullptr };
    GstElement *m_audioSink { nullptr };

    GstCaps *m_audioCapsStereo { nullptr };
    GstCaps *m_audioCapsMono { nullptr };

    std::vector<GstDevice*> m_audioOutputDevices;

    std::array<int,10> m_eqLevels{0,0,0,0,0,0,0,0,0,0};


    /* VIDEO SINK */
    GstElement *m_videoBin { nullptr }; // GstBin
    GstElement *m_videoTee { nullptr };

    std::vector<VideoSinkData> m_videoSinks;

    accel m_accelMode{XVideo};
    int m_videoOffsetMs{0};

    QString m_filename;
    QString m_cdgFilename;
    QStringList m_outputDeviceNames;
    QTimer m_gstBusMsgHandlerTimer;
    QTimer m_timerFast;
    QTimer m_timerSlow;
    int m_silenceDuration{0};
    long m_positionWatchdogLastPos{0};

    double m_playbackRate{1.0};
    int m_volume{0};
    int m_lastPosition{0};
    int m_outputDeviceIdx{0};
    double m_currentRmsLevel{0.0};
    bool m_cdgMode{false};
    bool m_fade{false};
    bool m_currentlyFadedOut{false};
    bool m_silenceDetect{false};
    bool m_videoEnabled{true};
    bool m_bypass{false};
    bool m_loadPitchShift;
    bool m_downmix{false};
    std::atomic<bool> m_hasVideo{false};
    bool m_videoAccelEnabled{false};
    QPointer<AudioFader> m_fader;
    std::atomic<GstState> m_currentState { GST_STATE_NULL };

    void buildPipeline();
    void buildVideoSinkBin();
    void buildAudioSinkBin();
    void resetVideoSinks();
    void forceVideoExpose();
    const char* getVideoSinkElementNameForFactory();
    void getAudioOutputDevices();
    void writePipelineGraphToFile(GstBin *bin, const QString& filePath, QString fileName);
    static double getPitchForSemitone(const int &semitone);

    void gstBusFunc(GstMessage *message);
    static void padAddedToDecoder_cb(GstElement *element,  GstPad *pad, gpointer caller);

    void stopPipeline();
    void resetPipeline();
    void patchPipelineSinks();

private slots:
    void timerFast_timeout();
    void timerSlow_timeout();


public slots:
    void setVideoOffset(int offsetMs);
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
    void setEqLevel(const int &band, const int &level);
    void fadeInImmediate();
    void fadeOutImmediate();
    void setEnforceAspectRatio(const bool &enforce);

signals:
    void audioAvailableChanged(const bool audioAvailable);
    void bufferStatusChanged(const int status);
    void durationChanged(const qint64 duration);
    void mutedChanged(const bool muted);
    void positionChanged(const qint64 position);
    void stateChanged(const State state);
    void hasActiveVideoChanged(const bool hasVideo);
    void volumeChanged(const int vol);
    void silenceDetected();
    void pitchChanged(const int key);
    void audioError(const QString &msg);

};

#endif // MEDIABACKEND_H
