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

#include "fader.h"
#include <QApplication>

Fader::Fader(QMediaPlayer *mediaPlayer, QObject *parent) :
    QThread(parent)
{
    mPlayer = mediaPlayer;
    m_targetVolume = 0;
    m_preOutVolume = mediaPlayer->volume();
    fading = false;
    stopAfter = false;
    pauseAfter = false;
}

void Fader::run()
{

    fading = true;
    while (mPlayer->volume() != m_targetVolume)
    {
        if (mPlayer->volume() > m_targetVolume)
            mPlayer->setVolume(mPlayer->volume() - 1);
        if (mPlayer->volume() < m_targetVolume)
            mPlayer->setVolume(mPlayer->volume() + 1);
        QThread::msleep(30);
    }
    if (stopAfter)
    {
        mPlayer->stop();
        stopAfter = false;
    }
    if (pauseAfter)
    {
        mPlayer->pause();
        pauseAfter = false;
    }
    fading = false;
}

void Fader::fadeIn(int targetVolume)
{
    while(fading)
        QThread::msleep(20);
    m_targetVolume = targetVolume;
    start();
}

void Fader::fadeIn()
{
    while(fading)
        QThread::msleep(20);
    m_targetVolume = m_preOutVolume;
    start();
}

void Fader::fadeOut()
{
    while(fading)
        QThread::msleep(20);
    m_preOutVolume = mPlayer->volume();
    m_targetVolume = 0;
    start();
}

void Fader::fadeStop()
{
    stopAfter = true;
    while(fading)
        QThread::msleep(20);
    fading = true;
    m_preOutVolume = mPlayer->volume();
    m_targetVolume = 0;
    start();

}

void Fader::fadePause()
{
    pauseAfter = true;
    while(fading)
        QThread::msleep(20);
    m_preOutVolume = mPlayer->volume();
    m_targetVolume = 0;
    start();
}

void Fader::fadePlay()
{
    stopAfter = false;
    pauseAfter = false;
    while(fading)
        QApplication::processEvents();
    mPlayer->play();
//        QThread::msleep(20);
    m_targetVolume = m_preOutVolume;
    start();
}

bool Fader::isFading()
{
    return fading;
}

void Fader::setBaseVolume(int volume)
{
    m_preOutVolume = volume;
}
