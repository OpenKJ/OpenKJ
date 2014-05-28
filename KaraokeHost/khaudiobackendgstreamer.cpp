#include "khaudiobackendgstreamer.h"
#include <QApplication>
#include <QDebug>
#include <string.h>
#include <math.h>
#include <gst/app/gstappsink.h>

#define STUP 1.0594630943592952645618252949461
#define STDN 0.94387431268169349664191315666784

int outputChannels;
double currentRMSLevel;

static gboolean message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
    Q_UNUSED(bus);
    Q_UNUSED(data);
    if (message->type == GST_MESSAGE_ELEMENT) {
        const GstStructure *s = gst_message_get_structure (message);
        const gchar *name = gst_structure_get_name (s);
        if (strcmp (name, "level") == 0) {
            gint channels;
            GstClockTime endtime;
            gdouble rms_dB;
            gdouble rms;
            const GValue *array_val;
            const GValue *value;
            GValueArray *rms_arr;
            gint i;
            if (!gst_structure_get_clock_time (s, "endtime", &endtime))
                g_warning ("Could not parse endtime");
            array_val = gst_structure_get_value (s, "rms");
            rms_arr = (GValueArray *) g_value_get_boxed (array_val);
            channels = rms_arr->n_values;
            outputChannels = channels;
            double rmsValues = 0.0;
            for (i = 0; i < channels; ++i) {
                value = g_value_array_get_nth (rms_arr, i);
                rms_dB = g_value_get_double (value);
                rms = pow (10, rms_dB / 20);
                rmsValues = rmsValues + rms;
            }
            currentRMSLevel = rmsValues / channels;
        }
    }
    return TRUE;
}


KhAudioBackendGStreamer::KhAudioBackendGStreamer(QObject *parent) :
    KhAbstractAudioBackend(parent)
{
    gst_init(NULL,NULL);
    guint watch_id;
    m_volume = 0;
    audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    autoaudiosink = gst_element_factory_make("autoaudiosink", "autoaudiosink");
    audioresample = gst_element_factory_make("audioresample", "audioresample");
    rgvolume = gst_element_factory_make("rgvolume", "rgvolume");
    volumeElement = gst_element_factory_make("volume", "volumeElement");
    level = gst_element_factory_make("level", "level");
    pitch = gst_element_factory_make("pitch", "pitch");
    playBin = gst_element_factory_make("playbin", "playBin");
    sinkBin = gst_bin_new("sinkBin");
    gst_bin_add_many(GST_BIN (sinkBin), rgvolume, pitch, audioresample, level, volumeElement, autoaudiosink, NULL);
    gst_element_link_many(rgvolume, audioresample, level, volumeElement, autoaudiosink, NULL);
    pad = gst_element_get_static_pad(rgvolume, "sink");
    ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(sinkBin, ghostPad);
    gst_object_unref(pad);
    g_object_set(G_OBJECT(playBin), "audio-sink", sinkBin, NULL);
    g_object_set(G_OBJECT(pitch), "pitch", 1.0, "tempo", 1.0, NULL);
    g_object_set(G_OBJECT(rgvolume), "album-mode", false, NULL);
    g_object_set(G_OBJECT (level), "post-messages", TRUE, NULL);
    bus = gst_element_get_bus (playBin);
    watch_id = gst_bus_add_watch (bus, message_handler, NULL);


    fader = new FaderGStreamer(this);
    fader->setVolumeElement(volumeElement);
    silenceDetectTimer = new QTimer(this);
    connect(silenceDetectTimer, SIGNAL(timeout()), this, SLOT(silenceDetectTimer_timeout()));
    silenceDetectTimer->start(1000);
    signalTimer = new QTimer(this);
    connect(signalTimer, SIGNAL(timeout()), this, SLOT(signalTimer_timeout()));
    connect(fader, SIGNAL(volumeChanged(int)), this, SLOT(faderChangedVolume(int)));
    signalTimer->start(40);
    m_fade = false;
    m_silenceDetect = false;
    outputChannels = 0;
    currentRMSLevel = 0.0;
    m_keyChangerOn = false;
    m_keyChange = 0;

}

KhAudioBackendGStreamer::~KhAudioBackendGStreamer()
{
}


