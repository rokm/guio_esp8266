#ifndef GUIO_ESP8266__PROGRAM_AP_H
#define GUIO_ESP8266__PROGRAM_AP_H

#include "program_base.h"


class ProgramAp : public Program
{
public:
    ProgramAp (parameters_t &parameters);

    void setup () override;
    void loop () override;
};


#endif
