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
    explicit FaderGStreamer(QObject *parent = 0);
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
    GstElement *pitch;
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
    QTimer *silenceDetectTimer;
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
    void silenceDetectTimer_timeout();
    void faderChangedVolume(int volume);

    // KhAbstractAudioBackend interface
public:
    bool canPitchShift();
    int pitchShift();

public slots:
    void setPitchShift(int pitchShift);

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
