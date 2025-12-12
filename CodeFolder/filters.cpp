#include "filters.h"
#include "main.h"
#include "settings.h"
#include "core2.h"

int normalizeAmount;

int highBuffer = 0;
int lowBuffer = 0;

int hardClip(int in)
{
    int th = lowBuffer / (1.0f + distortionLevel * 4.0f);
    if (th < 1000)
        th = 1000;

    if (in > th)
        in = th;
    if (in < -th)
        in = -th;

    return in;
}

int tanhClip(int in, float drive)
{

    return (tanh(((float)in / 32767) * distortionLevel) / tanh(distortionLevel)) * 32767;
}

int bitCrush(int in)
{
    int sign = (in < 0) ? -1 : 1;
    int x = abs(in);

    x = (x >> bitCrushLevel) << bitCrushLevel + 1;

    return sign * x;
}

int lpLast = 0;
float lpCoef = 0.1f; // smaller = darker

int lowpass(int in)
{
    lpCoef = 0.1f * LFCutOff;

    lpLast = lpLast + lpCoef * (in - lpLast);
    return lpLast;
}

int hpLast = 0;
int hpOut = 0;
float hpCoef = 0.999f;

int highpass(int in)
{
    hpCoef = 0.999 - .002 * HPCutOff;

    hpOut = hpCoef * (hpOut + in - hpLast);
    hpLast = in;
    return hpOut;
}

int retroLP(int in, int amount)
{
    static int hold = 0;
    hold = hold + ((in - hold) >> amount);
    return hold;
}

int warm(int in)
{
    float x = in / 32767.0f;
    float y = (3.0f * x) / (1.0f + fabs(2.0f * x));
    return (int)(y * 32767);
}

int softLimit(int in)
{
    float x = in / 32767.0f;
    float y = tanh(2.0f * x);
    return (int)(y * 32767);
}

int dsHold = 0;
int dsLast = 0;
int dsCounter = 0;

int downsample(int in, int factor)
{
    if (dsCounter == 0)
    {
        dsLast = dsHold;
        dsHold = in;
    }

    float t = (float)dsCounter / factor;
    int out = (int)(dsLast + (dsHold - dsLast) * t); // linear interpolation

    dsCounter++;
    if (dsCounter >= factor)
        dsCounter = 0;

    return out;
}

int applyEffects(int in)
{

    in *= GAIN;

    if (in > highBuffer)
        highBuffer = in;
    if (in < lowBuffer)
        lowBuffer = in;

    if (HPCutOff > 0)
    {
        in = highpass(in);
    }

    if (distortionLevel > 0)
    {
        in = tanhClip(in, distortionLevel);
    }

    if (LFCutOff > 0)
    {
        in = lowpass(in);
    }

    if (RetroLFCutOff > 0)
    {
        in = retroLP(in, RetroLFCutOff);
    }

    if (downSample > 0)
    {
        in = downsample(in, downSample);
    }

    if (bitCrushLevel > 0)
    {
        in = bitCrush(in);
    }

    return in;
}