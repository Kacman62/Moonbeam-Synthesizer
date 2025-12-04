#include "Wire.h"
#include "pico/multicore.h"
#include "core_shared.h"

#include "MozziConfigValues.h" // for named option values
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_I2S_DAC
#define MOZZI_AUDIO_CHANNELS MOZZI_STEREO
#define MOZZI_AUDIO_BITS 16
#define MOZZI_AUDIO_RATE 32768
#define MOZZI_I2S_PIN_BCK 2
#define MOZZI_I2S_PIN_WS (MOZZI_I2S_PIN_BCK + 1) // HAS TO BE NEXT TO pBCLK, i.e. default is 21
#define MOZZI_I2S_PIN_DATA 18
#define MOZZI_CONTROL_RATE 128 // mozzi rate for updateControl()
#include "Mozzi.h"             // *after* all configuration options, include the main Mozzi headers

#include <Oscil.h>
#include <ADSR.h>

#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/triangle2048_int8.h>

#define VOL_POT A0   // GP26
#define PITCH_POT A1 // GP27
#define VIB_POT A2   // GP28
#define VOL_POT A0   // GP26
#define PITCH_POT A1 // GP27
#define VIB_POT A2   // GP28

volatile float masterVol = 1.0f;
volatile float pitchOffset = 1.0f;
volatile float vibDepth = 1.0f;

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
extern const unsigned char *waveTable[];

Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> lfo(SIN2048_DATA); // LFO for vibrato

int ButtonMatrixRow[NUM_ROWS] = {4, 5, 6, 7, 8};
int ButtonMatrixCol[NUM_COLS] = {9, 10, 11, 12, 13};

float notePlayArray[NUM_VOICES] = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
int notePlayButtons[NUM_VOICES] = {-1, -1, -1, -1, -1};

float noteFreqs[NUM_ROWS * NUM_COLS] = {
    /*261.63, 277.18, 293.66, 311.13, 329.63,
    349.23, 369.99, 392.00, 415.30, 440.00,
    466.16, 493.88, 523.25, 554.37, 587.33,
    622.25, 659.25, 698.46, 739.99, 783.99,
    830.61, 880.00, 932.33, 987.77, 1046.5*/
    261.63, 277.18, 293.66, 311.13, 329.63,
    349.23, 392.00, 369.99, 415.30, 440.00,
    466.16, 523.25, 587.33, 659.25, 493.88,
    739.99, 554.37, 622.25, 698.46, 783.99,
    830.61, 880.00, 932.33, 987.77, 1046.5}; // C4-C6
                                             // Nomalized to my weird button layout

void updateADSR();
void updateOSC();
void scanMatrix();
int getFreeVoice();
int isButtonPlaying(int button);

void setup()
{

  // Blink LED so i know its working (ONLY FOR DEBUGGING)
  // Blink LED so i know its working (ONLY FOR DEBUGGING)
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
  delay(500);
  digitalWrite(0, LOW);
  delay(200);

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

  for (int r = 0; r < NUM_ROWS; r++)
  {
    pinMode(ButtonMatrixRow[r], OUTPUT);
    digitalWrite(ButtonMatrixRow[r], LOW);
  }

  for (int c = 0; c < NUM_COLS; c++)
  {
    pinMode(ButtonMatrixCol[c], INPUT_PULLDOWN);
  }

  analogReadResolution(12); // 0–4095

  startMozzi();
}

const int WAVEFORM_SAMPLES = 128;
volatile int16_t waveformBuffer[WAVEFORM_SAMPLES];
volatile int waveformIndex = 0;

AudioOutput updateAudio()
{
  int32_t mix = 0;

  // must loop through every voice, because no voice stealing or auto adjust voice positions
  for (int i = 0; i < NUM_VOICES; i++)
  {
    if (notePlayButtons[i] < 0)
    {
      continue;
    }
    int32_t oscSample = osc[i].next();
    mix += (int)((oscSample * env[i].next() / 255.0f));
  }

  mix = (mix * masterVol);

  int GAIN = 30;

  mix *= GAIN;

  // Clamp after applying gain
  if (mix > 32767)
    mix = 32767;
  if (mix < -32768)
    mix = -32768;

  /* waveformBuffer[waveformIndex] = mix; // hope this doesnt cause to much overhead. may need to move to update control loop
   waveformIndex = (waveformIndex + 1) % WAVEFORM_SAMPLES; */

  return StereoOutput::from16Bit((int16_t)mix, (int16_t)mix);
}

