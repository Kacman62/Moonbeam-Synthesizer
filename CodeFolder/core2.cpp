//
//*********** CORE 2 AREA **************/
#include <Adafruit_SSD1306.h>
#include <Adafruit_gfx.h>
#include <EEncoder.h>
#include <Button2.h>
#include <cmath>

#include "core_shared.h"
#include "images.h"

#define SDA 16
#define SCL 17

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int RotaryButton1 = 15;
EEncoder encoder1(19, 20);
Button2 button1(RotaryButton1);

int RotaryButton2 = 14;
EEncoder encoder2(21, 22);
Button2 button2(RotaryButton2);

long lastPos1;
long lastPos2;

void onButtonClick1(Button2 &btn);
void onButtonClick2(Button2 &btn);
void onButtonDoubleClick1(Button2 &btn);
void onButtonDoubleClick2(Button2 &btn);
void animationIntro();
void openCloseAnimation(bool open, const unsigned char *bitMapOut);
void updateScreen();

void showSettings();
void showMain();
void showOscSettings();
void showADSR();
void showWaveForm();

void onEncoderRotate(EEncoder &enc);

extern const unsigned char PKI_Logo_BW[];
extern const unsigned char MoonBeamSynth[];
extern const unsigned char slider[];
extern const unsigned char menuHeader[];

extern const unsigned char settingsMenu[];
extern const unsigned char mainMenu[];
extern const unsigned char OSCMenu[];
extern const unsigned char ADSRMenu[];
extern const unsigned char waveFormMenu[];

extern const unsigned char *menuItems[];
extern const int menuItemsNum;

extern const unsigned char *notes[];
extern const unsigned char *numbers[];
extern const unsigned char sharp[];
extern const unsigned char flat[];

extern volatile float masterVol;
extern volatile float pitchOffset;
extern volatile float vibDepth;

extern volatile bool OSC_FLAG;
extern volatile bool ADSR_FLAG;

extern const unsigned char *waveTable[];

extern volatile float notePlayArray[];

enum menuState
{
  MAIN_MENU,     // Sekect other menus here
  SETTINGS,      // Show current pot values and other settings (maybe also what note is playing)
  ADSR_SETTINGS, // Tweak the envolope show a waveform of what it looks like
  WAVEFORM,      // Show waveform of current played note
  OSC_SETTINGS,  // Change osc shape and frequency
  PRESETS,       // preset ADSR etc
  LOOPER,        // looper or sequencer probably do this last
  CHORDS         // play chords with single button and change major minor etc.
};

menuState Menu = MAIN_MENU;

extern volatile SharedADSR adsrSettings;

int mainMenuItem = 0;

int oscSettingSelected = 0;
int oscSettingMax = 4;

int adsrSettingSelected = 0;
int adsrSettingMax = 4;

void setup1()
{

  Serial.begin(115200);

  // Blink LED so i know its working (ONLY FOR DEBUGGING)
  pinMode(25, OUTPUT);
  digitalWrite(25, HIGH);
  delay(500);
  digitalWrite(25, LOW);

  Wire.setSDA(SDA);
  Wire.setSCL(SCL);
  Wire.begin();

  delay(50);

  Serial.println("Init OLED...");
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.display();
  Serial.println("OLED init done");

  display.clearDisplay();

  display.drawBitmap(0, 0, PKI_Logo_BW, 127, 64, 1);
  display.display();

  button1.begin(RotaryButton1, INPUT_PULLUP, true);
  button1.setClickHandler(onButtonClick1);
  button1.setDoubleClickHandler(onButtonDoubleClick1);

  button2.begin(RotaryButton2, INPUT_PULLUP, true);
  button2.setClickHandler(onButtonClick2);
  button2.setDoubleClickHandler(onButtonDoubleClick2);

  encoder1.setEncoderHandler(onEncoderRotate);
  encoder2.setEncoderHandler(onEncoderRotate);

  encoder1.setAcceleration(false);
  // encoder1.setAccelerationRate(5);
  encoder2.setAcceleration(false);
  // encoder2.setAccelerationRate(5);

  delay(1000);

  display.clearDisplay();

  display.drawBitmap(0, 0, MoonBeamSynth, 127, 64, 1);
  display.display();

  delay(1500);

  display.clearDisplay();
}

