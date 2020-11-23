#include "program_base.h"

#include <EEPROM.h>


// ISR must be cached in RAM!
ICACHE_RAM_ATTR void isrButton ()
{
    program->registerButtonStateChange();
}


Program::Program (parameters_t &parameters)
    : parameters(parameters), // store reference to parameters struct
      scheduler(),
      taskBlinkLed(1*TASK_SECOND, TASK_FOREVER, [] () { program->taskBlinkLedFcn(); }, &scheduler, false, nullptr, nullptr),
      taskCheckButton(100*TASK_MILLISECOND, TASK_ONCE, [] () { program->taskCheckButtonFcn(); }, &scheduler, false, nullptr, nullptr),
      buttonStateChanged(false),
      buttonPressTime(0)
{
}

void Program::setup ()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(D13, OUTPUT);

    pinMode(D4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(D4), isrButton, CHANGE);

    taskBlinkLed.enable();
}


void Program::loop ()
{
    // Check button state
    if (buttonStateChanged) {
        taskCheckButton.restartDelayed();
        buttonStateChanged = false;
    }
    
    // Schedule tasks
    scheduler.execute();
}

void Program::registerButtonStateChange ()
{
    buttonStateChanged = true;
}


void Program::taskBlinkLedFcn ()
{
    if (scheduler.currentTask().getRunCounter() % 2) {
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void Program::buttonPressHandler (unsigned int duration)
{
#ifdef GUIO_DEBUG
    Serial.print(F("Button press lasted "));
    Serial.print(duration);
    Serial.println(F(" ms..."));
#endif
    
    if (duration > 15*TASK_SECOND) {
        Serial.println(F("Long press! Clearing EEPROM and rebooting!"));
        // clear EEPROM
        for (int i = 0 ; i < EEPROM.length() ; i++) {
            EEPROM.write(i, 0);
        }
        EEPROM.commit();
        // reset
        ESP.restart();
    } else if (duration > 1*TASK_SECOND) {
        Serial.println(F("Short press! Rebooting into AP!"));
        // set forced AP flag
        parameters.force_ap = true;
        // write to EEPROM
        EEPROM.put(0, parameters);
        EEPROM.commit();
        // reset
        ESP.restart();
    }
}

void Program::taskCheckButtonFcn ()
{
    // Pull-up; HIGH = no contact, LOW = contact
    byte state = digitalRead(D4);
    if (state == LOW) {
        // Store press time, but only if it is invalid; if it is valid, we're likely observing bounce
        if (!buttonPressTime) {
            buttonPressTime = millis();
        }
    } else {
        if (buttonPressTime) {
            buttonPressHandler(millis() - buttonPressTime);
            buttonPressTime = 0;
        }
    }
}
