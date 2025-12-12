// Lets find the memory leak

#include <Adafruit_SSD1306.h>
#include <Adafruit_gfx.h>
#include <Fonts/FreeMono12pt7b.h>
#include <EEncoder.h>
#include <Button2.h>
#include <cmath>

#include "core_shared.h"
#include "images.h"
#include "settings.h"
#include "presets.h"

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

void showDiagnostics();
void showMain();
void showOscSettings();
void showADSR();
void showWaveForm();
void showSettings();
void showPreset();

void onEncoderRotate(EEncoder &enc);

extern const unsigned char *waveTable[];

extern volatile float notePlayArray[];

enum menuState
{
    MAIN_MENU,     // Sekect other menus here
    DIAGNOSTICS,   // Show current pot values and other settings (maybe also what note is playing)
    ADSR_SETTINGS, // Tweak the envolope show a waveform of what it looks like
    WAVEFORM,      // Show waveform of current played note
    OSC_SETTINGS,  // Change osc shape
    SETTINGS,      // Change Settings
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

int settingMenuSelected = 0;
int presetMenuSelected = 0;

volatile float userOctaveOffset = -1.0f;

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
    display.setFont(NULL);

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

        ADSR_FLAG = true;
        adsrSettingSelected++;

        // Wrap around
        if (adsrSettingSelected < 0)
            adsrSettingSelected = adsrSettingMax - 1;
        if (adsrSettingSelected >= adsrSettingMax)
            adsrSettingSelected = 0;

