#ifndef GUIO_ESP8266__PROGRAM_BASE_H
#define GUIO_ESP8266__PROGRAM_BASE_H

#include "config.h"
#include "parameters.h"

#include <TaskSchedulerDeclarations.h>


class Program
{
public:
    Program (parameters_t &parameters);

    virtual void setup ();
    virtual void loop ();

protected:
    // Called as ISR
    void ICACHE_RAM_ATTR buttonPressIsr ();

    void buttonPressHandler (unsigned int duration);

    void taskBlinkLedFcn ();
    void taskCheckButtonFcn ();

    void clearParametersInEeprom () const;
    void writeParametersToEeprom () const;
    void restartSystem () const;
    
protected:
    parameters_t &parameters;
    
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
