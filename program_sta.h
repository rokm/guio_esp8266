#ifndef GUIO_ESP8266__PROGRAM_STA_H
#define GUIO_ESP8266__PROGRAM_STA_H

#include "program_base.h"


class ProgramSta : public Program
{
public:
    ProgramSta (parameters_t &parameters);

    void setup () override;
    void loop () override;
};


#endif
