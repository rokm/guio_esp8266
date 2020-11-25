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
        std::bind(&ProgramAp::taskCommitParametersOnEnableFcn, this),
        std::bind(&ProgramAp::taskCommitParametersOnDisableFcn, this)
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
}

void ProgramAp::loop ()
{
    Program::loop();
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
    StaticJsonDocument<256> responseDocument;

    // Parse request (and validate fields)
    parameters_t newParams;
    parameters_init(&newParams);

    if (json.is<JsonObject>()) {
        const JsonObject &requestObject = json.as<JsonObject>();
        JsonVariant field;

        // Network SSID
        field = requestObject["networkSsid"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing networkSsid string field.");
            goto end;
        }
        if (snprintf(newParams.networkSsid, sizeof(newParams.networkSsid), "%s", field.as<const char *>()) >= sizeof(newParams.networkSsid)) {
            responseDocument["pairingResponseDetail"] = F("networkSsid string too long.");
            goto end;
        }

        // Network password
        field = requestObject["networkPassword"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing networkPassword string field.");
            goto end;
        }
        if (snprintf(newParams.networkPassword, sizeof(newParams.networkPassword), "%s", field.as<const char *>()) >= sizeof(newParams.networkPassword)) {
            responseDocument["pairingResponseDetail"] = F("networkPassword string too long.");
            goto end;
        }

        // MQTT host
        field = requestObject["mqttHostName"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing mqttHostName string field.");
            goto end;
        }
        if (snprintf(newParams.mqttHostName, sizeof(newParams.mqttHostName), "%s", field.as<const char *>()) >= sizeof(newParams.mqttHostName)) {
            responseDocument["pairingResponseDetail"] = F("mqttHostName string too long.");
            goto end;
        }

        // MQTT user name
        field = requestObject["mqttUserName"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing mqttUserName string field.");
            goto end;
        }
        if (snprintf(newParams.mqttUserName, sizeof(newParams.mqttUserName), "%s", field.as<const char *>()) >= sizeof(newParams.mqttUserName)) {
            responseDocument["pairingResponseDetail"] = F("mqttUserName string too long.");
            goto end;
        }

        // MQTT user password
        field = requestObject["mqttUserPassword"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing mqttUserPassword string field.");
            goto end;
        }
        if (snprintf(newParams.mqttUserPassword, sizeof(newParams.mqttUserPassword), "%s", field.as<const char *>()) >= sizeof(newParams.mqttUserPassword)) {
            responseDocument["pairingResponseDetail"] = F("mqttUserPassword string too long.");
            goto end;
        }

        // MQTT subscribe topic
        field = requestObject["subscribeTopic"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing subscribeTopic string field.");
            goto end;
        }
        if (snprintf(newParams.subscribeTopic, sizeof(newParams.subscribeTopic), "%s", field.as<const char *>()) >= sizeof(newParams.subscribeTopic)) {
            responseDocument["pairingResponseDetail"] = F("subscribeTopic string too long.");
            goto end;
        }

        // MQTT publish topic
        field = requestObject["publishTopic"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing publishTopic string field.");
            goto end;
        }
        if (snprintf(newParams.publishTopic, sizeof(newParams.publishTopic), "%s", field.as<const char *>()) >= sizeof(newParams.publishTopic)) {
            responseDocument["pairingResponseDetail"] = F("publishTopic string too long.");
            goto end;
        }

        // Configured
        newParams.configured = true;
    } else {
        responseDocument["pairingResponseDetail"] = F("Request not a JSON object.");
    }

end:

    // If parameters are valid (configured flag is set), we succeeded
    if (newParams.configured) {
        responseDocument["pairingResponse"] = 0; // Succeeded
        responseDocument["pairingDeviceName"] = deviceId;
    } else {
        responseDocument["pairingResponse"] = -1; // Failed; detail is stored in pairingResponseDetail
        GDBG_print(F("ERROR: "));
        GDBG_println(responseDocument["pairingResponseDetail"].as<const char *>());
    }

    // Send response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(responseDocument, *response);
    request->send(response);

    // On success, copy parameters and schedule commit to EEPROM
    if (newParams.configured) {
        GDBG_println(F("Pairing succeeded! Copying parameters and scheduling restart..."));
        memcpy(&parameters, &newParams, sizeof(parameters_t));
        taskCommitParameters.enableDelayed(); // Enable commit task
    } else {
        // On failure, re-enable blinking LEDs
        GDBG_println(F("Pairing failed!"));
        taskBlinkLed.enableIfNot();
    }
}


bool ProgramAp::taskCommitParametersOnEnableFcn ()
{
    writeParametersToEeprom();
    return true;
}

void ProgramAp::taskCommitParametersOnDisableFcn ()
{
    restartSystem();
}