void onButtonClick1(Button2 &btn)
{

  if (Menu == MAIN_MENU)
  {
    Menu = static_cast<menuState>(mainMenuItem + 1); //+1 because 0th item is main menu

    if (mainMenuItem == 1)
    {
      adsrSettingSelected--; // maybe shoulda debounced it or smth
      encoder1.setAcceleration(true);
      encoder1.setAccelerationRate(5);
    }
  }

  if (Menu == OSC_SETTINGS)
  {
    wave = static_cast<OSCState>(oscSettingSelected);
    OSC_FLAG = true;
  }

  if (Menu == ADSR_SETTINGS)
  {
    adsrSettingSelected++;

    // Wrap around
    if (adsrSettingSelected < 0)
      adsrSettingSelected = adsrSettingMax - 1;
    if (adsrSettingSelected >= adsrSettingMax)
      adsrSettingSelected = 0;

    Serial.print("adsr Setting Item: ");
    Serial.println(adsrSettingSelected);
  }
}

void onButtonClick2(Button2 &btn)
{

  if (Menu == MAIN_MENU)
  {
    Menu = static_cast<menuState>(static_cast<int>(Menu) + mainMenuItem + 1);

    if (mainMenuItem == 1)
    {
      adsrSettingSelected--;
      encoder1.setAcceleration(true);
      encoder1.setAccelerationRate(5);
    }
  }

  if (Menu == OSC_SETTINGS)
  {
    OSC_FLAG = true;
    wave = static_cast<OSCState>(static_cast<int>(wave) + oscSettingSelected);
  }

  if (Menu == ADSR_SETTINGS)
  {
    adsrSettingSelected++;

    // Wrap around
    if (adsrSettingSelected < 0)
      adsrSettingSelected = adsrSettingMax - 1;
    if (adsrSettingSelected >= adsrSettingMax)
      adsrSettingSelected = 0;

    Serial.print("adsr Setting Item: ");
    Serial.println(adsrSettingSelected);
  }
}

void onButtonDoubleClick1(Button2 &btn)
{

  if (Menu == ADSR_SETTINGS)
  {
    ADSR_FLAG = true;
    encoder1.setAcceleration(false);
    encoder2.setAcceleration(false);
  }

  Menu = MAIN_MENU;
}

void onButtonDoubleClick2(Button2 &btn)
{

  if (Menu == ADSR_SETTINGS)
  {
    ADSR_FLAG = true;
    encoder1.setAcceleration(false);
    encoder2.setAcceleration(false);
  }

  Menu = MAIN_MENU;
}

long lastRefresh = millis();

void loop1()
{

  if (millis() - lastRefresh > 50)
  { // yeah this fixes issue below. theres 2 hours ill never get back

    updateScreen(); // i think calling this to much is messing with the encoders
    lastRefresh = millis();
  }

  encoder1.update();
  encoder2.update();

  button1.loop();
  button2.loop();
}

void updateScreen()
{

  display.clearDisplay();

  switch (Menu)
  {

  case SETTINGS:
    showSettings();
    break;

  case MAIN_MENU:
    showMain();
    break;

  case OSC_SETTINGS:
    showOscSettings();
    break;

  case ADSR_SETTINGS:
    showADSR();
    break;

  case WAVEFORM:
    showWaveForm();
    break;
  }

  display.drawBitmap(0, 0, menuHeader, 128, 16, 1); // header

  display.fillRect(95, 6, 10, 3, 0); // volume on header
  display.fillRect(95, 6, (int)10 * masterVol, 3, 1);

  display.drawBitmap(112, 0, waveTable[static_cast<int>(wave)], 16, 16, 1); // fun hack to draw wave type

  display.display();
}

