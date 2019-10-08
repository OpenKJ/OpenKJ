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

#include "audiobackendgstreamer.h"
#include <QApplication>
#include <QDebug>
#include <string.h>
#include <math.h>
#include <QFile>
#include <gst/audio/streamvolume.h>
#include <gst/gstdebugutils.h>
#include "settings.h"
#include <functional>
#include <gst/video/videooverlay.h>

extern Settings *settings;
GstElement *playBinPub;


Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<GstMessage>);

AudioBackendGstreamer::AudioBackendGstreamer(bool loadPitchShift, QObject *parent, QString objectName) :
    AbstractAudioBackend(parent)
{
    initDone = false;
#ifdef Q_OS_MACOS
    QString appPath = qApp->applicationDirPath();
    qputenv("GST_PLUGIN_SYSTEM_PATH", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0").toLocal8Bit());
    qputenv("GST_PLUGIN_SCANNER", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0/gst-plugin-scanner").toLocal8Bit());
    qputenv("GTK_PATH", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/").toLocal8Bit());
    qputenv("GIO_EXTRA_MODULES", QString("/Applications/OpenKJ.app/Contents/Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules").toLocal8Bit());
    qInfo() << "MacOS detected, changed GST env vars to point to the bundled framework";
    qInfo() << qgetenv("GST_PLUGIN_SYSTEM_PATH") << endl << qgetenv("GST_PLUGIN_SCANNER") << endl << qgetenv("GTK_PATH") << endl << qgetenv("GIO_EXTRA_MODULES") << endl;
#endif
    qInfo() << "Start constructing GStreamer backend";
    QMetaTypeId<std::shared_ptr<GstMessage>>::qt_metatype_id();
    this->loadPitchShift = loadPitchShift;
    objName = objectName;
    m_hasVideo = false;
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
    m_vidMuted = false;
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
    outputDeviceIdx = 0;
    downmix = false;
    bypass = false;
    lastState = AbstractAudioBackend::StoppedState;

    slowTimer = new QTimer(this);
    connect(slowTimer, SIGNAL(timeout()), this, SLOT(slowTimer_timeout()));
//    slowTimer->start(1000);
    fastTimer = new QTimer(this);
    connect(fastTimer, SIGNAL(timeout()), this, SLOT(fastTimer_timeout()));
    fader = new AudioFader(this);
    fader->setObjName(objName + "Fader");
    connect(fader, SIGNAL(fadeStarted()), this, SLOT(faderStarted()));
    connect(fader, SIGNAL(fadeComplete()), this, SLOT(faderFinished()));
//    connect(fader, SIGNAL(volumeChanged(double)), this, SLOT(faderChangedVolume(double)));


    if (!gst_is_initialized())
    {
        qInfo() << objName << " - gst not initialized - initializing";
        gst_init(NULL,NULL);
    }

    faderVolumeElement = gst_element_factory_make("volume", "FaderVolumeElement");
    g_object_set(G_OBJECT(faderVolumeElement), "volume", 1.0, NULL);
    fader->setVolumeElement(faderVolumeElement);
    gst_object_ref(faderVolumeElement);
    connect(fader, SIGNAL(faderStateChanged(AudioFader::FaderState)), this, SLOT(faderStateChanged(AudioFader::FaderState)));

    buildPipeline();

    monitor = gst_device_monitor_new ();
    GstCaps *moncaps;
    moncaps = gst_caps_new_empty_simple ("audio/x-raw");
    gst_device_monitor_add_filter (monitor, "Audio/Sink", moncaps);
    gst_caps_unref (moncaps);
    gst_device_monitor_start (monitor);
    outputDeviceNames.clear();
    outputDeviceNames.append("0 - Default");
    GList *devices, *elem;
    devices = gst_device_monitor_get_devices(monitor);
    for(elem = devices; elem; elem = elem->next) {
        GstDevice *device = (GstDevice*)elem->data;
        gchar *deviceName = gst_device_get_display_name(device);
        outputDeviceNames.append(QString::number(outputDeviceNames.size()) + " - " + deviceName);
        g_free(deviceName);
        outputDevices.append(device);
    }
    g_object_set(G_OBJECT(playBin), "volume", 1.0, NULL);
    gst_device_monitor_stop(monitor);

    connect(settings, SIGNAL(mplxModeChanged(int)), this, SLOT(setMplxMode(int)));
//    g_timeout_add (40,(GSourceFunc) gstTimerDispatcher, this);
    qInfo() << "Done constructing GStreamer backend";
    initDone = true;
}

void AudioBackendGstreamer::videoMute(bool mute)
{
    gint flags;
    if (mute)
    {
        g_object_get (playBin, "flags", &flags, NULL);
        flags |= GST_PLAY_FLAG_AUDIO;
        flags &= ~GST_PLAY_FLAG_TEXT;
        flags &= ~GST_PLAY_FLAG_VIDEO;
        g_object_set (playBin, "flags", flags, NULL);
    }
    else if (!mute)
    {
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink), videoWinId);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink2), videoWinId2);
        g_object_get (playBin, "flags", &flags, NULL);
        flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO;
        flags &= ~GST_PLAY_FLAG_TEXT;
        g_object_set (playBin, "flags", flags, NULL);
    }
    m_vidMuted = mute;
}

