#ifndef GUIO_ESP8266__PROGRAM_BASE_H
#define GUIO_ESP8266__PROGRAM_BASE_H

#include "config.h"
#include "parameters.h"

#include <TaskSchedulerDeclarations.h>
#include <ESP8266WiFi.h>


class Program
{
public:
    Program (parameters_t &parameters);

    virtual void setup ();
    virtual void loop ();

protected:
    void toggleLed (bool on);

    // Called as ISR
    void ICACHE_RAM_ATTR buttonPressIsr ();

    void buttonPressHandler (unsigned int duration);

    bool taskBlinkLedOnEnable ();
    virtual void taskBlinkLedFcn ();
    void taskBlinkLedOnDisable ();

    void taskCheckButtonFcn ();

    void clearParametersInEeprom () const;
    void writeParametersToEeprom () const;
    void restartSystem () const;

protected:
    // Device parameters (EEPROM)
    parameters_t &parameters;

    // Device ID: guio_ + WiFi MAC
    char deviceId[20]; // guio_AABBCCDDEEFF

    // Task scheduler
    Scheduler scheduler;

    // Tasks
    Task taskBlinkLed;
    Task taskCheckButton;

    // Button handling
    volatile bool buttonStateChanged;
    unsigned int buttonPressTime;
};


#endif
