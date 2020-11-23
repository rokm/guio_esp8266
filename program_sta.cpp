#include "program_sta.h"


ProgramSta::ProgramSta (parameters_t &parameters)
    : Program(parameters)
{
}

void ProgramSta::setup ()
{
    Program::setup ();
    Serial.println(F("*** STA mode ***"));
}

void ProgramSta::loop ()
{
    Program::loop();
}
