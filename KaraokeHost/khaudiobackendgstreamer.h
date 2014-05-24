#ifndef KHAUDIOBACKENDGSTREAMER_H
#define KHAUDIOBACKENDGSTREAMER_H

#include "khabstractaudiobackend.h"
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <gst/gst.h>
#include <QTimer>
#include <QThread>



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

class KhAudioBackendGStreamer : public KhAbstractAudioBackend
{
    Q_OBJECT
public:
    explicit KhAudioBackendGStreamer(QObject *parent = 0);
    ~KhAudioBackendGStreamer();
signals:

public slots:

private:
    GstElement *sinkBin;
    GstElement *playBin;
    GstElement *audioconvert;
    GstElement *autoaudiosink;
    GstElement *audioresample;
    GstElement *rgvolume;
    GstElement *pitch;
    GstElement *volumeElement;
    GstElement *level;
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


    // KhAbstractAudioBackend interface
public:
    int volume();
    qint64 position();
    bool isMuted();
    qint64 duration();
    QMediaPlayer::State state();
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
};

#endif // KHAUDIOBACKENDGSTREAMER_H
