#ifndef GUIO_ESP8266__PROGRAM_AP_H
#define GUIO_ESP8266__PROGRAM_AP_H

#include "program_base.h"

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>


class ProgramAp : public Program
{
public:
    ProgramAp (parameters_t &parameters);

    void setup () override;
    void loop () override;

protected:
    void pairingRequestHandler (AsyncWebServerRequest *request, JsonVariant &json);

    bool taskCommitParametersOnEnableFcn ();
    void taskCommitParametersOnDisableFcn ();

protected:
    AsyncWebServer webServer;

    Task taskCommitParameters;
};


#endif
