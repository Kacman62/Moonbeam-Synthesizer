# MoonBeam Synthesizer
A standalone DAWLESS synthesizer based on a Raspberry Pi Pico. Includes a custom circuit board and codebase with the Mozzi Audio Library

![Promo Image for the Moonbeam Synth](https://github.com/Kacman62/MoonBeam-Midi-Keyboard/blob/main/images/Moonbeam%20Synth%20Promo.png?raw=true)

# Features

- 25 multiplexed piano style keys
- Support for up to 5 notes at the same time
- 3 Analog sliders for volume pitch and vibrato
- OLED display and 2 rotary encoders with a custom code base
- Sound synthesis using the Mozzi audio library
- 4 different wave forms
- Custom ADSR editing
- Oscilloscope style waveform viewer
- Toggleable speaker and headphone out
- And more planned features for the future!

# Initial Idea
After reading the guide from Hack club and others online to make a MIDI keyboard, it seemed like a great idea. After more research though I wanted a standalone synthesizer that does not need a computer and couldn't find many examples online. This seemed like it would be easy enough so I decided to do it myself

# Schematic and Board
The whole project is built around a raspberry pi pico 1 (a pico 2 should work too but untested). A 5x5 multiplexed array of key cap switches serve as the main input. 
On the Audio side, a PCM5100 I2S DAC is channeled into a LM4810 and LM4861 audio amplifiers, that go to a speaker and headphone jack.

![Image showing routing of PCB](https://github.com/Kacman62/MoonBeam-Midi-Keyboard/blob/main/images/PCB.png)

The PCB was a lot more challenging, just due to the sheer size of it. It ends up being about 350mm x 90mm. But the routing wasn't too hard and I like the look of how it turned out

# Case Design
There is an included case. Currently I have not received the board so I can not confirm if it works perfectly, but I designed it around the 3D cad of the board so it should work fine. 
Only 1 of each file needs to be printed and it is assembled using M4 hardware

![Image showing CAD of the case](https://github.com/Kacman62/MoonBeam-Midi-Keyboard/blob/main/images/CadImage.png)

# Software
I initially dreaded the software but it ended up being very fun after the initial hurdles. The code is split into 2 cores, 1 for audio and 1 for the digital UI. It should work with the final assembly, all the digital has been tested, but the audio is using I2S which I can't test currently. I did use it a little bit with PWM audio and that worked fine. The waveform viewer also does show an audio signal so Im pretty confident that it will work

# UI
You navigate the UI with both rotary encoders. Double clicking either brings you into the main menu. Rotate either to select an app.
* Settings (Wrench): View current notes on each audio voice and the values of the slide potentiometers
* ADSR Editor (Envelope): Use one dial to change the level of the ADSR and the other to change the time. Changing the level of the decay, sustain and release will change the other two with it. this is intended to avoid weird audio clipping
* Wave Viewer (Wave): View the current outputted wave form. Automatically adjusts the zoom if multiple voices are playing
* Wave Changer (Square Wave): Choose from either a sin, triangle, saw, or square wave

![Image showing the settings UI](https://github.com/Kacman62/Moonbeam-Synthesizer/blob/main/images/OLEDUI.png)

# Art
All pixel art was created by me using Aseprite. The PK Industries and the Moonbeam synth logo are unregistered trademarks, not real companies and products, and have no legal binding on anything. I am not a lawyer, and this joke has gone on too long.

![Image showing the pixel art for the project](https://github.com/Kacman62/MoonBeam-Midi-Keyboard/blob/main/images/PixelArt.png)

# Disclaimers
If you want to know more about the design and build process check out the journal

This project is sponsored by and made in collaboration with Hack Club, go check them out

Consider this project untested. I have put my full effort into this project and fully believe that it will work, but I have not received boards yet and thus do not know if this will work first try

# Hack Club Stuff

Estimate Total Hours: 48

My Final Total: 60.31

Your Final Total: 96.73

https://blueprint.hackclub.com/projects/928 

# Update 11/11

Huge thanks to Hack Club and the Blueprint program for assisting in this project. I have now received funding for the project and will be ordering parts. Expect future updates at the start of December

# Update 11/25

Boards have been received. Still waiting on LCSC order. Small errors have been found in the board and have been fixed in the PCB files

# Update 12/1

Boards have been assembled and multiple errors have been found. The files will still be up, but assume they do not work.
Nothing shorts together, but both the button matrix is broken, as well as the I2S DAC and amplifier not working.

# Update 12/3

After reflowing the baord, the issue has been fixed. Two of the data lines into the DAC were shorted and after fixing that, it makes sound. The code still has some issues, but they are being fixed rapidly and the current board is almost done.

# Update 12/4 

This project is now shipped. I had a lot of fun working on this, and will continue to update the software, but for now the project is in a working state so expect less updates.

# Known issues

Code: 

In settings, the note played is incorrect (going to wait for boards to arrive so I can assign keys different notes)

In Wave changer, the picture of the sawtooth wave is reversed from what is seen in the wave viewer

In Wave Viewer, the wave slides around randomly. (The wave is still representative of what the audio out looks like, it's just hard to focus on it)

Button scanning matrix is incorrect, every button plays the same 5 notes FIXED 12/3/25

Board:

Slide pot pin spacing 2.00mm instead of 2.54mm FIXED 11/25/25

Resistor footprint 7.62mm instead of 12.04mm FIXED 11/25/25

