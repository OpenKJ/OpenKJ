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
    this->volumeElement = volumeElement;
}

void AudioFader::setPaused(bool paused)
{
    this->paused = paused;
}

double AudioFader::volume()
{
    static double lastCubic = 0.0;
    if (paused)
        return lastCubic;
    gdouble volume;
    g_object_get(G_OBJECT(volumeElement), "volume", &volume, NULL);
//    qWarning() << "Linear volume: " << volume;
    double cubicVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
//    qWarning() << "Cubic volume: " << QString::number(cubicVolume);
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
    timer->setInterval(200);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

void AudioFader::fadeOut(bool block)
{
    qWarning() << "fadeOut( " << block << " ) called";
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
    qWarning() << "fadeOut returned";
}

void AudioFader::fadeIn(bool block)
{
    fading = true;
    targetVol = preFadeVol;
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
}

void AudioFader::timerTimeout()
{
    if (paused)
        return;
    qWarning() << "Timer fader - Current: " << volume() << " Target: " << targetVol;
    double increment = .05;
    if (fading)
    {
        if (volume() == targetVol)
        {
            qWarning() << "target volume reached";
            timer->stop();
            fading = false;
            return;
        }

        if (volume() > targetVol)
        {
            if ((volume() - increment) < targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                fading = false;
                return;
            }
            else
                setVolume(volume() - increment);
        }
        if (volume() < targetVol)
        {
            if ((volume() + increment) > targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                fading = false;
                return;
            }
            else
                setVolume(volume() + increment);
        }
    }
}
