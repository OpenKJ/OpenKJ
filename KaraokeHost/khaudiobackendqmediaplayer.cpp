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

#include "khaudiobackendqmediaplayer.h"
#include <QDebug>

KhAudioBackendQMediaPlayer::KhAudioBackendQMediaPlayer(QObject *parent) :
    KhAbstractAudioBackend(parent)
{
    mplayer = new QMediaPlayer(this);
    mplayer->setVolume(25);
    mplayer->setNotifyInterval(40);
    connect(mplayer, SIGNAL(audioAvailableChanged(bool)), this, SIGNAL(audioAvailableChanged(bool)));
    connect(mplayer, SIGNAL(bufferStatusChanged(int)), this, SIGNAL(bufferStatusChanged(int)));
//    connect(mplayer, SIGNAL(currentMediaChanged(QMediaContent)), this, SIGNAL(currentMediaChanged(QMediaContent)));
    connect(mplayer, SIGNAL(durationChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
    connect(mplayer, SIGNAL(error(QMediaPlayer::Error)), this, SIGNAL(error(QMediaPlayer::Error)));
//    connect(mplayer, SIGNAL(mediaChanged(QMediaContent)), this, SIGNAL(mediaChanged(QMediaContent)));
    connect(mplayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    connect(mplayer, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
    connect(mplayer, SIGNAL(positionChanged(qint64)), this, SIGNAL(positionChanged(qint64)));
    connect(mplayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SIGNAL(stateChanged(QMediaPlayer::State)));
    connect(mplayer, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
}


int KhAudioBackendQMediaPlayer::volume()
{
    return mplayer->volume();
}

qint64 KhAudioBackendQMediaPlayer::position()
{
    return mplayer->position();
}

bool KhAudioBackendQMediaPlayer::isMuted()
{
    return mplayer->isMuted();
}

qint64 KhAudioBackendQMediaPlayer::duration()
{
    return mplayer->duration();
}

void KhAudioBackendQMediaPlayer::play()
{
    mplayer->play();
}

void KhAudioBackendQMediaPlayer::pause()
{
    mplayer->pause();
}

void KhAudioBackendQMediaPlayer::setMedia(QString filename)
{
    qDebug() << "AudioBackend - Playing file: " << filename;
    mplayer->setMedia(QUrl::fromLocalFile(filename));
}

void KhAudioBackendQMediaPlayer::setMuted(bool muted)
{
    mplayer->setMuted(muted);
}

void KhAudioBackendQMediaPlayer::setPosition(qint64 position)
{
    mplayer->setPosition(position);
}

void KhAudioBackendQMediaPlayer::setVolume(int volume)
{
    mplayer->setVolume(volume);
}

void KhAudioBackendQMediaPlayer::stop()
{
    mplayer->stop();
}


QMediaPlayer::State KhAudioBackendQMediaPlayer::state()
{
    return mplayer->state();
}
