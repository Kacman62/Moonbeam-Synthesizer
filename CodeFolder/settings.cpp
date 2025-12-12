#include "settings.h"
#include "main.h"
#include <string>
#include <Adafruit_SSD1306.h>
#include <Adafruit_gfx.h>
#include <Fonts/FreeMono12pt7b.h>

extern Adafruit_SSD1306 display;

extern int settingMenuSelected;

volatile int distortionLevel = 0;
volatile int bitCrushLevel = 0;
volatile int LFCutOff = 0;
volatile int HPCutOff = 0;
volatile int RetroLFCutOff = 0;
volatile int downSample = 0;

int inItem = -1;

void drawWrappedText(int x, int y, int maxWidth, const char *text);

settingSelection settings[] = {
    {"Vibrato rate", "Select the frequency of the vibrato", 5, 0, 9, false},
    {"Lowpass filter", "select level of lowpass filter", 0, 0, 4, false},
    {"Pitch shift octave", "Change the octave of the instrument", 2, 0, 6, false},
    {"Distortion", "Select the level of distortion", 0, 0, 10, false},
    {"Bitcrushing", "Select the level of bitcrush", 0, 0, 8, false},
    {"Gain", "Adjust the gain", 25, 10, 60, false},
    {"Highpass filter", "select level of Highpass filter", 0, 0, 4, false},
    {"Retro Lowpass filter", "select level of lowpass filter", 0, 0, 6, false},
    {"Down Sampling", "select level of down sampling", 0, 0, 16, false}};

const int NUM_SETTINGS = sizeof(settings) / sizeof(settings[0]);

void showSetting(int index, int row)
{

    if (inItem > -1)
    {
        display.fillRoundRect(2, 20, 124, 42, 2, 0);
        display.drawRoundRect(2, 20, 124, 42, 2, 1);
        drawWrappedText(6, 24, 116, settings[inItem].longText);

        if (settings[inItem].value != -1)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d", settings[inItem].min);

            drawWrappedText(6, 50, 116, buffer);
            snprintf(buffer, sizeof(buffer), "%d", settings[inItem].max);

            drawWrappedText(114, 50, 116, buffer);

            display.drawLine(16, 54, 108, 54, 1);
            display.fillRoundRect(16 + 92 / settings[inItem].max * settings[inItem].value, 50, 4, 8, 2, 1);
        }
        if (settings[inItem].value == -1)
        {
            drawWrappedText(6, 50, 116, "Active:");
            display.drawRect(100, 44, 12, 12, 1);

            if (settings[inItem].effectActive)
            {
                display.fillRect(102, 46, 8, 8, 1);
            }
        }
        return;
    }

    display.setTextColor(WHITE);
    display.setTextSize(1);

    if (index == settingMenuSelected)
    {
        display.drawRoundRect(2, 18 + (12 * (index - settingMenuSelected)), 124, 12, 2, 1);
    }
    display.setCursor(4, 20 + (12 * (index - settingMenuSelected)));
    display.printf(settings[index].shortText);
}

void applySettings()
{
    LFO_FREQ = settings[0].value;
    LFO_FLAG = true;

    LFCutOff = settings[1].value;

    userOctaveOffset = settings[2].value - 3;

    distortionLevel = settings[3].value;

    bitCrushLevel = settings[4].value;

    GAIN = settings[5].value;

    HPCutOff = settings[6].value;

    RetroLFCutOff = settings[7].value;

    downSample = settings[8].value;
}

void drawWrappedText(int x, int y, int maxWidth, const char *text)
{
    int cursorX = x;
    int cursorY = y;

    const char *ptr = text;
    char word[32]; // Max word length
    int wi = 0;

    while (*ptr)
    {
        // Build word
        if (*ptr != ' ' && *ptr != '\n')
        {
            word[wi++] = *ptr;
            word[wi] = '\0';
            ptr++;
            continue;
        }

        // Word ready → measure width
        int16_t bx, by;
        uint16_t bw, bh;
        display.getTextBounds(word, cursorX, cursorY, &bx, &by, &bw, &bh);

        // If word doesn't fit → new line
        if (cursorX + bw > x + maxWidth)
        {
            cursorX = x;
            cursorY += 8; // line height for textSize=1
        }

        // Print word
        display.setCursor(cursorX, cursorY);
        display.print(word);

        // Space
        cursorX += bw + 4; // 4px space
        wi = 0;            // reset word buffer

        if (*ptr == '\n')
        {
            cursorX = x;
            cursorY += 8;
        }

        ptr++;
    }

    // Print the last word if any
    if (wi > 0)
    {
        int16_t bx, by;
        uint16_t bw, bh;
        display.getTextBounds(word, cursorX, cursorY, &bx, &by, &bw, &bh);

        if (cursorX + bw > x + maxWidth)
        {
            cursorX = x;
            cursorY += 8;
        }

        display.setCursor(cursorX, cursorY);
        display.print(word);
    }
}
