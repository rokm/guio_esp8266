#ifndef GUIO_ESP8266__PARAMETERS_H
#define GUIO_ESP8266__PARAMETERS_H

#include <Arduino.h>


#define GUIO_MODE_STA 0xAA
#define GUIO_MODE_AP  0xBB


static const char GUIO_SIG[4] PROGMEM = { 'G', 'U', 'I', 'O' };

struct parameters_t
{
    char sig[4]; // GUIO
    uint16_t ver; // version
    bool configured; // configuration valid
    bool force_ap; // force AP mode
};


#endif
