#ifndef MEDIABACKEND_H
#define MEDIABACKEND_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QImage>
#include <QPointer>
#include <QProcess>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <memory>
#include <vector>
#include <atomic>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "settings.h"
#include "audiofader.h"
#include "videodisplay.h"
#include <spdlog/spdlog.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

#define STUP 1.0594630943592952645618252949461
#define STDN 0.94387431268169349664191315666784
#define Multiplex_Normal 0
#define Multiplex_LeftChannel 1
#define Multiplex_RightChannel 2

class MediaBackend : public QObject
{
    Q_OBJECT
public:
    std::string m_loggingPrefix;
    std::shared_ptr<spdlog::logger> m_logger;
    
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

    struct AudioOutputDevice {
        QString id;
        QString name;
    };

    explicit MediaBackend(QObject *parent, QString objectName, MediaType type);
    ~MediaBackend() override;

    static bool canChangeTempo() { return true; }
    static bool canFade() { return true; }
    static bool canPitchShift() { return true; }
    bool hasVideo() { return m_hasVideo; }
    bool isSilent();
    void setAccelType(const accel &type=accel::XVideo) {}
    void setAudioOutputDevice(const AudioOutputDevice &device);
    void setAudioOutputDevice(const QString &deviceName);
    void setVideoOutputWidgets(const std::vector<QWidget*>& surfaces);
    void setVideoEnabled(const bool &enabled);
    [[nodiscard]] bool isVideoEnabled() const { return m_videoEnabled; }
    bool hasActiveVideo();
    [[nodiscard]] int getVolume() const { return m_volume; }
    void forceVideoExpose() {}
    QString getName() { return m_objName; }
    void writePipelinesGraphToFile(const QString& filePath) {}

    qint64 position();
    qint64 duration();
    State state();
    QStringList getOutputDevices();
    
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
    QString m_objName;
    MediaType m_type;
    Settings m_settings;

    // GStreamer elements
    GstElement *m_pipeline{nullptr};
    GstElement *m_cdgPipeline{nullptr};
    GstClock *m_sharedClock{nullptr};
    GstElement *m_pitch{nullptr};
    GstElement *m_eq{nullptr};
    GstElement *m_panorama{nullptr};
    GstElement *m_audioSink{nullptr};
    GstElement *m_volumeElement{nullptr};
    GstElement *m_audioBin{nullptr};

    std::atomic<bool> m_hasVideo{false};
    bool m_videoEnabled{true};
    bool m_fade{false};

    // Connected display widgets
    QList<VideoDisplay*> m_videoSinks;

    QString m_filename;
    QString m_cdgFilename;
    QStringList m_outputDeviceNames;
    std::vector<AudioOutputDevice> m_audioOutputDevices;

    QTimer m_eventTimer;
    QTimer m_cdgTempoTimer;
    QPointer<AudioFader> m_fader{nullptr};

    int m_volume{100};
    bool m_muted{false};
    int m_pitchShift{0};
    int m_tempoPercent{100};
    int m_mplxMode{Multiplex_Normal};
    bool m_eqBypass{true};
    std::array<int,10> m_eqLevels{0,0,0,0,0,0,0,0,0,0};

    std::unique_ptr<QTemporaryDir> m_tempDir;
    State m_state{StoppedState};

    bool m_silenceDetect{false};
    bool m_silenceDetectedEmitted{false};
    bool m_isMonoInitialized{false};
    qint64 m_cdgLastDrawPosMs{0};
    qint64 m_lastDuration{-1};

    void setupPipeline();
    bool setupCdgPipeline(const QString &cdgPath);
    bool seekCdg(qint64 positionMs);
    void destroyCdgPipeline();
    void updateVolume();
    void updateAudioFilters();
    qint64 getCdgLastDrawPositionMs(const QString &cdgPath);
    QElapsedTimer m_cdgSyncTimer;
    qint64 m_lastCdgSyncCheckMs{-1};
    qint64 m_lastCdgCorrectionMs{-1};

    static gboolean busCall(GstBus *bus, GstMessage *msg, gpointer data);
    void handleBusMessage(GstMessage *msg);

    std::vector<QMetaObject::Connection> m_videoConnections;

    static GstFlowReturn onNewVideoSample(
        GstAppSink *sink,
        gpointer userData
    );

    // Coordinated seek handling code
    enum class SeekPhase {
        Idle,
        WaitingForPause,
        WaitingForSeek
    };

    SeekPhase m_seekPhase{SeekPhase::Idle};

    qint64 m_seekTargetMs{0};
    bool m_resumeAfterSeek{false};

    QElapsedTimer m_seekTimer;

    void processCoordinatedSeek();
    void finishCoordinatedSeek(bool success);

private slots:
    void eventTimer_timeout();

public slots:
    void setVideoOffset(int offsetMs) {}
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
    void setUseFader(const bool &fade) { m_fade = fade; }
    void setUseSilenceDetection(const bool &enabled) { m_silenceDetect = enabled; }
    void setDownmix(const bool &enabled);
    void setTempo(const int &percent);
    void setMplxMode(const int &mode);
    void setEqBypass(const bool &m_bypass);
    void setEqLevel(const int &band, const int &level);
    void fadeInImmediate() { fadeIn(false); }
    void fadeOutImmediate() { fadeOut(false); }
    void setEnforceAspectRatio(const bool &enforce) {}

signals:
    void frameReady(const QImage &image);
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
