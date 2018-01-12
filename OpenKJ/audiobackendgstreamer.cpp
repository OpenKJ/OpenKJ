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
    m_tempo = 100;
    m_canKeyChange = false;
    m_canChangeTempo = false;
    m_keyChangerRubberBand = false;
    m_keyChangerSoundtouch = false;
    m_fade = false;
    m_silenceDetect = false;
    m_outputChannels = 0;
    m_currentRmsLevel = 0.0;
    m_keyChange = 0;
    m_silenceDuration = 0;
    m_muted = false;
    isFading = false;
    eq1 = 0;
    eq2 = 0;
    eq3 = 0;
    eq4 = 0;
    eq5 = 0;
    eq6 = 0;
    eq7 = 0;
    eq8 = 0;
    eq9 = 0;
    eq10 = 0;

    GstAppSinkCallbacks appsinkCallbacks;
    appsinkCallbacks.new_preroll	= &AudioBackendGstreamer::NewPrerollCallback;
    appsinkCallbacks.new_sample		= &AudioBackendGstreamer::NewSampleCallback;
    appsinkCallbacks.eos			= &AudioBackendGstreamer::EndOfStreamCallback;

    qCritical() << "Initializing gst\n";
    gst_init(NULL,NULL);
    aConvInput = gst_element_factory_make("audioconvert", NULL);
    aConvPreSplit = gst_element_factory_make("audioconvert", NULL);
    aConvPrePitchShift = gst_element_factory_make("audioconvert", NULL);
    aConvPostPitchShift = gst_element_factory_make("audioconvert", NULL);
    audioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    videoAppSink = gst_element_factory_make("appsink", "videoAppSink");
    rgVolume = gst_element_factory_make("rgvolume", NULL);
    level = gst_element_factory_make("level", "level");
    audioPanorama = gst_element_factory_make("audiopanorama", "audioPanorama");
    pitchShifterSoundtouch = gst_element_factory_make("pitch", "pitch");
    pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
    equalizer = gst_element_factory_make("equalizer-10bands", NULL);
    playBin = gst_element_factory_make("playbin", "playBin");
    fltrMplxInput = gst_element_factory_make("capsfilter", "filter");
    fltrEnd = gst_element_factory_make("capsfilter", NULL);
    audioCapsStereo = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, NULL);
    audioCapsMono = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 1, NULL);
    videoCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRx", NULL);
    g_object_set(fltrMplxInput, "caps", audioCapsStereo, NULL);
    g_object_set(videoAppSink, "caps", videoCaps, NULL);

    //g_object_set(fltrEnd, "caps", audioCapsStereo, NULL);

    audioMixer = gst_element_factory_make("audiomixer", NULL);
    mixerSinkPadL = gst_element_get_request_pad(audioMixer, "sink_0");
    mixerSinkPadR = gst_element_get_request_pad(audioMixer, "sink_1");
    deInterleave = gst_element_factory_make("deinterleave", NULL);
    g_signal_connect (deInterleave, "pad-added", G_CALLBACK (this->cb_new_pad), this);


    customBin = gst_bin_new("newBin");
    tee = gst_element_factory_make("tee", NULL);
    teeSrcPadN = gst_element_get_request_pad(tee, "src_1");
    teeSrcPadM = gst_element_get_request_pad(tee, "src_0");
    queueS = gst_element_factory_make("queue", NULL);
    queueSinkPadN = gst_element_get_static_pad(queueS, "sink");
    queueSrcPadN = gst_element_get_static_pad(queueS, "src");
    queueM = gst_element_factory_make("queue", NULL);
    queueSinkPadM = gst_element_get_static_pad(queueM, "sink");
    queueSrcPadM = gst_element_get_static_pad(queueM, "src");
    scaleTempo = gst_element_factory_make("scaletempo", NULL);
    aConvL = gst_element_factory_make("audioconvert", NULL);
    aConvSrcPadL = gst_element_get_static_pad(aConvL, "src");
    aConvR = gst_element_factory_make("audioconvert", NULL);
    aConvSrcPadR = gst_element_get_static_pad(aConvR, "src");
    audioMixer = gst_element_factory_make("audiomixer", NULL);
    aConvPostMixer = gst_element_factory_make("audioconvert", NULL);
    aConvEnd = gst_element_factory_make("audioconvert", NULL);
    audioResample = gst_element_factory_make("audioresample", NULL);
    mixerSinkPadL = gst_element_get_request_pad(audioMixer, "sink_0");
    mixerSinkPadR = gst_element_get_request_pad(audioMixer, "sink_1");
    mixerSinkPadN = gst_element_get_request_pad(audioMixer, "sink_2");
    queueL = gst_element_factory_make("queue", NULL);
    queueSinkPadL = gst_element_get_static_pad(queueL, "sink");
    queueSrcPadL = gst_element_get_static_pad(queueL, "src");
    queueR = gst_element_factory_make("queue", NULL);
    queueSinkPadR = gst_element_get_static_pad(queueR, "sink");
    queueSrcPadR = gst_element_get_static_pad(queueR, "src");
    fltrPostMixer = gst_element_factory_make("capsfilter", NULL);
    g_object_set(fltrPostMixer, "caps", audioCapsStereo, NULL);
    volumeElement = gst_element_factory_make("volume", NULL);

