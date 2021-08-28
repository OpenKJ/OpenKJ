#include "tagreader.h"
#include <tag.h>
#include <taglib/fileref.h>

TagReader::TagReader(QObject *parent) : QObject(parent)
{
    m_logger = spdlog::get("logger");
    m_duration = 0;
    discoverer = gst_discoverer_new(2 * GST_SECOND, nullptr);
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

unsigned int TagReader::getDuration() const
{
    return m_duration;
}

void TagReader::setMedia(const QString& path)
{
    m_logger->info("{} Getting tags for: {}", m_loggingPrefix, path);
    if ((path.endsWith(".mp3", Qt::CaseInsensitive)) || (path.endsWith(".ogg", Qt::CaseInsensitive)) || path.endsWith(".mp4", Qt::CaseInsensitive) || path.endsWith(".m4v", Qt::CaseInsensitive))
    {
        m_logger->info("{} Using taglib to get tags", m_loggingPrefix);
        taglibTags(path);
        return;
    }
    m_logger->info("{} Using GStreamer to get tags", m_loggingPrefix);
    QString uri;
#ifdef Q_OS_WIN
    uri = "file:///" + path;
#else
    uri = "file://" + path;
#endif
    GError *err = nullptr;
    GstDiscovererInfo *discovererInfo = gst_discoverer_discover_uri(discoverer, uri.toUtf8().toPercentEncoding("!$&'()*+,;=:/?[]@").constData(), &err);
    if (err)
    {
        g_printerr("Got error: %s\n", err->message);
        g_error_free(err);
    }
    if (GST_IS_DISCOVERER_INFO(discovererInfo))
    {
        auto duration = gst_discoverer_info_get_duration(discovererInfo);
        m_duration = duration / 1000000;
        m_logger->debug("{} Got duration: {}", m_loggingPrefix, m_duration);
        const GstTagList *tags = gst_discoverer_info_get_tags(discovererInfo);
        if (GST_IS_TAG_LIST(tags))
        {
            gchar *tagVal;
            if (gst_tag_list_get_string(tags,"artist",&tagVal))
            {
                m_artist = tagVal;
                m_logger->info("{} Got artist tag: {}", m_loggingPrefix, m_artist);
            }
            if (gst_tag_list_get_string(tags,"title",&tagVal))
            {
                m_title = tagVal;
                m_logger->info("{} Got title tag: {}", m_loggingPrefix, m_title);
            }
        }
        else
            m_logger->warn("{} Invalid or missing metadata tags", m_loggingPrefix);
        gst_discoverer_info_unref(discovererInfo);
    }
    else
        m_logger->error("{} Error retrieving discovererInfo", m_loggingPrefix);
    m_logger->info("{} Done getting tags for: {}", m_loggingPrefix, path);
}

void TagReader::taglibTags(const QString& path)
{
    TagLib::FileRef f(path.toLocal8Bit().data());
    if (!f.isNull())
    {
        m_artist = f.tag()->artist().toCString(true);
        m_title = f.tag()->title().toCString(true);
        m_duration = f.audioProperties()->length() * 1000;
        m_album = f.tag()->album().toCString(true);
        auto track = f.tag()->track();
        if (track == 0)
            m_track = QString();
        else if (track < 10)
            m_track = "0" + QString::number(track);
        else
            m_track = QString::number(track);
        m_logger->info("{} Taglib result: Artist: {} - Title: {} - Album: {} - Track: {} - Duration: {}", m_loggingPrefix, m_artist, m_title, m_album, m_track, m_duration);
    }
    else
    {
        m_logger->error("{} Taglib was unable to process the specified file", m_loggingPrefix);
        m_artist = QString();
        m_title = QString();
        m_album = QString();
        m_duration = 0;
    }
}
