#ifndef GSTREAMERHELPER_H
#define GSTREAMERHELPER_H

#include <qstring.h>
#include <gst/gst.h>

struct PadInfo
{
    GstElement *element;
    std::string pad;
};

bool gsthlp_is_sink_linked(GstElement *element);

GstElement* gsthlp_get_peer_element(GstElement *element, const gchar* sinkName);

PadInfo getPadInfo(GstElement *element, GstPad *pad);

#endif // GSTREAMERHELPER_H
