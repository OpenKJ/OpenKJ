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

// This is pretty much a 1:1 wrapper for the QMediaPlayer functionality we use or might use
// just to have it abstracted out so that the same abstract object type can be used in the
// main program for other audio backends.  This is the default backend on all platforms.

#ifndef KHAUDIOBACKENDQMEDIAPLAYER_H
#define KHAUDIOBACKENDQMEDIAPLAYER_H

#include <QObject>
#include "khabstractaudiobackend.h"

class KhAudioBackendQMediaPlayer : public KhAbstractAudioBackend
{
    Q_OBJECT
private:
    QMediaPlayer *mplayer;
public:
    explicit KhAudioBackendQMediaPlayer(QObject *parent = 0);
    QString backendName() {return QString("QMediaPlayer");}
signals:

public slots:


    // KhAbstractAudioBackend interface
public:
    int volume();
    qint64 position();
    bool isMuted();
    qint64 duration();
    QMediaPlayer::State state();


public slots:
    void play();
    void pause();
    void setMedia(QString filename);
    void setMuted(bool muted);
    void setPosition(qint64 position);
    void setVolume(int volume);
    void stop();

};

#endif // KHAUDIOBACKENDQMEDIAPLAYER_H
