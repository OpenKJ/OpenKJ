/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#ifndef KHAUDIORECORDER_H
#define KHAUDIORECORDER_H

#include <QObject>
#include <QAudioRecorder>

class KhAudioRecorder : public QObject
{
    Q_OBJECT

private:
    QAudioRecorder *audioRecorder;
    QString outputFile;
    QString startDateTime;

public:
    explicit KhAudioRecorder(QObject *parent = 0);
    void record(QString filename);
    void stop();
    void pause();
    void unpause();

private slots:
    void audioRecorderError(QMediaRecorder::Error error);

};

#endif // KHAUDIORECORDER_H
