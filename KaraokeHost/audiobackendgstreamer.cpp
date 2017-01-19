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

#include "audiobackendgstreamer.h"
#include <QApplication>
#include <QDebug>
#include <string.h>
#include <math.h>





AudioBackendGstreamer::AudioBackendGstreamer(QObject *parent) :
    AbstractAudioBackend(parent)
{
    m_volume = 0;
    m_canKeyChange = true;
    m_fade = false;
    m_silenceDetect = false;
    m_outputChannels = 0;
    m_currentRmsLevel = 0.0;
    m_keyChange = 0;

    gst_init(NULL,NULL);
    audioConvert = gst_element_factory_make("audioconvert", "audioConvert");
    autoAudioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    rgVolume = gst_element_factory_make("rgvolume", "rgVolume");
    volumeElement = gst_element_factory_make("volume", "volumeElement");
    level = gst_element_factory_make("level", "level");
    pitchShifter = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
    playBin = gst_element_factory_make("playbin", "playBin");
    filter = gst_element_factory_make("capsfilter", "filter");
    sinkBin = gst_bin_new("sinkBin");
    audioCapsStereo = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, NULL);
    audioCapsMono = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 1, NULL);
    g_object_set(filter, "caps", audioCapsStereo, NULL);
    if (!pitchShifter)
    {
        qCritical() << "gst plugin 'pitch' not found, key changing disabled";
        gst_bin_add_many(GST_BIN (sinkBin), rgVolume, audioConvert, filter, level, volumeElement, autoAudioSink, NULL);
        gst_element_link_many(rgVolume, audioConvert, filter, level, volumeElement, autoAudioSink, NULL);
        m_canKeyChange = false;
    }
    else
    {
        gst_bin_add_many(GST_BIN (sinkBin), rgVolume, filter, audioConvert, pitchShifter, level, volumeElement, autoAudioSink, NULL);
        gst_element_link_many(rgVolume, filter, audioConvert, pitchShifter, level, volumeElement, autoAudioSink, NULL);
        g_object_set(G_OBJECT(pitchShifter), "formant-preserving", true, NULL);
        g_object_set(G_OBJECT(pitchShifter), "crispness", 1, NULL);
        g_object_set(G_OBJECT(pitchShifter), "semitones", 1.0, NULL);
    }
    pad = gst_element_get_static_pad(rgVolume, "sink");
    ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(sinkBin, ghostPad);
    gst_object_unref(pad);
    g_object_set(G_OBJECT(playBin), "audio-sink", sinkBin, NULL);
    g_object_set(G_OBJECT(rgVolume), "album-mode", false, NULL);
    g_object_set(G_OBJECT (level), "message", TRUE, NULL);
    bus = gst_element_get_bus (playBin);
    fader = new FaderGStreamer(volumeElement, this);
    slowTimer = new QTimer(this);
    connect(slowTimer, SIGNAL(timeout()), this, SLOT(slowTimer_timeout()));
    slowTimer->start(1000);
    fastTimer = new QTimer(this);
    connect(fastTimer, SIGNAL(timeout()), this, SLOT(fastTimer_timeout()));
    connect(fader, SIGNAL(volumeChanged(int)), this, SLOT(faderChangedVolume(int)));
    fastTimer->start(40);
}

AudioBackendGstreamer::~AudioBackendGstreamer()
{
}

void AudioBackendGstreamer::processGstMessages()
{
    GstMessage *message = NULL;
    bool done = false;
    while (!done)
    {
        message = gst_bus_pop(bus);
        if (message != NULL)
        {
            if (message->type == GST_MESSAGE_ELEMENT) {
                const GstStructure *s = gst_message_get_structure (message);
                const gchar *name = gst_structure_get_name (s);
                if (strcmp (name, "level") == 0)
                {
                    gint channels;
                    gdouble rms_dB;
                    gdouble rms;
                    const GValue *value;
                    const GValue *array_val;
                    GValueArray *rms_arr;
                    gint i;
                    array_val = gst_structure_get_value (s, "rms");
                    rms_arr = (GValueArray *) g_value_get_boxed (array_val);
                    channels = rms_arr->n_values;
                    double rmsValues = 0.0;
                    for (i = 0; i < channels; ++i)
                    {
                        value = g_value_array_get_nth (rms_arr, i);
                        rms_dB = g_value_get_double (value);
                        rms = pow (10, rms_dB / 20);
                        rmsValues = rmsValues + rms;
                    }
                    m_currentRmsLevel = rmsValues / channels;
                }
            }
            else if (message->type == GST_MESSAGE_EOS)
            {
                emit stateChanged(EndOfMediaState);
            }
            gst_message_unref(message);
        }
        else
        {
            done = true;
        }
    }
}

