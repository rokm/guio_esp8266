#ifndef GUIO_ESP8266__PARAMETERS_H
#define GUIO_ESP8266__PARAMETERS_H

#include <Arduino.h>


struct parameters_t
{
    // Header (8 bytes)
    char sig[4]; // GUIO
    uint16_t version; // version
    bool configured; // configuration valid
    bool force_ap; // force AP mode

    // WiFi network settings (64 bytes)
    char networkSsid[32];
    char networkPassword[32];

    // MQTT broker settings
    char mqttHostName[32];
    char mqttUserName[32];
    char mqttUserPassword[32];

    // NOTE: subscribe and public topics are defined with respect to
    // our (ESP2866) side. In other words, they are swapped with respect
    // to what we get in the pairing request (where the meaning is
    // defined from the perspective of the GUI-O phone app).
    char subscribeTopic[48];
    char publishTopic[48];
};


void parameters_init (parameters_t *const params);
bool parameters_valid (const parameters_t *params);


#endif
