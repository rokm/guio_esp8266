#ifndef GUIO_ESP8266__CONFIG_H
#define GUIO_ESP8266__CONFIG_H


// Debug output from GUIO ESP8266 program
#define _GUIO_DEBUG


// Settings for TaskScheduler
#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_STD_FUNCTION


// Debug macros
// Avoid littering the code with #ifdef _GUIO_DEBUG blocks.
// Also allows easy switch to another Serial object (e.g., Serial1).
#ifdef _GUIO_DEBUG
    #define GDBG_print(x) Serial.print(x)
    #define GDBG_println(x) Serial.println(x)
    #define GDBG_print_json(x) serializeJsonPretty(x, Serial)
#else
    #define GDBG_print(x)
    #define GDBG_println(x)
    #define GDBG_print_json(x)
#endif


#endif
