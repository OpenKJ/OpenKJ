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


static void on_new_buffer (GstElement* object,
                           gpointer user_data)
{
  qWarning() << "on_new_buffer called";
  //FILE* file = (FILE*) user_data;
  GstAppSink* app_sink = (GstAppSink*) object;
  GstSample * sample = gst_app_sink_pull_sample(app_sink);
  gst_sample_unref(sample);
  qWarning() << "Received video frame";

}

AudioBackendGstreamer::AudioBackendGstreamer(bool loadPitchShift, QObject *parent) :
    AbstractAudioBackend(parent)
{
    m_volume = 0;
    m_canKeyChange = false;
    m_keyChangerRubberBand = false;
    m_keyChangerSoundtouch = false;
    m_fade = false;
    m_silenceDetect = false;
    m_outputChannels = 0;
    m_currentRmsLevel = 0.0;
    m_keyChange = 0;
    m_silenceDuration = 0;

    GstAppSinkCallbacks appsinkCallbacks;
    appsinkCallbacks.new_preroll	= &AudioBackendGstreamer::NewPrerollCallback;
    appsinkCallbacks.new_sample		= &AudioBackendGstreamer::NewSampleCallback;
    appsinkCallbacks.eos			= &AudioBackendGstreamer::EndOfStreamCallback;

    qCritical() << "Initializing gst\n";
    gst_init(NULL,NULL);
    audioConvert = gst_element_factory_make("audioconvert", "audioConvert");
    audioConvert2 = gst_element_factory_make("audioconvert", "audioConvert2");
    autoAudioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    videoAppSink = gst_element_factory_make("appsink", "videoAppSink");
    rgVolume = gst_element_factory_make("rgvolume", "rgVolume");
//    volumeElement = gst_element_factory_make("volume", "volumeElement");
    level = gst_element_factory_make("level", "level");
    pitchShifterSoundtouch = gst_element_factory_make("pitch", "pitch");
    pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
    playBin = gst_element_factory_make("playbin", "playBin");
    filter = gst_element_factory_make("capsfilter", "filter");
    sinkBin = gst_bin_new("sinkBin");
    audioCapsStereo = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, NULL);
    audioCapsMono = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 1, NULL);
    videoCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRx", NULL);
    g_object_set(filter, "caps", audioCapsStereo, NULL);
    g_object_set(videoAppSink, "caps", videoCaps, NULL);
    if ((pitchShifterRubberBand) && (loadPitchShift))
    {
        // This is our preferred pitch shifter because it sounds better, but it's only available on Linux
        qCritical() << "Pitch shift ladspa plugin \"Rubber Band\" found, enabling key changing";
        gst_bin_add_many(GST_BIN (sinkBin), filter, rgVolume, audioConvert, pitchShifterRubberBand, level, autoAudioSink, NULL);
        gst_element_link_many(filter, rgVolume, audioConvert, pitchShifterRubberBand, level, autoAudioSink, NULL);
        g_object_set(G_OBJECT(pitchShifterRubberBand), "formant-preserving", true, NULL);
        g_object_set(G_OBJECT(pitchShifterRubberBand), "crispness", 1, NULL);
        g_object_set(G_OBJECT(pitchShifterRubberBand), "semitones", 1.0, NULL);
        m_canKeyChange = true;
        m_keyChangerRubberBand = true;
    }
    else if ((pitchShifterSoundtouch) && (loadPitchShift))
    {
        // This is our fallback, and the only one reliably available on Windows.  It's not ideal, but it works.
        qCritical() << "Pitch shift plugin \"soundtouch pitch\" found, enabling key changing";
        gst_bin_add_many(GST_BIN (sinkBin), filter, rgVolume, audioConvert, pitchShifterSoundtouch, audioConvert2, level, autoAudioSink, NULL);
        gst_element_link_many(filter, rgVolume, audioConvert, pitchShifterSoundtouch, audioConvert2, level, autoAudioSink, NULL);
        g_object_set(G_OBJECT(pitchShifterSoundtouch), "pitch", 1.0, "tempo", 1.0, NULL);
        m_canKeyChange = true;
        m_keyChangerSoundtouch = true;
    }
    else
    {
        // No supported pitch shift plugin on the system
        if (loadPitchShift)
            qCritical() << "No supported pitch shifting gstreamer plugin found, key changing disabled";
        gst_bin_add_many(GST_BIN (sinkBin), filter, rgVolume, audioConvert, level, autoAudioSink, NULL);
        gst_element_link_many(filter, rgVolume, audioConvert, level, autoAudioSink, NULL);
        m_canKeyChange = false;
    }
    pad = gst_element_get_static_pad(filter, "sink");
    ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(sinkBin, ghostPad);
    gst_object_unref(pad);
    g_object_set(G_OBJECT(playBin), "audio-sink", sinkBin, NULL);
    g_object_set(G_OBJECT(playBin), "video-sink", videoAppSink, NULL);
