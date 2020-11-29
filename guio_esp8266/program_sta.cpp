/*
 * GUI-O ESP8266 bridge
 * STA program implementation.
 *
 * Copyright (C) 2020, Rok Mandeljc
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "program_sta.h"


ProgramSta::ProgramSta (parameters_t &parameters)
    : Program(parameters),
      wifiClient(),
      mqttClient(),
      // Task that checks and attempts to re-establish connection.
      taskCheckConnection(
        15*TASK_SECOND,
        TASK_FOREVER,
        std::bind(&ProgramSta::taskCheckConnectionFcn, this),
        &scheduler,
        false,
        nullptr,
        nullptr
      )
{
}

void ProgramSta::setup ()
{
    Program::setup ();

    GDBG_println(F("*** STA mode ***"));

    // Turn off the LED
    digitalWrite(BUILTIN_LED, HIGH);

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

    // Hostname
    if (false) {
        // FIXME: add support for setting hostname from EEPROM parameters
    } else {
        // Use device ID
        WiFi.hostname(deviceId);
    }

    // Set up WiFi
    WiFi.begin(parameters.networkSsid, parameters.networkPassword);

    // Set up MQTT client
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(parameters.mqttHostName, 1883);
    mqttClient.setCallback(std::bind(&ProgramSta::mqttReceiveCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    taskCheckConnection.enableDelayed(5*TASK_SECOND); // Schedule first check after 5 seconds

    // Set status
    statusCode = STATUS_STA_NOWIFI;
    taskBlinkLed.setInterval(TASK_SECOND);
    taskBlinkLed.enableIfNot();
}

void ProgramSta::loop ()
{
    Program::loop();

    mqttClient.loop();
}


void ProgramSta::taskCheckConnectionFcn ()
{
    if (WiFi.status() == WL_CONNECTED) {
        GDBG_print(F("WiFi connected; local IP: "));
        GDBG_println(WiFi.localIP());

        if (!mqttClient.connected()) {
            // Try connecting again
            if (mqttClient.connect(deviceId, parameters.mqttUserName, parameters.mqttUserPassword)) {
                GDBG_print(F("MQTT client established connection! Subscribing to topic "));
                GDBG_println(parameters.subscribeTopic);

                if (mqttClient.subscribe(parameters.subscribeTopic)) {
                    GDBG_println(F("MQTT client subscribed to topic!"));
                    statusCode = STATUS_STA_READY;
                } else {
                    GDBG_println(F("MQTT client failed to subscribe to topic!"));
                    statusCode = STATUS_STA_NOSUB;
                    // FIXME: how to properly handle this? Disconnect?
                }
            } else {
                GDBG_println(F("MQTT client failed to connect!"));
                statusCode = STATUS_STA_NOMQTT;
            }
        }

        if (mqttClient.connected()) {
            GDBG_println(F("MQTT client is connected..."));
            // Permanently turn the LED on
            taskBlinkLed.disable();
            toggleLed(true);
        } else {
            GDBG_println(F("MQTT client is not connected!"));
            // Fast blinking
            taskBlinkLed.setInterval(500*TASK_MILLISECOND);
            taskBlinkLed.enableIfNot();
        }
    } else {
        GDBG_print(F("WiFi not connected; status: "));
        GDBG_println(WiFi.status());

        statusCode = STATUS_STA_NOWIFI;

        mqttClient.disconnect();

        // Blinking
        taskBlinkLed.setInterval(TASK_SECOND);
        taskBlinkLed.enableIfNot();
    }
}

void ProgramSta::mqttReceiveCallback (char *topic, byte *payload, unsigned int length)
{
    GDBG_print(F("Received "));
    GDBG_print(length);
    GDBG_print(F(" bytes from MQTT topic "));
    GDBG_println(topic);

    // Strip trailing newline character(s) to avoid duplication (we
    // forward the line with added CRLF)
    while (length > 0 && (payload[length-1] == '\n' || payload[length-1] == '\r')) {
        length--;
    }

    GDBG_print(F("Message length: "));
    GDBG_println(length);

    // Write payload to serial as a pass-through message
    Serial.print('$');
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)(payload[i]));
    }
    Serial.println();
}


bool ProgramSta::serialInputHandler ()
{
    // First try with parent handler...
    if (Program::serialInputHandler()) {
        return true;
    }

    // ... then check if it is a pass-through message
    if (serialBuffer[0] == '$') {
        if (mqttClient.connected()) {
            // Publish the message, skipping the pass-through character
            mqttClient.publish(parameters.publishTopic, serialBuffer + 1);
        } else {
            GDBG_println(F("Cannot forward message - MQTT client not connected!"));
        }
    }
}
