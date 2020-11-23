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

    // Called from ISR
    void registerButtonStateChange ();

    void buttonPressHandler (unsigned int duration);

    void taskBlinkLedFcn ();
    void taskCheckButtonFcn ();
    
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


extern Program *program; // global pointer; found in main sketch


#endif
