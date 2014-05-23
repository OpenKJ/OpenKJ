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
    GstElement *pbin;
    GstElement *decodebin;
    GstElement *audioconvert;
    GstElement *autoaudiosink;
    GstElement *audioresample;
    GstElement *rgvolume;
    GstElement *filesrc;
    GstElement *pitch;
    GstElement *pipeline;
    GstBus *bus;
    QString m_filename;
    QTimer *signalTimer;


    // KhAbstractAudioBackend interface
public:
    int volume();
    qint64 position();
    bool isMuted();
    qint64 duration();
    QMediaPlayer::State state();
    QString backendName();
    bool stopping();

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


};

#endif // KHAUDIOBACKENDGSTREAMER_H
