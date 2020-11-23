#include "program_base.h"

#include <EEPROM.h>
#include <FunctionalInterrupt.h>


Program::Program (parameters_t &parameters)
    : parameters(parameters), // store reference to parameters struct
      scheduler(),
      taskBlinkLed(1*TASK_SECOND, TASK_FOREVER, std::bind(&Program::taskBlinkLedFcn, this), &scheduler, false, nullptr, nullptr),
      taskCheckButton(100*TASK_MILLISECOND, TASK_ONCE, std::bind(&Program::taskCheckButtonFcn, this), &scheduler, false, nullptr, nullptr),
      buttonStateChanged(false),
      buttonPressTime(0)
{
}

void Program::setup ()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(D13, OUTPUT);

    pinMode(D4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(D4), std::bind(&Program::buttonPressIsr, this), CHANGE);

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

void Program::buttonPressIsr ()
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

void Program::clearParametersInEeprom () const
{
    for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

void Program::writeParametersToEeprom () const
{
    EEPROM.put(0, parameters);
    EEPROM.commit();
}

void Program::restartSystem () const
{
    ESP.restart();
}

void Program::buttonPressHandler (unsigned int duration)
{
    GDBG_print(F("Button press lasted "));
    GDBG_print(duration);
    GDBG_println(F(" ms..."));

    if (duration > 15*TASK_SECOND) {
        GDBG_println(F("Long press! Clearing EEPROM and rebooting!"));
        clearParametersInEeprom(); // clear EEPROM
        restartSystem(); // restart
    } else if (duration > 1*TASK_SECOND) {
        GDBG_println(F("Short press! Rebooting into AP!"));
        parameters.force_ap = true; // set forced AP flag
        writeParametersToEeprom(); // write to EEPROM
        restartSystem(); // restart
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
