#include "audiofader.h"
#include <gst/audio/streamvolume.h>
#include <QApplication>
#include <QDebug>


void AudioFader::setVolume(double volume)
{
    double linearVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, volume);
    g_object_set(G_OBJECT(volumeElement), "volume", linearVolume, NULL);
//    g_object_set(G_OBJECT(volumeElement), "volume", volume, NULL);
    emit volumeChanged(volume);
}

void AudioFader::immediateIn()
{
    timer->stop();
    if (volume() == 1.0 && curState == FadedIn)
        return;
    setVolume(1.0);
    curState = FadedIn;
    emit faderStateChanged(curState);
}

void AudioFader::immediateOut()
{
    timer->stop();
    if (volume() == 0.0 && curState == FadedOut)
        return;
    setVolume(0);
    curState = FadedOut;
    emit faderStateChanged(curState);
}

AudioFader::FaderState AudioFader::state()
{
    return curState;
}

void AudioFader::setVolumeElement(GstElement *volumeElement)
{
    qInfo() << "AudioFader::setVolumeElement() called";
    if (!volumeElement)
    {
        qInfo() << "Fader - Volume element invalid";
    }
    else {
        qInfo() << "Fader - Volume element okay";
    }
    this->volumeElement = volumeElement;
}

void AudioFader::setObjName(QString name)
{
    objName = name;
}

bool AudioFader::isFading() {
    if (curState == FadingIn || curState == FadingOut)
        return true;
    return false;
}

double AudioFader::volume()
{
    gdouble volume;
    g_object_get(G_OBJECT(volumeElement), "volume", &volume, NULL);
//    qInfo() << "Linear volume: " << volume;
    double cubicVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
    return cubicVolume;
}

AudioFader::AudioFader(QObject *parent) : QObject(parent)
{
    curState = FadedIn;
    emit faderStateChanged(curState);
    targetVol = 0;
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

QString AudioFader::stateToStr(AudioFader::FaderState state)
{
    switch (state)
    {
    case AudioFader::FadedIn:
        return "AudioFader::FadedIn";
    case AudioFader::FadingIn:
        return "AudioFader::FadingIn";
    case AudioFader::FadedOut:
        return "AudioFader::FadedOut";
    case AudioFader::FadingOut:
        return "AudioFader::FadingOut";
    }
    return "Unknown";
}

void AudioFader::fadeOut(bool block)
{
    qInfo() << objName << " - fadeOut( " << block << " ) called";
    emit fadeStarted();
    targetVol = 0;
    curState = FadingOut;
    emit faderStateChanged(curState);
    timer->start();
    if (block)
    {
        while (volume() != targetVol  && curState == FadingOut)
        {
            QApplication::processEvents();
        }
    }
    qInfo() << objName << " - fadeOut() returned";
}

void AudioFader::fadeIn(bool block)
{
    qInfo() << objName << " - fadeIn( " << block << " ) called";
    emit fadeStarted();
    targetVol = 1.0;
    curState = FadingIn;
    emit faderStateChanged(curState);
    timer->start();
    if (block)
    {
        while (volume() != targetVol && curState == FadingIn)
        {
            QApplication::processEvents();
        }
    }
    qInfo() << objName << " - fadeIn() returned";
}

void AudioFader::timerTimeout()
{
    qInfo() << objName << " - Timer - Current: " << volume() << " Target: " << targetVol;
    double increment = .05;
    if (isFading())
    {
        if (volume() == targetVol)
        {
            qInfo() << objName << " - Timer - target volume reached";
            timer->stop();
            if (curState == FadingOut)
                curState = FadedOut;
            if (curState == FadingIn)
                curState = FadedIn;
            emit faderStateChanged(curState);
            emit fadeComplete();
            return;
        }
        if (volume() > targetVol)
        {
            if ((volume() - increment) < targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                curState = FadedOut;
                emit faderStateChanged(curState);
                emit fadeComplete();
                return;
            }
            setVolume(volume() - increment);
        }
        if (volume() < targetVol)
        {
            if ((volume() + increment) > targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                curState = FadedIn;
                emit faderStateChanged(curState);
                emit fadeComplete();
                return;
            }
            setVolume(volume() + increment);
        }
    }
}
