#include <Arduino.h>
#include <TimeLib.h>
#include "font.h"
#include <Streaming.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h" // Include the UI lib
#include <SerialCommand.h>
#include <Bounce2.h>

#define SDA D2
#define SCL D1
#define BUTTON_PIN D3
#define BUZZER_PIN D5

SerialCommand sCmd;
SSD1306 display(0x3c, SDA, SCL);

Bounce button = Bounce();
int Minutes;
bool Idle = true;
bool Paused = true;
bool Finished = true;
time_t PausedTime, FinishedTime;
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
    if(Finished && !Idle)
    {
        notifyFinished();
    }
    else if(!Finished && !Paused)
    {
        String timeString = formatDigits(Minutes-minute()) + ":" + formatDigits(59-second());
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
            beep();
        }
    }
    else
    {
        showIdle();
    }
}

void showIdle()
{
    display.clear();
    display.drawString(64, 0, "(^_^)");
    display.display();
    Idle = true;
    // (o_o) (°_°) (^_^)
}

void startTimer(int minutes, int seconds)
{
    Serial << "Starting Timer: " << minutes << ":" << seconds << endl;
    Minutes = minutes;
    Idle = false;
    Paused = false;
    Finished = false;
    setTime(0, 0, 59-seconds, 1, 1, 1970);
    beep();
}

void pauseTimer()
{
    Serial.println("Timer Paused.");
    PausedTime = now();
    Paused = true;
    beep();
}

void resumeTimer()
{
    Serial.println("Timer Resumed.");
    setTime(PausedTime);
    Paused = false;
    beep();
}

void finishTimer()
{
    Serial.println("Timer Finished.");
    FinishedTime = now();
    FinishedFlashingLastChange = millis();
    Finished = true;
    beep();
    beep();
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

void beep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(5);
    digitalWrite(BUZZER_PIN, LOW);
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
    // display.flipScreenVertically(); // If screen is upside down
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(Open_Sans_Hebrew_Bold_40);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    button.attach(BUTTON_PIN);
    button.interval(5);

    pinMode(BUZZER_PIN, OUTPUT);

    showIdle();
}

void loop()
{
    sCmd.readSerial();
    button.update();

    if(button.fell())
    {
        if(Idle || Finished)
        {
            startTimer(1, 30);
        }
        else if(Paused)
        {
            resumeTimer();
        }
        else
        {
            pauseTimer();
        }
    }

    countDownTimer();
}
