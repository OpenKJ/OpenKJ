#ifndef CDGAPPSRC_H
#define CDGAPPSRC_H

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <QRecursiveMutex>
#include "cdgfilereader.h"
#include <spdlog/logger.h>

class CdgAppSrc
{

private:
    GstAppSrc *m_cdgAppSrc { nullptr };

    CdgFileReader *m_cdgFileReader { nullptr };
    std::atomic<bool> g_appSrcNeedData { false };
    QRecursiveMutex m_cdgFileReaderLock{};

    // AppSrc callbacks
    static void cb_need_data(GstAppSrc *appsrc, guint unused_size, gpointer user_data);
    static void cb_enough_data(GstAppSrc *appsrc, gpointer user_data);
    static gboolean cb_seek_data(GstAppSrc *appsrc, guint64 position, gpointer user_data);

public:
    std::shared_ptr<spdlog::logger> logger;
    std::string m_loggingPrefix{"[CDGAppSrc]"};
    explicit CdgAppSrc();
    ~CdgAppSrc();

    GstElement* getSrcElement();
    void reset();
    void load(const QString& filename);

    /**
     * Returns the position of the very last frame.
     * This can be less than the total duration, beceause: "total duration = position + duration of final frame".
     *
     * @return The value is not known until all data is read. If so, -1 is returned.
     */
    int positionOfFinalFrameMS();

signals:

};

#endif // CDGAPPSRC_H
