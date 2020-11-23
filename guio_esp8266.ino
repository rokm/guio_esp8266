#include "config.h"
#include "parameters.h"
#include "program_ap.h"
#include "program_sta.h"


#include <EEPROM.h>
#include <TaskScheduler.h>  // must be included only once, as it contains implementation!


// global instances
Program *program = nullptr;
parameters_t parameters; 


void setup() 
{
    // Initialize serial
    Serial.begin(115200);
    while (!Serial) {
        // Wait for serial port to connect
    }

    // Print initial newline to clear garbage
#ifdef GUIO_DEBUG
    Serial.println();
    Serial.println(F("*** GUI-O loader ***"));
#endif

    // Restore settings from EEPROM
    EEPROM.begin(sizeof(parameters_t));
    EEPROM.get(0, parameters);

    bool sta_mode = false;

    if (memcmp_P(parameters.sig, GUIO_SIG, 4)) {
#ifdef GUIO_DEBUG
        Serial.println(F("GUIO parameters not found... initializing..."));
#endif

        memcpy_P(parameters.sig, GUIO_SIG, 4);
        parameters.ver = 1;
        parameters.configured = false;
        parameters.force_ap = true; // this will force write to EEPROM
    }

    // TODO: validate version, upgrade format, etc.
    sta_mode = parameters.configured && !parameters.force_ap;

#ifdef GUIO_DEBUG
    Serial.print(F("STA mode: "));
    Serial.println(sta_mode);
#endif

    // Immediately reset the force-AP flag
    if (parameters.force_ap) {
        parameters.force_ap = false;
        EEPROM.put(0, parameters);
        EEPROM.commit();
    }

    // Choose program...
    if (sta_mode) {
        program = new ProgramSta(parameters);
    } else {
        program = new ProgramAp(parameters);
    }
    
    // ... and initialize it
    program->setup();
}

void loop() 
{
    // Use program's loop function
    program->loop();
}
