#include "khaudiorecorder.h"
#include <QAudioEncoderSettings>
#include <QUrl>
#include <QDir>
#include "khsettings.h"
#include <QDebug>
#include <QDateTime>

extern KhSettings *settings;


KhAudioRecorder::KhAudioRecorder(QObject *parent) :
    QObject(parent)
{
    audioRecorder = new QAudioRecorder(this);
    connect(audioRecorder, SIGNAL(error(QMediaRecorder::Error)), this, SLOT(audioRecorderError(QMediaRecorder::Error)));
    outputFile = "";
    startDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
}

void KhAudioRecorder::record(QString filename)
{
    outputFile = filename;
    qDebug() << "KhAudioRecorder::record(" << filename << ") called";
//    qDebug() << "Recording to: " << QUrl::fromLocalFile(settings->recordingOutputDir() + QDir::separator() + filename).toString();
////    audioRecorder->setAudioInput(settings->recordingInput());
//    audioSettings.setCodec("audio/vorbis");
//    audioSettings.setEncodingMode(QMultimedia::ConstantQualityEncoding);
//    audioSettings.setQuality(QMultimedia::HighQuality);
//    //audioSettings.setChannelCount(2);
//    audioRecorder->setEncodingSettings(audioSettings, QVideoEncoderSettings(), "ogg");
//    if (!audioRecorder->setOutputLocation(QUrl::fromLocalFile(settings->recordingOutputDir() + QDir::separator() + filename)))
//        qDebug() << "Recording - Faile to set output location";
//    audioRecorder->record();
    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec(settings->recordingCodec());
    audioSettings.setQuality(QMultimedia::HighQuality);
    audioRecorder->setAudioInput(settings->recordingInput());
    audioRecorder->setEncodingSettings(audioSettings);
    audioRecorder->setContainerFormat(settings->recordingContainer());
    audioRecorder->setOutputLocation(QUrl(filename));
    audioRecorder->setVolume(1.0);
    audioRecorder->record();
    qDebug() << "Output file location: " << audioRecorder->outputLocation().toString();
}

void KhAudioRecorder::stop()
{
    qDebug() << "KhAudioRecorder::stop() called";
    if (audioRecorder->state() == QMediaRecorder::RecordingState)
    {
        audioRecorder->stop();
        QFileInfo fileInfo(audioRecorder->actualLocation().toString());
        QString outputDir = settings->recordingOutputDir() + QDir::separator() + "Karaoke Recordings" + QDir::separator() + "Show Beginning " + startDateTime;

        QString outputFilePath;
        if (settings->recordingContainer() != "raw")
            outputFilePath = outputDir + QDir::separator() + fileInfo.fileName() + "." + audioRecorder->containerFormat();
        else
            outputFilePath = outputDir + QDir::separator() + fileInfo.fileName() + "." + settings->recordingRawExtension();
        QDir dir;
        dir.mkpath(outputDir);
        QFile::rename(fileInfo.absoluteFilePath(), outputFilePath);
    }
}

void KhAudioRecorder::pause()
{
    qDebug() << "KhAudioRecorder::pause() called";
    audioRecorder->pause();
}

void KhAudioRecorder::unpause()
{
    qDebug() << "KhAudioRecorder::unpause() called";
    audioRecorder->record();
}

void KhAudioRecorder::audioRecorderError(QMediaRecorder::Error error)
{
    qDebug() << "QAudioRecorder error: " << audioRecorder->errorString();
}
