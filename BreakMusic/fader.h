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

#ifndef FADER_H
#define FADER_H

#include <QThread>
#include <QMediaPlayer>

class Fader : public QThread
{
    Q_OBJECT
public:
    explicit Fader(QMediaPlayer *mediaPlayer, QObject *parent = 0);
    void run();
    void fadeIn(int targetVolume);
    void fadeIn();
    void fadeOut();
    void fadeStop();
    void fadePause();
    void fadePlay();
    bool isFading();

signals:
    
public slots:
    void setBaseVolume(int volume);
    
private:
    int m_targetVolume;
    int m_preOutVolume;
    QMediaPlayer *mPlayer;
};

#endif // FADER_H