//    gst_app_sink_set_emit_signals ((GstAppSink*) videoAppSink, TRUE);
    g_object_set(G_OBJECT(rgVolume), "album-mode", false, NULL);
    g_object_set(G_OBJECT (level), "message", TRUE, NULL);
    bus = gst_element_get_bus (playBin);
//    g_signal_connect (videoAppSink, "new-sample",  G_CALLBACK (on_new_buffer), NULL);
//    fader = new FaderGStreamer(volumeElement, this);
    fader = new FaderGStreamer(playBin, this);
    slowTimer = new QTimer(this);
    connect(slowTimer, SIGNAL(timeout()), this, SLOT(slowTimer_timeout()));
    slowTimer->start(1000);
    fastTimer = new QTimer(this);
    connect(fastTimer, SIGNAL(timeout()), this, SLOT(fastTimer_timeout()));
    connect(fader, SIGNAL(volumeChanged(int)), this, SLOT(faderChangedVolume(int)));
    fastTimer->start(40);
    gst_app_sink_set_callbacks(GST_APP_SINK(videoAppSink), &appsinkCallbacks, this, (GDestroyNotify)AudioBackendGstreamer::DestroyCallback);

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
            if (message->type == GST_MESSAGE_STATE_CHANGED)
            {
                GstState state;
                gst_element_get_state(playBin, &state, NULL, GST_CLOCK_TIME_NONE);
                if (state == GST_STATE_PLAYING)
                    emit stateChanged(AbstractAudioBackend::PlayingState);
                else if (state == GST_STATE_PAUSED)
                    emit stateChanged(AbstractAudioBackend::PausedState);
                else if (state == GST_STATE_NULL)
                    emit stateChanged(AbstractAudioBackend::StoppedState);
                else
                    emit stateChanged(AbstractAudioBackend::UnknownState);
            }
            else if (message->type == GST_MESSAGE_ELEMENT) {
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
                    //qWarning() << "RMS Level: " << m_currentRmsLevel;
                }
            }
            else if (message->type == GST_MESSAGE_EOS)
            {
                emit stateChanged(EndOfMediaState);
            }
            else if (message->type == GST_MESSAGE_TAG)
            {
                // do nothing
            }
            else
            {
                g_print("Msg type[%d], Msg type name[%s]\n", GST_MESSAGE_TYPE(message), GST_MESSAGE_TYPE_NAME(message));
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
        g_object_set(G_OBJECT(playBin), "volume", 0.0, NULL);
        muted = true;
    }
    else
    {
        g_object_set(G_OBJECT(playBin), "volume", m_volume * .01, NULL);
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
//    fader->setBaseVolume(volume);
    g_object_set(G_OBJECT(playBin), "volume", volume * .01, NULL);
    emit volumeChanged(volume);
}

