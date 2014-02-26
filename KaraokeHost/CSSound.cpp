/***************************************************************
 * Author:    T. Isaac Lightburn (isaac@coyotesoft.biz)
 * Created:   2008-06-23
 * Copyright: T. Isaac Lightburn (www.coyotesoft.biz)
 * $Revision: 268 $.
 * $Date: 2012-12-21 14:49:48 -0600 (Fri, 21 Dec 2012) $.
 * $Author: isaac $.
 **************************************************************/

//---------------------------------------------------------------------------
#include "CSSound.h"
#include <iostream>
#include <fstream>
//---------------------------------------------------------------------------
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <QDebug>


//extern string OS;
//extern ofstream dbg;

CSSound::CSSound() {
    Opened = false;
}
void CSSound::Init(int soundcard, bool mono) {
    this->KeyChange = 0;
    fmresult = System_Create(&system);
//    FMOD_System_Create(&bmsystem);
//    if (OS == "Unix") {
#ifdef Q_OS_LINUX
    fmresult = system->setOutput(FMOD_OUTPUTTYPE_PULSEAUDIO);
#endif
    //    }
    if (soundcard != 0) {
        fmresult = system->setDriver(soundcard);
    }
    SetMono(mono);
    fmresult = system->init(2, FMOD_INIT_NORMAL, NULL);
    Vol = 1;

    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
}

void CSSound::SetMono(bool val) {
    FMOD_RESULT result;
    if (val) result = system->setSpeakerMode(FMOD_SPEAKERMODE_MONO);
    else result = system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
    qDebug() << "Setting mono to: " << val << " Result: " << FMOD_ErrorString(result) << endl;
}

int CSSound::GetLength(string filename) {
    int seconds = 0;
    TagLib::FileRef f(filename.c_str());
    if (!f.isNull() && f.audioProperties()) {
        TagLib::AudioProperties *properties = f.audioProperties();
        seconds = properties->length();
    }
    return seconds;
}

void CSSound::Release() {
    if (this->IsKeychanged) {
        dsp->release();
        this->IsKeychanged = false;
    }
    system->close();
    system->release();
}

int CSSound::GetNumDrivers() {
    int cards = 0;
    system->getNumDrivers(&cards);
    return cards;
}

string CSSound::GetDriverName(int CardID) {
    char Name[254];
    system->getDriverInfo(CardID, Name, 256, NULL);
    return string(Name);
}

void CSSound::Play(string SoundFile, int Key) {
    qDebug() << "CSSound::Play called for " << SoundFile.c_str();
    if (IsPlaying()) {
        Stop();
    }
    KeyChange = 0;
    IsKeychanged = false;
    system->createStream(SoundFile.c_str(), FMOD_SOFTWARE | FMOD_DEFAULT | FMOD_2D, 0, &sound);
    Opened = true;
    sound->getLength(&this->Length,1);
    system->playSound(FMOD_CHANNEL_FREE, this->sound, 0, &this->channel);
    channel->setVolume(Vol);
    SetKey(Key);
}

float CSSound::GetOutputLevel() {
    float spectrumarray[64];
    float avglevel = 0;
    float sumlevel = 0;
    int numvalues = 64;
    channel->getSpectrum(spectrumarray,numvalues, 0, FMOD_DSP_FFT_WINDOW_RECT);
    for (int i = 0; i <= 63; i++) {
        sumlevel = sumlevel + spectrumarray[i];
    }
    avglevel = sumlevel / 64;
    return avglevel;
}

bool CSSound::IsSilent() {
#ifndef Q_OS_WIN32
        float levels = GetOutputLevel();
        if (levels > .0001) {
            return false;
        } else {
            return true;
        }
#endif
        // always return false if on a windows platform til I get this fixed for win32
        return false;
}

void CSSound::SetKey(int Key) {
    if (Key != 0) {
        if (!this->IsKeychanged) {
            system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &this->dsp);
            system->addDSP(this->dsp, NULL);
            dsp->setParameter(1, 4096);
            dsp->setParameter(2, 8);
            dsp->setParameter(0,GetKeyFloat(Key));
            this->IsKeychanged = true;
        } else {
            dsp->setParameter(0,GetKeyFloat(Key));
        }
        this->KeyChange = Key;
    } else {
        if (this->IsKeychanged) {
            dsp->release();
            this->IsKeychanged = false;
            this->KeyChange = 0;
        }
    }
}

double CSSound::GetKeyFloat(int keychange) {
    int i;
    double keyfloat = 1.0;
    if (keychange > 0) {
        for (i = 0; i < keychange; i++) {
            keyfloat = keyfloat * 1.05946;
        }
    }
    if (keychange < 0) {
        for (i = 0; i > keychange; i--) {
            keyfloat = keyfloat * 0.9438;
        }
    }
    return keyfloat;
}

bool CSSound::IsPlaying() {
    bool playing = false;
    channel->isPlaying(&playing);
    return playing;
}

void CSSound::Stop() {
    if (Opened) {
        if (this->IsKeychanged) {
            dsp->release();
            this->IsKeychanged = false;
        }
        channel->stop();
        sound->release();
        Opened = false;
    }
    this->Length = 0;
    this->KeyChange = 0;
    this->IsKeychanged = false;
//    this->RecordStop();
}

bool CSSound::IsOpened() {
    return Opened;
}

unsigned int CSSound::GetPosition() {
    unsigned int pos = 0;
    channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
    return pos;
}

bool CSSound::IsPaused() {
    bool paused = false;
    channel->getPaused(&paused);
    return paused;
}

void CSSound::Pause() {
    channel->setPaused(true);
}

void CSSound::UnPause() {
    channel->setPaused(false);
}

void CSSound::SetVolume(int lvl) {
    Vol = lvl * .01;
    channel->setVolume(Vol);
}

int CSSound::GetVolume() {
    float volume;
    channel->getVolume(&volume);
    return (volume * 100);
}

void CSSound::Mute(bool on) {
    static int lastvol;
    if (on) {
        int vol = GetVolume();
        if (vol > 0) lastvol = vol;
        SetVolume(0);
    } else {
        SetVolume(lastvol);
    }
}

void CSSound::JumpToPos(int position) {
    channel->setPosition(position, FMOD_TIMEUNIT_MS);
}