void loop()
{
  audioHook();
}

void updateControl()
{
  masterVol = analogRead(VOL_POT) / 4095.0;

  float rawSemis = (analogRead(PITCH_POT) / 4095.0f) * 24.0f - 12.0f;
  pitchOffset = roundf(rawSemis);
  float pitchShift = powf(2.0f, pitchOffset / 12.0f);

  if (ADSR_FLAG)
  {
    updateADSR();
  }

  if (OSC_FLAG)
  {
    updateOSC();
  }
  if (OSC_FLAG)
  {
    updateOSC();
  }

  scanMatrix();

  for (int i = 0; i < NUM_VOICES; i++)
  {
    if (notePlayArray[i] < 0)
      continue; // ignore inactive voices

    float baseFreq = notePlayArray[i];

    // final frequency for this voice
    float finalFreq = baseFreq * pitchShift;

    osc[i].setFreq(finalFreq);
  }
}

void scanMatrix()
{

  for (int r = 0; r < NUM_ROWS; r++)
  {
    pinMode(ButtonMatrixRow[r], OUTPUT);
    digitalWrite(ButtonMatrixRow[r], HIGH);

    for (int c = 0; c < NUM_COLS; c++)
    {
      int currentButton = r * NUM_COLS + c;
      int buttonPlaying = isButtonPlaying(currentButton);

      auto state = digitalRead(ButtonMatrixCol[c]);

      int freeVoice = getFreeVoice();

      if (state == HIGH && freeVoice != -1 && buttonPlaying == -1)
      {

        notePlayArray[freeVoice] = noteFreqs[currentButton];
        notePlayButtons[freeVoice] = currentButton;
        env[freeVoice].noteOn();

        osc[freeVoice].setFreq(notePlayArray[freeVoice]);
        /*
                Serial.print("Button pressed "); // remove later to speed up audio
                Serial.println(currentButton);
                Serial.print("Button Playing ");
                Serial.println(buttonPlaying);

                Serial.print("Assigned voice ");
                Serial.println(freeVoice);
                Serial.print("notePlayButtons: ");
                for (int _i = 0; _i < NUM_VOICES; _i++)
                {
                  Serial.print(notePlayButtons[_i]);
                  Serial.print(",");
                }
                Serial.println();

                Serial.print("notePlayArray: ");
                for (int _i = 0; _i < NUM_VOICES; _i++)
                {
                  Serial.print(notePlayArray[_i]);
                  Serial.print(",");
                }
                Serial.println(); */
      }

      if (state == LOW && buttonPlaying > -1)
      {
        notePlayArray[buttonPlaying] = -1.0f;
        notePlayButtons[buttonPlaying] = -1;
        env[freeVoice].noteOff();

        /*
                Serial.print("Button released "); // remove later to speed up audio
                Serial.println(currentButton);
                Serial.print("Button Playing ");
                Serial.println(buttonPlaying);

                Serial.print("Released voice ");
                Serial.println(buttonPlaying);
                Serial.print("notePlayButtons: ");
                for (int _i = 0; _i < NUM_VOICES; _i++)
                {
                  Serial.print(notePlayButtons[_i]);
                  Serial.print(",");
                }
                Serial.println();

                Serial.print("notePlayArray: ");
                for (int _i = 0; _i < NUM_VOICES; _i++)
                {
                  Serial.print(notePlayArray[_i]);
                  Serial.print(",");
                }
                Serial.println(); */
      }

      pinMode(ButtonMatrixCol[c], INPUT_PULLDOWN);
    }
    digitalWrite(ButtonMatrixRow[r], LOW);
  }
}

int getFreeVoice()
{
  for (int i = 0; i < NUM_VOICES; i++)
  {
    if (notePlayArray[i] < 0)
    {
      return i;
    }
  }
  return -1;
}

int isButtonPlaying(int button)
{
  for (int i = 0; i < NUM_VOICES; i++)
  {
    if (notePlayButtons[i] == button)
    {
      return i;
    }
  }
  return -1;
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
