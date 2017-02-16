#include <Arduino.h>
#include <TimeLib.h>
#include "font.h"
#include <Streaming.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h" // Include the UI lib
#include <SerialCommand.h>

SerialCommand sCmd;
SSD1306 display(0x3c, D2, D1);

int Minutes;
bool Idle, Paused, Finished = false;
time_t PausedTime, FinishedTime;//, FinishedFlashingInterval;
unsigned long FinishedFlashingLastChange;
unsigned long FinishedFlashingInterval = 1 * 1000; // milliseconds to flash on/off
unsigned long FinishedDuration = 10; // seconds to show finish notification
int FinishedIndex = 0;
String FinishedMessage[] = {"Finish", ""};

String formatDigits(int digits)
{
    if (digits < 10)
    {
        return '0' + String(digits);
    }
    return String(digits);
}

void countDownTimer()
{
    String timeString = formatDigits(Minutes-minute()) + ":" + formatDigits(59-second());

    if(Finished)
    {
        notifyFinished();
    }
    else if(!Paused)
    {
        display.clear();
        display.drawString(64, 0, timeString);
        display.display();
        if(timeString.equals("00:00"))
        {
            finishTimer();
        }
    }
}

void notifyFinished()
{
    if(now() - FinishedTime < FinishedDuration)
    {
        // Flash Finished messages
        if (millis() - FinishedFlashingLastChange > FinishedFlashingInterval) {
            FinishedIndex = (FinishedIndex + 1)  % 2;
            display.clear();
            display.drawString(64, 0, FinishedMessage[FinishedIndex]);
            display.display();
            FinishedFlashingLastChange = millis();
        }
    }
    else
    {
        showIdle();
    }
}

void showIdle()
{
    if(!Idle)
    {
        display.clear();
        display.drawString(64, 0, "(^_^)");
        display.display();
        Idle = true;
        // (o_o) (°_°) (^_^)
    }
}

void startTimer(int minutes, int seconds)
{
    Serial << "Starting Timer: " << minutes << ":" << seconds << endl;
    Minutes = minutes;
    Idle = false;
    Paused = false;
    Finished = false;
    setTime(0, 0, 59-seconds, 1, 1, 1970);
}

void pauseTimer()
{
    Serial.println("Timer Paused.");
    PausedTime = now();
    Paused = true;
}

void resumeTimer()
{
    Serial.println("Timer Resumed.");
    setTime(PausedTime);
    Paused = false;
}

void finishTimer()
{
    Serial.println("Timer Finished.");
    FinishedTime = now();
    FinishedFlashingLastChange = millis();
    Finished = true;
}

void setTimer()
{
    char *arg;
    int minutes, seconds = 0;
    arg = sCmd.next();
    if (arg != NULL) {
        minutes = atoi(arg);
    }

    arg = sCmd.next();
    if (arg != NULL) {
        seconds = atoi(arg);
    }

    if(seconds > 59) {
        Serial.println("Converting to minutes:seconds format");
        minutes = minutes + seconds/60;
        seconds = seconds % 60;
    }

    startTimer(minutes, seconds);
}

void setup()
{
    Serial.begin(115200);
    Serial.println();
    sCmd.addCommand("set", setTimer);
    sCmd.addCommand("pause", pauseTimer);
    sCmd.addCommand("play", resumeTimer);
    Serial.println("Ready");

    display.init();
    //  display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    // display.setFont(ArialMT_Plain_24); //10,16,24
    display.setFont(Open_Sans_Hebrew_Bold_40);

    startTimer(1, 30);
}

void loop()
{
    sCmd.readSerial();
    countDownTimer();
}
