#include "khaudiobackendgstreamer.h"
#include <QDebug>

KhAudioBackendGStreamer::KhAudioBackendGStreamer(QObject *parent) :
    KhAbstractAudioBackend(parent)
{
    gst_init(NULL,NULL);
    pipeline = gst_pipeline_new("audio-player");
    decodebin = gst_element_factory_make("decodebin", "decodebin");
    audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    autoaudiosink = gst_element_factory_make("autoaudiosink", "autoaudiosink");
    audioresample = gst_element_factory_make("audioresample", "audioresample");
    rgvolume = gst_element_factory_make("rgvolume", "rgvolume");
    //audiosrc = gst_element_factory_make("autoaudiosrc", "audiosrc");
    filesrc = gst_element_factory_make("filesrc", "filesrc");
    pitch = gst_element_factory_make("pitch", "pitch");

    gst_bin_add_many(GST_BIN (pipeline), filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);
    gst_element_link_many(filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);


}

KhAudioBackendGStreamer::~KhAudioBackendGStreamer()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}


int KhAudioBackendGStreamer::volume()
{
    return 0;
}

qint64 KhAudioBackendGStreamer::position()
{
    gint64 pos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (pipeline, fmt, &pos))
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
    if (gst_element_query_duration (pipeline, fmt, &duration))
    {
        return duration / 1000000;
    }
    return 0;
}

QMediaPlayer::State KhAudioBackendGStreamer::state()
{
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
    if (state == GST_STATE_PLAYING)
        return QMediaPlayer::PlayingState;
    if (state == GST_STATE_PAUSED)
        return QMediaPlayer::PausedState;
    if (state == GST_STATE_NULL)
        return QMediaPlayer::StoppedState;
    return QMediaPlayer::StoppedState;
}

QString KhAudioBackendGStreamer::backendName()
{
    return "GStreamer";
}

bool KhAudioBackendGStreamer::stopping()
{
    return false;
}

void KhAudioBackendGStreamer::play()
{
    qDebug() << "GSSound - play() called";
    g_object_set (G_OBJECT(filesrc), "location", m_filename.toStdString().c_str(), NULL);
    bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    //gst_object_unref (bus);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    qDebug() << "GSSound - play() exit";
}

void KhAudioBackendGStreamer::pause()
{
    qDebug() << "GSSound - pause() called";
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    qDebug() << "GSSound - pause() exit";
}

void KhAudioBackendGStreamer::setMedia(QString filename)
{
    m_filename = filename;
}

void KhAudioBackendGStreamer::setMuted(bool muted)
{
}

void KhAudioBackendGStreamer::setPosition(qint64 position)
{
}

void KhAudioBackendGStreamer::setVolume(int volume)
{
}

void KhAudioBackendGStreamer::stop(bool skipFade)
{
    Q_UNUSED(skipFade);
    qDebug() << "GSSound - stop() called";
    gst_element_set_state(pipeline, GST_STATE_NULL);
    qDebug() << "GSSound - stop() exit";
}

