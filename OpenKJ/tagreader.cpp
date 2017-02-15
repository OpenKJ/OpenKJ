#include "tagreader.h"
#include <QDebug>
#include <QThread>

TagReader::TagReader(QObject *parent) : QObject(parent)
{

}

QString TagReader::getArtist()
{
    return m_artist;
}

QString TagReader::getTitle()
{
    return m_title;
}

unsigned int TagReader::getDuration()
{
    return m_duration;
}

static void on_new_pad (GstElement * dec, GstPad * pad, GstElement * fakesink)
{
    Q_UNUSED(dec)
    GstPad *sinkpad;
    sinkpad = gst_element_get_static_pad (fakesink, "sink");
    if (!gst_pad_is_linked (sinkpad)) {
        if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)
            g_error ("Failed to link pads!");
    }
    gst_object_unref (sinkpad);
}

void TagReader::setMedia(QString path)
{
    m_path = path;
    m_duration = 0;
    m_artist = QString();
    m_title = QString();
    GstElement *pipe, *dec, *sink;
    GstMessage *msg;
    std::string uri = "file:///" + m_path.toStdString();

    gst_init (NULL, NULL);
    pipe = gst_pipeline_new ("pipeline");
    dec = gst_element_factory_make ("uridecodebin", NULL);
    g_object_set (dec, "uri", uri.c_str(), NULL);
    gst_bin_add (GST_BIN (pipe), dec);
    sink = gst_element_factory_make ("fakesink", NULL);
    gst_bin_add (GST_BIN (pipe), sink);
    g_signal_connect (dec, "pad-added", G_CALLBACK (on_new_pad), sink);
    gst_element_set_state (pipe, GST_STATE_PLAYING);

    gint64 duration;
    GstFormat fmt = GST_FORMAT_TIME;
    int tries = 0;
    while (m_duration <= 0)
    {
        if (gst_element_query_duration (dec, fmt, &duration))
        {
            m_duration = duration / 1000000;
        }
        else
            QThread::msleep(10);
           // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (tries > 50)
        {
            qWarning() << "Failed to get duration for: " << path;
            break;
        }
        tries++;

    }
    gst_element_set_state(pipe, GST_STATE_PAUSED);
    while (TRUE) {
      GstTagList *tags = NULL;
      msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipe), GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG | GST_MESSAGE_ERROR));
      if (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_TAG) /* error or async_done */
        break;
      gst_message_parse_tag (msg, &tags);
      gchar *tagVal;
      if (gst_tag_list_get_string(tags,"artist",&tagVal))
      {
          m_artist = tagVal;
      }
      if (gst_tag_list_get_string(tags,"title",&tagVal))
      {
          m_title = tagVal;
      }
      gst_tag_list_unref (tags);
      gst_message_unref (msg);
      if ((m_title != "") && (m_artist != ""))
      {
          //break;
      }
    }
    if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
      GError *err = NULL;
      gst_message_parse_error (msg, &err, NULL);
      g_printerr ("Got error: %s\n", err->message);
      g_error_free (err);
    }

    gst_message_unref (msg);
    gst_element_set_state (pipe, GST_STATE_NULL);
    gst_object_unref (pipe);
}