int KhAudioBackendGStreamer::volume()
{
    return m_volume;
}

qint64 KhAudioBackendGStreamer::position()
{
    gint64 pos;
//    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (playBin, GST_FORMAT_TIME, &pos))
    {
        //cout << "Position:" << pos / 1000000 << endl;
        return pos / 1000000;
    }
    return 0;
}

bool KhAudioBackendGStreamer::isMuted()
{
    return false;
}

qint64 KhAudioBackendGStreamer::duration()
{
    gint64 duration;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_duration (playBin, fmt, &duration))
    {
        return duration / 1000000;
    }
    return 0;
}

KhAbstractAudioBackend::State KhAudioBackendGStreamer::state()
{
    GstState state;
    gst_element_get_state(playBin, &state, NULL, GST_CLOCK_TIME_NONE);
    if (state == GST_STATE_PLAYING)
        return KhAbstractAudioBackend::PlayingState;
    if (state == GST_STATE_PAUSED)
        return KhAbstractAudioBackend::PausedState;
    if (state == GST_STATE_NULL)
        return KhAbstractAudioBackend::StoppedState;
    return KhAbstractAudioBackend::StoppedState;
}

QString KhAudioBackendGStreamer::backendName()
{
    return "GStreamer";
}

bool KhAudioBackendGStreamer::stopping()
{
    return false;
}

void KhAudioBackendGStreamer::keyChangerOn()
{
    KhAbstractAudioBackend::State curstate = state();
    int pos = position();
    qDebug() << "keyChangerOn() fired";
    gst_element_set_state(playBin, GST_STATE_NULL);
    while (state() != KhAbstractAudioBackend::StoppedState)
        QApplication::processEvents();
    gst_element_unlink(rgvolume,audioresample);
    if (!gst_element_link_many(rgvolume,pitch,audioresample,NULL))
        qDebug() << "Failed to link gstreamer elements";
    gst_element_set_state(playBin, GST_STATE_PLAYING);
    while (state() != KhAbstractAudioBackend::PlayingState)
        QApplication::processEvents();
    setPosition(pos);
    if (curstate == KhAbstractAudioBackend::PausedState)
        pause();
    m_keyChangerOn = true;

}

void KhAudioBackendGStreamer::keyChangerOff()
{
    KhAbstractAudioBackend::State curstate = state();
    int pos = position();
    qDebug() << "keyChangerOff() fired";
    gst_element_set_state(playBin, GST_STATE_NULL);
    while (state() != KhAbstractAudioBackend::StoppedState)
        QApplication::processEvents();
    gst_element_unlink(rgvolume,pitch);
    gst_element_unlink(pitch,audioresample);
    if (!gst_element_link(rgvolume,audioresample))
        qDebug() << "Failed to link gstreamer elements";
    gst_element_set_state(playBin, GST_STATE_PLAYING);
    while (state() != KhAbstractAudioBackend::PlayingState)
        QApplication::processEvents();
    setPosition(pos);
    if (curstate == KhAbstractAudioBackend::PausedState)
        pause();
    m_keyChangerOn = false;
}

