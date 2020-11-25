#ifndef GUIO_ESP8266__PROGRAM_STA_H
#define GUIO_ESP8266__PROGRAM_STA_H

#include "program_base.h"

#include <PubSubClient.h>


class ProgramSta : public Program
{
public:
    ProgramSta (parameters_t &parameters);

    void setup () override;
    void loop () override;

protected:
    void taskCheckConnectionFcn ();

    void mqttReceiveCallback (char *topic, byte *payload, unsigned int length);

protected:
    char mqttClientId[20]; // guio_MAC

    WiFiClient wifiClient;
    PubSubClient mqttClient;

    Task taskCheckConnection;
};


#endif
