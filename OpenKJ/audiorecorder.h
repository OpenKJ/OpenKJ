#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QStringList>
#include <QTimer>
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <gst/gst.h>
#include <gst/gstdevicemonitor.h>
#include <gst/gstdevice.h>
#include <gst/gstplugin.h>
#include "settings.h"

class AudioRecorder : public QObject
{
    Q_OBJECT
private:
    Settings settings;
    GstElement *pipeline;
    GstElement *audioConvert;
    GstElement *fileSink;
    GstElement *audioSrc;
    GstElement *oggMux;
    GstElement *vorbisEnc;
    GstElement *lameMp3Enc;
    GstElement *wavEnc;
    GstElement *audioRate;
    GstElement *autoAudioSrc{nullptr};
    GstBus *bus;
    QList<GstDevice*> inputDevices;
    QStringList inputDeviceNames;
    QStringList codecs;
    QStringList fileExtensions;
    void generateDeviceList();
    void initGStreamer();
    int currentCodec;
    int currentDevice;
    QString currentFileExt;
    QString startDateTime;
    void processGstMessage();
    QTimer *timer;

    void getRecordingSettings();



public:
    explicit AudioRecorder(QObject *parent = 0);
    ~AudioRecorder();
    void refreshDeviceList();
    QStringList getDeviceList();
    QStringList getCodecs();
    QString getCodecFileExtension(int CodecId);
    void setOutputFile(QString filename);
    void setInputDevice(int inputDeviceId);
    void record(QString filename);
    void stop();
    void pause();
    void unpause();
    int getCurrentCodec() const;
    void setCurrentCodec(int value);

signals:

public slots:

private slots:
    void timerFired();
};

#endif // AUDIORECORDER_H