bool AudioBackendGstreamer::videoMuted()
{
    return m_vidMuted;
}

AudioBackendGstreamer::~AudioBackendGstreamer()
{
    slowTimer->stop();
    fastTimer->stop();
    if (state() == PlayingState)
        stop(true);
    gst_object_unref(playBin);
    gst_caps_unref(audioCapsMono);
    gst_caps_unref(audioCapsStereo);
    gst_caps_unref(videoCaps);
    gst_object_unref(bus.get());
    gst_object_unref(monitor);
    gst_object_unref(tv_csource);
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
    //gst_timed_value_control_source_unset_all(tv_csource);
    qInfo() << objName << " - play() called";
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(playBin), GST_STREAM_VOLUME_FORMAT_LINEAR, 1.0);
    if (state() == AbstractAudioBackend::PausedState)
    {
        qInfo() << objName << " - play - playback is currently paused, unpausing";
        gst_element_set_state(playBin, GST_STATE_PLAYING);
        if (m_fade)
            fadeIn();
    }
    else
    {
        resetPipeline();
        if (!QFile::exists(m_filename))
        {
            qInfo() << "File doesn't exist, bailing out";
            emit stateChanged(PlayingState);
            QApplication::processEvents();
            emit stateChanged(EndOfMediaState);
            return;
        }
        gchar *uri = gst_filename_to_uri(m_filename.toLocal8Bit(), NULL);
        g_object_set(GST_OBJECT(playBin), "uri", uri, NULL);
        g_free(uri);
        m_hasVideo = false;
        qInfo() << objName << " - playing file: " << m_filename;
        gst_element_set_state(playBin, GST_STATE_PLAYING);
    }
    qInfo() << objName << " - play() completed";
}

void AudioBackendGstreamer::pause()
{
    if (m_fade)
        fadeOut();
    gst_element_set_state(playBin, GST_STATE_PAUSED);
}

void AudioBackendGstreamer::setMedia(QString filename)
{
    m_hasVideo = false;
    m_filename = filename;
}

void AudioBackendGstreamer::setMuted(bool muted)
{

    gst_stream_volume_set_mute(GST_STREAM_VOLUME(volumeElement), muted);
    m_muted = muted;

}

void AudioBackendGstreamer::setPosition(qint64 position)
{
    qDebug() << objName << " - Seeking to: " << position << "ms";
    GstSeekFlags flags = GST_SEEK_FLAG_FLUSH;
    if (!gst_element_seek_simple(playBin, GST_FORMAT_TIME, flags, GST_MSECOND * position))
    {
      qDebug() << objName << " - Seek failed!";
    }
    emit positionChanged(position);
}

void AudioBackendGstreamer::setVolume(int volume)
{
    m_volume = volume;
    double floatVol = volume * .01;
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC, floatVol);
    emit volumeChanged(volume);
}

void AudioBackendGstreamer::stop(bool skipFade)
{
    qInfo() << objName << " - AudioBackendGstreamer::stop(" << skipFade << ") called";
    if (state() == AbstractAudioBackend::StoppedState)
    {
        qInfo() << objName << " - AudioBackendGstreamer::stop -- Already stopped, skipping";
        emit stateChanged(AbstractAudioBackend::StoppedState);
        return;
    }
    if (state() == AbstractAudioBackend::PausedState)
    {
        qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping paused song";
        gst_element_set_state(playBin, GST_STATE_NULL);
        emit stateChanged(AbstractAudioBackend::StoppedState);
        qInfo() << objName << " - stop() completed";
        fader->immediateIn();
        return;
    }
    if ((m_fade) && (!skipFade) && (state() == AbstractAudioBackend::PlayingState))
    {
        if (fader->state() == AudioFader::FadedIn || fader->state() == AudioFader::FadingIn)
        {
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Fading enabled.  Fading out audio volume";
            fadeOut(true);
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Fading complete";
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping playback";
            gst_element_set_state(playBin, GST_STATE_NULL);
            emit stateChanged(AbstractAudioBackend::StoppedState);
            fader->immediateIn();
            return;
        }
    }
    qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping playback without fading";
    gst_element_set_state(playBin, GST_STATE_NULL);
    emit stateChanged(AbstractAudioBackend::StoppedState);
    qInfo() << objName << " - stop() completed";


}

