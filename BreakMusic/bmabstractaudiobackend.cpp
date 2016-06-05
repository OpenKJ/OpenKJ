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

#include "bmabstractaudiobackend.h"
#include <QStringList>
BmAbstractAudioBackend::BmAbstractAudioBackend(QObject *parent) :
    QObject(parent)
{
}

QString BmAbstractAudioBackend::msToMMSS(qint64 msec)
{
    QString sec;
    QString min;
    int seconds = (int) (msec / 1000) % 60 ;
    int minutes = (int) ((msec / (1000*60)) % 60);
    if (seconds < 10)
        sec = "0" + QString::number(seconds);
    else
        sec = QString::number(seconds);   
    min = QString::number(minutes);
    return QString(min + ":" + sec);
}

QStringList BmAbstractAudioBackend::getOutputDevices()
{
    QStringList devices;
    devices << "System Default Output";
    return devices;
}
