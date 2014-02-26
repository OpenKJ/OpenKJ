/***************************************************************
 * Name:      CSSound.h
 * Purpose:   Sound abstraction for FMOD backend
 * Author:    T. Isaac Lightburn (isaac@coyotesoft.biz)
 * Created:   2008-06-23
 * Copyright: T. Isaac Lightburn (www.coyotesoft.biz)
 * $Revision: 259 $.
 * $Date: 2012-12-21 11:44:42 -0600 (Fri, 21 Dec 2012) $.
 * $Author: isaac $.
 **************************************************************/

//---------------------------------------------------------------------------

#ifndef CSSoundH
#define CSSoundH
//#include <vcl.h>
#include <string>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

//---------------------------------------------------------------------------
using namespace std;
//using namespace FMOD;

class CSSound {
private:

    FMOD::Sound      *sound;
    FMOD::Channel    *channel;
    FMOD_RESULT      fmresult;
    FMOD::DSP        *dsp;
    FMOD_CREATESOUNDEXINFO exinfo;
    double GetKeyFloat(int keychange);
    bool Opened;
    float Vol;
    void SaveWav(string filename);

public:
    FMOD::System *system;
    unsigned int Length;
    unsigned int  GetPosition();
    CSSound();
    FMOD::System* GetSystem() { return system; }
    void Init(int soundcard = 0, bool mono = true);
    void Release();
    void Play(string SoundFile, int SoundDevice = 0);
    void Stop();
    void Pause();
    void UnPause();
    void SetKey(int Key);
    void SetMono(bool val);
    void RemoveKeyChange();
    bool IsPaused();
    bool IsOpened();
    bool IsPlaying();
    void JumpToPos(int position);
    bool IsKeychanged;
    void SetVolume(int lvl);
    void Mute(bool on);
    int GetVolume();
    float GetOutputLevel();
    bool IsSilent();
    bool bmIsSilent();
    int KeyChange;
    int GetNumDrivers();
    int GetLength(string SoundFile);
    string GetDriverName(int CardID);
};












#endif
