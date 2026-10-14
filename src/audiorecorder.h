#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QMediaCaptureSession>
#include <QAudioInput>
#include <QMediaRecorder>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QUrl>
#include "settings.h"
#include <spdlog/logger.h>

class AudioRecorder : public QObject
{
    Q_OBJECT
private:
    std::shared_ptr<spdlog::logger> logger;
    std::string m_loggingPrefix{"[AudioRecorder]"};
    Settings m_settings;
    QStringList m_inputDeviceNames;
    QStringList m_codecs{"MPEG 2 Layer 3 (mp3)", "OGG Vorbis", "WAV/PCM"};
    QStringList m_fileExtensions{".mp3", ".ogg", ".wav"};
    QString m_currentFileExt{".ogg"};
    QString m_startDateTime;
    int m_currentDevice{0};

    // Qt6 Multimedia members
    QMediaCaptureSession *m_captureSession{nullptr};
    QAudioInput *m_audioInput{nullptr};
    QMediaRecorder *m_recorder{nullptr};
    QList<QAudioDevice> m_audioDevices;

    void ensureRecorder();
    void generateDeviceList();
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
