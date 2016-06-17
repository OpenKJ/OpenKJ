/****************************************************************************
*
* NAME: smbPitchShift.cpp
* VERSION: 1.2
* HOME URL: http://www.dspdimension.com
* KNOWN BUGS: none
*
* SYNOPSIS: Routine for doing pitch shifting while maintaining
* duration using the Short Time Fourier Transform.
*
* DESCRIPTION: The routine takes a pitchShift factor value which is between 0.5
* (one octave down) and 2. (one octave up). A value of exactly 1 does not change
* the pitch. numSampsToProcess tells the routine how many samples in indata[0...
* numSampsToProcess-1] should be pitch shifted and moved to outdata[0 ...
* numSampsToProcess-1]. The two buffers can be identical (ie. it can process the
* data in-place). fftFrameSize defines the FFT frame size used for the
* processing. Typical values are 1024, 2048 and 4096. It may be any value <=
* MAX_FRAME_LENGTH but it MUST be a power of 2. osamp is the STFT
* oversampling factor which also determines the overlap between adjacent STFT
* frames. It should at least be 4 for moderate scaling ratios. A value of 32 is
* recommended for best quality. sampleRate takes the sample rate for the signal 
* in unit Hz, ie. 44100 for 44.1 kHz audio. The data passed to the routine in 
* indata[] should be in the range [-1.0, 1.0), which is also the output range 
* for the data, make sure you scale the data accordingly (for 16bit signed integers
* you would have to divide (and multiply) by 32768). 
*
* COPYRIGHT 1999-2006 Stephan M. Bernsee <smb [AT] dspdimension [DOT] com>
*
* 						The Wide Open License (WOL)
*
* Permission to use, copy, modify, distribute and sell this software and its
* documentation for any purpose is hereby granted without fee, provided that
* the above copyright notice and this license appear in all source copies. 
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
* ANY KIND. See http://www.dspguru.com/wol.htm for more information.
*
*****************************************************************************/


#ifndef PITCH_H
#define PITCH_H

#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <fftw3.h>

#define MAX_FRAME_LENGTH 2048
#define D_PI 6.283185f
#define LOG_2  0.693147f

class PitchShifter
{
public:PitchShifter (long fftFrameSize, long osamp, float sampleRate);
   ~PitchShifter ();
  void smbPitchShift (float pitchShift, long numSampsToProcess,
		      long fftFrameSize, long osamp, float sampleRate,
		      float *indata, float *outdata);
  void smbFft (float *fftBuffer, long fftFrameSize, long sign);
  double smbAtan2 (double x, double y);
  float ratio;
private:
  void makeWindow(long fftFrameSize);
  float gInFIFO[MAX_FRAME_LENGTH];
  float gOutFIFO[MAX_FRAME_LENGTH];
  float gFFTworksp[2 * MAX_FRAME_LENGTH];
  float gLastPhase[MAX_FRAME_LENGTH / 2 + 1];
  float gSumPhase[MAX_FRAME_LENGTH / 2 + 1];
  float gOutputAccum[2 * MAX_FRAME_LENGTH];
  float gAnaFreq[MAX_FRAME_LENGTH];
  float gAnaMagn[MAX_FRAME_LENGTH];
  float gSynFreq[MAX_FRAME_LENGTH];
  float gSynMagn[MAX_FRAME_LENGTH];
  double window[MAX_FRAME_LENGTH];
  double dfftFrameSize, coef_dfftFrameSize, dpi_coef;
  double magn, phase, tmp, real, imag;
  double freqPerBin, expct, coefPB, coef_dpi, coef_mpi;
  long k, qpd, index, inFifoLatency, stepSize, fftFrameSize2, gRover, FS_osamp;

  //FFTW variables
     fftw_complex fftw_in[MAX_FRAME_LENGTH], fftw_out[MAX_FRAME_LENGTH];
     fftw_plan ftPlanForward, ftPlanInverse;
};


#endif /*  */

