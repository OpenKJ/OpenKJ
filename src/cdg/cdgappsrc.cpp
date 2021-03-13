#include "cdgappsrc.h"
#include <gst/app/gstappsrc.h>
#include "cdg/cdgfilereader.h"
#include <QMutex>
#include <QDebug>

CdgAppSrc::CdgAppSrc()
{
    m_cdgAppSrc = reinterpret_cast<GstAppSrc*>(gst_element_factory_make("appsrc", "cdgAppSrc"));
    g_object_ref(m_cdgAppSrc);

    auto appSrcCaps = gst_caps_new_simple(
                "video/x-raw",
                "format", G_TYPE_STRING, "RGB8P",
                "width",  G_TYPE_INT, cdg::FRAME_DIM_CROPPED.width(),
                "height", G_TYPE_INT, cdg::FRAME_DIM_CROPPED.height(),
                "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                NULL);

    g_object_set(m_cdgAppSrc, "caps", appSrcCaps, NULL);
    gst_caps_unref(appSrcCaps);

    g_object_set(m_cdgAppSrc, "stream-type", GST_APP_STREAM_TYPE_SEEKABLE, "format", GST_FORMAT_TIME, NULL);
    gst_app_src_set_max_bytes(m_cdgAppSrc, cdg::CDG_IMAGE_SIZE * 200);

    GstAppSrcCallbacks callbacks;
    callbacks.need_data	  = &CdgAppSrc::cb_need_data;
    callbacks.enough_data = &CdgAppSrc::cb_enough_data;
    callbacks.seek_data   = &CdgAppSrc::cb_seek_data;
    gst_app_src_set_callbacks(m_cdgAppSrc, &callbacks, this, nullptr);
}

CdgAppSrc::~CdgAppSrc()
{
    reset();
    g_object_unref(m_cdgAppSrc);
}

GstElement *CdgAppSrc::getSrcElement()
{
    return reinterpret_cast<GstElement*>(m_cdgAppSrc);
}

void CdgAppSrc::reset()
{
    g_appSrcNeedData = false;
    QMutexLocker locker(&m_cdgFileReaderLock);
    delete m_cdgFileReader;
    m_cdgFileReader = nullptr;
}

void CdgAppSrc::load(const QString filename)
{
    QMutexLocker locker(&m_cdgFileReaderLock);
    reset();
    m_cdgFileReader = new CdgFileReader(filename);
    gst_app_src_set_duration(m_cdgAppSrc, m_cdgFileReader->getTotalDurationMS() * GST_MSECOND);
}

int CdgAppSrc::positionOfFinalFrameMS()
{
    QMutexLocker locker(&m_cdgFileReaderLock);
    return m_cdgFileReader ? m_cdgFileReader->positionOfFinalFrameMS() : -1;
}

void CdgAppSrc::cb_need_data(GstAppSrc *appsrc, [[maybe_unused]]guint unused_size, gpointer user_data)
{
    auto instance = reinterpret_cast<CdgAppSrc *>(user_data);

    QMutexLocker locker(&instance->m_cdgFileReaderLock);
    if (instance->m_cdgFileReader == nullptr) return;

    instance->g_appSrcNeedData = true;

    while (instance->g_appSrcNeedData)
    {
        if(instance->m_cdgFileReader->moveToNextFrame())
        {
            auto buffer = gst_buffer_new_and_alloc(cdg::CDG_IMAGE_SIZE);
            gst_buffer_fill(buffer,
                            0,
                            instance->m_cdgFileReader->currentFrame().data(),
                            cdg::CDG_IMAGE_SIZE
                            );

            GST_BUFFER_PTS(buffer) = instance->m_cdgFileReader->currentFramePositionMS() * GST_MSECOND;
            GST_BUFFER_DURATION(buffer) = instance->m_cdgFileReader->currentFrameDurationMS() * GST_MSECOND;

            auto rc = gst_app_src_push_buffer(appsrc, buffer);

            if (rc != GST_FLOW_OK)
            {
                qWarning() << "push buffer returned non-OK status: " << rc;
                break;
            }
        }
        else
        {
            gst_app_src_end_of_stream(appsrc);
            return;
        }
    }
}

void CdgAppSrc::cb_enough_data([[maybe_unused]]GstAppSrc *appsrc, [[maybe_unused]]gpointer user_data)
{
    auto instance = reinterpret_cast<CdgAppSrc *>(user_data);
    instance->g_appSrcNeedData = false;
}

gboolean CdgAppSrc::cb_seek_data([[maybe_unused]]GstAppSrc *appsrc, guint64 position, [[maybe_unused]]gpointer user_data)
{
    qDebug() << "Got seek request to position " << position / GST_MSECOND << " ms.";

    auto instance = reinterpret_cast<CdgAppSrc *>(user_data);

    QMutexLocker locker(&instance->m_cdgFileReaderLock);
    if (instance->m_cdgFileReader == nullptr) return false;

    return instance->m_cdgFileReader->seek(position / GST_MSECOND);
}