int AudioBackendGstreamer::volume()
{
    return m_volume;
}

qint64 AudioBackendGstreamer::position()
{
    gint64 pos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (playBin, fmt, &pos))
    {
        return pos / 1000000;
    }
    return 0;
}

bool AudioBackendGstreamer::isMuted()
{
    return false;
}

qint64 AudioBackendGstreamer::duration()
{
    gint64 duration;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_duration (playBin, fmt, &duration))
    {
        return duration / 1000000;
    }
    return 0;
}

AbstractAudioBackend::State AudioBackendGstreamer::state()
{
    GstState state;
    gst_element_get_state(playBin, &state, NULL, GST_CLOCK_TIME_NONE);
    if (state == GST_STATE_PLAYING)
        return AbstractAudioBackend::PlayingState;
    if (state == GST_STATE_PAUSED)
        return AbstractAudioBackend::PausedState;
    if (state == GST_STATE_NULL)
        return AbstractAudioBackend::StoppedState;
    return AbstractAudioBackend::StoppedState;
}

QString AudioBackendGstreamer::backendName()
{
    return "GStreamer";
}

bool AudioBackendGstreamer::stopping()
{
    return false;
}

void AudioBackendGstreamer::play()
{
    if (state() == AbstractAudioBackend::PausedState)
    {
        gst_element_set_state(playBin, GST_STATE_PLAYING);
        if (m_fade)
            fadeIn();
    }
    else
    {
        gst_element_set_state(playBin, GST_STATE_PLAYING);
    }
}

void AudioBackendGstreamer::pause()
{
    if (m_fade)
        fadeOut();
    gst_element_set_state(playBin, GST_STATE_PAUSED);
}

void AudioBackendGstreamer::setMedia(QString filename)
{
    m_filename = filename;
#ifdef Q_OS_WIN
    std::string uri = "file:///" + filename.toStdString();
#else
    std::string uri = "file://" + filename.toStdString();
#endif
    qDebug() << "AudioBackendHybrid - Playing: " << uri.c_str();
    g_object_set(GST_OBJECT(playBin), "uri", uri.c_str(), NULL);
}

void AudioBackendGstreamer::setMuted(bool muted)
{
    if (muted)
    {
        g_object_set(G_OBJECT(volumeElement), "volume", 0.0, NULL);
        muted = true;
    }
    else
    {
        g_object_set(G_OBJECT(volumeElement), "volume", m_volume * .01, NULL);
        muted = false;
    }

}

void AudioBackendGstreamer::setPosition(qint64 position)
{
    qDebug() << "Seeking to: " << position << "ms";
    GstSeekFlags flags = GST_SEEK_FLAG_FLUSH;
    if (!gst_element_seek_simple(playBin, GST_FORMAT_TIME, flags, GST_MSECOND * position))
    {
      qDebug() << "Seek failed!";
    }
    emit positionChanged(position);
}

void AudioBackendGstreamer::setVolume(int volume)
{
    m_volume = volume;
    fader->setBaseVolume(volume);
    g_object_set(G_OBJECT(volumeElement), "volume", volume * .01, NULL);
    emit volumeChanged(volume);
}

void AudioBackendGstreamer::stop(bool skipFade)
{
    int curVolume = volume();
    if ((m_fade) && (!skipFade) && (state() == AbstractAudioBackend::PlayingState))
        fadeOut();
    gst_element_set_state(playBin, GST_STATE_NULL);
    emit stateChanged(AbstractAudioBackend::StoppedState);
    if ((m_fade) && (!skipFade))
        setVolume(curVolume);
}

void AudioBackendGstreamer::fastTimer_timeout()
{
    static int curDuration;
    if (duration() != curDuration)
    {
        emit durationChanged(duration());
        curDuration = duration();
    }
    static int curPosition;
    if(state() == AbstractAudioBackend::PlayingState)
    {
        if (position() != curPosition)
            emit positionChanged(position());
        curPosition = position();
    }
}

void AudioBackendGstreamer::slowTimer_timeout()
{
    static AbstractAudioBackend::State currentState;
    if (state() != currentState)
    {
        currentState = state();
        emit stateChanged(currentState);
        if (currentState == AbstractAudioBackend::StoppedState)
            stop();
    }
    else if((state() == AbstractAudioBackend::StoppedState) && (pitchShift() != 0))
            setPitchShift(0);

    processGstMessages();

    static int seconds = 0;
    if (m_silenceDetect)
    {
        if ((state() == AbstractAudioBackend::PlayingState) && (isSilent()))
        {
            qDebug() << "Silence detected for " << seconds + 1 << " seconds";
            if (seconds >= 2)
            {
                seconds = 0;
                emit silenceDetected();
                return;
            }
            seconds++;
        }
        else
            seconds = 0;
    }
}