        Serial.print("adsr Setting Item: ");
        Serial.println(adsrSettingSelected);
    }

    if (Menu == SETTINGS && inItem == -1)
    {
        inItem = settingMenuSelected;
    }

    if (Menu == SETTINGS && inItem > -1)
    {
        settings[settingMenuSelected].effectActive = !settings[settingMenuSelected].effectActive;
    }

    if (Menu == PRESETS)
    {
        applyPreset(presetMenuSelected);
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
        ADSR_FLAG = true;
        adsrSettingSelected++;

        // Wrap around
        if (adsrSettingSelected < 0)
            adsrSettingSelected = adsrSettingMax - 1;
        if (adsrSettingSelected >= adsrSettingMax)
            adsrSettingSelected = 0;

        Serial.print("adsr Setting Item: ");
        Serial.println(adsrSettingSelected);
    }

    if (Menu == SETTINGS && inItem == -1)
    {
        inItem = settingMenuSelected;
    }

    if (Menu == SETTINGS && inItem > -1)
    {
        settings[settingMenuSelected].effectActive = !settings[settingMenuSelected].effectActive;
    }

    if (Menu == PRESETS)
    {
        applyPreset(presetMenuSelected);
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

    if (Menu == SETTINGS && inItem > -1)
    {
        inItem = -1;
        return;
    }

    if (Menu == SETTINGS)
    {
        applySettings();
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

    if (Menu == SETTINGS && inItem > -1)
    {
        inItem = -1;
        return;
    }

    Menu = MAIN_MENU;
}

long lastRefresh = micros();

void loop1()
{

    if (micros() - lastRefresh > 16666)
    { // yeah this fixes issue below. theres 2 hours ill never get back

        updateScreen(); // i think calling this to much is messing with the encoders
        lastRefresh = micros();
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

    case DIAGNOSTICS:
        showDiagnostics();
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
    case SETTINGS:
        showSettings();
        break;
    case PRESETS:
        showPreset();
        break;
    default:
        showMain();
        break;
    }

    display.drawBitmap(0, 0, menuHeader, 128, 16, 1); // header

    display.fillRect(95, 6, 10, 3, 0); // volume on header
    display.fillRect(95, 6, (int)10 * masterVol, 3, 1);

    display.drawBitmap(112, 0, waveTable[static_cast<int>(wave)], 16, 16, 1); // fun hack to draw wave type

    display.display();
}

void showDiagnostics()
{

    display.drawBitmap(0, 0, diagnosticsMenu, 128, 64, 1);

    display.drawBitmap(100, 47 - (int)27 * (pitchOffset - 12.0f * (userOctaveOffset + 1)) / 24, slider, 8, 8, 1);

    if (pitchOffset < 0)
    {
        display.drawBitmap(96, 26, minus, 3, 5, 1);
    }
    else
    {
        display.drawBitmap(96, 26, plus, 3, 5, 1);
    }
    display.drawBitmap(100, 26, numbers[abs((int)pitchOffset / 10)], 3, 5, 1);
    display.drawBitmap(104, 26, numbers[abs((int)pitchOffset % 10)], 3, 5, 1);

    display.drawBitmap(114, 26, numbers[(int)round(LFO_FREQ)], 3, 5, 1);
    display.drawBitmap(116, 59 - (int)27 * vibDepth, slider, 8, 8, 1);

    display.drawBitmap(84, 59 - (int)27 * masterVol, slider, 8, 8, 1);

    int normalizedVol = masterVol * 100;
    display.drawBitmap(80, 26, numbers[(int)round(normalizedVol / 100)], 3, 5, 1);
    if ((int)round(normalizedVol / 10) > 9)
    {
        display.drawBitmap(84, 26, numbers[0], 3, 5, 1);
    }
    else
        display.drawBitmap(84, 26, numbers[(int)round(normalizedVol / 10)], 3, 5, 1);
    display.drawBitmap(88, 26, numbers[((int)round(normalizedVol / 10)) % 10], 3, 5, 1);

    // note names
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (notePlayArray[i] == -1)
            continue;

        float freq = notePlayArray[i];
        bool sharpFlag = false;

        int notesFromC1 = round(12 * log2(freq / 32.7f)) + pitchOffset;
        int letterIndex = 0;

        switch (notesFromC1 % 12)
        {
        case 0:
            letterIndex = 2;
            break;
        case 1:
            letterIndex = 2;
            sharpFlag = true;
            break;
        case 2:
            letterIndex = 3;
            break;
        case 3:
            letterIndex = 3;
            sharpFlag = true;
            break;
        case 4:
            letterIndex = 4;
            break;
        case 5:
            letterIndex = 5;
            break;
        case 6:
            letterIndex = 5;
            sharpFlag = true;
            break;
        case 7:
            letterIndex = 6;
            break;
        case 8:
            letterIndex = 6;
            sharpFlag = true;
            break;
        case 9:
            letterIndex = 0;
            break;
        case 10:
            letterIndex = 0;
            sharpFlag = true;
            break;
        case 11:
            letterIndex = 1;
            break;
        }

        display.drawBitmap(10 + (16 * i), 44, numbers[(notesFromC1 / 12) + 1], 3, 5, 1);
        display.drawBitmap(1 + (16 * i), 33, notes[letterIndex], 16, 16, 1);

        // Draw accidental
        if (sharpFlag)
        {
            display.drawBitmap(9 + (16 * i), 34, sharp, 8, 8, 1);
        }
    }
}

