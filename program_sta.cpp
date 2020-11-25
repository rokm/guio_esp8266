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

    taskCheckConnection.enable();
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
                } else {
                    // FIXME: what to do?
                    GDBG_println(F("MQTT client failed to subscribe to topic!"));
                }
            } else {
                GDBG_println(F("MQTT client failed to connect!"));
            }
        }

        if (mqttClient.connected()) {
            GDBG_println(F("MQTT client is connected..."));
            taskBlinkLed.disable();
            toggleLed(true);
        } else {
            GDBG_println(F("MQTT client is not connected!"));
            taskBlinkLed.setInterval(500*TASK_MILLISECOND);
            taskBlinkLed.enableIfNot();
        }
    } else {
        GDBG_print(F("WiFi not connected; status: "));
        GDBG_println(WiFi.status());

        mqttClient.disconnect();

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

    // Write payload to serial as a pass-through message
    Serial.print('$');
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)(payload[i]));
    }
    Serial.println();
}