void AudioBackendGstreamer::rawStop()
{
    qInfo() << objName << " - rawStop() called, just ending gstreamer playback";
    gst_element_set_state(playBin, GST_STATE_NULL);
}

void AudioBackendGstreamer::fastTimer_timeout()
{
    static qint64 lastPos = 0;
    if (lastState != PlayingState)
    {
        if (lastPos == 0)
            return;
        lastPos = 0;
        emit positionChanged(0);
        return;
    }
    gint64 pos, mspos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (playBin, fmt, &pos))
        mspos = pos / 1000000;
    else
        mspos = 0;
    if (lastPos != mspos)
    {
        lastPos = mspos;
        emit positionChanged(mspos);
    }
}



void AudioBackendGstreamer::slowTimer_timeout()
{
    AbstractAudioBackend::State curState = state();
    if (lastState != curState)
        emit stateChanged(curState);
    if (m_silenceDetect)
    {
        if ((curState == AbstractAudioBackend::PlayingState) && (isSilent()))
        {
            if (m_silenceDuration >= 2)
            {
                emit silenceDetected();
                m_silenceDuration++;
                return;
            }
            m_silenceDuration++;
        }

        else
            m_silenceDuration = 0;
    }
    double cubicVolume = gst_stream_volume_get_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC);
    int intVol = cubicVolume * 100;
    if (m_volume != intVol)
    {
        qInfo() << objName << " - emitting volume changed: " << intVol;
        emit faderChangedVolume(intVol);
        m_volume = intVol;
    }
    // Detect hung playback
    static int lastpos = 0;
    if (curState == PlayingState)
    {
        if (lastpos == position())
        {
            qWarning() << objName << " - Playback appears to be hung, emitting end of stream";
            emit stateChanged(EndOfMediaState);
        }
        lastpos = position();
    }
}

void AudioBackendGstreamer::faderChangedVolume(int volume)
{
    m_volume = volume;
    emit volumeChanged(volume);
}

void AudioBackendGstreamer::faderStarted()
{
    qInfo() << objName << " - Fader started";
}

void AudioBackendGstreamer::faderFinished()
{
    qInfo() << objName << " - fader finished";
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
            qInfo() << gst_plugin_get_name(plugin);
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
                qInfo() << QString(GST_OBJECT_NAME(factory));
            }
        }
        fnode = g_list_next(fnode);
    }
    gst_plugin_feature_list_free(features);
    return list;
}

template <typename T> std::shared_ptr<T> takeGstObject(T *o)
{
  std::shared_ptr<T> ptr(o, [] (T *d) {
    gst_object_unref(reinterpret_cast<GstObject*>(d));
  });
  return ptr;
}

template <typename T> std::shared_ptr<T> takeGstMiniObject(T *o)
{
    std::shared_ptr<T> ptr(o, [] (T *d) {
        gst_mini_object_unref(reinterpret_cast<GstMiniObject*>(d));
    });
    return ptr;
}

