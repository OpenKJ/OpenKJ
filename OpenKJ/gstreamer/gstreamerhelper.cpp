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

void gsthlp_bin_try_remove(GstBin *bin, std::vector<GstElement *> elements)
{
    for (auto &el : elements)
    {
        if (el)
        {
            auto name = gst_element_get_name(el);
            if(el == gst_bin_get_by_name(bin, name))
            {
                gst_bin_remove(bin, el);
            }
            g_free(name);
        }
    }
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

void optimize_scaleTempo_for_rate(GstElement *scaleTempo, double playBackRate)
{
    // Adjust scaleTempo properties to optimize for a given playback rate.
    // Logic from:
    //    https://gitlab.com/soundtouch/soundtouch/-/blob/master/source/SoundTouch/TDStretch.cpp
    // ------------------------------------------------------------------------------------------------
    //
    // Adjust tempo param according to tempo, so that variating processing sequence length is used
    // at various tempo settings, between the given low...top limits
    #define AUTOSEQ_TEMPO_LOW   0.5     // auto setting low tempo range (-50%)
    #define AUTOSEQ_TEMPO_TOP   2.0     // auto setting top tempo range (+100%)

    // sequence-ms setting values at above low & top tempo
    #define AUTOSEQ_AT_MIN      90.0
    #define AUTOSEQ_AT_MAX      40.0
    #define AUTOSEQ_K           ((AUTOSEQ_AT_MAX - AUTOSEQ_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEQ_C           (AUTOSEQ_AT_MIN - (AUTOSEQ_K) * (AUTOSEQ_TEMPO_LOW))

    // seek-window-ms setting values at above low & top tempoq
    #define AUTOSEEK_AT_MIN     20.0
    #define AUTOSEEK_AT_MAX     15.0
    #define AUTOSEEK_K          ((AUTOSEEK_AT_MAX - AUTOSEEK_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEEK_C          (AUTOSEEK_AT_MIN - (AUTOSEEK_K) * (AUTOSEQ_TEMPO_LOW))

    #define CHECK_LIMITS(x, mi, ma) (((x) < (mi)) ? (mi) : (((x) > (ma)) ? (ma) : (x)))

    double seq, seek;
    int strideMS, seekMS;

    seq = AUTOSEQ_C + AUTOSEQ_K * playBackRate;
    seq = CHECK_LIMITS(seq, AUTOSEQ_AT_MAX, AUTOSEQ_AT_MIN);
    strideMS = (int)(seq + 0.5);

    seek = AUTOSEEK_C + AUTOSEEK_K * playBackRate;
    seek = CHECK_LIMITS(seek, AUTOSEEK_AT_MAX, AUTOSEEK_AT_MIN);
    seekMS = (int)(seek + 0.5);

    g_object_set(scaleTempo, "search", seekMS, "stride", strideMS, nullptr);
}
