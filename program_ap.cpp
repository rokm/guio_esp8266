#include "program_ap.h"


ProgramAp::ProgramAp (parameters_t &parameters)
    : Program(parameters),
      webServer(80),
      // This task writes the parameters to EEPROM, blinks led 5 times, and restarts the E2866 after 5 seconds...
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

#ifdef GUIO_DEBUG
    Serial.println(F("*** AP mode ***"));
#endif

    // Construct SSID and password
    //  ssid: guio_MAC
    //  password: MAC
    uint8_t macAddr[6];
    WiFi.softAPmacAddress(macAddr);

    char ssid[20];
    char password[20];
    snprintf_P(ssid, sizeof(ssid), PSTR("guio_%02x%02x%02x%02x%02x%02x"), macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    //snprintf_P(password, sizeof(password), PSTR("%02x%02x%02x%02x%02x%02x"), macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    snprintf_P(password, sizeof(password), PSTR("12345678"));

#ifdef GUIO_DEBUG
    Serial.print(F("ssid: "));
    Serial.println(ssid);
    Serial.print(F("password: "));
    Serial.println(password);
#endif

    // Start the AP
    bool rc;
    rc = WiFi.softAP(ssid, password);

#ifdef GUIO_DEBUG
    Serial.print(F("AP status: "));
    Serial.println(rc);
#endif

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
    digitalWrite(LED_BUILTIN, HIGH);

    // Dump request
#ifdef GUIO_DEBUG
    Serial.print(F("Pairing request: "));
    serializeJsonPretty(json, Serial);
    Serial.println();
#endif

    // Prepare response object
    StaticJsonDocument<256> responseDocument;

    // Parse request (and validate fields)
    parameters_t newParameters;
    memset(&newParameters, 0, sizeof(parameters_t));

    if (json.is<JsonObject>()) {
        const JsonObject &requestObject = json.as<JsonObject>();
        JsonVariant field;

        // Network SSID
        field = requestObject["networkSsid"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing networkSsid string field.");
            goto end;
        }
        if (snprintf(newParameters.networkSsid, sizeof(newParameters.networkSsid), "%s", field.as<const char *>()) >= sizeof(newParameters.networkSsid)) {
            responseDocument["pairingResponseDetail"] = F("networkSsid string too long.");
            goto end;
        }

        // Network password
        field = requestObject["networkPassword"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing networkPassword string field.");
            goto end;
        }
        if (snprintf(newParameters.networkPassword, sizeof(newParameters.networkPassword), "%s", field.as<const char *>()) >= sizeof(newParameters.networkPassword)) {
            responseDocument["pairingResponseDetail"] = F("networkPassword string too long.");
            goto end;
        }

        // MQTT host
        field = requestObject["mqttHostName"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing mqttHostName string field.");
            goto end;
        }
        if (snprintf(newParameters.mqttHostName, sizeof(newParameters.mqttHostName), "%s", field.as<const char *>()) >= sizeof(newParameters.mqttHostName)) {
            responseDocument["pairingResponseDetail"] = F("mqttHostName string too long.");
            goto end;
        }

        // MQTT user name
        field = requestObject["mqttUserName"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing mqttUserName string field.");
            goto end;
        }
        if (snprintf(newParameters.mqttUserName, sizeof(newParameters.mqttUserName), "%s", field.as<const char *>()) >= sizeof(newParameters.mqttUserName)) {
            responseDocument["pairingResponseDetail"] = F("mqttUserName string too long.");
            goto end;
        }

        // MQTT user password
        field = requestObject["mqttUserPassword"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing mqttUserPassword string field.");
            goto end;
        }
        if (snprintf(newParameters.mqttUserPassword, sizeof(newParameters.mqttUserPassword), "%s", field.as<const char *>()) >= sizeof(newParameters.mqttUserPassword)) {
            responseDocument["pairingResponseDetail"] = F("mqttUserPassword string too long.");
            goto end;
        }

        // MQTT subscribe topic
        field = requestObject["subscribeTopic"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing subscribeTopic string field.");
            goto end;
        }
        if (snprintf(newParameters.subscribeTopic, sizeof(newParameters.subscribeTopic), "%s", field.as<const char *>()) >= sizeof(newParameters.subscribeTopic)) {
            responseDocument["pairingResponseDetail"] = F("subscribeTopic string too long.");
            goto end;
        }

        // MQTT publish topic
        field = requestObject["publishTopic"];
        if (!field) {
            responseDocument["pairingResponseDetail"] = F("Missing publishTopic string field.");
            goto end;
        }
        if (snprintf(newParameters.publishTopic, sizeof(newParameters.publishTopic), "%s", field.as<const char *>()) >= sizeof(newParameters.publishTopic)) {
            responseDocument["pairingResponseDetail"] = F("publishTopic string too long.");
            goto end;
        }

        // Initialize header
        memcpy_P(newParameters.sig, GUIO_SIG, 4);
        newParameters.version = 1;
        newParameters.configured = true;
        newParameters.force_ap = false;
    } else {
        responseDocument["pairingResponseDetail"] = F("Request not a JSON object.");
    }

end:

    // If parameters are valid (configured flag is set), we succeeded
    if (newParameters.configured) {
        responseDocument["pairingResponse"] = 0; // Succeeded
    } else {
        responseDocument["pairingResponse"] = -1; // Failed; detail is stored in pairingResponseDetail

#ifdef GUIO_DEBUG
        Serial.print(F("ERROR: "));
        Serial.println(responseDocument["pairingResponseDetail"].as<const char *>());
#endif
    }

    // Send response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(responseDocument, *response);
    request->send(response);

    // On success, write parameters and schedule permanent commit
    if (newParameters.configured) {
#ifdef GUIO_DEBUG
        Serial.println(F("Pairing succeeded! Copying parameters and scheduling restart..."));
#endif
        memcpy(&parameters, &newParameters, sizeof(parameters_t));
        taskCommitParameters.enableDelayed(); // Enable commit task
    } else {
        // On failure, re-enable blinking LEDs
#ifdef GUIO_DEBUG
        Serial.println(F("Pairing failed!"));
#endif        
        taskBlinkLed.enable();
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
