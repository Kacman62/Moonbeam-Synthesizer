#ifndef SETTINGS_H
#define SETTINGS_H

struct settingSelection
{
    const char *shortText;
    const char *longText;

    int value;
    int min;
    int max;

    bool effectActive;
};

extern settingSelection settings[];
extern void showSetting(int index, int row);
extern void applySettings();
extern int inItem;
extern const int NUM_SETTINGS;

extern volatile int distortionLevel;
extern volatile int bitCrushLevel;

extern volatile int LFCutOff;
extern volatile int HPCutOff;
extern volatile int RetroLFCutOff;
extern volatile int downSample;

#endif