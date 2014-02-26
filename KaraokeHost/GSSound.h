 #ifndef GSSOUND_H
#define GSSOUND_H

#include <gst/gst.h>
#include <string>

using namespace std;

class GSSound
{
    public:
        /** Default constructor */
        GSSound();
        /** Default destructor */
        virtual ~GSSound();
        void play();
        void stop();
        bool getStopped();
        void pause();
        void unPause();
        bool getPaused();
        void setSrcFile(string filename);
        void setKeyChange(int semitones = 0);
        void enableKeyChanger();
        void disableKeyChanger();
        unsigned int getPosition();
        unsigned int getDuration();
        void setPosition(unsigned int milliseconds);
        bool getPlaying();
        std::string getDurationStr();
        std::string getPositionStr();
        std::string getRemainStr();
        int getKeyChange();
        bool getKeyChangerEnabled();


    protected:
    private:
     GstElement *pbin;
     GstElement *decodebin;
     GstElement *audioconvert;
     GstElement *autoaudiosink;
     GstElement *audioresample;
     GstElement *rgvolume;
     //GstElement *audiosrc;
     GstElement *filesrc;
     GstElement *pitch;
     GstElement *pipeline;
     GstBus *bus;
     string srcfile;
     bool keyChangerEnabled;
     int keychange;
};

#endif // GSSOUND_H
