#include <Arduino.h>
#include <ArduinoOTA.h>
#include "ota_update.h"
#include "display_control.h"
#include "servo_control.h"
#include "wifi_manager.h"

namespace {
const char *OTA_HOSTNAME = "astro-orbit";
bool otaEnabled = false;
unsigned int lastProgressPercent = 101;
}

void initOTAUpdate() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);

    ArduinoOTA.onStart([]() {
        stopServo();
        lastProgressPercent = 101;
        Serial.println("OTA update started");
        displayMessage("OTA Update", "Receiving...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        unsigned int percent = total > 0 ? (progress * 100U) / total : 0;
        if(percent == lastProgressPercent) {
            return;
        }

        lastProgressPercent = percent;
        Serial.printf("OTA progress: %u%%\r", percent);

        if(percent % 10U == 0U) {
            String progressText = String(percent) + "%";
            displayMessage("OTA Update", progressText.c_str());
        }
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA update complete - restarting");
        displayMessage("OTA Update", "Complete", "Restarting...");
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("\nOTA error [%u]: ", error);
        switch(error) {
            case OTA_AUTH_ERROR: Serial.println("authentication failed"); break;
            case OTA_BEGIN_ERROR: Serial.println("begin failed"); break;
            case OTA_CONNECT_ERROR: Serial.println("connection failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("receive failed"); break;
            case OTA_END_ERROR: Serial.println("end failed"); break;
            default: Serial.println("unknown error"); break;
        }

        displayMessage("OTA Update", "Error");
    });

    ArduinoOTA.begin();
    otaEnabled = true;

    Serial.print("OTA ready: ");
    Serial.print(OTA_HOSTNAME);
    Serial.print(".local (");
    Serial.print(getIPAddress());
    Serial.println(")");
}

void processOTAUpdate() {
    if(otaEnabled) {
        ArduinoOTA.handle();
    }
}
