#include "khaudiorecorder.h"
#include <QAudioEncoderSettings>
#include <QUrl>
#include <QDir>
#include "khsettings.h"
#include <QDebug>

extern KhSettings *settings;


KhAudioRecorder::KhAudioRecorder(QObject *parent) :
    QObject(parent)
{
    audioRecorder = new QAudioRecorder(this);
    connect(audioRecorder, SIGNAL(error(QMediaRecorder::Error)), this, SLOT(audioRecorderError(QMediaRecorder::Error)));


}

void KhAudioRecorder::record(QString filename)
{
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
    audioSettings.setCodec("audio/vorbis");
    audioSettings.setQuality(QMultimedia::HighQuality);
    audioRecorder->setEncodingSettings(audioSettings);
    audioRecorder->setContainerFormat("ogg");
    audioRecorder->setOutputLocation(QUrl("test.ogg"));
    audioRecorder->record();
    qDebug() << "Output file location: " << audioRecorder->outputLocation().toString();
}

void KhAudioRecorder::stop()
{
    qDebug() << "KhAudioRecorder::stop() called";
    audioRecorder->stop();
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
