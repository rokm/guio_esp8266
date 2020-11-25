#include "program_base.h"

#include <EEPROM.h>
#include <FunctionalInterrupt.h>


Program::Program (parameters_t &parameters)
    : parameters(parameters), // store reference to parameters struct
      statusCode(STATUS_UNKNOWN), // reset status
      scheduler(),
      // Task for blinking the built-in LED. Used as a signalling mechanism
      taskBlinkLed(
        250*TASK_MILLISECOND, // this will be later adjusted within the program
        TASK_FOREVER,
        std::bind(&Program::taskBlinkLedFcn, this),
        &scheduler,
        false,
        std::bind(&Program::taskBlinkLedOnEnable, this),
        std::bind(&Program::taskBlinkLedOnDisable, this)
      ),
      // Task for checking the button state (for debounce).
      taskCheckButton(
        100*TASK_MILLISECOND,
        TASK_ONCE,
        std::bind(&Program::taskCheckButtonFcn, this),
        &scheduler,
        false,
        nullptr,
        nullptr
      ),
      buttonStateChanged(false),
      buttonPressTime(0)
{
}

void Program::setup ()
{
    // LEDs
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(D13, OUTPUT);

    // Button pin (force AP mode, reset EEPROM data)
    pinMode(D4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(D4), std::bind(&Program::buttonPressIsr, this), CHANGE);

    // Initialize device ID (guio_ + MAC); used as
    //  - SSID in AP mode
    //  - pairing device name in AP mode
    //  - hostname and in STA mode
    //  - mqtt client ID in STA mode
    uint8_t macAddr[6];
    WiFi.softAPmacAddress(macAddr);
    snprintf_P(deviceId, sizeof(deviceId), PSTR("guio_%02x%02x%02x%02x%02x%02x"), macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}


void Program::loop ()
{
    // Check button state
    if (buttonStateChanged) {
        taskCheckButton.restartDelayed();
        buttonStateChanged = false;
    }

    // Schedule tasks
    scheduler.execute();

    // Serial input
    // Read in small batches to avoid potential flood from starving
    // task scheduler...
    if (Serial.available()) {
        serialBatch = 0; // Reset batch counter
        while (Serial.available() && serialBatch < 32) {
            char c = Serial.read();
            serialBatch++;

            if (c == '\n') {
                serialBuffer[serialLen] = 0; // NULL terminate the buffer
                serialInputHandler();
                serialLen = 0; // Reset
            } else {
                // Read into buffer, truncate on overflow
                if (serialLen < sizeof(serialBuffer) - 1) {
                    serialBuffer[serialLen++] = c;
                }
            }
        }
    }
}


void Program::toggleLed (bool on)
{
    // LOW = on, HIGH = off
    digitalWrite(LED_BUILTIN, on ? LOW : HIGH);
}


void Program::buttonPressIsr ()
{
    buttonStateChanged = true;
}


bool Program::taskBlinkLedOnEnable ()
{
    toggleLed(false); // turn off
    return true;
}

void Program::taskBlinkLedOnDisable ()
{
    toggleLed(false); // turn off
}

void Program::taskBlinkLedFcn ()
{
    if (scheduler.currentTask().getRunCounter() % 2) {
        toggleLed(true); // turn on
    } else {
        toggleLed(false); // turn off
    }
}


void Program::clearParametersInEeprom () const
{
    for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

void Program::writeParametersToEeprom () const
{
    EEPROM.put(0, parameters);
    EEPROM.commit();
}

void Program::restartSystem () const
{
    ESP.restart();
}

void Program::buttonPressHandler (unsigned int duration)
{
    GDBG_print(F("Button press lasted "));
    GDBG_print(duration);
    GDBG_println(F(" ms..."));

    if (duration > 15*TASK_SECOND) {
        GDBG_println(F("Long press! Clearing EEPROM and rebooting!"));
        clearParametersInEeprom(); // clear EEPROM
        restartSystem(); // restart
    } else if (duration > 1*TASK_SECOND) {
        GDBG_println(F("Short press! Rebooting into AP!"));
        parameters.force_ap = true; // set forced AP flag
        writeParametersToEeprom(); // write to EEPROM
        restartSystem(); // restart
    }
}

void Program::taskCheckButtonFcn ()
{
    // Pull-up; HIGH = no contact, LOW = contact
    byte state = digitalRead(D4);
    if (state == LOW) {
        // Store press time, but only if it is invalid; if it is valid, we're likely observing bounce
        if (!buttonPressTime) {
            buttonPressTime = millis();
        }
    } else {
        if (buttonPressTime) {
            buttonPressHandler(millis() - buttonPressTime);
            buttonPressTime = 0;
        }
    }
}


bool Program::serialInputHandler ()
{
    GDBG_print(F("Received line: "));
    GDBG_println(serialBuffer);

    if (serialBuffer[0] == '!') {
        // Protocol commands
        if (strcmp_P(serialBuffer, PSTR("!PING")) == 0) {
            // Ping - FIXME: add state code
            Serial.print(F("!PONG "));
            Serial.println(statusCode);
            return true;
        } else if (strcmp_P(serialBuffer, PSTR("!REBOOT")) == 0) {
            // Reboot in preferred mode
            restartSystem();
            return true;
        } else if (strcmp_P(serialBuffer, PSTR("!REBOOT_AP")) == 0) {
            parameters.force_ap = true; // set forced AP flag
            writeParametersToEeprom(); // write to EEPROM
            restartSystem(); // restart
            return true;
        } else if (strcmp_P(serialBuffer, PSTR("!CLEAR_PARAMS")) == 0) {
            clearParametersInEeprom(); // clear EEPROM
            restartSystem(); // restart
            return true;
        }
    }

    return false; // Line not processed
}
