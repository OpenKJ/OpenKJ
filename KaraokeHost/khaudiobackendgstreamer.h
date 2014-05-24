#ifndef KHAUDIOBACKENDGSTREAMER_H
#define KHAUDIOBACKENDGSTREAMER_H

#include "khabstractaudiobackend.h"
#include <gst/gst.h>
#include <QTimer>


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
    GstPad *pad;
    GstPad *ghostPad;
    GstBus *bus;
    QString m_filename;
    QTimer *signalTimer;
    bool m_keyChangerOn;
    int m_keyChange;
    int m_volume;

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



    // KhAbstractAudioBackend interface
public:
    bool canPitchShift();
    int pitchShift();

public slots:
    void setPitchShift(int pitchShift);
};

#endif // KHAUDIOBACKENDGSTREAMER_H
