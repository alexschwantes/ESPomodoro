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

int DefaultWorkMinutes = 25;
int DefaultWorkSeconds = 0;
int DefaultBreakMinutes = 5;
int DefaultBreakSeconds = 0;
bool WorkMode = true;
int Minutes = DefaultWorkMinutes;
int Seconds = DefaultWorkSeconds;
bool Idle = true;
bool Paused = true;
bool Finished = true;
time_t PausedTime, FinishedTime;
unsigned long FinishedFlashingLastChange;
unsigned long FinishedFlashingInterval = 1 * 1000; // milliseconds to flash on/off
unsigned long FinishedDuration = 30; // seconds to show finish notification
int FinishedIndex = 0;
String FinishedMessage[] = {"Finish", ""};
unsigned long longButtonPressTimeStamp = 0;
unsigned long buttonPresses[] = {0, 0};

void beep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(5);
    digitalWrite(BUZZER_PIN, LOW);
}

void beepBig()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
}

String formatDigits(int digits)
{
    if (digits < 10)
    {
        return '0' + String(digits);
    }
    return String(digits);
}

void startTimer(int minutes, int seconds)
{
    Serial << "Starting Timer: " << minutes << ":" << seconds << endl;
    Minutes = minutes;
    Seconds = seconds;
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

void resetTimer()
{
    Serial.println("Timer Reset.");
    startTimer(Minutes, Seconds);
}

void finishTimer()
{
    Serial.println("Timer Finished.");
    FinishedTime = now();
    FinishedFlashingLastChange = millis();
    Finished = true;
    beepBig();
}

void setTimerSerial()
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

void showIdle()
{
    display.clear();
    display.drawString(64, 0, "(^_^)");
    display.display();
    Idle = true;
    // (o_o) (°_°) (^_^)
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
            beepBig();
        }
    }
    else
    {
        showIdle();
    }
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

bool multiButtonPress()
{
    // if button is pressed for a second time within 500 ms
    if(buttonPresses[0] > 0 && millis() - buttonPresses[0] < 500)
    {
        buttonPresses[1] = millis();
        return true;
    }
    else
    {
        // record as first press
        buttonPresses[0] = millis();
        buttonPresses[1] = 0;
        return false;
    }
}

void toggleMode()
{
    String modeText;
    if(WorkMode) {
        modeText = "Break";
        startTimer(DefaultBreakMinutes, DefaultBreakSeconds);
    }
    else
    {
        modeText = "Work";
        startTimer(DefaultWorkMinutes, DefaultWorkSeconds);
    }
    pauseTimer();
    WorkMode = !WorkMode;
    // trigger showing of Idle screen after FinishedDuration elapsed
    FinishedTime = now();
    // Idle = false;
    // Finished = true;

    // update display
    display.clear();
    display.drawString(64, 0, modeText);
    display.display();
}

// Start state: Idle
// Single press: start, stop
// Double press: reset timer
// Long press: switch Mode: work/break
void handleButton()
{
    button.update();
    if(button.fell())
    {
        longButtonPressTimeStamp = millis();
        if(multiButtonPress())
        {
            // second press
            resetTimer();
        }
        else
        {
            if(Idle || Finished)
            {
                // first press
                startTimer(DefaultWorkMinutes, DefaultWorkSeconds);
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
    }
    else if(button.rose())
    {
        // reset button press
        longButtonPressTimeStamp = 0;
    }

    // long press (longer than 500 ms)
    if(longButtonPressTimeStamp > 0 && millis() - longButtonPressTimeStamp >= 500)
    {
        longButtonPressTimeStamp = 0;
        toggleMode();
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println();
    sCmd.addCommand("set", setTimerSerial);
    sCmd.addCommand("pause", pauseTimer);
    sCmd.addCommand("play", resumeTimer);
    Serial.println("Ready");

    display.init();
    // display.flipScreenVertically(); // If screen is upside down
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(Nimbus_Sans_L_Bold_40);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    button.attach(BUTTON_PIN);
    button.interval(5);

    pinMode(BUZZER_PIN, OUTPUT);
    pauseTimer();
    showIdle();
}

void loop()
{
    sCmd.readSerial();
    handleButton();
    countDownTimer();
}