void AudioBackendGstreamer::busMessage(std::shared_ptr<GstMessage> message)
{
    switch (GST_MESSAGE_TYPE(message.get())) {
    case GST_MESSAGE_LATENCY:
        break;
    case GST_MESSAGE_ASYNC_DONE:
        break;
    case GST_MESSAGE_NEW_CLOCK:
        break;
    case GST_MESSAGE_ERROR:
        GError *err;
        gchar *debug;
        gst_message_parse_error(message.get(), &err, &debug);
        //g_print("GStreamer error: %s\n", err->message);
        //g_print("GStreamer debug output: %s\n", debug);
        qInfo() << objName << " - Gst error: " << err->message;
        qInfo() << objName << " - Gst debug: " << debug;
        if (QString(err->message) == "Your GStreamer installation is missing a plug-in.")
        {
            QString player;
            if (objName == "KA")
                player = "karaoke";
            else
                player = "break music";
            qInfo() << objName << " - PLAYBACK ERROR - Missing Codec";
            emit audioError("Unable to play " + player + " file, missing gstreamer plugin");
            stop(true);
        }
        g_error_free(err);
        g_free(debug);
        break;
    case GST_MESSAGE_WARNING:
        GError *err2;
        gchar *debug2;
        gst_message_parse_warning(message.get(), &err2, &debug2);
        qInfo() << objName << " - Gst warning: " << err2->message;
        qInfo() << objName << " - Gst debug: " << debug2;
        g_error_free(err2);
        g_free(debug2);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        GstState state;
        gst_element_get_state(playBin, &state, NULL, GST_CLOCK_TIME_NONE);
        if (state == GST_STATE_PLAYING && lastState != AbstractAudioBackend::PlayingState)
        {
            qInfo() << "GST notified of state change to PLAYING";
            lastState = AbstractAudioBackend::PlayingState;
            emit stateChanged(AbstractAudioBackend::PlayingState);
        }
        else if (state == GST_STATE_PAUSED && lastState != AbstractAudioBackend::PausedState)
        {
            qInfo() << "GST notified of state change to PAUSED";
            lastState = AbstractAudioBackend::PausedState;
            emit stateChanged(AbstractAudioBackend::PausedState);
        }
        else if (state == GST_STATE_NULL && lastState != AbstractAudioBackend::StoppedState)
        {
            qInfo() << "GST notified of state change to STOPPED";
            if (lastState != AbstractAudioBackend::StoppedState)
            {
                lastState = AbstractAudioBackend::StoppedState;
                emit stateChanged(AbstractAudioBackend::StoppedState);
            }
        }
        else if (lastState != AbstractAudioBackend::UnknownState)
        {
            //qInfo() << "GST notified of state change to UNKNOWN";
            //lastState = AbstractAudioBackend::UnknownState;
            //emit stateChanged(AbstractAudioBackend::UnknownState);
        }
        break;
    case GST_MESSAGE_ELEMENT:
        if (QString(gst_structure_get_name (gst_message_get_structure(message.get()))) == "level")
        {
            guint channels;
            gdouble rms_dB;
            gdouble rms;
            const GValue *value;
            const GValue *array_val;
            GValueArray *rms_arr;
            guint i;
            array_val = gst_structure_get_value(gst_message_get_structure(message.get()), "rms");
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
        break;
    case GST_MESSAGE_STREAM_START:
        // workaround for gstreamer changing the volume unsolicited on playback start
        double cubicVolume;
        cubicVolume = gst_stream_volume_get_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC);
        int intVol;
        intVol = cubicVolume * 100;
        if (m_volume != intVol)
        {
            if ((!fader->isFading()) && ((intVol < m_volume - 1) || (intVol > m_volume + 1)))
            {
                qInfo() << objName << " - stream start - Unrequested volume change detected, squashing";
                setVolume(m_volume);
            }
        }
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        gint64 dur, msdur;
        qInfo() << objName << " - GST reports duration changed";
        if (gst_element_query_duration(playBinPub,GST_FORMAT_TIME,&dur))
            msdur = dur / 1000000;
        else
            msdur = 0;
        emit durationChanged(msdur);
        break;
    case GST_MESSAGE_EOS:
        qInfo() << objName << " - state change to EndOfMediaState emitted";
        emit stateChanged(EndOfMediaState);
        //emit stateChanged(StoppedState);
        break;
    case GST_MESSAGE_TAG:
        // do nothing
        break;
    case GST_MESSAGE_STREAM_STATUS:
        // do nothing
        break;
    default:
        //g_print("Msg type[%d], Msg type name[%s]\n", GST_MESSAGE_TYPE(message), GST_MESSAGE_TYPE_NAME(message));
        qInfo() << objName << " - Gst msg type: " << GST_MESSAGE_TYPE(message.get()) << " Gst msg name: " << GST_MESSAGE_TYPE_NAME(message.get());
        break;
    }
}

void AudioBackendGstreamer::gstPositionChanged(qint64 position)
{
    qInfo() << "gstPositionChanged(" << position << ") called";
    emit positionChanged(position);
}

void AudioBackendGstreamer::gstDurationChanged(qint64 duration)
{
    emit durationChanged(duration);
}

