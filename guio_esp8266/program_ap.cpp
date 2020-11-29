/*
 * GUI-O ESP8266 bridge
 * AP program implementation.
 *
 * Copyright (C) 2020, Rok Mandeljc
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "program_ap.h"


ProgramAp::ProgramAp (parameters_t &parameters)
    : Program(parameters),
      webServer(80),
      // This task writes the parameters to EEPROM, blinks led 5 times, and restarts the E8266 after 5 seconds...
      taskCommitParameters(
        500*TASK_MILLISECOND,
        10,
        std::bind(&ProgramAp::taskBlinkLedFcn, this),
        &scheduler,
        false,
        [this] () {
            // OnEnable
            writeParametersToEeprom();
            return true;
        },
        [this] () {
            // OnDisable
            restartSystem();
        }
      )
{
}

void ProgramAp::setup ()
{
    Program::setup();

    GDBG_println(F("*** AP mode ***"));

    // Set up blinking LEDs to signal AP mode
    taskBlinkLed.setInterval(500*TASK_MILLISECOND);
    taskBlinkLed.enable();

    // Construct SSID and password
    // FIXME: set password to MAC by using (deviceId + 5)
    char *ssid = deviceId;
#if 0
    char *password = deviceId + 5;
#else
    char password[20];
    snprintf_P(password, sizeof(password), PSTR("12345678"));
#endif

    GDBG_print(F("ssid: "));
    GDBG_println(deviceId);
    GDBG_print(F("password: "));
    GDBG_println(password);

    // Start the AP
    bool rc;
    rc = WiFi.softAP(ssid, password);

    GDBG_print(F("AP status: "));
    GDBG_println(rc);

    // Start the web server
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/pair", std::bind(&ProgramAp::pairingRequestHandler, this, std::placeholders::_1, std::placeholders::_2));
    webServer.addHandler(handler);
    webServer.begin();

    // Set status code
    statusCode = STATUS_AP_READY;
}


// A helper for copying a string field from JSON object
static bool copy_string_parameter (const JsonObject &object, const char *fieldName, char *destBuffer, unsigned int destBufferSize, char *errorBuffer, unsigned int errorBufferSize)
{
    // Grab the field
    JsonVariant field = object[fieldName];
    if (!field) {
        snprintf_P(errorBuffer, errorBufferSize, PSTR("Field '%s' not found in request object!"), fieldName);
        return false;
    }

    // Ensure it is a string
    const char *value = field.as<const char *>();
    if (!value) {
        snprintf_P(errorBuffer, errorBufferSize, PSTR("Field '%s' in request object is not a string!"), fieldName);
        return false;
    }

    // Copy its contents (with overflow check)
    if (snprintf(destBuffer, destBufferSize, "%s", value) >= destBufferSize) {
        snprintf_P(errorBuffer, errorBufferSize, PSTR("Field '%s' in request object is too long!"), fieldName);
        return false;
    }

    return true;
}

void ProgramAp::pairingRequestHandler (AsyncWebServerRequest *request, JsonVariant &json)
{
    // Stop blinking LEDs...
    taskBlinkLed.disable();
    // ... and force them ON
    toggleLed(true);

    // Dump request
    GDBG_print(F("Pairing request: "));
    GDBG_print_json(json);
    GDBG_println();

    // Prepare response object
    StaticJsonDocument<320> responseDocument;

    // Parse request (and validate fields)
    parameters_t newParams;
    parameters_init(&newParams);

    char errorMessage[256];

    if (json.is<JsonObject>()) {
        const JsonObject &requestObject = json.as<JsonObject>();

        // Network SSID
        if (!copy_string_parameter(requestObject, "networkSsid", newParams.networkSsid, sizeof(newParams.networkSsid), errorMessage, sizeof(errorMessage))) {
            goto end;
        }

        // Network password
        if (!copy_string_parameter(requestObject, "networkPassword", newParams.networkPassword, sizeof(newParams.networkPassword), errorMessage, sizeof(errorMessage))) {
            goto end;
        }

        // MQTT host
        if (!copy_string_parameter(requestObject, "mqttHostName", newParams.mqttHostName, sizeof(newParams.mqttHostName), errorMessage, sizeof(errorMessage))) {
            goto end;
        }

        // MQTT user name
        if (!copy_string_parameter(requestObject, "mqttUserName", newParams.mqttUserName, sizeof(newParams.mqttUserName), errorMessage, sizeof(errorMessage))) {
            goto end;
        }

        // MQTT user password
        if (!copy_string_parameter(requestObject, "mqttUserPassword", newParams.mqttUserPassword, sizeof(newParams.mqttUserPassword), errorMessage, sizeof(errorMessage))) {
            goto end;
        }

        // MQTT subscribe topic (NOTE: inverted meaning!)
        if (!copy_string_parameter(requestObject, "subscribeTopic", newParams.publishTopic, sizeof(newParams.publishTopic), errorMessage, sizeof(errorMessage))) {
            goto end;
        }

        // MQTT publish topic (NOTE: inverted meaning!)
        if (!copy_string_parameter(requestObject, "publishTopic", newParams.subscribeTopic, sizeof(newParams.subscribeTopic), errorMessage, sizeof(errorMessage))) {
            goto end;
        }

        // Mark as configured
        newParams.configured = true;
    } else {
        snprintf_P(errorMessage, sizeof(errorMessage), PSTR("Request payload is not a JSON object!"));
    }

end:

    // If parameters are valid (configured flag is set), we succeeded
    if (newParams.configured) {
        responseDocument["pairingResponse"] = 0; // Succeeded
        responseDocument["pairingDeviceName"] = deviceId;

        // Copy back the parameter fields that were given in the input
        // request. This is somewhat redundant, but on the other hand,
        // it will ensure that they were correctly copied over into
        // our internal parameters...
        responseDocument["networkSsid"] = newParams.networkSsid;
        responseDocument["networkPassword"] = newParams.networkPassword;
        responseDocument["mqttHostName"] = newParams.mqttHostName;
        responseDocument["mqttUserName"] = newParams.mqttUserName;
        responseDocument["mqttUserPassword"] = newParams.mqttUserPassword;
        responseDocument["subscribeTopic"] = newParams.publishTopic; // inverted meaning!
        responseDocument["publishTopic"] = newParams.subscribeTopic; // inverted meaning!
    } else {
        responseDocument["pairingResponse"] = -1; // Failed; error message is stored in pairingResponseDetail
        responseDocument["pairingResponseDetail"] = errorMessage;

        GDBG_print(F("ERROR: "));
        GDBG_println(errorMessage);
    }

    // Send response
    GDBG_print(F("Pairing response: "));
    GDBG_print_json(responseDocument);
    GDBG_println();

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(responseDocument, *response);
    request->send(response);

    // On success, copy parameters and schedule commit to EEPROM
    if (newParams.configured) {
        GDBG_println(F("Pairing succeeded! Copying parameters and scheduling restart..."));
        memcpy(&parameters, &newParams, sizeof(parameters_t));
        taskCommitParameters.enableDelayed(5*TASK_SECOND); // Enable commit task
    } else {
        // On failure, re-enable blinking LEDs
        GDBG_println(F("Pairing failed!"));
        taskBlinkLed.enableIfNot();
    }
}
