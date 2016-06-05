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

// This is pretty much a 1:1 wrapper for the QMediaPlayer functionality we use or might use
// just to have it abstracted out so that the same abstract object type can be used in the
// main program for other audio backends.

#ifndef KHAUDIOBACKENDQMEDIAPLAYER_H
#define KHAUDIOBACKENDQMEDIAPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QThread>
#include "khabstractaudiobackend.h"

class FaderQMediaPlayer : public QThread
{
    Q_OBJECT

private:
    int m_targetVolume;
    int m_preOutVolume;
    QMediaPlayer *mPlayer;
    bool fading;

public:
    explicit FaderQMediaPlayer(QMediaPlayer *mediaPlayer, QObject *parent = 0);
    void run();
    void fadeIn();
    void fadeOut();
    bool isFading();
    void restoreVolume();

public slots:
    void setBaseVolume(int volume);

};

class KhAudioBackendQMediaPlayer : public KhAbstractAudioBackend
{
    Q_OBJECT
private:
    QMediaPlayer *mplayer;
    bool m_fade;
    bool m_stopping;

public:
    explicit KhAudioBackendQMediaPlayer(QObject *parent = 0);
    ~KhAudioBackendQMediaPlayer();
    QString backendName() {return QString("QMediaPlayer");}
    int volume();
    qint64 position();
    bool isMuted();
    qint64 duration();
    KhAbstractAudioBackend::State state();
    FaderQMediaPlayer *fader;
    bool canFade() { return true; }
    void setUseFader(bool fade) {m_fade = fade;}
    bool stopping() {return m_stopping;}

private slots:
    void qmStateChanged(QMediaPlayer::State qmstate);

public slots:
    void play();
    void pause();
    void setMedia(QString filename);
    void setMuted(bool muted);
    void setPosition(qint64 position);
    void setVolume(int volume);
    void stop(bool skipFade = false);
    void fadeOut();
    void fadeIn();

};

#endif // KHAUDIOBACKENDQMEDIAPLAYER_H
