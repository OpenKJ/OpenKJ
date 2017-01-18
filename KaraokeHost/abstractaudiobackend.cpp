/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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

#include "abstractaudiobackend.h"
#include <math.h>

AbstractAudioBackend::AbstractAudioBackend(QObject *parent) :
    QObject(parent)
{
}

QString AbstractAudioBackend::msToMMSS(qint64 msec)
{
    QString sec;
    QString min;
    int seconds = (int) (msec / 1000) % 60 ;
    int minutes = (int) ((msec / (1000*60)) % 60);

    if (seconds < 10)
        sec = "0" + QString::number(seconds);
    else
    {
        sec = QString::number(seconds);
    }
    min = QString::number(minutes);
    return QString(min + ":" + sec);
}

QStringList AbstractAudioBackend::getOutputDevices()
{
    QStringList devices;
    devices << "System Default Output";
    return devices;
}

float AbstractAudioBackend::getPitchForSemitone(int semitone)
{
    double pitch;
    if (semitone > 0)
    {
        // shifting up
        pitch = pow(STUP,semitone);
    }
    else if (semitone < 0){
        // shifting down
        pitch = 1 - ((100 - (pow(STDN,abs(semitone)) * 100)) / 100);
    }
    else
    {
        // no change
        pitch = 1.0;
    }
    return pitch;
}
