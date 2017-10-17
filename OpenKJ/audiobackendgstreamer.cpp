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
#include <gst/audio/streamvolume.h>
#include "settings.h"

extern Settings *settings;

AudioBackendGstreamer::AudioBackendGstreamer(bool loadPitchShift, QObject *parent, QString objectName) :
    AbstractAudioBackend(parent)
{
#ifdef Q_OS_MACOS
    QString appPath = qApp->applicationDirPath();
    qputenv("GST_PLUGIN_SYSTEM_PATH", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0").toLocal8Bit());
    qputenv("GST_PLUGIN_SCANNER", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0/gst-plugin-scanner").toLocal8Bit());
    qputenv("GTK_PATH", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/").toLocal8Bit());
    qputenv("GIO_EXTRA_MODULES", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules").toLocal8Bit());
    qWarning() << "MacOS detected, changed GST env vars to point to the bundled framework";
    qWarning() << qgetenv("GST_PLUGIN_SYSTEM_PATH") << endl << qgetenv("GST_PLUGIN_SCANNER") << endl << qgetenv("GTK_PATH") << endl << qgetenv("GIO_EXTRA_MODULES") << endl;
#endif

    objName = objectName;
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
    m_muted = false;

    GstAppSinkCallbacks appsinkCallbacks;
    appsinkCallbacks.new_preroll	= &AudioBackendGstreamer::NewPrerollCallback;
    appsinkCallbacks.new_sample		= &AudioBackendGstreamer::NewSampleCallback;
    appsinkCallbacks.eos			= &AudioBackendGstreamer::EndOfStreamCallback;

    qCritical() << "Initializing gst\n";
    gst_init(NULL,NULL);
    audioConvert = gst_element_factory_make("audioconvert", "audioConvert");
    audioConvert2 = gst_element_factory_make("audioconvert", "audioConvert2");
    audioConvert3 = gst_element_factory_make("audioconvert", NULL);
    audioConvert4 = gst_element_factory_make("audioconvert", NULL);
    audioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    videoAppSink = gst_element_factory_make("appsink", "videoAppSink");
    rgVolume = gst_element_factory_make("rgvolume", "rgVolume");
//    volumeElement = gst_element_factory_make("volume", "volumeElement");
    level = gst_element_factory_make("level", "level");
    audioPanorama = gst_element_factory_make("audiopanorama", "audioPanorama");
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

    audioMixer = gst_element_factory_make("audiomixer", NULL);
    mixerPadL = gst_element_get_request_pad(audioMixer, "sink_0");
    mixerPadR = gst_element_get_request_pad(audioMixer, "sink_1");
    deInterleave = gst_element_factory_make("deinterleave", NULL);
    g_signal_connect (deInterleave, "pad-added", G_CALLBACK (this->cb_new_pad), this);

    if ((pitchShifterRubberBand) && (loadPitchShift))
    {
        // This is our preferred pitch shifter because it sounds better, but it's only available on Linux
        qCritical() << "Pitch shift ladspa plugin \"Rubber Band\" found, enabling key changing";
//        gst_bin_add_many(GST_BIN (sinkBin), filter, rgVolume, audioConvert, pitchShifterRubberBand, level, autoAudioSink, NULL);
//        gst_element_link_many(filter, rgVolume, audioConvert, pitchShifterRubberBand, level, autoAudioSink, NULL);
        gst_bin_add_many(GST_BIN (sinkBin), audioConvert3, deInterleave, audioMixer, audioConvert4, filter, rgVolume, audioConvert, pitchShifterRubberBand, level, audioSink, NULL);
        gst_element_link(audioConvert3, deInterleave);
        gst_element_link_many(audioMixer, audioConvert4, filter, rgVolume, audioConvert, pitchShifterRubberBand, level, audioSink, NULL);
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
        gst_bin_add_many(GST_BIN (sinkBin), audioConvert3, deInterleave, audioMixer, audioConvert4, filter, rgVolume, audioConvert, pitchShifterSoundtouch, audioConvert2, level, audioSink, NULL);
        gst_element_link(audioConvert3, deInterleave);
        gst_element_link_many(audioMixer, audioConvert4, filter, rgVolume, audioConvert, pitchShifterSoundtouch, audioConvert2, level, audioSink, NULL);
        g_object_set(G_OBJECT(pitchShifterSoundtouch), "pitch", 1.0, "tempo", 1.0, NULL);
        m_canKeyChange = true;
        m_keyChangerSoundtouch = true;
    }
    else
    {
        // No supported pitch shift plugin on the system
        if (loadPitchShift)
            qCritical() << "No supported pitch shifting gstreamer plugin found, key changing disabled";
        gst_bin_add_many(GST_BIN (sinkBin), audioConvert3, deInterleave, audioMixer, audioConvert4, filter, rgVolume, audioConvert, level, audioSink, NULL);
        gst_element_link_many(audioMixer, audioConvert4, filter, rgVolume, audioConvert, level, audioSink, NULL);
        m_canKeyChange = false;
    }
    pad = gst_element_get_static_pad(audioConvert3, "sink");
    ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(sinkBin, ghostPad);
    gst_object_unref(pad);
    g_object_set(G_OBJECT(playBin), "audio-sink", sinkBin, NULL);
    g_object_set(G_OBJECT(playBin), "video-sink", videoAppSink, NULL);
    g_object_set(G_OBJECT(rgVolume), "album-mode", false, NULL);
    g_object_set(G_OBJECT (level), "message", TRUE, NULL);
    bus = gst_element_get_bus (playBin);
    fader = new FaderGStreamer(playBin, this);
    fader->objName = objName;
    slowTimer = new QTimer(this);
    connect(slowTimer, SIGNAL(timeout()), this, SLOT(slowTimer_timeout()));
    slowTimer->start(1000);
    fastTimer = new QTimer(this);
    connect(fastTimer, SIGNAL(timeout()), this, SLOT(fastTimer_timeout()));
    connect(fader, SIGNAL(volumeChanged(int)), this, SLOT(faderChangedVolume(int)));
    fastTimer->start(40);
    gst_app_sink_set_callbacks(GST_APP_SINK(videoAppSink), &appsinkCallbacks, this, (GDestroyNotify)AudioBackendGstreamer::DestroyCallback);

    monitor = gst_device_monitor_new ();
    GstCaps *moncaps;
    moncaps = gst_caps_new_empty_simple ("audio/x-raw");
    gst_device_monitor_add_filter (monitor, "Audio/Sink", moncaps);
    gst_caps_unref (moncaps);
    gst_device_monitor_start (monitor);
    outputDeviceNames.clear();
    outputDeviceNames.append("Default");
    GList *devices, *elem;
    devices = gst_device_monitor_get_devices(monitor);
    for(elem = devices; elem; elem = elem->next) {
        GstDevice *device = (GstDevice*)elem->data;
        outputDeviceNames.append(gst_device_get_display_name(device));
        outputDevices.append(device);
    }
    connect(settings, SIGNAL(mplxModeChanged(int)), this, SLOT(setMplxMode(int)));

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
            if (message->type == GST_MESSAGE_ERROR)
            {
                GError *err;
                gchar *debug;
                gst_message_parse_error(message, &err, &debug);
                g_print("GStreamer error: %s\n", err->message);
                g_print("GStreamer debug output: %s\n", debug);
            }
            else if (message->type == GST_MESSAGE_WARNING)
            {
                GError *err;
                gchar *debug;
                gst_message_parse_warning(message, &err, &debug);
                g_print("GStreamer warning: %s\n", err->message);
                g_print("GStreamer debug output: %s\n", debug);
            }
            else if (message->type == GST_MESSAGE_STATE_CHANGED)
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
    return m_muted;
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
        setVolume(m_volume);
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
    }
    else
    {
        g_object_set(G_OBJECT(playBin), "volume", m_volume * .01, NULL);
    }
    m_muted = muted;

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
    double cubicVolume = volume * .01;
    double linearVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, cubicVolume);
    m_volume = volume;
//    fader->setBaseVolume(volume);
    g_object_set(G_OBJECT(playBin), "volume", linearVolume, NULL);
    emit volumeChanged(volume);
}

void AudioBackendGstreamer::stop(bool skipFade)
{
    qWarning() << objName << " - AudioBackendGstreamer::stop(" << skipFade << ") called";
    if (state() == AbstractAudioBackend::StoppedState)
    {
        qWarning() << objName << " - AudioBackendGstreamer::stop -- Already stopped, skipping";
        return;
    }
    int curVolume = volume();
    if ((m_fade) && (!skipFade) && (state() == AbstractAudioBackend::PlayingState))
    {
        qWarning() << objName << " - AudioBackendGstreamer::stop -- Fading enabled.  Fading out audio volume";
        fadeOut(true);
        qWarning() << objName << " - AudioBackendGstreamer::stop -- Fading complete";
    }
    qWarning() << objName << " - AudioBackendGstreamer::stop -- Stoping playback";
    gst_element_set_state(playBin, GST_STATE_NULL);
    emit stateChanged(AbstractAudioBackend::StoppedState);
    if ((m_fade) && (!skipFade))
    {
        qWarning() << objName << " - AudioBackendGstreamer::stop -- Setting volume back to original setting: " << curVolume;
        setVolume(curVolume);
    }
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
//        if (currentState == AbstractAudioBackend::StoppedState)
//            stop();
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
    qWarning() << objName << " - Fader started - Target volume: " << m_targetVolume;
    fading = true;
    while ((volume() != m_targetVolume) && (fading))
    {
//        qWarning() << "Fader - current: " << volume() << " target: " << m_targetVolume;
        if (volume() > m_targetVolume)
        {
            if (volume() < m_targetVolume + .02)
            {
                qWarning() << objName << " - Fader - Approximate target reached, exiting";
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
                qWarning() << objName << " - Fader - Approximate target reached, exiting";
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
    qWarning() << objName << " - fadeIn() - Started";
    m_targetVolume = m_preOutVolume;
    if (!fading)
    {
        fading = true;
        start();
    }
    else
    {
        qWarning() << objName << " - fadeIn() - A fade operation is already in progress... skipping";
        return;
    }
    if (waitForFade)
    {
        while(fading)
            QApplication::processEvents();
    }
    qWarning() << objName << " - fadeIn() - Finished";
}

void FaderGStreamer::fadeOut(bool waitForFade)
{
    qWarning() << objName << " - fadeOut() - Started";
    m_targetVolume = 0;
    if (!fading)
    {
        fading = true;
        m_preOutVolume = volume();
        start();
    }
    else
    {
        qWarning() << objName << " - fadeOut() - A fade operation is already in progress... skipping";
        return;
    }
    if (waitForFade)
    {
        while(fading)
            QApplication::processEvents();
    }
    qWarning() << objName << " - fadeOut() - Finished";
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
    double cubicVolume = targetVolume;
    double linearVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, cubicVolume);
    g_object_set(G_OBJECT(volumeElement), "volume", linearVolume, NULL);
    emit volumeChanged(cubicVolume * 100);
}

double FaderGStreamer::volume()
{
    gdouble curVolume;
    g_object_get(G_OBJECT(volumeElement), "volume", &curVolume, NULL);
    double linearVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, curVolume);
    return linearVolume;
}

QStringList AudioBackendGstreamer::GstGetPlugins()
{
    QStringList list;
    GList* plugins; /* The head of the plug-in list */
    GList* pnode;   /* The currently viewed node */

    /* Empty the list out here */
    list.clear();

    plugins = pnode = gst_registry_get_plugin_list(gst_registry_get());
    while(pnode) {
            /* plugin: the plug-in info object pointed to by pnode */
            GstPlugin* plugin = (GstPlugin*)pnode->data;

            list << gst_plugin_get_name(plugin);
            qWarning() << gst_plugin_get_name(plugin);
            pnode = g_list_next(pnode);
    }

    /* Clean-up */
    gst_plugin_list_free (plugins);
    return list;
}

QStringList AudioBackendGstreamer::GstGetElements(QString plugin)
{
    QStringList list;
    GList* features;        /* The list of plug-in features */
    GList* fnode;           /* The currently viewed node */

    /* Empty the list out here */
    list.clear();

    features = fnode = gst_registry_get_feature_list_by_plugin( gst_registry_get(), plugin.toUtf8().data());
    while(fnode) {
        if (fnode->data) {
            /* Currently pointed-to feature */
            GstPluginFeature* feature = GST_PLUGIN_FEATURE(fnode->data);

            if (GST_IS_ELEMENT_FACTORY (feature)) {
                GstElementFactory* factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(feature));
                list << QString(GST_OBJECT_NAME(factory));
                qWarning() << QString(GST_OBJECT_NAME(factory));
            }
        }
        fnode = g_list_next(fnode);
    }
    gst_plugin_feature_list_free(features);
    return list;
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
        caps = gst_sample_get_caps (sample);
        s = gst_caps_get_structure (caps, 0);
        gst_structure_get_int (s, "width", &width);
        gst_structure_get_int (s, "height", &height);
        buffer = gst_sample_get_buffer (sample);
        GstMapInfo bufferInfo;
        gst_buffer_map(buffer,&bufferInfo,GST_MAP_READ);
        guint8 *rawFrame = bufferInfo.data;
        QImage frame = QImage(rawFrame,width,height,QImage::Format_RGBX8888);
        emit newVideoFrame(frame, getName());
        gst_buffer_unmap(buffer, &bufferInfo);
        gst_sample_unref(sample);
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

QStringList AudioBackendGstreamer::getOutputDevices()
{
    return outputDeviceNames;
}

void AudioBackendGstreamer::setOutputDevice(int deviceIndex)
{
    gst_element_unlink(level, audioSink);
    gst_bin_remove(GST_BIN(sinkBin), audioSink);
    if (deviceIndex == 0)
    {
        qWarning() << "Default device selected";
        //default device selected
        audioSink = gst_element_factory_make("autoaudiosink", "audioSink");;
    }
    else
    {
        audioSink = gst_device_create_element(outputDevices.at(deviceIndex - 1), NULL);
        qWarning() << "Non default device selected: " << outputDeviceNames.at(deviceIndex);
    }
    gst_bin_add(GST_BIN(sinkBin), audioSink);
    gst_element_link(level, audioSink);
}

void AudioBackendGstreamer::EndOfStreamCallback(GstAppSink* appsink, gpointer user_data)
{
    Q_UNUSED(appsink)
    Q_UNUSED(user_data)
}

GstFlowReturn AudioBackendGstreamer::NewPrerollCallback(GstAppSink* appsink, gpointer user_data)
{
    Q_UNUSED(user_data)
    Q_UNUSED(appsink)
    return GST_FLOW_OK;

}

GstFlowReturn AudioBackendGstreamer::NewSampleCallback(GstAppSink* appsink, gpointer user_data)
{
    Q_UNUSED(appsink)
    AudioBackendGstreamer *myObject = (AudioBackendGstreamer*) user_data;
    myObject->newFrame();
    return GST_FLOW_OK;

}

void AudioBackendGstreamer::cb_new_pad(GstElement *element, GstPad *pad, gpointer data)
{
    Q_UNUSED(element);
    AudioBackendGstreamer *parent = reinterpret_cast<AudioBackendGstreamer*>(data);
    QString name = QString(gst_pad_get_name(pad));
    if (name == "src_0")
    {
        gst_pad_link(pad, parent->mixerPadL);
        if (settings->mplxMode() == Multiplex::RightChannel)
            g_object_set(parent->mixerPadL, "mute", true, NULL);
        else
            g_object_set(parent->mixerPadL, "mute", false, NULL);
    }
    if (name == "src_1")
    {
        gst_pad_link(pad, parent->mixerPadR);
        if (settings->mplxMode() == Multiplex::LeftChannel)
            g_object_set(parent->mixerPadR, "mute", true, NULL);
        else
            g_object_set(parent->mixerPadR, "mute", false, NULL);
    }
}

void AudioBackendGstreamer::DestroyCallback(gpointer user_data)
{
    Q_UNUSED(user_data)
}


void AudioBackendGstreamer::setMplxMode(int mode)
{

    if (mode == Multiplex::Normal)
    {
        g_object_set(mixerPadL, "mute", false, NULL);
        g_object_set(mixerPadR, "mute", false, NULL);
    }
    else if (mode == Multiplex::LeftChannel)
    {
        g_object_set(mixerPadL, "mute", false, NULL);
        g_object_set(mixerPadR, "mute", true, NULL);
    }
    else if (mode == Multiplex::RightChannel)
    {
        g_object_set(mixerPadL, "mute", true, NULL);
        g_object_set(mixerPadR, "mute", false, NULL);
    }
    //settings->setMplxMode(mode);
}
