

#include <Arduino.h>
#include <TimeLib.h>
#include "font.h"
#include <Streaming.h>
#include <Wire.h>
#include "SSD1306.h"
// #include "OLEDDisplayUi.h"
#include <SerialCommand.h>
#include <Bounce2.h>

#define SDA_PIN D2
#define SCL_PIN D1
#define BUTTON_PIN D3
#define BUZZER_PIN D5

SerialCommand sCmd;
SSD1306 display(0x3c, SDA_PIN, SCL_PIN);
Bounce button = Bounce();

int DefaultWorkMinutes = 25;
int DefaultWorkSeconds = 0;
int DefaultBreakMinutes = 5;
int DefaultBreakSeconds = 0;
bool WorkMode = true;
int Minutes = DefaultWorkMinutes;
int Seconds = DefaultWorkSeconds;

enum States: byte {
    IDLE,
    RUNNING,
    PAUSED,
    FINISHED,
    TOGGLE
};
States State;

time_t PausedTime, TimeoutTime;
unsigned long TimeoutDuration = 30; // seconds to wait until showing idle screen
unsigned long FinishedFlashingLastChange;
unsigned long FinishedFlashingInterval = 1 * 1000; // milliseconds to flash on/off
String FinishedMessage[] = {"Finish", ""};
int FinishedMessageIndex = 0;
unsigned long longButtonPressTimeStamp = 0;
unsigned long buttonPresses[] = {0, 0};

void beep(bool big=false)
{
    int duration = (big)? 100 : 5;
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
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
    State = RUNNING;
    setTime(0, 0, 59-seconds, 1, 1, 1970);
    beep();
}

void pauseTimer()
{
    Serial.println("Timer Paused.");
    PausedTime = now();
    State = PAUSED;
    beep();
}

void resumeTimer()
{
    Serial.println("Timer Resumed.");
    setTime(PausedTime);
    State = RUNNING;
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
    TimeoutTime = now();
    FinishedFlashingLastChange = millis();
    State = FINISHED;
    beep(true);
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

void toggleTimerMode()
{
    if(WorkMode) {
        Minutes = DefaultBreakMinutes;
        Seconds = DefaultBreakSeconds;
    }
    else
    {
        Minutes = DefaultWorkMinutes;
        Seconds = DefaultWorkSeconds;
    }
    WorkMode = !WorkMode;
}

void displayTimerMode()
{
    String modeText = (WorkMode) ? "Work" : "Break";
    startTimer(Minutes, Seconds);
    pauseTimer();

    // trigger showing of Idle screen after TimeoutDuration elapsed
    TimeoutTime = now();
    State = TOGGLE;

    // update display
    display.clear();
    display.drawString(64, 0, modeText);
    display.display();
}

void showIdle()
{
    display.clear();
    display.drawString(64, 0, "(^_^)");
    display.display();
    State = IDLE;
    // (o_o) (°_°) (^_^)
}

void notifyFinished()
{
    // Flash Finished messages
    if (millis() - FinishedFlashingLastChange > FinishedFlashingInterval) {
        FinishedMessageIndex = (FinishedMessageIndex + 1)  % 2;
        display.clear();
        display.drawString(64, 0, FinishedMessage[FinishedMessageIndex]);
        display.display();
        FinishedFlashingLastChange = millis();
        beep(true);
    }
}

void countDownTimer()
{
    switch (State) {
        // case IDLE, PAUSED:
        //     break;
        case RUNNING:
            {
                String timeString = formatDigits(Minutes-minute()) + ":" + formatDigits(59-second());
                display.clear();
                display.drawString(64, 0, timeString);
                display.display();
                if(timeString.equals("00:00"))
                {
                    toggleTimerMode();
                    finishTimer();
                }
            }
            break;
        case TOGGLE:
            if(now() - TimeoutTime > TimeoutDuration)
            {
                showIdle();
            }
            break;
        case FINISHED:
            notifyFinished();
            if(now() - TimeoutTime > TimeoutDuration)
            {
                showIdle();
            }
            break;
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
            // Double press
            resetTimer();
        }
        else
        {
            if(State == IDLE || State == FINISHED || State == TOGGLE)
            {
                // Single press
                startTimer(Minutes, Seconds);
            }
            else if(State == PAUSED)
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
        toggleTimerMode();
        displayTimerMode();
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
    showIdle();
}

void loop()
{
    sCmd.readSerial();
    handleButton();
    countDownTimer();
}