void AudioBackendGstreamer::stop(bool skipFade)
{
    int curVolume = volume();
    if ((m_fade) && (!skipFade) && (state() == AbstractAudioBackend::PlayingState))
        fadeOut(true);
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
    //qWarning() << "Silence detection enabled state: " << m_silenceDetect;
    //qWarning() << "Audio backend state            : " << state();
    if (m_silenceDetect)
    {
        if ((state() == AbstractAudioBackend::PlayingState) && (isSilent()))
        {
            if (m_silenceDuration >= 2)
            {
                //m_silenceDuration = 0;
                emit silenceDetected();
                m_silenceDuration++;
                return;
            }
            m_silenceDuration++;
        }
        else
            m_silenceDuration = 0;
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
    m_keyChange = pitchShift;
    if (m_keyChangerRubberBand)
        g_object_set(G_OBJECT(pitchShifterRubberBand), "semitones", pitchShift, NULL);
    else if (m_keyChangerSoundtouch)
    {
        g_object_set(G_OBJECT(pitchShifterSoundtouch), "pitch", getPitchForSemitone(pitchShift), NULL);
    }
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
    qWarning() << "Fader started - Target volume: " << m_targetVolume;
    fading = true;
    while ((volume() != m_targetVolume) && (fading))
    {
//        qWarning() << "Fader - current: " << volume() << " target: " << m_targetVolume;
        if (volume() > m_targetVolume)
        {
            if (volume() < m_targetVolume + .02)
            {
                qWarning() << "Fader - Approximate target reached, exiting";
                setVolume(m_targetVolume);
                fading = false;
                return;
            }
            else
                setVolume(volume() - .02);
        }
        if (volume() < m_targetVolume)
        {
            if (volume() > m_targetVolume - .02)
            {
                qWarning() << "Fader - Approximate target reached, exiting";
                setVolume(m_targetVolume);
                fading = false;
                return;
            }
            else
                setVolume(volume() + .02);
        }
        QThread::msleep(100);
    }
    fading = false;
}

void FaderGStreamer::fadeIn(bool waitForFade)
{
    qWarning() << "fadeIn() - Started";
    m_targetVolume = m_preOutVolume;
    if (!fading)
    {
        fading = true;
        start();
    }
    else
    {
        qWarning() << "fadeIn() - A fade operation is already in progress... skipping";
        return;
    }
    if (waitForFade)
    {
        while(fading)
            QApplication::processEvents();
    }
    qWarning() << "fadeIn() - Finished";
}

void FaderGStreamer::fadeOut(bool waitForFade)
{
    qWarning() << "fadeOut() - Started";
    m_targetVolume = 0;
    if (!fading)
    {
        fading = true;
        m_preOutVolume = volume();
        start();
    }
    else
    {
        qWarning() << "fadeOut() - A fade operation is already in progress... skipping";
        return;
    }
    if (waitForFade)
    {
        while(fading)
            QApplication::processEvents();
    }
    qWarning() << "fadeOut() - Finished";
}

bool FaderGStreamer::isFading()
{
    return fading;
}

void FaderGStreamer::restoreVolume()
{
    fading = false;
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

void AudioBackendGstreamer::fadeOut(bool waitForFade)
{
    fader->fadeOut(waitForFade);
}

void AudioBackendGstreamer::fadeIn(bool waitForFade)
{
    fader->fadeIn(waitForFade);
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
    if ((m_currentRmsLevel <= 0.01) && (m_volume > 0))
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

void AudioBackendGstreamer::newFrame()
{
    GstSample* sample = gst_app_sink_pull_sample((GstAppSink*)videoAppSink);

    if (sample) {
      GstBuffer *buffer;
      GstCaps *caps;
      GstStructure *s;
      int width, height;
      /* get the snapshot buffer format now. We set the caps on the appsink so
       * that it can only be an rgb buffer. The only thing we have not specified
       * on the caps is the height, which is dependant on the pixel-aspect-ratio
       * of the source material */
      caps = gst_sample_get_caps (sample);
      s = gst_caps_get_structure (caps, 0);
      /* we need to get the final caps on the buffer to get the size */
      gst_structure_get_int (s, "width", &width);
      gst_structure_get_int (s, "height", &height);

      /* create pixmap from buffer and save, gstreamer video buffers have a stride
       * that is rounded up to the nearest multiple of 4 */
      buffer = gst_sample_get_buffer (sample);

      GstMapInfo bufferInfo;
          gst_buffer_map(buffer,&bufferInfo,GST_MAP_READ);
          guint8 *rawFrame = bufferInfo.data;
          QImage frame = QImage(rawFrame,width,height,QImage::Format_RGBX8888);
      emit newVideoFrame(frame, getName());

 //     gst_sample_unref(sample);

//      gst_buffer_map (buffer, &map, GST_MAP_READ);
//      pixbuf = gdk_pixbuf_new_from_data (map.data,
//          GDK_COLORSPACE_RGB, FALSE, 8, width, height,
//          GST_ROUND_UP_4 (width * 3), NULL, NULL);

      /* save the pixbuf */
//      gdk_pixbuf_save (pixbuf, "snapshot.png", "png", &error, NULL);
//      gst_buffer_unmap (buffer, &map);
//    } else {
//      g_print ("could not make snapshot\n");
    }


}

void AudioBackendGstreamer::setDownmix(bool enabled)
{
    qDebug() << "AudioBackendHybrid::setDownmix(" << enabled << ") called";
    if (enabled)
        g_object_set(filter, "caps", audioCapsMono, NULL);
    else
        g_object_set(filter, "caps", audioCapsStereo, NULL);
}

void AudioBackendGstreamer::EndOfStreamCallback(GstAppSink* appsink, gpointer user_data)
{
}

GstFlowReturn AudioBackendGstreamer::NewPrerollCallback(GstAppSink* appsink, gpointer user_data)
{
    GstSample* sample = gst_app_sink_pull_preroll(appsink);
    gst_sample_unref(sample);
    return GST_FLOW_OK;

}

GstFlowReturn AudioBackendGstreamer::NewSampleCallback(GstAppSink* appsink, gpointer user_data)
{
    //static double timer = glfwGetTime(); std::cout << "SAMPLE" << (glfwGetTime()-timer)*1000 << std::endl; timer = glfwGetTime();
    //((GstAppSinkPipeline*) user_data)->ReceiveNewSample();
    AudioBackendGstreamer *myObject = (AudioBackendGstreamer*) user_data;
    myObject->newFrame();
    return GST_FLOW_OK;

}

void AudioBackendGstreamer::DestroyCallback(gpointer user_data)
{
    //std::cout << "DESTROY" << std::endl;
}
