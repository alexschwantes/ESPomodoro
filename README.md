ESPomodoro
==========

This is a little take on implementing a physical digital Pomodoro timer for the ESP8266. There is lots of Pomodoro software out there to do this, but I needed something physically flashing and beeping at me to get me out of my chair and stay focused.

## Background
The Pomodoro Technique is a time management method that uses a timer to break down work into intervals, traditionally 25 minutes in length, separated by short breaks. See https://en.wikipedia.org/wiki/Pomodoro_Technique for further details.

## Components
* 1 x ESP8266  (~$4)
* 1 x Push Button (~$0.20)
* 1 x 3V Piezo Buzzer (~$0.20)
* 1 x SSD1306 128x64 I2C OLED Screen (~$4)

## Required Libraries
All these libraries should be found in the standard Arduino Library Manager
* ESP8266_SSD1306
* Time
* SerialCommand
* Streaming
* Bounce2

## Installation
* If your using PlatformIO, it should be as simple as downloading the project, importing it into PlatformIO and flashing it to your ESP.
* If your using Arduino IDE, checkout or download a copy of the repository. Arduino requires the folder name to match the main file, so rename the src folder to *espomodoro*. The only files you really need are the espomodoro.ino and font.h files. Then double click the espomodoro.ino file in that folder and flash to your device.

If your setup is different, you can edit the following lines of code to configure the pins on the ESP8266. The SDA and SCL pins connect to the corresponding pins on the OLED screen.
```c
#define SDA_PIN D2
#define SCL_PIN D1
#define BUTTON_PIN D3
#define BUZZER_PIN D5
```

## Usage
ESPomodoro is split into a *work* timer of 25 minutes and a *break* timer of 5 minutes.
* It will start up in the *work* mode.
* Single press the button: start or stop the current timer.
* Double press the button: reset the current timer and start again.
* Long press the button: switch mode between *work/break* timers.
* When the timer is up, it will beep and flash the screen to let you know.
* After a timer has finished, it will switch mode to the next timer, ie when the *work* timer is done it will switch to a *break* timer, so next press of the button will start the break timer.
