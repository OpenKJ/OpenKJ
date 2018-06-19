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

double AudioFader::volume()
{
    gdouble volume;
    g_object_get(G_OBJECT(volumeElement), "volume", &volume, NULL);
//    qWarning() << "Linear volume: " << volume;
    double cubicVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
    qWarning() << "Cubic volume: " << QString::number(cubicVolume);
    return cubicVolume;
}

AudioFader::AudioFader(GstElement *volElement, QObject *parent) : QObject(parent)
{
    fading = false;
    preFadeVol = 0;
    targetVol = 0;
    volumeElement = volElement;
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
        while (volume() != targetVol)
        {
            QApplication::processEvents();
        }
    }
}

void AudioFader::timerTimeout()
{
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