void AudioBackendGstreamer::gstFastTimerFired()
{

    static qint64 lastPos = 0;
    if (lastState != PlayingState)
    {
        if (lastPos == 0)
            return;
        lastPos = 0;
        emit positionChanged(0);
        return;
    }
    gint64 pos, mspos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (playBin, fmt, &pos))
        mspos = pos / 1000000;
    else
        mspos = 0;
    if (lastPos != mspos)
    {
        lastPos = mspos;
        emit positionChanged(mspos);
    }
}

void AudioBackendGstreamer::faderChangedVolume(double volume)
{
    Q_UNUSED(volume)
    int intVol = volume * 100;
    emit volumeChanged(intVol);
}

void AudioBackendGstreamer::faderStateChanged(AudioFader::FaderState state)
{
    qInfo() << objName << " - Fader state changed to: " << fader->stateToStr(state);
}



GstBusSyncReply AudioBackendGstreamer::busMessageDispatcher(GstBus *bus, GstMessage *message, gpointer userData)
{
  auto myObj = static_cast<AudioBackendGstreamer*>(userData);

  Q_UNUSED(bus);

  auto messagePtr = takeGstMiniObject(message);
  QMetaObject::invokeMethod(myObj, "busMessage", Qt::QueuedConnection, Q_ARG(std::shared_ptr<GstMessage>, messagePtr));

  return GST_BUS_DROP;
}

gboolean AudioBackendGstreamer::gstTimerDispatcher(QObject *qObj)
{
    auto abObj = static_cast<AudioBackendGstreamer*>(qObj);
    QMetaObject::invokeMethod(abObj, "gstFastTimerFired");
    /* call me again */
    return TRUE;
}


void AudioBackendGstreamer::buildPipeline()
{
    qInfo() << objName << " - buildPipeline() called";
    GstAppSinkCallbacks appsinkCallbacks;
    appsinkCallbacks.new_preroll	= &AudioBackendGstreamer::NewPrerollCallback;
    appsinkCallbacks.new_sample		= &AudioBackendGstreamer::NewSampleCallback;
    appsinkCallbacks.eos			= &AudioBackendGstreamer::EndOfStreamCallback;

    if (!gst_is_initialized())
    {
        qInfo() << objName << " - gst not initialized - initializing";
        gst_init(NULL,NULL);
    }
    aConvInput = gst_element_factory_make("audioconvert", NULL);
    aConvPreSplit = gst_element_factory_make("audioconvert", NULL);
    aConvPrePitchShift = gst_element_factory_make("audioconvert", NULL);
    aConvPostPitchShift = gst_element_factory_make("audioconvert", NULL);
    audioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    videoAppSink = gst_element_factory_make("appsink", "videoAppSink");
    rgVolume = gst_element_factory_make("rgvolume", NULL);
    level = gst_element_factory_make("level", "level");
    pitchShifterSoundtouch = gst_element_factory_make("pitch", "pitch");
    pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
    equalizer = gst_element_factory_make("equalizer-10bands", NULL);
    playBin = gst_element_factory_make("playbin", "playBin");
    playBinPub = playBin;
    fltrMplxInput = gst_element_factory_make("capsfilter", "filter");
    fltrEnd = gst_element_factory_make("capsfilter", NULL);
    audioCapsStereo = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, NULL);
    audioCapsMono = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 1, NULL);
    videoCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRx", NULL);
    g_object_set(fltrMplxInput, "caps", audioCapsStereo, NULL);
    g_object_set(videoAppSink, "caps", videoCaps, NULL);
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
#ifdef Q_OS_LINUX
    videoSink = gst_element_factory_make ("xvimagesink", NULL);
    videoSink2 = gst_element_factory_make("xvimagesink", NULL);
#endif
#ifdef Q_OS_WIN
    videoSink = gst_element_factory_make ("directdrawsink", NULL);
    videoSink2 = gst_element_factory_make("directdrawsink", NULL);
#endif
#ifdef Q_OS_MAC
    videoSink = gst_element_factory_make ("osxvideosink", NULL);
    videoSink2 = gst_element_factory_make("osxvideosink", NULL);
