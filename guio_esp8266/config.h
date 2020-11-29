/*
 * GUI-O ESP8266 bridge
 * Project configuration and settings.
 *
 * Copyright (C) 2020, Rok Mandeljc
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GUIO_ESP8266__CONFIG_H
#define GUIO_ESP8266__CONFIG_H


// Debug output from GUIO ESP8266 program
#define _GUIO_DEBUG


// Serial communication baud rate
#define _GUIO_SERIAL_BAUDRATE 115200

// LED used for main signalling tasks (e.g., built-in LED)
#define _GUIO_LED_MAIN LED_BUILTIN

// Pin that serves as AP/reset button
#define _GUIO_AP_BUTTON D4


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


// Settings for TaskScheduler
#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_STD_FUNCTION


#endif
