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
#include <QApplication>
#include <QDebug>

KhAudioBackendFMOD::KhAudioBackendFMOD(bool downmix, QObject *parent) :
    KhAbstractAudioBackend(parent)
{
    fmresult = System_Create(&system);
#ifdef Q_OS_LINUX
    fmresult = system->setOutput(FMOD_OUTPUTTYPE_PULSEAUDIO);
#endif
    if (downmix)
        fmresult = system->setSpeakerMode(FMOD_SPEAKERMODE_MONO);
    fmresult = system->init(2, FMOD_INIT_NORMAL, NULL);
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    m_pitchShift = 0;
    m_pitchShifterEnabled = false;
    m_soundOpened = false;
    signalTimer = new QTimer(this);
    connect(signalTimer, SIGNAL(timeout()), this, SLOT(signalTimer_timeout()));
    signalTimer->start(40);
    silenceDetectTimer = new QTimer(this);
    connect(silenceDetectTimer, SIGNAL(timeout()), this, SLOT(silenceDetectTimer_timeout()));
    silenceDetectTimer->start(1000);
    m_fade = true;
    fader = new FaderFmod(this);
    setVolume(25);
    connect(fader, SIGNAL(volumeChanged(int)), this, SLOT(faderChangedVolume(int)));
}

double KhAudioBackendFMOD::getPitchAdjustment(int semitones)
{
    double pitchAdjustment = 1.0;
    if (semitones > 0) {
        for (int i = 0; i < semitones; i++) {
            pitchAdjustment = pitchAdjustment * 1.05946;
        }
    }
    else {
        for (int i = 0; i > semitones; i--) {
            pitchAdjustment = pitchAdjustment * 0.9438;
        }
    }
    return pitchAdjustment;
}

void KhAudioBackendFMOD::pitchShifter(bool enable)
{
    if ((enable) && (!m_pitchShifterEnabled))
    {
        system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &this->dsp);
        system->addDSP(this->dsp, NULL);
        dsp->setParameter(1, 4096);
        dsp->setParameter(2, 8);
        dsp->setParameter(0, getPitchAdjustment(0));
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
    float volume;
    int intVolume;
    channel->getVolume(&volume);
    intVolume = volume / .01;
    return intVolume;
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

bool KhAudioBackendFMOD::isSilent()
{
    float spectrumarray[64];
    float sumlevel = 0;
    int numvalues = 64;
    channel->getSpectrum(spectrumarray,numvalues, 0, FMOD_DSP_FFT_WINDOW_RECT);
    for (int i = 0; i <= 63; i++) {
        sumlevel = sumlevel + spectrumarray[i];
    }
    if ((sumlevel / 64) < .0001)
        return true;
    else
        return false;
}

void KhAudioBackendFMOD::play()
{
    if (state() == QMediaPlayer::PausedState)
    {
        channel->setPaused(false);
        emit stateChanged(QMediaPlayer::PlayingState);
        if (m_fade)
            fadeIn();
    }
    else
    {
        system->playSound(FMOD_CHANNEL_FREE, this->sound, 0, &this->channel);
        setVolume(m_volume);
        fader->setChannel(channel);
    }
    emit stateChanged(QMediaPlayer::PlayingState);
}

void KhAudioBackendFMOD::pause()
{
    if (m_fade)
        fadeOut();
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
    fader->setBaseVolume(volume);
    channel->setVolume(volume * .01);
    emit volumeChanged(volume);
}

void KhAudioBackendFMOD::stop(bool skipFade)
{
    if (m_soundOpened)
    {
        int curVolume = volume();
        if ((m_fade) && (!skipFade))
            fadeOut();
        channel->stop();
        sound->release();
        emit stateChanged(QMediaPlayer::StoppedState);
        emit durationChanged(0);
        emit positionChanged(0);
        m_soundOpened = false;
        if ((m_fade) && (!skipFade))
            setVolume(curVolume);
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
        dsp->setParameter(0,getPitchAdjustment(semitones));
    }
    m_pitchShift = semitones;
}

void KhAudioBackendFMOD::setDownmix(bool enabled)
{
    qDebug() << "KhAudioBackendFMOD - Downmix to mono " << enabled;
    if (enabled)
        system->setSpeakerMode(FMOD_SPEAKERMODE_MONO);
    else
        system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
}

void KhAudioBackendFMOD::signalTimer_timeout()
{
    if(state() == QMediaPlayer::PlayingState)
    {
        emit positionChanged(position());
    }
    else if (state() == QMediaPlayer::StoppedState)
        stop();
}

void KhAudioBackendFMOD::silenceDetectTimer_timeout()
{
    if (m_silenceDetect)
    {
        static int seconds = 0;
        if ((state() == QMediaPlayer::PlayingState) && (isSilent()))
        {
            if (seconds >= 2)
            {
                seconds = 0;
                emit silenceDetected();
                return;
            }
            seconds++;
        }
    }
}

void KhAudioBackendFMOD::faderChangedVolume(int volume)
{
    m_volume = volume;
    emit volumeChanged(volume);
}


FaderFmod::FaderFmod(QObject *parent) :
    QThread(parent)
{
    m_preOutVolume = 0;
    m_targetVolume = 0;
    fading = false;

}

void FaderFmod::run()
{
    fading = true;
    while (volume() != m_targetVolume)
    {
        if (volume() > m_targetVolume)
        {
            if (volume() < .01)
                setVolume(0);
            else
                setVolume(volume() - .01);
        }
        if (volume() < m_targetVolume)
            setVolume(volume() + .01);
        QThread::msleep(30);
    }
    fading = false;
}

void FaderFmod::fadeIn()
{
    m_targetVolume = m_preOutVolume;
    if (!fading)
    {
        start();
    }
    while(fading)
        QApplication::processEvents();
}

void FaderFmod::fadeOut()
{
    bool playing;
    channel->isPlaying(&playing);
    if (playing)
    {
    m_targetVolume = 0;
    if (!fading)
    {
        fading = true;
        m_preOutVolume = volume();
        start();
    }
    while(fading)
        QApplication::processEvents();
    }
}

bool FaderFmod::isFading()
{
    return fading;
}

void FaderFmod::restoreVolume()
{
    setVolume(m_preOutVolume);
}

void FaderFmod::setChannel(Channel *fmChannel)
{
    channel = fmChannel;
    m_preOutVolume = volume();
}

void FaderFmod::setBaseVolume(int volume)
{
    if (!fading)
        m_preOutVolume = volume;
}

void FaderFmod::setVolume(float volume)
{
    channel->setVolume(volume);
    emit volumeChanged(volume * 100);
}

float FaderFmod::volume()
{
    float volume;
    channel->getVolume(&volume);
    return volume;
}

void KhAudioBackendFMOD::fadeOut()
{
    fader->fadeOut();
}

void KhAudioBackendFMOD::fadeIn()
{
    fader->fadeIn();
}
