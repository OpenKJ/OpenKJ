#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <gst/gst.h>
#include <gst/gstdevicemonitor.h>
#include <gst/gstdevice.h>
#include <gst/gstplugin.h>
#include "settings.h"
#include <spdlog/logger.h>

class AudioRecorder : public QObject
{
    Q_OBJECT
private:
    std::shared_ptr<spdlog::logger> logger;
    std::string m_loggingPrefix{"[AudioRecorder]"};
    Settings m_settings;
    GstElement *m_pipeline{nullptr};
    GstElement *m_audioConvert{nullptr};
    GstElement *m_fileSink{nullptr};
    GstElement *m_audioSrc{nullptr};
    GstElement *m_oggMux{nullptr};
    GstElement *m_vorbisEnc{nullptr};
    GstElement *m_lameMp3Enc{nullptr};
    GstElement *m_wavEnc{nullptr};
    GstElement *m_audioRate{nullptr};
    GstElement *m_autoAudioSrc{nullptr};
    GstBus *m_bus{nullptr};
    QList<GstDevice*> m_inputDevices;
    QStringList m_inputDeviceNames;
    QStringList m_codecs{"MPEG 2 Layer 3 (mp3)", "OGG Vorbis", "WAV/PCM"};
    QStringList m_fileExtensions{".mp3", ".ogg", ".wav"};
    QString m_currentFileExt{".ogg"};
    QString m_startDateTime;
    int m_currentDevice{0};
    QTimer m_timer;

    void generateDeviceList();
    void initGStreamer();
    void processGstMessage();
    void getRecordingSettings();

public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder() override;
    QStringList getDeviceList();
    QStringList getCodecs();
    void setOutputFile(const QString& filename);
    void setInputDevice(int inputDeviceId);
    void record(const QString& filename);
    void stop();
    void pause();
    void unpause();
    void setCurrentCodec(int value);

};

#endif // AUDIORECORDER_H
