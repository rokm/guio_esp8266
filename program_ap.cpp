#include "program_ap.h"


ProgramAp::ProgramAp (parameters_t &parameters)
    : Program(parameters)
{
}

void ProgramAp::setup ()
{
    Program::setup();
    Serial.println(F("*** AP mode ***"));
}

void ProgramAp::loop ()
{
    Program::loop();
}
