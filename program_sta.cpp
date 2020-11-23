#include "program_sta.h"


ProgramSta::ProgramSta (parameters_t &parameters)
    : Program(parameters)
{
}

void ProgramSta::setup ()
{
    Program::setup ();

#ifdef GUIO_DEBUG
    Serial.println(F("*** STA mode ***"));
#endif

    // Display parameters
#ifdef GUIO_DEBUG
    Serial.print(F("Network SSID: "));
    Serial.println(parameters.networkSsid);

    Serial.print(F("Network password: "));
    Serial.println(parameters.networkPassword);

    Serial.print(F("MQTT host: "));
    Serial.println(parameters.mqttHostName);

    Serial.print(F("MQTT user name: "));
    Serial.println(parameters.mqttUserName);

    Serial.print(F("MQTT user password: "));
    Serial.println(parameters.mqttUserPassword);

    Serial.print(F("MQTT subscribe topic: "));
    Serial.println(parameters.subscribeTopic);

    Serial.print(F("MQTT publish topic: "));
    Serial.println(parameters.publishTopic);
#endif
}

void ProgramSta::loop ()
{
    Program::loop();
}