#endif
    videoQueue1 = gst_element_factory_make("queue", NULL);
    videoQueue2 = gst_element_factory_make("queue", NULL);
    videoTee = gst_element_factory_make("tee", NULL);
    videoTeePad1 = gst_element_get_request_pad(videoTee, "src_%u");
    videoTeePad2 = gst_element_get_request_pad(videoTee, "src_%u");
    videoQueue1SrcPad = gst_element_get_static_pad(videoQueue1, "sink");
    videoQueue2SrcPad = gst_element_get_static_pad(videoQueue2, "sink");

    videoBin = gst_bin_new("videoBin");
    gst_bin_add_many(GST_BIN(videoBin),videoTee,videoQueue1,videoQueue2,videoSink,videoSink2,NULL);
    gst_pad_link(videoTeePad1,videoQueue1SrcPad);
    gst_pad_link(videoTeePad2,videoQueue2SrcPad);
    gst_element_link(videoQueue1,videoSink);
    gst_element_link(videoQueue2,videoSink2);
//    faderVolumeElement = gst_element_factory_make("volume", "newFaderVolumeElement");
//    fader->setVolumeElement(faderVolumeElement);

//    if (settings->audioDownmix())
//        g_object_set(fltrEnd, "caps", audioCapsMono, NULL);
//    else
//        g_object_set(fltrEnd, "caps", audioCapsStereo, NULL);

    gst_bin_add_many(GST_BIN(customBin), level, aConvInput, aConvPreSplit, rgVolume, volumeElement, equalizer, aConvL, aConvR, fltrMplxInput, tee, queueS, queueM, queueR, queueL, audioMixer, deInterleave, aConvPostMixer, fltrPostMixer, gst_object_ref(faderVolumeElement), NULL);
    gst_element_link(aConvInput, rgVolume);
    gst_element_link(rgVolume, level);
    gst_element_link(level, volumeElement);
    gst_element_link(volumeElement, equalizer);
    gst_element_link(equalizer, faderVolumeElement);
    gst_element_link(faderVolumeElement, tee);
    //gst_element_link(equalizer, tee);

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
 //       qInfo() << objName << " - Pitch shift RubberBand enabled";
 //       qInfo() << objName << " - Also loaded SoundTouch for tempo control";
        m_canChangeTempo = true;
//        gst_bin_add_many(GST_BIN(customBin), aConvPrePitchShift, pitchShifterRubberBand, aConvPostPitchShift, pitchShifterSoundtouch, aConvEnd, faderVolumeElement, audioSink, NULL);
//        gst_element_link_many(fltrPostMixer, aConvPrePitchShift, pitchShifterRubberBand, aConvPostPitchShift, pitchShifterSoundtouch, aConvEnd, faderVolumeElement, audioSink, NULL);
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
//        qInfo() << objName << " - Pitch shifter SoundTouch enabled";
//        gst_bin_add_many(GST_BIN(customBin), aConvPrePitchShift, pitchShifterSoundtouch, aConvPostPitchShift, aConvEnd, faderVolumeElement, audioSink, NULL);
//        gst_element_link_many(fltrPostMixer, aConvPrePitchShift, pitchShifterSoundtouch, aConvPostPitchShift, aConvEnd, faderVolumeElement, audioSink, NULL);
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
//        gst_bin_add_many(GST_BIN(customBin), aConvEnd, audioSink, NULL);
//        gst_element_link_many(fltrPostMixer, aConvEnd, audioSink, NULL);
    }

    // Setup outputs from playBin
//    pad = gst_element_get_static_pad(aConvInput, "sink");
//    ghostPad = gst_ghost_pad_new("sink", pad);
//    gst_pad_set_active(ghostPad, true);
//    gst_element_add_pad(customBin, ghostPad);
//    gst_object_unref(pad);
//    g_object_set(G_OBJECT(playBin), "audio-sink", customBin, NULL);
//    g_object_set(G_OBJECT(playBin), "video-sink", videoAppSink, NULL);

    pad = gst_element_get_static_pad(aConvInput, "sink");
    ghostPad = gst_ghost_pad_new("sink", pad);
    ghostVideoPad = gst_ghost_pad_new("sink", gst_element_get_static_pad(videoTee, "sink"));
    gst_pad_set_active(ghostPad, true);
    gst_pad_set_active(ghostVideoPad,true);
    gst_element_add_pad(customBin, ghostPad);
    gst_element_add_pad(videoBin, ghostVideoPad);
    gst_object_unref(pad);
    g_object_set(G_OBJECT(playBin), "audio-sink", customBin, NULL);
