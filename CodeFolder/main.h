#ifndef MAIN_H
#define MAIN_H

#include <Oscil.h>
#include <ADSR.h>

#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/triangle2048_int8.h>

#include <ResonantFilter.h>

extern void updateADSR();
extern void updateOSC();
extern void updateLFO();
extern void scanMatrix();
extern int getFreeVoice();
extern int isButtonPlaying(int button);
extern void setGain(int amount);

extern volatile bool ADSR_FLAG;
extern volatile bool OSC_FLAG;
extern volatile bool LFO_FLAG;

extern volatile float LFO_FREQ;

extern volatile float userOctaveOffset;

extern float GAIN;

extern MultiResonantFilter<uint16_t> mf;

#endif