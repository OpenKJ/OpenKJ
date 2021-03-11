#ifndef GSTREAMERHELPER_H
#define GSTREAMERHELPER_H

#include <qstring.h>
#include <gst/gst.h>

#include <vector>

struct PadInfo
{
    GstElement *element;
    std::string pad;
};

bool gsthlp_is_sink_linked(GstElement *element);

GstElement* gsthlp_get_peer_element(GstElement *element, const gchar* sinkName);

void gsthlp_bin_try_remove(GstBin *bin, std::vector<GstElement *> elements);

PadInfo getPadInfo(GstElement *element, GstPad *pad);

void set_sink_ts_offset(GstBin *bin, gint64 offset);

void optimize_scaleTempo_for_rate(GstElement *scaleTempo, double playBackRate);

#endif // GSTREAMERHELPER_H
