#include "config.h"
#include "parameters.h"
#include "program_ap.h"
#include "program_sta.h"


#include <EEPROM.h>
#include <TaskScheduler.h>  // must be included only once, as it contains implementation!


// global instances
static Program *program = nullptr;
static parameters_t parameters;


void setup ()
{
    // Initialize serial
    Serial.begin(_GUIO_SERIAL_BAUDRATE);
    while (!Serial) {
        // Wait for serial port to connect
    }

    // Print banner (will probably be lost due to initial garbage from
    // bootloader, which runs at 74880 bps)
    GDBG_println();
    GDBG_println(F("**** GUI-O ESP8266 ****"));
    GDBG_println();

    // Restore parameters from EEPROM
    EEPROM.begin(sizeof(parameters_t));
    EEPROM.get(0, parameters);

    bool sta_mode = false;

    if (!parameters_valid(&parameters)) {
        GDBG_println(F("GUI-O parameters not found... initializing..."));
        // Clear & initialize
        parameters_init(&parameters);
        parameters.force_ap = true; // this will force write to EEPROM
    }

    // TODO: validate version, upgrade format, etc.
    sta_mode = parameters.configured && !parameters.force_ap;

    // Immediately reset the force-AP flag and store
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

void loop ()
{
    // Use program's loop function
    program->loop();
}
