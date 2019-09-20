/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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

#ifndef ABSTRACTAUDIOBACKEND_H
#define ABSTRACTAUDIOBACKEND_H

#include <QObject>
#include <QStringList>
#include <QImage>

#define STUP 1.0594630943592952645618252949461
#define STDN 0.94387431268169349664191315666784

#define Multiplex_Normal 0
#define Multiplex_LeftChannel 1
#define Multiplex_RightChannel 2

class AbstractAudioBackend : public QObject
{
    Q_OBJECT
private:
    QString name;
public:
    enum State{PlayingState=0,PausedState,StoppedState,EndOfMediaState,UnknownState};
    explicit AbstractAudioBackend(QObject *parent = nullptr);
    virtual int volume() {return 0;}
    virtual qint64 position() {return 0;}
    virtual bool isMuted() {return false;}
    virtual qint64 duration() {return 0;}
    virtual AbstractAudioBackend::State state() {return AbstractAudioBackend::StoppedState;}
    virtual bool canPitchShift() {return false;}
    virtual bool canChangeTempo() {return false;}
    virtual int pitchShift() {return 0;}
    virtual bool canFade() { return false; }
    QString msToMMSS(qint64 msec);
    virtual QString backendName() {return QString("Abstract - something is horribly wrong");}
    virtual bool canDetectSilence() { return false; }
    virtual bool isSilent() { return false; }
    virtual bool canDownmix() { return false; }
    virtual bool downmixChangeRequiresRestart() { return true; }
    virtual QStringList getOutputDevices();
    virtual void setOutputDevice(int deviceIndex) {Q_UNUSED(deviceIndex);}
    virtual bool stopping() {return false;}
    float getPitchForSemitone(int semitone);
    QString getName() const;
    void setName(const QString &value);
    virtual bool hasVideo() {return false;}

signals:
    void audioAvailableChanged(bool);
    void bufferStatusChanged(int);
    void durationChanged(qint64);
    void mutedChanged(bool);
    void positionChanged(qint64);
    void stateChanged(AbstractAudioBackend::State);
    void videoAvailableChanged(bool);
    void volumeChanged(int);
    void silenceDetected();
    void pitchChanged(int);
    void newVideoFrame(QImage frame, QString backendName);
    void audioError(QString msg);


public slots:
    virtual void play() {}
    virtual void pause() {}
    virtual void setMedia(QString filename) {Q_UNUSED(filename);}
    virtual void setMuted(bool muted) {Q_UNUSED(muted);}
    virtual void setPosition(qint64 position) {Q_UNUSED(position);}
    virtual void setVolume(int volume) {Q_UNUSED(volume);}
    virtual void stop(bool skipFade = false) {Q_UNUSED(skipFade);}
    virtual void rawStop() {}
    virtual void setPitchShift(int pitchShift) {Q_UNUSED(pitchShift);}
    virtual void fadeOut(bool waitForFade = true) {Q_UNUSED(waitForFade);}
    virtual void fadeIn(bool waitForFade = true) {Q_UNUSED(waitForFade);}
    virtual void fadeInImmediate() {}
    virtual void fadeOutImmediate() {}
    virtual void setUseFader(bool fade) {Q_UNUSED(fade);}
    virtual void setUseSilenceDetection(bool enabled) {Q_UNUSED(enabled);}
    virtual void setDownmix(bool enabled) {Q_UNUSED(enabled);}
    virtual void initialize() {}
    virtual void setMplxMode(int mode);
    virtual void setTempo(int percent) {Q_UNUSED(percent);}
    virtual void setEqBypass(bool bypass) {Q_UNUSED(bypass);}
    virtual void setEqLevel1(int level) {Q_UNUSED(level);}
    virtual void setEqLevel2(int level) {Q_UNUSED(level);}
    virtual void setEqLevel3(int level) {Q_UNUSED(level);}
    virtual void setEqLevel4(int level) {Q_UNUSED(level);}
    virtual void setEqLevel5(int level) {Q_UNUSED(level);}
    virtual void setEqLevel6(int level) {Q_UNUSED(level);}
    virtual void setEqLevel7(int level) {Q_UNUSED(level);}
    virtual void setEqLevel8(int level) {Q_UNUSED(level);}
    virtual void setEqLevel9(int level) {Q_UNUSED(level);}
    virtual void setEqLevel10(int level) {Q_UNUSED(level);}


};

typedef QList<AbstractAudioBackend *> KhAudioBackends;


#endif // ABSTRACTAUDIOBACKEND_H
