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

#include "khaudiobackendfmod.h"

KhAudioBackendFMOD::KhAudioBackendFMOD(QObject *parent) :
    KhAbstractAudioBackend(parent)
{
    fmresult = System_Create(&system);
#ifdef Q_OS_LINUX
    fmresult = system->setOutput(FMOD_OUTPUTTYPE_PULSEAUDIO);
#endif
    fmresult = system->setSpeakerMode(FMOD_SPEAKERMODE_MONO);
    fmresult = system->init(2, FMOD_INIT_NORMAL, NULL);
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    m_pitchShift = 0;
    m_pitchShifterEnabled = false;
    m_soundOpened = false;
    signalTimer = new QTimer(this);
    connect(signalTimer, SIGNAL(timeout()), this, SLOT(signalTimer_timeout()));
    signalTimer->start(40);
    setVolume(25);
}

double KhAudioBackendFMOD::GetKeyFloat(int keychange)
{
    int i;
    double keyfloat = 1.0;
    if (keychange > 0) {
        for (i = 0; i < keychange; i++) {
            keyfloat = keyfloat * 1.05946;
        }
    }
    if (keychange < 0) {
        for (i = 0; i > keychange; i--) {
            keyfloat = keyfloat * 0.9438;
        }
    }
    return keyfloat;
}

void KhAudioBackendFMOD::pitchShifter(bool enable)
{
    if ((enable) && (!m_pitchShifterEnabled))
    {
        system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &this->dsp);
        system->addDSP(this->dsp, NULL);
        dsp->setParameter(1, 4096);
        dsp->setParameter(2, 8);
        dsp->setParameter(0, GetKeyFloat(0));
        m_pitchShifterEnabled = true;
    }
    else if ((!enable) && (m_pitchShifterEnabled))
    {
        dsp->release();
        m_pitchShifterEnabled = false;
    }
}


int KhAudioBackendFMOD::volume()
{
    return m_volume;
}

qint64 KhAudioBackendFMOD::position()
{
    unsigned int pos = 0;
    channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
    return pos;
}

bool KhAudioBackendFMOD::isMuted()
{
    bool muted;
    channel->getMute(&muted);
    return muted;
}

qint64 KhAudioBackendFMOD::duration()
{
    unsigned int duration;
    sound->getLength(&duration,1);
    return duration;
}

QMediaPlayer::State KhAudioBackendFMOD::state()
{
    bool paused;
    bool playing;
    channel->getPaused(&paused);
    channel->isPlaying(&playing);
    if (paused)
        return QMediaPlayer::PausedState;
    else if (playing)
        return QMediaPlayer::PlayingState;
    else
        return QMediaPlayer::StoppedState;
}

bool KhAudioBackendFMOD::canPitchShift()
{
    return true;
}

int KhAudioBackendFMOD::pitchShift()
{
    return m_pitchShift;
}

void KhAudioBackendFMOD::play()
{
    if (state() == QMediaPlayer::PausedState)
    {
        channel->setPaused(false);

    }
    else
    {
        system->playSound(FMOD_CHANNEL_FREE, this->sound, 0, &this->channel);
        setVolume(m_volume);
    }
    emit stateChanged(QMediaPlayer::PlayingState);
}

void KhAudioBackendFMOD::pause()
{
    channel->setPaused(true);
    emit stateChanged(QMediaPlayer::PausedState);
}

void KhAudioBackendFMOD::setMedia(QString filename)
{
    if (m_soundOpened)
        stop();
    system->createStream(filename.toUtf8(), FMOD_SOFTWARE | FMOD_DEFAULT | FMOD_2D, 0, &sound);
    m_soundOpened = true;
    emit durationChanged(duration());
}

void KhAudioBackendFMOD::setMuted(bool muted)
{
    channel->setMute(muted);
    emit mutedChanged(muted);
}

void KhAudioBackendFMOD::setPosition(qint64 position)
{
    channel->setPosition(position, FMOD_TIMEUNIT_MS);
    emit positionChanged(position);
}

void KhAudioBackendFMOD::setVolume(int volume)
{
    m_volume = volume;
    channel->setVolume(volume * .01);
    emit volumeChanged(volume);
}

void KhAudioBackendFMOD::stop()
{
    if (m_soundOpened)
    {
        channel->stop();
        sound->release();
        emit stateChanged(QMediaPlayer::StoppedState);
        emit durationChanged(0);
        emit positionChanged(0);
        m_soundOpened = false;
    }
}

void KhAudioBackendFMOD::setPitchShift(int semitones)
{
    if (semitones == 0)
    {
        pitchShifter(false);
    }
    else
    {
        pitchShifter(true);
        dsp->setParameter(0,GetKeyFloat(semitones));
    }
    m_pitchShift = semitones;
}

void KhAudioBackendFMOD::signalTimer_timeout()
{
    if(state() == QMediaPlayer::PlayingState)
        emit positionChanged(position());
    if (state() == QMediaPlayer::StoppedState)
        stop();
}