void AudioBackendGstreamer::faderChangedVolume(int volume)
{
    m_volume = volume;
    emit volumeChanged(volume);
}



bool AudioBackendGstreamer::canPitchShift()
{
    return m_canKeyChange;
}

int AudioBackendGstreamer::pitchShift()
{
    return m_keyChange;
}

void AudioBackendGstreamer::setPitchShift(int pitchShift)
{
//    m_keyChange = pitchShift;
//    if ((pi& (m_keyChangerOn))
//            keyChangerOff();
//        else if ((pitchShift != 0) && (!m_keyChangerOn))ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo0
//            keyChangerOn();
//        qDebug() << "executing g_object_set(GST_OBJECT(pitch), \"pitch\", " << getPitchForSemitone(pitchShift) << ", NULL)";tchShift == 0) &
    g_object_set(G_OBJECT(pitchShifter), "semitones", pitchShift, NULL);
    emit pitchChanged(pitchShift);
}

FaderGStreamer::FaderGStreamer(GstElement *GstVolumeElement, QObject *parent) :
    QThread(parent)
{
    volumeElement = GstVolumeElement;
    m_preOutVolume = 0;
    m_targetVolume = 0;
    fading = false;

}

void FaderGStreamer::run()
{
    fading = true;
    while (volume() != m_targetVolume)
    {
        if (volume() > m_targetVolume)
        {
            if (volume() < .01)
                setVolume(0);
            else
                setVolume(volume() - .01);
        }
        if (volume() < m_targetVolume)
            setVolume(volume() + .01);
        QThread::msleep(30);
    }
    fading = false;
}

void FaderGStreamer::fadeIn()
{
    qDebug() << "fadeIn() - Started";
    m_targetVolume = m_preOutVolume;
    if (!fading)
    {
        fading = true;
        start();
    }
    else
    {
        qDebug() << "fadeIn() - A fade operation is already in progress... skipping";
        return;
    }
    while(fading)
        QApplication::processEvents();
    qDebug() << "fadeIn() - Finished";
}

void FaderGStreamer::fadeOut()
{
    qDebug() << "fadeOut() - Started";
    m_targetVolume = 0;
    if (!fading)
    {
        fading = true;
        m_preOutVolume = volume();
        start();
    }
    else
    {
        qDebug() << "fadeIn() - A fade operation is already in progress... skipping";
        return;
    }
    while(fading)
        QApplication::processEvents();
    qDebug() << "fadeOut() - Finished";
}

bool FaderGStreamer::isFading()
{
    return fading;
}

void FaderGStreamer::restoreVolume()
{
    setVolume(m_preOutVolume);
}

void FaderGStreamer::setVolumeElement(GstElement *GstVolumeElement)
{
    volumeElement = GstVolumeElement;
    m_preOutVolume = volume();
}

void FaderGStreamer::setBaseVolume(int volume)
{
    if (!fading)
        m_preOutVolume = volume * .01;
}

void FaderGStreamer::setVolume(double targetVolume)
{
    g_object_set(G_OBJECT(volumeElement), "volume", targetVolume, NULL);
    emit volumeChanged(targetVolume * 100);
}

double FaderGStreamer::volume()
{
    gdouble curVolume;
    g_object_get(G_OBJECT(volumeElement), "volume", &curVolume, NULL);
    return curVolume;
}

bool AudioBackendGstreamer::canFade()
{
    return true;
}

void AudioBackendGstreamer::fadeOut()
{
    fader->fadeOut();
}

void AudioBackendGstreamer::fadeIn()
{
    fader->fadeIn();
}

void AudioBackendGstreamer::setUseFader(bool fade)
{
    m_fade = fade;
}


bool AudioBackendGstreamer::canDetectSilence()
{
    return true;
}

bool AudioBackendGstreamer::isSilent()
{
    if (m_currentRmsLevel <= 0.01)
        return true;
    return false;
}

void AudioBackendGstreamer::setUseSilenceDetection(bool enabled)
{
    m_silenceDetect = enabled;
}


bool AudioBackendGstreamer::canDownmix()
{
    return true;
}

void AudioBackendGstreamer::setDownmix(bool enabled)
{
    qDebug() << "AudioBackendHybrid::setDownmix(" << enabled << ") called";
    if (enabled)
        g_object_set(filter, "caps", audioCapsMono, NULL);
    else
        g_object_set(filter, "caps", audioCapsStereo, NULL);
}
