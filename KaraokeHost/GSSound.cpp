#include "GSSound.h"
#include <iostream>
#include <math.h>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

#define STUP 1.0594630943592952645618252949461
#define STDN 0.94387431268169349664191315666784

string mstostring(int ms)
{
        int minutes = (ms / 1000) / 60;
        int seconds = (ms / 1000) % 60;
        string fseconds;
        if (seconds < 10)
        {
                fseconds = "0" + lexical_cast<string>(seconds);
        }
        else
        {
                fseconds = lexical_cast<string>(seconds);
        }
        return lexical_cast<string>(minutes) + ":" + fseconds;
}

float getPitchForSemitone(int semitone)
{
    float pitch;
    if (semitone > 0)
    {
        // shifting up
        pitch = pow(STUP,semitone);
    }
    else if (semitone < 0){
        // shifting down
        pitch = 1 - ((100 - (pow(STDN,abs(semitone)) * 100)) / 100);
    }
    else
    {
        // no change
        pitch = 1.0;
    }
    return pitch;
}


GSSound::GSSound()
{
    //ctor
    gst_init(NULL,NULL);
    pipeline = gst_pipeline_new("audio-player");
    decodebin = gst_element_factory_make("mad", "decodebin");
    audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    autoaudiosink = gst_element_factory_make("autoaudiosink", "autoaudiosink");
    audioresample = gst_element_factory_make("audioresample", "audioresample");
    rgvolume = gst_element_factory_make("rgvolume", "rgvolume");
    //audiosrc = gst_element_factory_make("autoaudiosrc", "audiosrc");
    filesrc = gst_element_factory_make("filesrc", "filesrc");
    pitch = gst_element_factory_make("pitch", "pitch");

    gst_bin_add_many(GST_BIN (pipeline), filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);
//    gst_bin_add_many(GST_BIN (pipeline), filesrc, decodebin, audioconvert, rgvolume, audioresample, autoaudiosink, NULL);
    gst_element_link_many(filesrc, decodebin, audioconvert, rgvolume, audioresample, autoaudiosink, NULL);
//    gst_element_link_many(filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);
    //disableKeyChanger();
    keyChangerEnabled = false;
}

GSSound::~GSSound()
{
    //dtor
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GSSound::enableKeyChanger()
{
    bool whilePlaying = getPlaying();
    gint64 pos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (!keyChangerEnabled)
    {
        if (whilePlaying)
        {
            gst_element_query_position (pipeline, &fmt, &pos);
            cout << "Stopping at position: " << pos  << endl;
            stop();
        }
        gst_element_unlink_many(filesrc, decodebin, audioconvert, rgvolume, audioresample, autoaudiosink, NULL);
        gst_element_link_many(filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);
        keyChangerEnabled = true;
        if (whilePlaying)
        {
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
            while (!getPlaying())
            {
                cout << "Waiting for playback start" << endl;
                sleep(1);
            }
            cout << "seeking back to position: " << pos << endl;
            gst_element_seek_simple(pipeline, fmt, GST_SEEK_FLAG_FLUSH, pos);

        }
    }
}

void GSSound::disableKeyChanger()
{
    bool whilePlaying = getPlaying();
    gint64 pos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (keyChangerEnabled)
    {
        if (whilePlaying)
        {
            gst_element_query_position (pipeline, &fmt, &pos);
            cout << "Stopping at position: " << pos  << endl;
            stop();
        }
        gst_element_unlink_many(filesrc, decodebin, audioconvert, rgvolume, pitch, audioresample, autoaudiosink, NULL);
        gst_element_link_many(filesrc, decodebin, audioconvert, rgvolume, audioresample, autoaudiosink, NULL);
        keyChangerEnabled = false;
        if (whilePlaying)
        {
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
            while (!getPlaying())
            {
                cout << "Waiting for playback start" << endl;
                sleep(1);
            }
            cout << "seeking back to position: " << pos << endl;
            gst_element_seek_simple(pipeline, fmt, GST_SEEK_FLAG_FLUSH, pos);

        }
    }
}

void GSSound::play()
{
    cout << "GSSound - play() called" << endl;
//    pbin = gst_element_factory_make("playbin", "pbin");
    g_object_set (G_OBJECT(filesrc), "location", srcfile.c_str(), NULL);

    bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    gst_object_unref (bus);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    cout << "GSSound - play() exit" << endl;
}

void GSSound::stop()
{
    cout << "GSSound - stop() called" << endl;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    cout << "GSSound - stop() exit" << endl;
}

void GSSound::pause()
{
    cout << "GSSound - pause() called" << endl;
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    cout << "GSSound - pause() exit" << endl;
}

void GSSound::unPause()
{
    cout << "GSSound - unPause() called" << endl;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    cout << "GSSound - unPause() exit" << endl;
}

bool GSSound::getPaused()
{
    cout << "GSSound - getPaused() called" << endl;
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
    if (state == GST_STATE_PAUSED)
    {
        cout << "GSSound - getPaused() returned " << true << endl;
        return true;
    }
    else
    {
        cout << "GSSound - getPaused() returned " << false << endl;
        return false;
    }
}

bool GSSound::getPlaying()
{
    //cout << "GSSound - getPlaying() called" << endl;
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
    if (state == GST_STATE_PLAYING)
    {
        //cout << "GSSound - getPlaying() returned " << true << endl;
        return true;
    }
    else
    {
        //cout << "GSSound - getPlaying() returned " << false << endl;
        return false;
    }
}

bool GSSound::getStopped()
{
    cout << "GSSound - getStopped() called" << endl;
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
    if (state == GST_STATE_NULL)
    {
        cout << "GSSound - getStopped() returned " << true << endl;
        return true;
    }
    else
    {
        cout << "GSSound - getStopped() returned " << false << endl;
        return false;
    }
}

void GSSound::setSrcFile(string filename)
{
    srcfile = filename;
    cout << "GSSound - Source set to: " << srcfile << endl;
}

void GSSound::setKeyChange(int semitones)
{
    keychange = semitones;
    float modpitch;
    modpitch = getPitchForSemitone(semitones);
    g_object_set(G_OBJECT (pitch), "pitch", modpitch, "tempo", 1.0, NULL);
}

unsigned int GSSound::getPosition()
{
    gint64 pos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (pipeline, &fmt, &pos))
    {
        //cout << "Position:" << pos / 1000000 << endl;
        return pos / 1000000;
    }
    return 0;
}

std::string GSSound::getPositionStr()
{
    return mstostring(getPosition());
}

std::string GSSound::getDurationStr()
{
    return mstostring(getDuration());
}

std::string GSSound::getRemainStr()
{
    return mstostring(getDuration() - getPosition());
}

int GSSound::getKeyChange()
{
    return keychange;
}

bool GSSound::getKeyChangerEnabled()
{
    return keyChangerEnabled;
}

unsigned int GSSound::getDuration()
{
    gint64 duration;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_duration (pipeline, &fmt, &duration))
    {
        return duration / 1000000;
    }
    return 0;
}
