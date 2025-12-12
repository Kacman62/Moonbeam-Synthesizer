#include "presets.h"
#include "settings.h"
#include "main.h"
#include "core_shared.h"

#include <Adafruit_SSD1306.h>
#include <Adafruit_gfx.h>
#include <Fonts/FreeMono12pt7b.h>

extern Adafruit_SSD1306 display;

/*struct synthPreset
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
};*/

synthPreset presets[] = {

    {"Silky Pad", SAW, 3, 2, 0, 0, 1, 1, 0, 0, 35, 20, 256, 600, 180, 60},
    {"Bright Lead", SAW, 3, 1, 3, 0, 3, 0, 0, 0, 35, 5, 256, 120, 180, 80},
    {"Smooth EP", TRIANGLE, 3, 1, 1, 0, 2, 1, 0, 0, 30, 15, 256, 200, 128, 350},
    {"Trance Pluck", SAW, 3, 1, 2, 0, 2, 1, 0, 0, 35, 5, 256, 280, 1, 120},
    {"Warm Bass", SQUARE, 2, 0, 4, 0, 3, 1, 0, 0, 50, 1, 256, 90, 200, 100},
    {"Triangle Bass", TRIANGLE, 2, 0, 1, 0, 3, 1, 0, 0, 45, 2, 256, 120, 180, 80},
    {"Metallic Lead", SAW, 3, 2, 8, 0, 0, 0, 0, 0, 40, 3, 256, 100, 200, 120},
    {"Hard Square Lead", SQUARE, 3, 2, 2, 0, 0, 1, 0, 0, 40, 2, 256, 200, 128, 140},

    {"Warm Pad", SIN, 3, 5, 0, 0, 2, 2, 0, 0, 30, 50, 256, 200, 180, 300},
    {"Dreamy Keys", TRIANGLE, 3, 5, 0, 2, 4, 3, 0, 2, 30, 10, 256, 120, 128, 200},
    {"Lofi Cassette Piano", SAW, 4, 5, 4, 4, 2, 4, 0, 3, 30, 5, 256, 200, 150, 250},
    {"Dirty Bass", SQUARE, 2, 0, 8, 6, 1, 4, 0, 0, 50, 1, 256, 80, 200, 80},
    {"Crunchy Chiptune", SQUARE, 4, 2, 3, 8, 0, 1, 0, 6, 35, 2, 256, 60, 150, 80},
    {"VHS Lead", SAW, 4, 2, 6, 6, 3, 5, 0, 4, 55, 80, 256, 150, 180, 300},
    {"Dusty Pad", SIN, 3, 2, 2, 3, 1, 6, 0, 5, 30, 120, 256, 500, 128, 800},
    {"MPC60 Sampler", SAW, 3, 0, 3, 4, 2, 4, 1, 2, 40, 2, 256, 100, 128, 200}};

const int NUM_PRESETS = sizeof(presets) / sizeof(presets[0]);

void applyPreset(int index)
{

    if (index < 0 || index >= NUM_PRESETS)
        return;

    settings[0].value = presets[index].vibrato;
    settings[1].value = presets[index].lowpass;
    settings[2].value = presets[index].octave;
    // settings[3].value = presets[index].distortion; distortion lowkey broken
    settings[4].value = presets[index].bitcrush;
    settings[5].value = presets[index].gain;
    settings[6].value = presets[index].highpass;
    settings[7].value = presets[index].retroLowpass;
    settings[8].value = presets[index].downsample;

    // your waveform probably stored elsewhere — add this wherever needed:
    wave = presets[index].waveform;

    applySettings();
    /*
        adsrSettings.attackTime = presets[index].a;
        adsrSettings.attack = presets[index].al;
        adsrSettings.decayTime = presets[index].d;
        adsrSettings.decay = presets[index].dl;
        adsrSettings.release = presets[index].dl;
        adsrSettings.releaseTime = presets[index].r;
    */
    ADSR_FLAG = true;
    OSC_FLAG = true;
    LFO_FLAG = true;
}

extern int presetMenuSelected;

void showPresets(int index)
{
    display.setTextColor(WHITE);
    display.setTextSize(1);

    display.setCursor(4, 20 + (12 * (index - presetMenuSelected)));
    display.print(presets[index].name);

    if (index == presetMenuSelected)
    {
        display.drawRoundRect(2, 18 + (12 * (index - presetMenuSelected)), 124, 12, 2, 1);
    }
}