void showSettings()
{

  display.drawBitmap(0, 0, settingsMenu, 128, 64, 1);

  // sliders
  display.drawBitmap(84, 59 - (int)27 * pitchOffset, slider, 8, 8, 1); // pitch
  display.drawBitmap(100, 59 - (int)27 * vibDepth, slider, 8, 8, 1);   // vibrato
  display.drawBitmap(116, 59 - (int)27 * masterVol, slider, 8, 8, 1);  // volume

  // note names
  for (int i = 0; i < NUM_VOICES; i++)
  {

    if (notePlayArray[i] == -1)
    {
      break; // this note invalid and all next notes also invalid
    }

    int noteNumber = (int)round((log2((notePlayArray[i] / 261.63))) * 12); // in reference to C4

    if (noteNumber == 1 || noteNumber == 6)
    { // c#/f#
      display.drawBitmap(9 + (16 * i), 44, sharp, 8, 8, 1);
      noteNumber--;
    }
    else if (noteNumber == 3 || noteNumber == 8 || noteNumber == 10)
    { // Db/Eb/Bb
      display.drawBitmap(9 + (16 * i), 44, flat, 8, 8, 1);
      noteNumber++;
    }

    display.drawBitmap(9 + (16 * i), 34, numbers[noteNumber / 12], 8, 8, 1); // number

    noteNumber = (int)(noteNumber * (7 / 12)); // I feel so smart and also know that there is no way this works first try

    display.drawBitmap(1 + (16 * i), 33, notes[noteNumber], 16, 16, 1); // note
  }
}

void showMain()
{

  display.drawBitmap(0, 0, mainMenu, 128, 64, 1);

  for (int i = 0; i < menuItemsNum; i++)
  {

    if (i == mainMenuItem)
    {

      display.fillRoundRect(7 + (32 * i), 23, 16, 16, 2, 1);

      display.drawBitmap(7 + (32 * i), 23, menuItems[i], 16, 16, 0);
    }
    else
    {
      display.drawBitmap(7 + (32 * i), 23, menuItems[i], 16, 16, 1);
    }
  }
}

void showOscSettings()
{

  display.drawBitmap(0, 0, OSCMenu, 128, 64, 1);

  display.drawRect(1 + (oscSettingSelected * 32), 58, 22, 2, 1); // should probably redo this like the main menu later ^^
}

void showADSR()
{

  display.drawBitmap(0, 0, ADSRMenu, 128, 64, 1);

  display.drawRect(3 + (adsrSettingSelected * 32), 24, 24, 2, 1);

  float totalTime = adsrSettings.attackTime + adsrSettings.decayTime + adsrSettings.sustainTime + adsrSettings.releaseTime;

  totalTime = constrain(totalTime, 0.001f, 5.0f);

  const float scaleX = 124.0f / 5.0f;

  const float scaleY = 42.0f; // vertical scale (amplitude height)
  const int baseY = 56;       // bottom of graph (0 amplitude)
  const int startX = 2;       // left margin

  float x = startX;
  float y = baseY;

  // Attack
  float attackX = x + adsrSettings.attackTime * scaleX;
  float attackY = y - (adsrSettings.attack / 256.0f) * scaleY;
  display.drawLine((int)x, (int)y, (int)attackX, (int)attackY, 1);

  // Decay
  float decayX = attackX + adsrSettings.decayTime * scaleX;
  float decayY = baseY - (adsrSettings.sustain / 256.0f) * scaleY;
  display.drawLine((int)attackX, (int)attackY, (int)decayX, (int)decayY, 1);

  // Sustain
  float sustainX = decayX + adsrSettings.sustainTime * scaleX;
  display.drawLine((int)decayX, (int)decayY, (int)sustainX, (int)decayY, 1);

  // Release
  float releaseX = sustainX + adsrSettings.releaseTime * scaleX;
  float releaseY = baseY;
  display.drawLine((int)sustainX, (int)decayY, (int)releaseX, (int)releaseY, 1);
}

