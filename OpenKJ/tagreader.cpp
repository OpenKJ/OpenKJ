#include "tagreader.h"
#include <QDebug>
#include <tag.h>
#include <taglib/fileref.h>

TagReader::TagReader(QObject *parent) : QObject(parent)
{
    m_duration = 0;
    discoverer = gst_discoverer_new(2 * GST_SECOND, NULL);

}

TagReader::~TagReader()
{
    gst_object_unref(discoverer);
}

QString TagReader::getArtist()
{
    return m_artist;
}

QString TagReader::getTitle()
{
    return m_title;
}

QString TagReader::getAlbum()
{
    return m_album;
}

QString TagReader::getTrack()
{
    return m_track;
}

unsigned int TagReader::getDuration()
{
    return m_duration;
}

void TagReader::setMedia(QString path)
{
    qInfo() << "Getting tags for: " << path;
    if ((path.endsWith(".mp3", Qt::CaseInsensitive)) || (path.endsWith(".ogg", Qt::CaseInsensitive)) || path.endsWith(".mp4", Qt::CaseInsensitive))
    {
        qInfo() << "Using taglib to get tags";
        taglibTags(path);
        return;
    }
    qInfo() << "Using gstreamer to get tags";
    QString uri;
#ifdef Q_OS_WIN
    uri = "file:///" + path;
#else
    uri = "file://" + path;
#endif
    GError *err = NULL;
    GstDiscovererInfo *discovererInfo = gst_discoverer_discover_uri(discoverer, uri.toUtf8().toPercentEncoding("!$&'()*+,;=:/?[]@").constData(), &err);
    if (err)
    {
        g_printerr("Got error: %s\n", err->message);
        g_error_free(err);
    }
    if (GST_IS_DISCOVERER_INFO(discovererInfo))
    {
        gint64 duration = gst_discoverer_info_get_duration(discovererInfo);
        m_duration = duration / 1000000;
        qInfo() << "Got duration: " << m_duration;
        const GstTagList *tags = gst_discoverer_info_get_tags(discovererInfo);
        if (GST_IS_TAG_LIST(tags))
        {
            gchar *tagVal;
            if (gst_tag_list_get_string(tags,"artist",&tagVal))
            {
                m_artist = tagVal;
                qInfo() << "Got artist tag: " << m_artist;
            }
            if (gst_tag_list_get_string(tags,"title",&tagVal))
            {
                m_title = tagVal;
                qInfo() << "Got title tag: " << m_title;
            }
        }
        else
            qInfo() << "Invalid or missing metadata tags";
        gst_discoverer_info_unref(discovererInfo);
    }
    else
        qInfo() << "Error retreiving discovererInfo";
    qInfo() << "Done getting tags for: " << path;
}

void TagReader::taglibTags(QString path)
{
    TagLib::FileRef f(path.toLocal8Bit().data());
    if (!f.isNull())
    {
        m_artist = f.tag()->artist().toCString(true);
        m_title = f.tag()->title().toCString(true);
        m_duration = f.audioProperties()->length() * 1000;
        m_album = f.tag()->album().toCString(true);
        int track = f.tag()->track();
        if (track == 0)
            m_track = QString();
        else if (track < 10)
            m_track = "0" + QString::number(track);
        else
            m_track = QString::number(track);
        qInfo() << "Taglib result - Artist: " << m_artist << " Title: " << m_title << " Album: " << m_album << " Track: " << m_track << " Duration: " << m_duration;
    }
    else
    {
        qWarning() << "Taglib was unable to process the file";
        m_artist = QString();
        m_title = QString();
        m_album = QString();
        m_duration = 0;
    }
}
