#ifndef PRESETS_H
#define PRESETS_H

#include <string>
#include "core_shared.h"

struct synthPreset
{
    const char *name;
    OSCState waveform;
    int octave;
    int vibrato;
    int distortion;
    int bitcrush;
    int lowpass;
    int retroLowpass;
    int highpass;
    int downsample;
    int gain;
    int a;
    int al;
    int d;
    int dl;
    int r;
};

extern synthPreset presets[];
extern const int NUM_PRESETS;

void applyPreset(int index);
void showPresets(int index);

#endif
