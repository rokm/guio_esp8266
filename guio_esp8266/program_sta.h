/*
 * GUI-O ESP8266 bridge
 * STA program implementation.
 *
 * Copyright (C) 2020, Rok Mandeljc
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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
    bool serialInputHandler () override;

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
