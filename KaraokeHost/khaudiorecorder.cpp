/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
    if (audioRecorder->state() == QMediaRecorder::RecordingState)
    {
        audioRecorder->pause();
    }
}

void KhAudioRecorder::unpause()
{
    qDebug() << "KhAudioRecorder::unpause() called";
    if (audioRecorder->state() == QMediaRecorder::PausedState)
    {
        audioRecorder->record();
    }
}

void KhAudioRecorder::audioRecorderError(QMediaRecorder::Error error)
{
    Q_UNUSED(error);
    qDebug() << "QAudioRecorder error: " << audioRecorder->errorString();
}
