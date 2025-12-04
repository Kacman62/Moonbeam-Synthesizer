#include "Wire.h"
#include "pico/multicore.h"
#include "core_shared.h"

///////
#include <MozziConfigValues.h> // include this first, for named option values

#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_I2S_DAC

#define MOZZI_AUDIO_CHANNELS MOZZI_STEREO // set to stereo mode

#define MOZZI_AUDIO_BITS 16                      // available values are 8, 16 (default), 24 (LEFT ALIGN in 32 bits type!!) and 32 bits
#define MOZZI_I2S_PIN_BCK 2                      // /BLCK) default is 20
#define MOZZI_I2S_PIN_WS (MOZZI_I2S_PIN_BCK + 1) // CANNOT BE CHANGED, HAS TO BE NEXT TO pBCLK, i.e. default is 21
#define MOZZI_I2S_PIN_DATA 18                    // (DOUT) default is 22
#define MOZZI_I2S_FORMAT MOZZI_I2S_FORMAT_PLAIN

#define AUDIO_RATE 16384
#define MOZZI_CONTROL_RATE 128

#include <Mozzi.h> // *after* all configuration options, include the main Mozzi headers

#include <Oscil.h>
#include <ADSR.h>

#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/triangle2048_int8.h>

#define VOL_POT A0   // GP26
#define PITCH_POT A1 // GP27
#define VIB_POT A2   // GP28

volatile float masterVol = 1.0f;
volatile float pitchOffset = 0.0f;
volatile float vibDepth = 0.0f;

#define NUM_COLS 5
#define NUM_ROWS 5

Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> osc[NUM_VOICES] = {
    Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>(SIN2048_DATA),
    Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>(SIN2048_DATA),
    Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>(SIN2048_DATA),
    Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>(SIN2048_DATA),
    Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>(SIN2048_DATA)};

ADSR<CONTROL_RATE, AUDIO_RATE> env[NUM_VOICES];

volatile SharedADSR adsrSettings;

volatile bool ADSR_FLAG = false;
volatile bool OSC_FLAG = false;

volatile OSCState wave = SIN;

extern const unsigned char sinWave[];
extern const unsigned char triWave[];
extern const unsigned char sawWave[];
extern const unsigned char squareWave[];
extern const unsigned char *waveTable[];

Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> lfo(SIN2048_DATA); // LFO for vibrato

int voiceNoteIndex[NUM_VOICES] = {-1, -1, -1, -1, -1};

volatile float notePlayArray[NUM_VOICES] = {-1, -1, -1, -1, -1};

int voicesPlaying = 0;

int ButtonMatrixRow[NUM_ROWS] = {4, 5, 6, 7, 8};
int ButtonMatrixCol[NUM_COLS] = {9, 10, 11, 12, 13};

float noteFreqs[NUM_ROWS * NUM_COLS] = {
    261.63, 277.18, 293.66, 311.13, 329.63,
    349.23, 369.99, 392.00, 415.30, 440.00,
    466.16, 493.88, 523.25, 554.37, 587.33,
    622.25, 659.25, 698.46, 739.99, 783.99,
    830.61, 880.00, 932.33, 987.77, 1046.5}; // C4-C6

void scanMatrix();
void triggerVoice(int);
void releaseVoice(int);
void core1_entry();
void updateADSR();
void updateOSC();

void setup()
{

  // Blink LED so i know its working (ONLY FOR DEBUGGING)
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
  delay(500);
  digitalWrite(0, LOW);
  delay(200);

  for (int r = 0; r < NUM_ROWS; r++)
  {
    pinMode(ButtonMatrixRow[r], OUTPUT);
    digitalWrite(ButtonMatrixRow[r], LOW);
  }
  for (int c = 0; c < NUM_COLS; c++)
  {
    pinMode(ButtonMatrixCol[c], INPUT_PULLDOWN);
  }

  updateADSR();
  updateOSC();

  lfo.setFreq(5.0f);

  digitalWrite(0, HIGH);
  delay(500);
  digitalWrite(0, LOW);
  delay(20);
  digitalWrite(0, HIGH);
  delay(500);
  digitalWrite(0, LOW);
  delay(200);

  analogReadResolution(12); // 0–4095

  startMozzi();
}

const int WAVEFORM_SAMPLES = 128;
volatile int16_t waveformBuffer[WAVEFORM_SAMPLES];
volatile int waveformIndex = 0;

