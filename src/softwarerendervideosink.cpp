#include "softwarerendervideosink.h"
#include <QObject>
#include <QPainter>
#include <QResizeEvent>

SoftwareRenderVideoSink::SoftwareRenderVideoSink(QWidget *surface)
{
    m_surface = surface;

    m_appSink = (GstAppSink*)gst_element_factory_make("appsink", nullptr);
    g_object_ref(m_appSink);

    GstAppSinkCallbacks appsinkCallbacks;
    memset(&appsinkCallbacks, 0, sizeof(GstAppSinkCallbacks));
    appsinkCallbacks.new_sample = &SoftwareRenderVideoSink::NewSampleCallback;
    gst_app_sink_set_callbacks(m_appSink, &appsinkCallbacks, this, nullptr);

    // Formats we can handle:
    m_videoCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB16", NULL);   // QImage::Format_RGB16
    gst_caps_append(m_videoCaps, gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRx", NULL));  // QImage::Format_RGB32
    gst_caps_set_simple(m_videoCaps, "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, nullptr);
    g_object_set(m_appSink, "caps", m_videoCaps, NULL);

    // Only store one frame and drop old ones
    gst_app_sink_set_max_buffers (m_appSink, 1);
    gst_app_sink_set_drop(m_appSink, true);

    // Process eos even if there are unread samples.
    // As samples are only read when the widget is painted, samples are not read if the widget is hidden.
    // Without this, the sink will hang/never change state from playing->ready->null.
    gst_app_sink_set_wait_on_eos(m_appSink, false);

    // Allow the sink to send qos messages to pipeline to allow skipping frames
    gst_base_sink_set_qos_enabled(reinterpret_cast<GstBaseSink*>(m_appSink), true);

    m_surface->installEventFilter(this);

    connect(this, SIGNAL(newFrameAvailable()), m_surface, SLOT(update()), Qt::QueuedConnection);
}

SoftwareRenderVideoSink::~SoftwareRenderVideoSink()
{
    m_surface->removeEventFilter(this);
    g_object_unref(m_appSink);
    gst_caps_unref(m_videoCaps);
    m_appSink = nullptr;
}

bool SoftwareRenderVideoSink::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Paint)
    {
        m_pendingRepaint = false;
        if (m_active)
        {
            return pullSampleAndDrawImage();
        }
        else
        {
            return false;
        }
    }
    else if (event->type() == QEvent::Resize)
    {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        onSurfaceResized(resizeEvent->size());
        return false;
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}

void SoftwareRenderVideoSink::onSurfaceResized(const QSize &size)
{
    // Tell what image dimension we can handle and let
    // the Videoscale element earlier in the pipeline do the actual scaline.
    gst_caps_set_simple(m_videoCaps, "width", G_TYPE_INT, size.width(), "height", G_TYPE_INT, size.height(), nullptr);
    gst_app_sink_set_caps(m_appSink, m_videoCaps);
    gst_element_send_event(GST_ELEMENT(m_appSink), gst_event_new_reconfigure());
}

GstFlowReturn SoftwareRenderVideoSink::NewSampleCallback([[maybe_unused]]GstAppSink *appsink, gpointer user_data)
{
    SoftwareRenderVideoSink *me = (SoftwareRenderVideoSink*) user_data;
    me->m_active = true;
    if (!me->m_pendingRepaint)
    {
        me->m_pendingRepaint = true;
        emit me->newFrameAvailable();
    }

    return GST_FLOW_OK;
}

void SoftwareRenderVideoSink::cleanupFunction(void* _info)
{
    SampleInfo *info = static_cast<SampleInfo*>(_info);

    gst_buffer_unmap(info->buffer, info->bufferInfo);
    gst_sample_unref(info->sample);
    delete info->bufferInfo;
    delete info;
}

bool SoftwareRenderVideoSink::pullSampleAndDrawImage()
{
    // Pull sample and paint it. Must be called from gui thread!
    GstSample* sample = gst_app_sink_try_pull_sample(m_appSink, 0);

    if (sample)
    {
        SampleInfo *info = new SampleInfo();
        info->sample = sample;
        info->bufferInfo = new GstMapInfo;
        GstCaps *caps;
        GstStructure *s;
        const gchar *format;
        int width, height;

        caps = gst_sample_get_caps(sample);
        s = gst_caps_get_structure(caps, 0);
        gst_structure_get_int(s, "width", &width);
        gst_structure_get_int(s, "height", &height);
        format = gst_structure_get_string(s, "format");

        info->buffer = gst_sample_get_buffer (sample);

        gst_buffer_map(info->buffer, info->bufferInfo, GST_MAP_READ);
        guint8 *rawFrame = info->bufferInfo->data;

        QImage::Format qtFormat;

        if (strcmp(format, "RGB16") == 0)
            qtFormat = QImage::Format_RGB16;
        else if (strcmp(format, "BGRx") == 0)
            qtFormat = QImage::Format_RGB32;

        QImage frame(rawFrame, width, height, qtFormat, cleanupFunction, info);
        m_buffer = frame;
    }
    else
    {
        // No sample found in queue - are we still playing?
        GstState state = GST_STATE_NULL;
        gst_element_get_state(reinterpret_cast<GstElement*>(m_appSink), &state, nullptr, GST_CLOCK_TIME_NONE);

        if (state == GST_STATE_NULL)
        {
            m_active = false;
            m_buffer = QImage();
        }
    }

    if (!m_buffer.isNull())
    {
        QPainter painter(m_surface);
        painter.drawImage(m_surface->contentsRect(), m_buffer, m_buffer.rect());
        return true;
    }

    return false;
}
