#ifndef SOFTWARERENDERVIDEOSINK_H
#define SOFTWARERENDERVIDEOSINK_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <QWidget>


class SoftwareRenderVideoSink : public QObject
{
    Q_OBJECT

private:

    struct SampleInfo
    {
        GstSample *sample;
        GstBuffer *buffer;
        GstMapInfo *bufferInfo;
    };

    std::atomic<bool> m_active {false};
    std::atomic<bool> m_pendingRepaint {false};

    QWidget *m_surface;
    QImage m_buffer;

    void onSurfaceResized(const QSize &size);

    GstAppSink *m_appSink;
    GstCaps *m_videoCaps;

    static GstFlowReturn NewSampleCallback(GstAppSink *appsink, gpointer user_data);
    static void EosCallback(GstAppSink *appsink, void *user_data);
    bool pullFromSinkAndEmitNewVideoFrame();
    static void cleanupFunction(void *info);

signals:
    void newFrameAvailable();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public:
    SoftwareRenderVideoSink(QWidget *surface);
    ~SoftwareRenderVideoSink();
    GstAppSink* getSink() { return m_appSink; }


};

#endif // SOFTWARERENDERVIDEOSINK_H