AudioOutput updateAudio()
{
  int32_t mix = 0;
  for (int i = 0; i < voicesPlaying; i++)
  {
    int16_t envVal = env[i].next();
    int16_t oscVal = osc[i].next();
    mix += (oscVal * envVal) >> 8;
  }

  if (voicesPlaying == 0)
  {
    return StereoOutput::from16Bit(0, 0);
  }

  mix = (mix * masterVol) / voicesPlaying; // normalize and apply volume

  // constrain mix to 16 bit range
  mix = constrain(mix, -32767, 32767);

  waveformBuffer[waveformIndex] = mix; // hope this doesnt cause to much overhead. may need to move to update control loop
  waveformIndex = (waveformIndex + 1) % WAVEFORM_SAMPLES;

  return StereoOutput::from16Bit((int16_t)mix, (int16_t)mix);
}

void loop()
{
  audioHook();
}

void updateControl()
{

  masterVol = analogRead(VOL_POT) / 4095.0;
  pitchOffset = analogRead(PITCH_POT) / 4095.0;
  vibDepth = analogRead(VIB_POT) / 4095.0;

  float freq_shift = vibDepth * ((float)lfo.next() / 128.0f);

  for (int i = 0; i < NUM_VOICES; i++)
  {
    if (env[i].playing() && voiceNoteIndex[i] != -1)
    {
      float baseFreq = noteFreqs[voiceNoteIndex[i]];
      osc[i].setFreq(baseFreq * (1.0f + pitchOffset + freq_shift));
    }
  }

  if (ADSR_FLAG)
  {
    updateADSR();
  }

  if (OSC_FLAG)
  {
    updateOSC();
  }

  scanMatrix();
}

bool wasPressed[NUM_ROWS * NUM_COLS] = {false};

void scanMatrix()
{
  for (int r = 0; r < NUM_ROWS; r++)
  {
    digitalWrite(ButtonMatrixRow[r], HIGH); // enable row

    for (int c = 0; c < NUM_COLS; c++)
    {
      int idx = r * NUM_COLS + c;
      bool pressed = digitalRead(ButtonMatrixCol[c]) == HIGH;

      if (pressed && !wasPressed[idx])
      { // note on
        wasPressed[idx] = true;
        triggerVoice(idx);
      }
      else if (!pressed && wasPressed[idx])
      { // note off
        wasPressed[idx] = false;
        releaseVoice(idx);
      }
    }

    digitalWrite(ButtonMatrixRow[r], LOW); // disable row
  }
}

void triggerVoice(int noteIdx)
{
  for (int i = 0; i < NUM_VOICES; i++)
  {
    if (!env[i].playing())
    {
      float freq = noteFreqs[noteIdx] * (1.0f + pitchOffset);
      osc[i].setFreq(freq);

      notePlayArray[i] = freq;

      env[i].noteOn();
      voiceNoteIndex[i] = noteIdx;
      break;
    }
  }

  voicesPlaying++;
}

void releaseVoice(int noteIdx)
{
  for (int i = 0; i < NUM_VOICES; i++)
  {
    // Check if this voice is currently playing the note that was just released
    // This ensures the correct voice's envelope is sent to the release stage.
    if (voiceNoteIndex[i] == noteIdx)
    {
      env[i].noteOff();
      voiceNoteIndex[i] = -1; // Mark voice as free

      notePlayArray[i] = -1;

      break; // Found and released the note, stop searching
    }
  }
  voicesPlaying--;
}

void updateADSR()
{
  for (int i = 0; i < NUM_VOICES; i++)
  {
    env[i].setLevels(adsrSettings.attack, adsrSettings.decay, adsrSettings.sustain, adsrSettings.release);
    env[i].setTimes(adsrSettings.attackTime, adsrSettings.decayTime, adsrSettings.sustainTime, adsrSettings.releaseTime);
  }
  ADSR_FLAG = false;
}

void updateOSC()
{
  for (int i = 0; i < NUM_VOICES; i++)
  {

    switch (wave)
    {

    case SIN:
      osc[i].setTable(SIN2048_DATA);
      break;

    case TRIANGLE:
      osc[i].setTable(TRIANGLE2048_DATA);
      break;

    case SAW:
      osc[i].setTable(SAW2048_DATA);
      break;

    case SQUARE:
      osc[i].setTable(SQUARE_NO_ALIAS_2048_DATA);
      break;
    }
  }
  OSC_FLAG = false;
}