void KhAudioBackendGStreamer::play()
{
    if (state() == KhAbstractAudioBackend::PausedState)
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

void KhAudioBackendGStreamer::pause()
{
    if (m_fade)
        fadeOut();
    gst_element_set_state(playBin, GST_STATE_PAUSED);
}

void KhAudioBackendGStreamer::setMedia(QString filename)
{
    m_filename = filename;
    std::string uri = "file://" + filename.toStdString();
    qDebug() << "KhAudioBackendGStreamer - Playing: " << uri.c_str();
    g_object_set(GST_OBJECT(playBin), "uri", uri.c_str(), NULL);
}

void KhAudioBackendGStreamer::setMuted(bool muted)
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

void KhAudioBackendGStreamer::setPosition(qint64 position)
{
    qDebug() << "Seeking to: " << position << "ms";
    GstSeekFlags flags = GST_SEEK_FLAG_FLUSH;
    if (!gst_element_seek_simple(playBin, GST_FORMAT_TIME, flags, GST_MSECOND * position))
    {
      qDebug() << "Seek failed!";
    }
    emit positionChanged(position);
}

void KhAudioBackendGStreamer::setVolume(int volume)
{
    m_volume = volume;
    fader->setBaseVolume(volume);
    g_object_set(G_OBJECT(volumeElement), "volume", volume * .01, NULL);
    emit volumeChanged(volume);
}

void KhAudioBackendGStreamer::stop(bool skipFade)
{
    int curVolume = volume();
    if ((m_fade) && (!skipFade) && (state() == KhAbstractAudioBackend::PlayingState))
        fadeOut();
    gst_element_set_state(playBin, GST_STATE_NULL);
    emit stateChanged(KhAbstractAudioBackend::StoppedState);
    if ((m_fade) && (!skipFade))
        setVolume(curVolume);
}

void KhAudioBackendGStreamer::signalTimer_timeout()
{
    static KhAbstractAudioBackend::State currentState;
    if (state() != currentState)
    {
        qDebug() << "audio state changed - old: " << currentState << " new: " << state();
        currentState = state();
        emit stateChanged(currentState);
        if (currentState == KhAbstractAudioBackend::StoppedState)
            stop();
    }
    if(state() == KhAbstractAudioBackend::PlayingState)
    {
        emit positionChanged(position());
    }
//    else if (state() == QMediaPlayer::StoppedState)
    //        stop();
}

void KhAudioBackendGStreamer::silenceDetectTimer_timeout()
{
    static int seconds = 0;
    if (m_silenceDetect)
    {
        if ((state() == KhAbstractAudioBackend::PlayingState) && (isSilent()))
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

void KhAudioBackendGStreamer::faderChangedVolume(int volume)
{
    m_volume = volume;
    emit volumeChanged(volume);
}



bool KhAudioBackendGStreamer::canPitchShift()
{
    return true;
}

int KhAudioBackendGStreamer::pitchShift()
{
    return m_keyChange;
}

gfloat getPitchForSemitone(int semitone)
{
    double pitch;
    if (semitone > 0)
    {
        // shifting up
        pitch = pow(STUP,semitone);
    }
    else if (semitone < 0){
        // shifting down
        pitch = 1 - ((100 - (pow(STDN,abs(semitone)) * 100)) / 100);
    }
    else
    {
        // no change
        pitch = 1.0;
    }
    return pitch;
}

void KhAudioBackendGStreamer::setPitchShift(int pitchShift)
{
    m_keyChange = pitchShift;
    if ((pitchShift == 0) && (m_keyChangerOn))
        keyChangerOff();
    else if ((pitchShift != 0) && (!m_keyChangerOn))
        keyChangerOn();
    qDebug() << "executing g_object_set(GST_OBJECT(pitch), \"pitch\", " << getPitchForSemitone(pitchShift) << ", NULL)";
    g_object_set(G_OBJECT(pitch), "pitch", getPitchForSemitone(pitchShift), "tempo", 1.0, NULL);
}

FaderGStreamer::FaderGStreamer(QObject *parent) :
    QThread(parent)
{
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
    m_targetVolume = m_preOutVolume;
    if (!fading)
    {
        start();
    }
    while(fading)
        QApplication::processEvents();
}

void FaderGStreamer::fadeOut()
{
    m_targetVolume = 0;
    if (!fading)
    {
        fading = true;
        m_preOutVolume = volume();
        start();
    }
    while(fading)
        QApplication::processEvents();
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
        m_preOutVolume = volume;
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




bool KhAudioBackendGStreamer::canFade()
{
    return true;
}

void KhAudioBackendGStreamer::fadeOut()
{
    fader->fadeOut();
}

void KhAudioBackendGStreamer::fadeIn()
{
    fader->fadeIn();
}

void KhAudioBackendGStreamer::setUseFader(bool fade)
{
    m_fade = fade;
}


bool KhAudioBackendGStreamer::canDetectSilence()
{
    return true;
}

bool KhAudioBackendGStreamer::isSilent()
{
    if (currentRMSLevel <= 0.01)
        return true;
    return false;
}

void KhAudioBackendGStreamer::setUseSilenceDetection(bool enabled)
{
    m_silenceDetect = enabled;
}
