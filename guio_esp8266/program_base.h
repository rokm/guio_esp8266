#ifndef GUIO_ESP8266__PROGRAM_BASE_H
#define GUIO_ESP8266__PROGRAM_BASE_H

#include "config.h"
#include "parameters.h"

#include <TaskSchedulerDeclarations.h>
#include <ESP8266WiFi.h>


enum StatusCode
{
    // STA mode
    STATUS_STA_READY  = 0, // ready & fully operational
    STATUS_STA_NOSUB  = 1, // connected to both WiFi and MQTT, but MQTT subscribe failed
    STATUS_STA_NOMQTT = 2, // connected to WiFi, but not connected to MQTT
    STATUS_STA_NOWIFI = 3, // not connected to WiFi
    // AP mode
    STATUS_AP_READY = 100, // in AP mode, ready to be paired

    STATUS_UNKNOWN = 255, // unknown status
};


class Program
{
public:
    Program (parameters_t &parameters);

    virtual void setup ();
    virtual void loop ();

    virtual bool serialInputHandler ();

protected:
    void toggleLed (bool on);

    void ICACHE_RAM_ATTR buttonPressIsr (); // Called as ISR
    void taskCheckButtonFcn ();
    void buttonPressHandler (unsigned int duration);

    void taskBlinkLedFcn ();

    void clearParametersInEeprom () const;
    void writeParametersToEeprom () const;
    void restartSystem () const;

protected:
    // Device parameters (EEPROM)
    parameters_t &parameters;

    // Device ID: guio_ + WiFi MAC
    char deviceId[20]; // guio_AABBCCDDEEFF

    // Program status code - to send with PING reply
    uint8_t statusCode;

    // Task scheduler
    Scheduler scheduler;

    // Tasks
    Task taskBlinkLed;
    Task taskCheckButton;

    // Button handling
    volatile bool buttonStateChanged;
    unsigned int buttonPressTime;

    // Serial input
    char serialBuffer[256]; // 255 + NULL
    uint16_t serialLen;
    uint8_t serialBatch;
};


#endif
