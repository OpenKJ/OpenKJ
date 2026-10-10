#include "audiorecorder.h"
#include <QDir>
#include <QDateTime>
#include <QMediaFormat>
#include <spdlog/spdlog.h>
#include <QCoreApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent) {
    logger = spdlog::get("logger");
    if (!logger) {
        logger = spdlog::default_logger();
    }
    logger->info("{} Initializing AudioRecorder instance using Qt6 Multimedia", m_loggingPrefix);

    m_startDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm");

    m_audioInput = new QAudioInput(this);
    m_recorder = new QMediaRecorder(this);
    connect(
        m_recorder,
        &QMediaRecorder::errorOccurred,
        this,
        [this](QMediaRecorder::Error error,
           const QString &errorString) {

            logger->error(
                "{} Recording error {}: {}",
                m_loggingPrefix,
                static_cast<int>(error),
                errorString.toStdString()
            );
        }
    );

    m_captureSession.setAudioInput(m_audioInput);
    m_captureSession.setRecorder(m_recorder);

    // Set default quality
    m_recorder->setQuality(QMediaRecorder::HighQuality);

    generateDeviceList();
    getRecordingSettings();
}

AudioRecorder::~AudioRecorder() {
    logger->debug("{} AudioRecorder destructor called", m_loggingPrefix);
}

void AudioRecorder::generateDeviceList() {
    m_inputDeviceNames.clear();
    m_audioDevices = QMediaDevices::audioInputs();
    
    m_inputDeviceNames.append("System default");
    for (const auto &device : m_audioDevices) {
        m_inputDeviceNames.append(device.description());
    }
}

void AudioRecorder::getRecordingSettings() {
    generateDeviceList();
    QString captureDevice = m_settings.recordingInput();
    m_currentDevice = m_inputDeviceNames.indexOf(captureDevice);
    if (m_currentDevice == -1) {
        m_currentDevice = 0;
    }
    setInputDevice(m_currentDevice);
    
    QString codec = m_settings.recordingCodec();
    int codecIdx = m_codecs.indexOf(codec);
    if (codecIdx == -1) {
        codecIdx = 1; // Default to OGG Vorbis
    }
    setCurrentCodec(codecIdx);
}

QStringList AudioRecorder::getDeviceList() {
    return m_inputDeviceNames;
}

QStringList AudioRecorder::getCodecs() {
    return m_codecs;
}

void AudioRecorder::setOutputFile(const QString &filename) {
    if (m_recorder) {
        QString outputDir = m_settings.recordingOutputDir() + QDir::separator() + "Karaoke Recordings" + QDir::separator() + "Show Beginning " + m_startDateTime;
        QDir dir;
        dir.mkpath(outputDir);
        QString outputFilePath = outputDir + QDir::separator() + filename + m_currentFileExt;
        logger->info("{} Capturing to: {}", m_loggingPrefix, outputFilePath.toStdString());
        m_recorder->setOutputLocation(QUrl::fromLocalFile(outputFilePath));
    }
}

void AudioRecorder::setInputDevice(const int inputDeviceId) {
    if (!m_audioInput) return;
    
    if (inputDeviceId == 0) {
        logger->info("{} Setting input device to system default", m_loggingPrefix);
        m_audioInput->setDevice(QMediaDevices::defaultAudioInput());
    } else if (inputDeviceId > 0 && inputDeviceId <= m_audioDevices.size()) {
        const auto &device = m_audioDevices.at(inputDeviceId - 1);
        logger->info("{} Setting input device to: {}", m_loggingPrefix, device.description().toStdString());
        m_audioInput->setDevice(device);
    }
}

void AudioRecorder::setCurrentCodec(const int value) {
    if (!m_recorder) return;

    QMediaFormat format;
    if (value == 0) {
        format.setFileFormat(QMediaFormat::MP3);
        format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
        m_currentFileExt = ".mp3";
    } else if (value == 1) {
        format.setFileFormat(QMediaFormat::Ogg);
        format.setAudioCodec(QMediaFormat::AudioCodec::Vorbis);
        m_currentFileExt = ".ogg";
    } else {
        format.setFileFormat(QMediaFormat::Wave);
        format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
        m_currentFileExt = ".wav";
    }
    m_recorder->setMediaFormat(format);
    logger->info("{} Setting recording format to extension: {}", m_loggingPrefix, m_currentFileExt.toStdString());
}

void AudioRecorder::record(const QString &filename)
{
    if (!m_recorder)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QMicrophonePermission micPermission;

    switch (qApp->checkPermission(micPermission)) {

    case Qt::PermissionStatus::Undetermined:
        logger->info(
            "{} Requesting microphone permission",
            m_loggingPrefix
        );

        qApp->requestPermission(
            micPermission,
            this,
            [this, filename](const QPermission &permission) {
                if (permission.status() == Qt::PermissionStatus::Granted) {
                    logger->info(
                        "{} Microphone permission granted",
                        m_loggingPrefix
                    );

                    // Retry after permission has been granted.
                    record(filename);
                } else {
                    logger->warn(
                        "{} Microphone permission denied",
                        m_loggingPrefix
                    );
                }
            }
        );
        return;

    case Qt::PermissionStatus::Denied:
        logger->warn(
            "{} Cannot record: microphone access denied",
            m_loggingPrefix
        );
        return;

    case Qt::PermissionStatus::Granted:
        break;
    }
#endif

    getRecordingSettings();

    logger->info(
        "{} Recording to file: {}",
        m_loggingPrefix,
        filename.toStdString()
    );

    setOutputFile(filename);
    m_recorder->record();
}

void AudioRecorder::stop() {
    if (m_recorder && m_recorder->recorderState() != QMediaRecorder::StoppedState) {
        logger->info("{} Stopping recording", m_loggingPrefix);
        m_recorder->stop();
    }
}

void AudioRecorder::pause() {
    if (m_recorder && m_recorder->recorderState() == QMediaRecorder::RecordingState) {
        logger->info("{} Pausing recording", m_loggingPrefix);
        m_recorder->pause();
    }
}

void AudioRecorder::unpause() {
    if (m_recorder && m_recorder->recorderState() == QMediaRecorder::PausedState) {
        logger->info("{} Resuming recording", m_loggingPrefix);
        m_recorder->record(); // QMediaRecorder::record() resumes from paused state
    }
}
