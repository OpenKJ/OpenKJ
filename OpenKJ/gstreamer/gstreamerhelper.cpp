#include "gstreamerhelper.h"

bool gsthlp_is_sink_linked(GstElement *element)
{
    GstPad *pad = gst_element_get_static_pad(element, "sink");
    bool result = pad && gst_pad_is_linked(pad);
    gst_object_unref(pad);
    return result;
}

GstElement* gsthlp_get_peer_element(GstElement *element, const gchar* sinkName)
{
    GstElement *result = nullptr;
    GstPad *pad = gst_element_get_static_pad(element, sinkName);
    if (pad)
    {
        GstPad *peer = gst_pad_get_peer(pad);
        if (peer)
        {
            result = gst_pad_get_parent_element(peer);
            gst_object_unref(result);
        }
        gst_object_unref(peer);
    }
    gst_object_unref(pad);
    return result;
}


PadInfo getPadInfo(GstElement *element, GstPad *pad)
{
    PadInfo result;
    gchar *name;
    name = gst_pad_get_name(pad);
    result.element = element;
    result.pad = name;
    g_free (name);
    return result;
}

void iterater_set_ts_offset (const GValue *item, gpointer p_offset)
{
    gint64 offset = *(gint64*)p_offset;
    auto element = GST_ELEMENT(g_value_get_object (item));
    g_object_set(element , "ts-offset", offset, nullptr);
}

void set_sink_ts_offset(GstBin *bin, gint64 offset)
{
    auto it = gst_bin_iterate_sinks (bin);
    while (gst_iterator_foreach (it, iterater_set_ts_offset, &offset) == GST_ITERATOR_RESYNC)
    {
        gst_iterator_resync (it);
    }
    gst_iterator_free (it);
}