extern const int WAVEFORM_SAMPLES;
extern volatile int16_t waveformBuffer[];
extern volatile int waveformIndex;
// bug in here where waveform slides around. fix later
void showWaveForm()
{

  display.drawBitmap(0, 0, waveFormMenu, 128, 64, 1);

  int start_index = waveformIndex + 1;

  for (int i = 0; i < WAVEFORM_SAMPLES - 1; i++)
  {

    int current_buffer_idx = (start_index + i) % WAVEFORM_SAMPLES;
    int next_buffer_idx = (start_index + i + 1) % WAVEFORM_SAMPLES;

    // Get the sample values
    int16_t current_sample = waveformBuffer[current_buffer_idx];
    int16_t next_sample = waveformBuffer[next_buffer_idx];

    Serial.println(current_sample);

    display.drawLine(i, 38 - (current_sample * 0.25f), i + 1, 38 - (next_sample * 0.25f), 1);
  }
}

void onEncoderRotate(EEncoder &enc)
{

  int8_t increment = enc.getIncrement();

  if (Menu == MAIN_MENU)
  {
    // Rotate through main menu items
    mainMenuItem += increment;

    // Wrap around
    if (mainMenuItem < 0)
      mainMenuItem = menuItemsNum - 1;
    if (mainMenuItem >= menuItemsNum)
      mainMenuItem = 0;

    Serial.print("Main Menu Item: ");
    Serial.println(mainMenuItem);
  }

  if (Menu == OSC_SETTINGS)
  {
    oscSettingSelected += increment;

    // Wrap around
    if (oscSettingSelected < 0)
      oscSettingSelected = oscSettingMax - 1;
    if (oscSettingSelected >= oscSettingMax)
      oscSettingSelected = 0;

    Serial.print("Osc Setting Item: ");
    Serial.println(oscSettingSelected);
  }

  if (Menu == ADSR_SETTINGS)
  {

    if (&enc == &encoder1)
    {

      if (adsrSettingSelected == 0)
      {
        adsrSettings.attack += increment;
      }

      if (adsrSettingSelected == 1)
      {
        adsrSettings.decay += increment;   // should all start at same value ill see how it feels and maybe change it later, but without this
        adsrSettings.sustain += increment; // the drawing function breaks
        adsrSettings.release += increment;
      }

      if (adsrSettingSelected == 2)
      {
        adsrSettings.decay += increment;
        adsrSettings.sustain += increment;
        adsrSettings.release += increment;
      }

      if (adsrSettingSelected == 3)
      {
        adsrSettings.decay += increment;
        adsrSettings.sustain += increment;
        adsrSettings.release += increment;
      }

      adsrSettings.attack = max(1, min(adsrSettings.attack, 255)); // probably a function that does this already
      adsrSettings.decay = max(1, min(adsrSettings.decay, 255));
      adsrSettings.sustain = max(1, min(adsrSettings.sustain, 255));
      adsrSettings.release = max(1, min(adsrSettings.release, 255));
    }

    else if (&enc == &encoder2)
    {

      float timeStepSize = 0.05;

      if (adsrSettingSelected == 0)
        adsrSettings.attackTime += timeStepSize * increment;

      if (adsrSettingSelected == 1)
        adsrSettings.decayTime += timeStepSize * increment;

      if (adsrSettingSelected == 2)
        adsrSettings.sustainTime += timeStepSize * increment;

      if (adsrSettingSelected == 3)
        adsrSettings.releaseTime += timeStepSize * increment;

      adsrSettings.attackTime = max(0.05f, min(adsrSettings.attackTime, 1));
      adsrSettings.decayTime = max(0.05f, min(adsrSettings.decayTime, 1));
      adsrSettings.sustainTime = max(0.05f, min(adsrSettings.sustainTime, 2));
      adsrSettings.releaseTime = max(0.05f, min(adsrSettings.releaseTime, 1));
    }
  }
}