void showMain()
{

    display.drawBitmap(0, 0, mainMenu, 128, 64, 1);

    display.drawBitmap(0, 0, mainMenu, 128, 64, 1);

    for (int i = 0; i < menuItemsNum; i++)
    {
        int col = i % 4; // 4 icons per row
        int row = i / 4; // 2 rows

        int x = 7 + col * 32;
        int y = 20 + row * 24;

        if (i == mainMenuItem)
        {
            display.fillRoundRect(x, y, 16, 16, 2, 1);
            display.drawBitmap(x, y, menuItems[i], 16, 16, 0);
        }
        else
        {
            display.drawBitmap(x, y, menuItems[i], 16, 16, 1);
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

    float totalTime = adsrSettings.attackTime + adsrSettings.decayTime + adsrSettings.releaseTime + 500.0f;

    const float scaleX = 124.0f / totalTime;

    const float scaleY = 28.0f;
    const int baseY = 56;
    const int startX = 2;

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
    float sustainX = decayX + 500.0f * scaleX;
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

    int startx = 0;
    int starty = 40;
    int nextx = 0;
    int nexty = 0;

    int idx = waveformIndex;

    for (int i = 0; i < WAVEFORM_SAMPLES; i++)
    {
        nextx++;
        nexty = 40 + (int)((float)128 / 32768 * waveformBuffer[idx]) * (1 / masterVol);
        display.drawLine(startx, starty, nextx, nexty, 1);
        startx = nextx;
        starty = nexty;

        idx = (idx + 1) % WAVEFORM_SAMPLES;
    }
}

void showSettings()
{
    display.drawBitmap(0, 0, settingsMenu, 128, 64, 1);

    if (settingMenuSelected < NUM_SETTINGS)
        showSetting(settingMenuSelected, 1);
    if (settingMenuSelected + 1 < NUM_SETTINGS)
        showSetting(settingMenuSelected + 1, 2);
    if (settingMenuSelected + 2 < NUM_SETTINGS)
        showSetting(settingMenuSelected + 2, 3);
}

void showPreset()
{
    display.drawBitmap(0, 0, settingsMenu, 128, 64, 1);

    if (settingMenuSelected < NUM_SETTINGS)
        showPresets(presetMenuSelected);
    if (presetMenuSelected + 1 < NUM_PRESETS)
        showPresets(presetMenuSelected + 1);
    if (presetMenuSelected + 2 < NUM_PRESETS)
        showPresets(presetMenuSelected + 2);
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
                adsrSettings.decay += increment;
                adsrSettings.sustain += increment;
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

            adsrSettings.attack = constrain(adsrSettings.attack, 1, 255);
            adsrSettings.decay = constrain(adsrSettings.decay, 1, 255);
            adsrSettings.sustain = constrain(adsrSettings.sustain, 1, 255);
            adsrSettings.release = constrain(adsrSettings.release, 1, 255);
        }

        else if (&enc == &encoder2)
        {

            float timeStepSize = 20; // milliseconds

            if (adsrSettingSelected == 0)
                adsrSettings.attackTime += timeStepSize * increment;

            if (adsrSettingSelected == 1)
                adsrSettings.decayTime += timeStepSize * increment;

            if (adsrSettingSelected == 2)
                adsrSettings.sustainTime += timeStepSize * increment;

            if (adsrSettingSelected == 3)
                adsrSettings.releaseTime += timeStepSize * increment;

            adsrSettings.attackTime = constrain(adsrSettings.attackTime, 50, 1000);
            adsrSettings.decayTime = constrain(adsrSettings.decayTime, 50, 1000);
            // hehe adsrSettings.sustainTime = max(50, min(adsrSettings.sustainTime, 2000));
            adsrSettings.releaseTime = constrain(adsrSettings.releaseTime, 50, 1000);
        }
    }

    if (Menu == SETTINGS && inItem == -1)
    {
        settingMenuSelected += increment;

        if (settingMenuSelected < 0)
            settingMenuSelected = NUM_SETTINGS - 1;
        if (settingMenuSelected > NUM_SETTINGS - 1)
            settingMenuSelected = 0;
    }

    if (Menu == SETTINGS && inItem > -1)
    {
        settings[settingMenuSelected].value += increment;

        if (settings[settingMenuSelected].value > settings[settingMenuSelected].max)
            settings[settingMenuSelected].value = 0;
        if (settings[settingMenuSelected].value < 0)
            settings[settingMenuSelected].value = settings[settingMenuSelected].max;
    }

    if (Menu == PRESETS)
    {
        presetMenuSelected += increment;

        if (presetMenuSelected < 0)
            presetMenuSelected = NUM_PRESETS - 1;
        if (presetMenuSelected > NUM_PRESETS - 1)
            presetMenuSelected = 0;
    }
}