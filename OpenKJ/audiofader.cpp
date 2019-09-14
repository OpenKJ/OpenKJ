#include "audiofader.h"
#include <gst/audio/streamvolume.h>
#include <QApplication>
#include <QDebug>


void AudioFader::setVolume(double volume)
{
    while (paused)
        QApplication::processEvents();
    double linearVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, volume);
    g_object_set(G_OBJECT(volumeElement), "volume", linearVolume, NULL);
//    g_object_set(G_OBJECT(volumeElement), "volume", volume, NULL);
    emit volumeChanged(volume);
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

void AudioFader::setPaused(bool paused)
{
    qInfo() << objName << " - set paused state to: " << paused;
    this->paused = paused;
}

void AudioFader::setObjName(QString name)
{
    objName = name;
}

double AudioFader::volume()
{
    static double lastCubic = 0.0;
    if (paused)
        return lastCubic;
    gdouble volume;
    g_object_get(G_OBJECT(volumeElement), "volume", &volume, NULL);
//    qInfo() << "Linear volume: " << volume;
    double cubicVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
//    qInfo() << "Cubic volume: " << QString::number(cubicVolume);
    lastCubic = cubicVolume;
    return cubicVolume;
}

AudioFader::AudioFader(QObject *parent) : QObject(parent)
{
    paused = true;
    fading = false;
    preFadeVol = 0;
    targetVol = 0;
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

void AudioFader::fadeOut(bool block)
{
    qInfo() << objName << " - fadeOut( " << block << " ) called";
    emit fadeStarted();
    preFadeVol = volume();
    fading = true;
    targetVol = 0;
    timer->start();
    if (block)
    {
        while (paused)
            QApplication::processEvents();

        while (volume() != targetVol)
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
    fading = true;
    targetVol = 1.0;
    timer->start();
    if (block)
    {
        while (paused)
            QApplication::processEvents();

        while (volume() != targetVol)
        {
            QApplication::processEvents();
        }
    }
    qInfo() << objName << " - fadeIn() returned";
}

void AudioFader::timerTimeout()
{
    if (paused)
        return;
    qInfo() << objName << " - Timer - Current: " << volume() << " Target: " << targetVol;
    double increment = .05;
    if (fading)
    {
        if (volume() == targetVol)
        {
            qInfo() << objName << " - Timer - target volume reached";
            timer->stop();
            fading = false;
            emit fadeComplete();
            return;
        }

        if (volume() > targetVol)
        {
            if ((volume() - increment) < targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                fading = false;
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
                fading = false;
                emit fadeComplete();
                return;
            }
            setVolume(volume() + increment);
        }
    }
}