//    g_object_set(G_OBJECT(playBin), "video-sink", videoAppSink, NULL);
    g_object_set(G_OBJECT(playBin), "video-sink", videoBin, NULL);
    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink), videoWinId);
    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink2), videoWinId2);




    g_object_set(G_OBJECT(rgVolume), "album-mode", false, NULL);
    g_object_set(G_OBJECT (level), "message", TRUE, NULL);
    bus = takeGstObject(gst_element_get_bus(playBin));
    gst_bus_set_sync_handler(bus.get(), busMessageDispatcher, this, nullptr);
    gst_app_sink_set_callbacks(GST_APP_SINK(videoAppSink), &appsinkCallbacks, this, (GDestroyNotify)AudioBackendGstreamer::DestroyCallback);
    csource = gst_interpolation_control_source_new ();
    if (!csource)
        qInfo() << objName << " - Error createing control source";
    GstControlBinding *cbind = gst_direct_control_binding_new (GST_OBJECT_CAST(faderVolumeElement), "volume", csource);
    if (!cbind)
        qInfo() << objName << " - Error creating control binding";
    if (!gst_object_add_control_binding (GST_OBJECT_CAST(faderVolumeElement), cbind))
        qInfo() << objName << " - Error adding control binding to volumeElement for fader control";
    g_object_set(csource, "mode", GST_INTERPOLATION_MODE_CUBIC, NULL);
    tv_csource = (GstTimedValueControlSource *)csource;
    setVolume(m_volume);
    slowTimer->start(1000);
   // fastTimer->start(200);
    setOutputDevice(outputDeviceIdx);
    setEqBypass(bypass);
    setEqLevel1(eq1);
    setEqLevel2(eq2);
    setEqLevel3(eq3);
    setEqLevel4(eq4);
    setEqLevel5(eq5);
    setEqLevel6(eq6);
    setEqLevel7(eq7);
    setEqLevel8(eq8);
    setEqLevel9(eq9);
    setEqLevel10(eq10);
    setDownmix(downmix);
    setMuted(m_muted);
    videoMute(m_vidMuted);
    setVolume(m_volume);
    fastTimer->start(40);
    qInfo() << objName << " - buildPipeline() finished";
    GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)customBin, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
}

void AudioBackendGstreamer::destroyPipeline()
{
    qInfo() << objName << " - destroyPipeline() called";
    slowTimer->stop();
    fastTimer->stop();
    gst_bin_remove((GstBin*)customBin, faderVolumeElement);
    if (state() == PlayingState)
        stop(true);
    gst_object_unref(playBin);
    gst_caps_unref(audioCapsMono);
    gst_caps_unref(audioCapsStereo);
    gst_caps_unref(videoCaps);
    gst_object_unref(bus.get());
    gst_object_unref(tv_csource);
    qInfo() << objName << " - destroyPipeline() finished";
}

void AudioBackendGstreamer::resetPipeline()
{
    destroyPipeline();
    buildPipeline();
}

bool AudioBackendGstreamer::canFade()
{
    return true;
}

void AudioBackendGstreamer::fadeOut(bool waitForFade)
{
    gdouble curVolume;
    g_object_get(G_OBJECT(volumeElement), "volume", &curVolume, NULL);
    if (state() != PlayingState)
    {
        qInfo() << objName << " - fadeOut - State not playing, skipping fade and setting volume directly";
        fader->immediateOut();
        return;
    }
    fader->fadeOut(waitForFade);
}

void AudioBackendGstreamer::fadeIn(bool waitForFade)
{
    if (state() != PlayingState)
    {
        qInfo() << objName << " - fadeIn - State not playing, skipping fade and setting volume";
        fader->immediateIn();
        return;
    }
    if (isSilent())
    {
        qInfo() << objName << "- fadeOut - Audio is currently slient, skipping fade and setting volume immediately";
        fader->immediateIn();
        return;
    }
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
    if ((m_currentRmsLevel <= 0.01) && (m_volume > 0) && (!fader->isFading()))
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
        m_hasVideo = true;
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
    downmix = enabled;
 //   qDebug() << objName << " - AudioBackendGstreamer::setDownmix(" << enabled << ") called";
    if (enabled)
        g_object_set(fltrPostMixer, "caps", audioCapsMono, NULL);
    else
        g_object_set(fltrPostMixer, "caps", audioCapsStereo, NULL);
 //   qDebug() << objName << " - AudioBackendGstreamer::setDownmix() completed";
}