//    if (settings->audioDownmix())
//        g_object_set(fltrEnd, "caps", audioCapsMono, NULL);
//    else
//        g_object_set(fltrEnd, "caps", audioCapsStereo, NULL);

    gst_bin_add_many(GST_BIN(customBin), level, aConvInput, aConvPreSplit, rgVolume, volumeElement, equalizer, aConvL, aConvR, fltrMplxInput, tee, queueS, queueM, queueR, queueL, audioMixer, deInterleave, aConvPostMixer, fltrPostMixer, NULL);
    gst_element_link(aConvInput, rgVolume);
    gst_element_link(rgVolume, level);
    gst_element_link(level, volumeElement);
    //gst_element_link(volumeElement, tee);
    gst_element_link(volumeElement, equalizer);
    gst_element_link(equalizer, tee);

    // Normal path
    gst_pad_link(teeSrcPadN, queueSinkPadN);
    gst_pad_link(queueSrcPadN, mixerSinkPadN);

    // Multiplex path input to deinterleave
    gst_pad_link(teeSrcPadM, queueSinkPadM);
    gst_element_link(queueM, aConvPreSplit);
    gst_element_link(aConvPreSplit, fltrMplxInput);
    gst_element_link(fltrMplxInput, deInterleave);
    // Left Channel to mixer
    gst_element_link(queueL, aConvL);
    gst_pad_link(aConvSrcPadL, mixerSinkPadL);
    // Right Channel to mixer
    gst_element_link(queueR, aConvR);
    gst_pad_link(aConvSrcPadR, mixerSinkPadR);
    gst_element_link(audioMixer, aConvPostMixer);
    gst_element_link(aConvPostMixer, fltrPostMixer);


    // Normal or Multiplex stream to effects and end of chain

    if ((pitchShifterRubberBand) && (pitchShifterSoundtouch) && (loadPitchShift))
    {
        qWarning() << "Pitch shift RubberBand enabled";
        qWarning() << "Also loaded SoundTouch for tempo control";
        m_canChangeTempo = true;
        gst_bin_add_many(GST_BIN(customBin), aConvPrePitchShift, pitchShifterRubberBand, aConvPostPitchShift, pitchShifterSoundtouch, aConvEnd, audioSink, NULL);
        gst_element_link_many(fltrPostMixer, aConvPrePitchShift, pitchShifterRubberBand, aConvPostPitchShift, pitchShifterSoundtouch, aConvEnd, audioSink, NULL);
        m_canKeyChange = true;
        m_keyChangerRubberBand = true;
        g_object_set(G_OBJECT(pitchShifterRubberBand), "formant-preserving", true, NULL);
        g_object_set(G_OBJECT(pitchShifterRubberBand), "crispness", 1, NULL);
        g_object_set(G_OBJECT(pitchShifterRubberBand), "semitones", 1.0, NULL);
    }
    else if ((pitchShifterSoundtouch) && (loadPitchShift))
    {
        m_canChangeTempo = true;
        qWarning() << "Pitch shifter SoundTouch enabled";
        gst_bin_add_many(GST_BIN(customBin), aConvPrePitchShift, pitchShifterSoundtouch, aConvPostPitchShift, aConvEnd, audioSink, NULL);
        gst_element_link_many(fltrPostMixer, aConvPrePitchShift, pitchShifterSoundtouch, aConvPostPitchShift, aConvEnd, audioSink, NULL);
        m_canKeyChange = true;
        m_keyChangerSoundtouch = true;
        g_object_set(G_OBJECT(pitchShifterSoundtouch), "pitch", 1.0, "tempo", 1.0, NULL);
    }
    else
    {
        gst_bin_add_many(GST_BIN(customBin), aConvEnd, audioSink, NULL);
        gst_element_link_many(fltrPostMixer, aConvEnd, audioSink, NULL);
    }


    // Setup outputs from playBin
    pad = gst_element_get_static_pad(aConvInput, "sink");
    ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(customBin, ghostPad);
    gst_object_unref(pad);
    g_object_set(G_OBJECT(playBin), "audio-sink", customBin, NULL);
    g_object_set(G_OBJECT(playBin), "video-sink", videoAppSink, NULL);


    g_object_set(G_OBJECT(rgVolume), "album-mode", false, NULL);
    g_object_set(G_OBJECT (level), "message", TRUE, NULL);
    bus = gst_element_get_bus (playBin);
    slowTimer = new QTimer(this);
    connect(slowTimer, SIGNAL(timeout()), this, SLOT(slowTimer_timeout()));
    slowTimer->start(1000);
    fastTimer = new QTimer(this);
    connect(fastTimer, SIGNAL(timeout()), this, SLOT(fastTimer_timeout()));
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

    g_object_set(G_OBJECT(playBin), "volume", 1.0, NULL);

    csource = gst_interpolation_control_source_new ();
    if (!csource)
        qWarning() << objName << " - Error createing control source";
    GstControlBinding *cbind = gst_direct_control_binding_new (GST_OBJECT_CAST(volumeElement), "volume", csource);
    if (!cbind)
        qWarning() << objName << " - Error creating control binding";
    if (!gst_object_add_control_binding (GST_OBJECT_CAST(volumeElement), cbind))
        qWarning() << objName << " - Error adding control binding to volumeElement for fader control";

    g_object_set(csource, "mode", GST_INTERPOLATION_MODE_CUBIC, NULL);
    tv_csource = (GstTimedValueControlSource *)csource;
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
    gst_timed_value_control_source_unset_all(tv_csource);
    g_object_set(G_OBJECT(playBin), "volume", 1.0, NULL);
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
        g_object_set(G_OBJECT(volumeElement), "volume", 0.0, NULL);
    }
    else
    {
        g_object_set(G_OBJECT(volumeElement), "volume", m_volume * .01, NULL);
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
    g_object_set(G_OBJECT(volumeElement), "volume", linearVolume, NULL);
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
    if (state() == AbstractAudioBackend::PausedState)
    {
        setVolume(m_preFadeVolumeInt);
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
    gdouble curVolume;
    g_object_get(G_OBJECT(volumeElement), "volume", &curVolume, NULL);
    double cubicVolume = gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, curVolume);
    int intVol = cubicVolume * 100;
//    qWarning() << objName << " - volume currently" << intVol;
//    qWarning() << objName << " - last volume: " << m_volume;
    if (m_volume != intVol)
    {
        qWarning() << objName << " - emitting volume changed: " << intVol;
        emit faderChangedVolume(intVol);
        //m_volume = curVolume * 100;
        m_volume = intVol;
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

bool AudioBackendGstreamer::canChangeTempo()
{
    return m_canChangeTempo;
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
    qWarning() << objName << " - fadeOut called";
    if (isFading)
    {
        qWarning() << objName << "- fadeOut - Fade already in progress, waiting...";
        while (isFading)
            QApplication::processEvents();
        qWarning() << objName << "- fadeOut - In progress fade finished, continuing...";
    }
    gst_timed_value_control_source_unset_all(tv_csource);
    gdouble curVolume;
    g_object_get(G_OBJECT(volumeElement), "volume", &curVolume, NULL);
    qWarning() << objName << " - fadeOut - Current volume: " << curVolume;
    m_preFadeVolume = curVolume;
    m_preFadeVolumeInt = m_volume;
    if (state() != PlayingState)
    {
        qWarning() << objName << " - fadeOut - State not playing, skipping fade and setting volume directly";
        setVolume(0);
        return;
    }
    if (isSilent())
    {
        qWarning() << objName << "- fadeOut - Audio is currently slient, skipping fade and setting volume to zero immediately";
        setVolume(0);
        return;
    }
    isFading = true;
    gint64 pos;
    GstFormat fmt = GST_FORMAT_TIME;
    gst_element_query_position(playBin, fmt, &pos);
    qWarning() << objName << " - faceOut - Current pos: " << pos;
    //if (!gst_timed_value_control_source_set (tv_csource, pos + (10 * GST_MSECOND), curVolume * .12))
    if (!gst_timed_value_control_source_set (tv_csource, pos + (10 * GST_MSECOND), curVolume * .12))
    {
        qWarning() << objName << " - fadeOut - Error adding start fade timed value";
        isFading = false;
        gst_timed_value_control_source_unset_all(tv_csource);
        return;
    }
    if (!gst_timed_value_control_source_set (tv_csource, pos + (10 * GST_MSECOND) + (5 * GST_SECOND), 0.0))
    {
        qWarning() << objName << " - fadeOut - Error adding end fade timed value";
        isFading = false;
        gst_timed_value_control_source_unset_all(tv_csource);
        return;
    }
    while ((m_volume > 0) && state() == PlayingState)
    {
        QApplication::processEvents();
    }
    gst_timed_value_control_source_unset_all(tv_csource);
    setVolume(0);
    qWarning() << objName << " - fadeOut done";
    isFading = false;
}

void AudioBackendGstreamer::fadeIn(bool waitForFade)
{
    qWarning() << objName << " - fadeIn called";
    if (isFading)
    {
        qWarning() << objName << "- fadeOut - Fade already in progress, waiting...";
        while (isFading)
            QApplication::processEvents();
        qWarning() << objName << "- fadeOut - In progress fade finished, continuing...";
    }
    gst_timed_value_control_source_unset_all(tv_csource);
    if (state() != PlayingState)
    {
        qWarning() << objName << " - fadeIn - State not playing, skipping fade and setting volume";
        setVolume(m_preFadeVolume * 100);
        return;
    }
    if (isSilent())
    {
        qWarning() << objName << "- fadeOut - Audio is currently slient, skipping fade and setting volume immediately";
        setVolume(m_preFadeVolume * 100);
        return;
    }
    isFading = true;
    gint64 pos;
    GstFormat fmt = GST_FORMAT_TIME;
    gst_element_query_position(playBin, fmt, &pos);
    qWarning() << objName << " - fadeIn - Current pos: " << pos;
    qWarning() << objName << " - fadeIn - Target volume: " << m_preFadeVolume;
    if (!gst_timed_value_control_source_set (tv_csource, pos + (10 * GST_MSECOND), 0.0))
    {
        qWarning() << objName << " - fadeIn - Error adding start fade timed value";
        isFading = false;
        gst_timed_value_control_source_unset_all(tv_csource);
        return;
    }
    if (!gst_timed_value_control_source_set (tv_csource, pos + (10 * GST_MSECOND) + (5 * GST_SECOND), m_preFadeVolume * .12))
    {
        qWarning() << objName << " - fadeIn - Error adding end fade timed value";
        isFading = false;
        gst_timed_value_control_source_unset_all(tv_csource);
        return;
    }
    while ((m_volume < m_preFadeVolumeInt) && (state() == PlayingState))
        QApplication::processEvents();
    qWarning() << objName << " - fadeIn done";
    isFading = false;
    gst_timed_value_control_source_unset_all(tv_csource);
    qWarning() << objName << " - forcing volume back to prefade value to work around weirdness in gstreamer control elements";
    qWarning() << objName << " - preFade volume was: " << m_preFadeVolumeInt;
    setVolume(m_preFadeVolumeInt);
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
    if ((m_currentRmsLevel <= 0.01) && (m_volume > 0) && (!isFading))
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

int AudioBackendGstreamer::tempo()
{
    return m_tempo;
}

void AudioBackendGstreamer::setDownmix(bool enabled)
{
    qDebug() << "AudioBackendHybrid::setDownmix(" << enabled << ") called";
    if (enabled)
        g_object_set(fltrPostMixer, "caps", audioCapsMono, NULL);
    else
        g_object_set(fltrPostMixer, "caps", audioCapsStereo, NULL);
}

void AudioBackendGstreamer::setTempo(int percent)
{
    float tempo = (float)percent / 100.0;
    m_tempo = percent;
    g_object_set(pitchShifterSoundtouch, "tempo", tempo, NULL);
    setPosition(position());
    qWarning() << "Tempo changed to " << tempo;
}

QStringList AudioBackendGstreamer::getOutputDevices()
{
    return outputDeviceNames;
}

void AudioBackendGstreamer::setOutputDevice(int deviceIndex)
{
    gst_element_unlink(aConvEnd, audioSink);
    gst_bin_remove(GST_BIN(customBin), audioSink);
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
    gst_bin_add(GST_BIN(customBin), audioSink);
    gst_element_link(aConvEnd, audioSink);
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
        qWarning() << "Linking deinterleave pad src_0 to queueSinkPadL";
        gst_pad_link(pad, parent->queueSinkPadL);
    }
    if (name == "src_1")
    {
        qWarning() << "Linking deinterleave pad src_1 to queueSinkPadR";
        gst_pad_link(pad, parent->queueSinkPadR);
    }
    qWarning() << "Linking complete";
    parent->setMplxMode(settings->mplxMode());
}

void AudioBackendGstreamer::DestroyCallback(gpointer user_data)
{
    Q_UNUSED(user_data)
}


void AudioBackendGstreamer::setMplxMode(int mode)
{

    if (mode == Multiplex_Normal)
    {
        g_object_set(mixerSinkPadN, "mute", false, NULL);
        g_object_set(mixerSinkPadL, "mute", true, NULL);
        g_object_set(mixerSinkPadR, "mute", true, NULL);
    }
    else if (mode == Multiplex_LeftChannel)
    {
        g_object_set(mixerSinkPadN, "mute", true, NULL);
        g_object_set(mixerSinkPadL, "mute", false, NULL);
        g_object_set(mixerSinkPadR, "mute", true, NULL);
    }
    else if (mode == Multiplex_RightChannel)
    {
        g_object_set(mixerSinkPadN, "mute", true, NULL);
        g_object_set(mixerSinkPadL, "mute", true, NULL);
        g_object_set(mixerSinkPadR, "mute", false, NULL);
    }
    //settings->setMplxMode(mode);
}


void AudioBackendGstreamer::setEqBypass(bool bypass)
{
    if (bypass)
    {
        g_object_set(equalizer, "band0", 0.0, NULL);
        g_object_set(equalizer, "band1", 0.0, NULL);
        g_object_set(equalizer, "band2", 0.0, NULL);
        g_object_set(equalizer, "band3", 0.0, NULL);
        g_object_set(equalizer, "band4", 0.0, NULL);
        g_object_set(equalizer, "band5", 0.0, NULL);
        g_object_set(equalizer, "band6", 0.0, NULL);
        g_object_set(equalizer, "band7", 0.0, NULL);
        g_object_set(equalizer, "band8", 0.0, NULL);
        g_object_set(equalizer, "band0", 0.0, NULL);
    }
    else
    {
        g_object_set(equalizer, "band1", (double)eq1, NULL);
        g_object_set(equalizer, "band2", (double)eq2, NULL);
        g_object_set(equalizer, "band3", (double)eq3, NULL);
        g_object_set(equalizer, "band4", (double)eq4, NULL);
        g_object_set(equalizer, "band5", (double)eq5, NULL);
        g_object_set(equalizer, "band6", (double)eq6, NULL);
        g_object_set(equalizer, "band7", (double)eq7, NULL);
        g_object_set(equalizer, "band8", (double)eq8, NULL);
        g_object_set(equalizer, "band0", (double)eq9, NULL);
        g_object_set(equalizer, "band9", (double)eq10, NULL);

    }
    this->bypass = bypass;
}

void AudioBackendGstreamer::setEqLevel1(int level)
{
    g_object_set(equalizer, "band0", (double)level, NULL);
    eq1 = level;
}

void AudioBackendGstreamer::setEqLevel2(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band1", (double)level, NULL);
    eq2 = level;
}

void AudioBackendGstreamer::setEqLevel3(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band2", (double)level, NULL);
    eq3 = level;
}

void AudioBackendGstreamer::setEqLevel4(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band3", (double)level, NULL);
    eq4 = level;
}

void AudioBackendGstreamer::setEqLevel5(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band4", (double)level, NULL);
    eq5 = level;
}

void AudioBackendGstreamer::setEqLevel6(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band5", (double)level, NULL);
    eq6 = level;
}

void AudioBackendGstreamer::setEqLevel7(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band6", (double)level, NULL);
    eq7 = level;
}

void AudioBackendGstreamer::setEqLevel8(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band7", (double)level, NULL);
    eq8 = level;
}

void AudioBackendGstreamer::setEqLevel9(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band8", (double)level, NULL);
    eq9 = level;
}

void AudioBackendGstreamer::setEqLevel10(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band9", (double)level, NULL);
    eq10 = level;
}
