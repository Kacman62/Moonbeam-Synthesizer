#ifndef CORE_SHARED_H
#define CORE_SHARED_H

enum OSCState
{
    SIN,
    TRIANGLE,
    SAW,
    SQUARE
};

extern volatile OSCState wave;

// Shared ADSR settings
struct SharedADSR
{
    // defaults
    volatile float attack = 255;
    volatile float decay = 190;
    volatile float sustain = 190;
    volatile float release = 100;

    volatile float attackTime = 50; // milliseconds
    volatile float decayTime = 140;
    volatile float sustainTime = 60000;
    volatile float releaseTime = 100;
};

extern volatile SharedADSR adsrSettings;

// 6 is fine, 7 breaks it. leaving it at 5 to give overhead for effects
// to add more voices this is the only thing that needs to change
const int NUM_VOICES = 5;

extern const int WAVEFORM_SAMPLES;

extern volatile float masterVol;
extern volatile float pitchOffset;
extern volatile float vibDepth;

extern volatile bool OSC_FLAG;
extern volatile bool ADSR_FLAG;

extern volatile float LFO_FREQ;

#endif