void AudioBackendGstreamer::setTempo(int percent)
{
    float tempo = (float)percent / 100.0;
    m_tempo = percent;
    g_object_set(pitchShifterSoundtouch, "tempo", tempo, NULL);
    setPosition(position());
    qInfo() << objName << " - Tempo changed to " << tempo;
}

QStringList AudioBackendGstreamer::getOutputDevices()
{
    return outputDeviceNames;
}

void AudioBackendGstreamer::setOutputDevice(int deviceIndex)
{
    outputDeviceIdx = deviceIndex;
 //   qInfo() << objName << " - Setting output device to device idx: " << outputDeviceIdx;
    gst_element_unlink(aConvEnd, audioSink);
    gst_bin_remove(GST_BIN(customBin), audioSink);
    if (deviceIndex == 0)
    {
//        qInfo() << objName << " - Default device selected";
        //default device selected
        audioSink = gst_element_factory_make("autoaudiosink", "audioSink");;
    }
    else
    {
        audioSink = gst_device_create_element(outputDevices.at(deviceIndex - 1), NULL);
//        qInfo() << objName << " - Non default device selected: " << outputDeviceNames.at(deviceIndex);
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
    gchar *padName = gst_pad_get_name(pad);
    QString name = QString(padName);
    g_free(padName);
    if (name == "src_0")
    {
//        qInfo() << parent->objName << " - Linking deinterleave pad src_0 to queueSinkPadL";
        GstPadLinkReturn result = gst_pad_link(pad, parent->queueSinkPadL);
        switch (result) {
        case GST_PAD_LINK_OK:
   //         qInfo() << parent->objName << " - link succeeded";
            break;
        case GST_PAD_LINK_WRONG_HIERARCHY:
            qInfo() << parent->objName << " - pads have no common grandparent";
            break;
        case GST_PAD_LINK_WAS_LINKED:
            qInfo() << parent->objName << " - pad was already linked";
            break;
        case GST_PAD_LINK_WRONG_DIRECTION:
            qInfo() << parent->objName << " - pads have wrong direction";
            break;
        case GST_PAD_LINK_NOFORMAT:
            qInfo() << parent->objName << " - pads do not have common format";
            break;
        case GST_PAD_LINK_NOSCHED:
            qInfo() << parent->objName << " - pads cannot cooperate in scheduling";
            break;
        case GST_PAD_LINK_REFUSED:
            qInfo() << parent->objName << " - refused for some reason";
            break;
        default:
            qInfo() << parent->objName << " - Unknown return type";
            break;
        }
    }
    if (name == "src_1")
    {
  //      qInfo() << parent->objName << " - Linking deinterleave pad src_1 to queueSinkPadR";
        GstPadLinkReturn result = gst_pad_link(pad, parent->queueSinkPadR);
        switch (result) {
        case GST_PAD_LINK_OK:
 //           qInfo() << parent->objName << " - link succeeded";
            break;
        case GST_PAD_LINK_WRONG_HIERARCHY:
            qInfo() << parent->objName << " - pads have no common grandparent";
            break;
        case GST_PAD_LINK_WAS_LINKED:
            qInfo() << parent->objName << " - pad was already linked";
            break;
        case GST_PAD_LINK_WRONG_DIRECTION:
            qInfo() << parent->objName << " - pads have wrong direction";
            break;
        case GST_PAD_LINK_NOFORMAT:
            qInfo() << parent->objName << " - pads do not have common format";
            break;
        case GST_PAD_LINK_NOSCHED:
            qInfo() << parent->objName << " - pads cannot cooperate in scheduling";
            break;
        case GST_PAD_LINK_REFUSED:
            qInfo() << parent->objName << " - refused for some reason";
            break;
        default:
            qInfo() << parent->objName << " - Unknown return type";
            break;
        }
    }
 //   qInfo() << parent->objName << " - Linking complete";
    parent->setMplxMode(settings->mplxMode());
}

void AudioBackendGstreamer::DestroyCallback(gpointer user_data)
{
    Q_UNUSED(user_data)
}


void AudioBackendGstreamer::setMplxMode(int mode)
{
 //   qInfo() << objName << " - setMplxMode(" << mode << ") called";
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
 //   qInfo() << objName << " - setMplxMode() complete";

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


bool AudioBackendGstreamer::hasVideo()
{
    return m_hasVideo;
}

void AudioBackendGstreamer::fadeInImmediate()
{
    fader->immediateIn();
}

void AudioBackendGstreamer::fadeOutImmediate()
{
    fader->immediateOut();
}
