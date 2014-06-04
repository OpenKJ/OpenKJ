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
#include <QApplication>


KhAudioBackendQMediaPlayer::KhAudioBackendQMediaPlayer(QObject *parent) :
    KhAbstractAudioBackend(parent)
{
    mplayer = new QMediaPlayer(this);
    mplayer->setVolume(25);
    mplayer->setNotifyInterval(40);
    m_fade = true;
    fader = new FaderQMediaPlayer(mplayer,this);
    connect(mplayer, SIGNAL(audioAvailableChanged(bool)), this, SIGNAL(audioAvailableChanged(bool)));
    connect(mplayer, SIGNAL(bufferStatusChanged(int)), this, SIGNAL(bufferStatusChanged(int)));
    connect(mplayer, SIGNAL(durationChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
    connect(mplayer, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
    connect(mplayer, SIGNAL(positionChanged(qint64)), this, SIGNAL(positionChanged(qint64)));
    connect(mplayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(qmStateChanged(QMediaPlayer::State)));
    //connect(mplayer, SIGNAL(stateChanged(KhAbstractAudioBackend::State)), this, SIGNAL(stateChanged(KhAbstractAudioBackend::State)));
    connect(mplayer, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
    connect(mplayer, SIGNAL(volumeChanged(int)), fader, SLOT(setBaseVolume(int)));
    m_stopping = false;
}

KhAudioBackendQMediaPlayer::~KhAudioBackendQMediaPlayer()
{
    qDebug() << "KhAudioBackendQMedaiPlayer destructor called";
    mplayer->setMedia(QMediaContent());
    delete mplayer;
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
    if (m_fade)
        fadeIn();
}

void KhAudioBackendQMediaPlayer::pause()
{
    if (m_fade)
        fadeOut();
    mplayer->pause();
}

void KhAudioBackendQMediaPlayer::setMedia(QString filename)
{
    qDebug() << "AudioBackend - Playing file: " << filename;
    mplayer->setMedia(QUrl::fromLocalFile(filename));
    if (m_fade)
        fadeIn();
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

void KhAudioBackendQMediaPlayer::stop(bool skipFade)
{
    qDebug() << "KhAudioBackendQMediaPlayer::stop()";
    qDebug() << "Media state: " << mplayer->state();
    if (m_stopping)
        return;
    m_stopping = true;
    if (mplayer->state() == QMediaPlayer::PlayingState)
    {
        if ((m_fade) && (!skipFade))
            fadeOut();
        mplayer->stop();
        if (!skipFade) fader->restoreVolume();
    }
    else if (mplayer->state() == QMediaPlayer::PausedState)
    {
        mplayer->stop();
        if (!skipFade) fader->restoreVolume();
    }
    mplayer->setMedia(QMediaContent());
    m_stopping = false;
}

KhAudioBackendQMediaPlayer::State KhAudioBackendQMediaPlayer::state()
{
    if (mplayer->state() == QMediaPlayer::PlayingState)
        return KhAbstractAudioBackend::PlayingState;
    else if (mplayer->state() == QMediaPlayer::PausedState)
        return KhAbstractAudioBackend::PausedState;
    else
        return KhAbstractAudioBackend::StoppedState;
}


void KhAudioBackendQMediaPlayer::fadeOut()
{
    fader->fadeOut();
}

void KhAudioBackendQMediaPlayer::fadeIn()
{
    fader->fadeIn();
}

FaderQMediaPlayer::FaderQMediaPlayer(QMediaPlayer *mediaPlayer, QObject *parent) :
    QThread(parent)
{
    mPlayer = mediaPlayer;
    m_targetVolume = 0;
    m_preOutVolume = mediaPlayer->volume();
    fading = false;
}

void FaderQMediaPlayer::run()
{

    fading = true;
    qDebug() << "Fading - Current Volume: " << mPlayer->volume() << " Target Volume: " << m_targetVolume;
    while (mPlayer->volume() != m_targetVolume)
    {
        if (mPlayer->volume() > m_targetVolume)
            mPlayer->setVolume(mPlayer->volume() - 1);
        if (mPlayer->volume() < m_targetVolume)
            mPlayer->setVolume(mPlayer->volume() + 1);
        QThread::msleep(30);
    }
    fading = false;
}

void FaderQMediaPlayer::fadeIn()
{
    m_targetVolume = m_preOutVolume;
    if (!fading)
    {
        start();
    }
    while(fading)
        QApplication::processEvents();

}

void FaderQMediaPlayer::fadeOut()
{
    if (mPlayer->state() == QMediaPlayer::PlayingState)
    {
    m_targetVolume = 0;
    if (!fading)
    {
        fading = true;
        m_preOutVolume = mPlayer->volume();
        start();
    }
    while(fading)
        QApplication::processEvents();
    }
}

bool FaderQMediaPlayer::isFading()
{
    return fading;
}

void FaderQMediaPlayer::restoreVolume()
{
    mPlayer->setVolume(m_preOutVolume);
}

void KhAudioBackendQMediaPlayer::qmStateChanged(QMediaPlayer::State qmstate)
{
    if (qmstate == QMediaPlayer::PlayingState)
        emit stateChanged(KhAbstractAudioBackend::PlayingState);
    else if (qmstate == QMediaPlayer::PausedState)
        emit stateChanged(KhAbstractAudioBackend::PausedState);
    else
        emit stateChanged(KhAbstractAudioBackend::StoppedState);
}

void FaderQMediaPlayer::setBaseVolume(int volume)
{
    if (!fading)
        m_preOutVolume = volume;
}
