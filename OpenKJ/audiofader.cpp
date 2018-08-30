#include "audiofader.h"
#include <gst/audio/streamvolume.h>
#include <QApplication>
#include <QDebug>


void AudioFader::setVolume(double volume)
{
    if (stoppingEvents)
        return;
    double linearVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, volume);
    g_object_set(G_OBJECT(volumeElement), "volume", linearVolume, NULL);
//    g_object_set(G_OBJECT(volumeElement), "volume", volume, NULL);
    emit volumeChanged(volume);
}

double AudioFader::volume()
{
    static double lastVolume = 0.0;
    if (stoppingEvents)
        return lastVolume;
    gdouble volume;
    g_object_get(G_OBJECT(volumeElement), "volume", &volume, NULL);
//    qWarning() << "Linear volume: " << volume;
    double cubicVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
//    qWarning() << "Cubic volume: " << QString::number(cubicVolume);
    lastVolume = cubicVolume;
    return cubicVolume;
}

void AudioFader::setPreFadeVol(double preFadeVol)
{
    this->preFadeVol = preFadeVol;
}

double AudioFader::getPreFadeVol()
{
    if (fadingIn)
        return targetVol;
    return preFadeVol;
}

void AudioFader::stopEvents()
{
    stoppingEvents = true;
    timer->stop();
}

AudioFader::AudioFader(GstElement *volElement, QObject *parent) : QObject(parent)
{
    stoppingEvents = false;
    fading = false;
    fadingIn = false;
    fadingOut = false;
    preFadeVol = 0;
    targetVol = 0;
    volumeElement = volElement;
    timer = new QTimer(this);
    timer->setInterval(200);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

void AudioFader::fadeOut(bool block)
{
    if (stoppingEvents)
        return;
    qWarning() << "fadeOut( " << block << " ) called";
    preFadeVol = volume();
    fading = true;
    fadingOut = true;
    targetVol = 0;
    timer->start();
    if (block)
    {
        while (!stoppingEvents && volume() != targetVol)
        {
            QApplication::processEvents();
        }
    }
    qWarning() << "fadeOut returned";
}

void AudioFader::fadeIn(bool block)
{
    if (stoppingEvents)
        return;
    fading = true;
    fadingIn = true;
    targetVol = preFadeVol;
    timer->start();
    if (block)
    {
        while (!stoppingEvents && volume() != targetVol)
        {
            QApplication::processEvents();
        }
    }
}

void AudioFader::fadeInToTargetVolume(double vol, bool block)
{
    if (stoppingEvents)
        return;
    fading = true;
    fadingIn = true;
    targetVol = vol;
    timer->start();
    if (block)
    {
        while (!stoppingEvents && volume() != targetVol)
        {
            QApplication::processEvents();
        }
    }
    preFadeVol = vol;
}

void AudioFader::timerTimeout()
{
    if (stoppingEvents)
        return;
//    qWarning() << "Timer fader - Current: " << volume() << " Target: " << targetVol;
    double increment = .05;
    if (fading)
    {
        if (volume() == targetVol)
        {
            qWarning() << "target volume reached";
            timer->stop();
            fading = false;
            fadingIn = false;
            fadingOut = false;
            return;
        }

        if (volume() > targetVol)
        {
            fadingIn = false;
            if ((volume() - increment) < targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                fading = false;
                fadingIn = false;
                fadingOut = false;
                return;
            }
            else
                setVolume(volume() - increment);
        }
        if (volume() < targetVol)
        {
            fadingOut = false;
            if ((volume() + increment) > targetVol)
            {
                setVolume(targetVol);
                timer->stop();
                fading = false;
                fadingIn = false;
                fadingOut = false;
                return;
            }
            else
                setVolume(volume() + increment);
        }
    }
}
