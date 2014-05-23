#include "khaudiobackendgstreamer.h"
#include <QDebug>

KhAudioBackendGStreamer::KhAudioBackendGStreamer(QObject *parent) :
    KhAbstractAudioBackend(parent)
{
    gst_init(NULL,NULL);
    pipeline = gst_pipeline_new("audio-player");
    decodebin = gst_element_factory_make("mad", "decodebin");
    audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    autoaudiosink = gst_element_factory_make("autoaudiosink", "autoaudiosink");
    audioresample = gst_element_factory_make("audioresample", "audioresample");
    rgvolume = gst_element_factory_make("rgvolume", "rgvolume");
    //audiosrc = gst_element_factory_make("autoaudiosrc", "audiosrc");
    filesrc = gst_element_factory_make("filesrc", "filesrc");
    pitch = gst_element_factory_make("pitch", "pitch");

// Need to research using insertbin for dynamically adding/removing "pitch" to save CPU when it's not in use
    gst_bin_add_many(GST_BIN (pipeline), filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);
    gst_element_link_many(filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);

    signalTimer = new QTimer(this);
    connect(signalTimer, SIGNAL(timeout()), this, SLOT(signalTimer_timeout()));
    signalTimer->start(40);

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
    if (state() == QMediaPlayer::PausedState)
    {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }
    else
    {
        qDebug() << "GSSound - play() called";
        g_object_set (G_OBJECT(filesrc), "location", m_filename.toStdString().c_str(), NULL);
        bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
        //gst_object_unref (bus);
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        qDebug() << "GSSound - play() exit";
    }
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
    qDebug() << "Seeking to: " << position << "ms";
// data->playbin2, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT
    GstSeekFlags flags = GST_SEEK_FLAG_FLUSH;
    if (!gst_element_seek_simple(decodebin, GST_FORMAT_TIME, flags, GST_MSECOND * position))
    {
      qDebug() << "Seek failed!";
    }
    //emit positionChanged(position);
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

void KhAudioBackendGStreamer::signalTimer_timeout()
{
    static QMediaPlayer::State currentState;
    if (state() != currentState)
    {
        qDebug() << "audio state changed - old: " << currentState << " new: " << state();
        currentState = state();
        emit stateChanged(currentState);
        if (currentState == QMediaPlayer::StoppedState)
            stop();
    }
    if(state() == QMediaPlayer::PlayingState)
    {
        emit positionChanged(position());
    }
//    else if (state() == QMediaPlayer::StoppedState)
//        stop();
}

