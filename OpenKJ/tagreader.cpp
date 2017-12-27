#include "tagreader.h"
#include <QDebug>
#include <tag.h>
#include <taglib/fileref.h>

TagReader::TagReader(QObject *parent) : QObject(parent)
{
    m_duration = 0;
    discoverer = gst_discoverer_new(2 * GST_SECOND, NULL);

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

void TagReader::setMedia(QString path)
{
    qWarning() << "Getting tags for: " << path;
    if ((path.endsWith(".mp3", Qt::CaseInsensitive)) || (path.endsWith(".ogg", Qt::CaseInsensitive)))
    {
        taglibTags(path);
        return;
    }
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
        qWarning() << "Got duration: " << m_duration;
        const GstTagList *tags = gst_discoverer_info_get_tags(discovererInfo);
        if (GST_IS_TAG_LIST(tags))
        {
            gchar *tagVal;
            if (gst_tag_list_get_string(tags,"artist",&tagVal))
            {
                m_artist = tagVal;
                qWarning() << "Got artist tag: " << m_artist;
            }
            if (gst_tag_list_get_string(tags,"title",&tagVal))
            {
                m_title = tagVal;
                qWarning() << "Got title tag: " << m_title;
            }
        }
        else
            qWarning() << "Invalid or missing metadata tags";
        gst_discoverer_info_unref(discovererInfo);
    }
    else
        qWarning() << "Error retreiving discovererInfo";
    qWarning() << "Done getting tags for: " << path;
}

void TagReader::taglibTags(QString path)
{
    TagLib::FileRef f(path.toLocal8Bit().data());
    m_artist = f.tag()->artist().toCString(true);
    m_title = f.tag()->title().toCString(true);
    m_duration = f.audioProperties()->lengthInMilliseconds();
}
