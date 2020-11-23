#include "program_sta.h"


ProgramSta::ProgramSta (parameters_t &parameters)
    : Program(parameters)
{
}

void ProgramSta::setup ()
{
    Program::setup ();

    GDBG_println(F("*** STA mode ***"));

    // Display parameters
    GDBG_print(F("Network SSID: "));
    GDBG_println(parameters.networkSsid);

    GDBG_print(F("Network password: "));
    GDBG_println(parameters.networkPassword);

    GDBG_print(F("MQTT host: "));
    GDBG_println(parameters.mqttHostName);

    GDBG_print(F("MQTT user name: "));
    GDBG_println(parameters.mqttUserName);

    GDBG_print(F("MQTT user password: "));
    GDBG_println(parameters.mqttUserPassword);

    GDBG_print(F("MQTT subscribe topic: "));
    GDBG_println(parameters.subscribeTopic);

    GDBG_print(F("MQTT publish topic: "));
    GDBG_println(parameters.publishTopic);
}

void ProgramSta::loop ()
{
    Program::loop();
}
