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
}

void Fader::run()
{
    while (mPlayer->volume() != m_targetVolume)
    {
        if (mPlayer->volume() > m_targetVolume)
            mPlayer->setVolume(mPlayer->volume() - 1);
        if (mPlayer->volume() < m_targetVolume)
            mPlayer->setVolume(mPlayer->volume() + 1);
        QThread::msleep(30);
    }
}

void Fader::fadeIn(int targetVolume)
{
    while(isRunning())
        QApplication::processEvents();
    m_targetVolume = targetVolume;
    start();
    while(isRunning())
        QApplication::processEvents();
}

void Fader::fadeIn()
{
    while(isRunning())
        QApplication::processEvents();
    m_targetVolume = m_preOutVolume;
    start();
    while(isRunning())
        QApplication::processEvents();
}

void Fader::fadeOut()
{
    while(isRunning())
        QApplication::processEvents();
    m_preOutVolume = mPlayer->volume();
    m_targetVolume = 0;
    start();
    while(isRunning())
        QApplication::processEvents();
}

void Fader::fadeStop()
{
    while(isRunning())
        QApplication::processEvents();
    int curVolume = mPlayer->volume();
    m_preOutVolume = curVolume;
    m_targetVolume = 0;
    start();
    while(isRunning())
        QApplication::processEvents();
    mPlayer->stop();
    mPlayer->setVolume(curVolume);

}

void Fader::fadePause()
{
    while(isRunning())
        QApplication::processEvents();
    m_preOutVolume = mPlayer->volume();
    m_targetVolume = 0;
    start();
    while(isRunning())
        QApplication::processEvents();
    mPlayer->pause();
}

void Fader::fadePlay()
{
    while(isRunning())
        QApplication::processEvents();
    mPlayer->play();
    m_targetVolume = m_preOutVolume;
    start();
    while(isRunning())
        QApplication::processEvents();
}

bool Fader::isFading()
{
    return isRunning();
}

void Fader::setBaseVolume(int volume)
{
    m_preOutVolume = volume;
}
