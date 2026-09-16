#include "rotator_transport.h"
#include "servo_control.h"
#include "usb_line_buffer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <math.h>
#include <string.h>

static SemaphoreHandle_t controlMutex;
static String usbSession;
static String bootId;
static unsigned long lastUsbRequest = 0;
static const unsigned long LeaseMs = 15000;
static const char* Prefix = "@MOROTA ";

void initRotatorTransport() {
    controlMutex = xSemaphoreCreateRecursiveMutex();
    configASSERT(controlMutex != nullptr);
    char token[17];
    snprintf(token, sizeof(token), "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
    bootId = token;
}
void lockRotatorControl() { xSemaphoreTakeRecursive(controlMutex, portMAX_DELAY); }
void unlockRotatorControl() { xSemaphoreGiveRecursive(controlMutex); }
bool usbOwnsRotator() { return !usbSession.isEmpty(); }

static double wrap(double v) {
    v = fmod(v, 360.0);
    return v < 0 ? v + 360.0 : v;
}
static bool tokenValid(const char* s) {
    if(!s || strlen(s) != 32) return false;
    for(size_t i = 0; i < 32; ++i)
        if(!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return false;
    return true;
}
static void fail(JsonDocument& r, int code, const char* message) {
    r["error"] = code;
    r["message"] = message;
}
static void status(JsonDocument& r) {
    JsonObject v = r["value"].to<JsonObject>();
    v["position"] = rotatorPosition();
    v["mechanical"] = getServoAngle();
    v["target"] = rotatorTarget();
    v["moving"] = isServoMoving();
    v["reverse"] = getReverseDirection();
    v["stepSize"] = 360.0 / 4096.0 / 2.0;
    v["motorHealthy"] = isServoFeedbackHealthy();
    v["motionError"] = getServoMotionError();
}
void executeUsbRotator(JsonDocument& q, JsonDocument& r) {
    RotatorControlGuard guard;
    r["id"] = q["id"].as<String>();
    r["boot"] = bootId;
    r["error"] = 0;
    r["message"] = "";
    if(!tokenValid(q["id"].as<const char*>()) || !q["cmd"].is<const char*>()) {
        fail(r, 1025, "Invalid id or command"); return;
    }
    const String cmd = q["cmd"].as<String>();
    if(cmd == "hello") {
        JsonObject v = r["value"].to<JsonObject>();
        v["device"] = "MoMaRoTa";
        v["protocol"] = 1;
        v["firmware"] = "1.3.0-usb";
        v["leaseMs"] = LeaseMs;
        return;
    }
    const char* session = q["session"].as<const char*>();
    if(!tokenValid(session)) { fail(r, 1025, "Invalid session"); return; }
    if(cmd == "connect") {
        if(usbOwnsRotator() && usbSession != session) {
            fail(r, 1035, "USB is owned by another session"); return;
        }
        if(alpacaOwnsRotator() || (!usbOwnsRotator() && isServoMoving())) {
            fail(r, 1035, "Disconnect Alpaca and stop movement before USB connect"); return;
        }
        usbSession = session;
        lastUsbRequest = millis();
        status(r); return;
    }
    if(!usbOwnsRotator() || usbSession != session) {
        fail(r, 1031, "USB session expired; reconnect"); return;
    }
    lastUsbRequest = millis();
    if(cmd == "disconnect") {
        if(isServoMoving() || !isServoFeedbackHealthy()) stopServo();
        rotatorSetTarget(rotatorPosition());
        usbSession = "";
        return;
    }
    if(cmd == "status") { status(r); return; }
    if(cmd == "zero") {
        if(isServoMoving()) { fail(r, 1035, "Rotator is moving; halt first"); return; }
        setZeroPointExact();
        rotatorSync(0.0);
        rotatorSetTarget(0.0);
        return;
    }
    if(cmd == "halt") {
        stopServo(); rotatorSetTarget(rotatorPosition());
        if(!isServoFeedbackHealthy()) fail(r, 1280, "Stop sent but motor feedback is unavailable");
        return;
    }
    if(isServoMoving()) { fail(r, 1035, "Rotator is moving; halt first"); return; }
    if(cmd == "reverse") {
        if(!q["value"].is<bool>()) { fail(r, 1025, "Reverse requires a boolean"); return; }
        setReverseDirection(q["value"].as<bool>());
        return;
    }
    if(cmd != "move" && cmd != "absolute" && cmd != "mechanical" && cmd != "sync") {
        fail(r, 1024, "Unknown command"); return;
    }
    if(!q["value"].is<double>()) { fail(r, 1025, "Position must be a number"); return; }
    const double value = q["value"].as<double>();
    if(!isfinite(value) || (cmd == "move" ? (value < -360.0 || value > 360.0) : (value < 0 || value >= 360.0))) {
        fail(r, 1025, "Position outside allowed range"); return;
    }
    if(!isServoFeedbackHealthy()) { fail(r, 1280, "Motor feedback unavailable"); return; }
    if(cmd == "sync") { rotatorSync(value); return; }
    bool accepted;
    double target;
    if(cmd == "move") {
        target = wrap(rotatorPosition() + value);
        accepted = moveServoByAngle(value);
    } else {
        const double mechanical = cmd == "absolute" ? wrap(value - rotatorSyncOffset()) : value;
        target = wrap(mechanical + rotatorSyncOffset());
        accepted = moveServoToAngle(mechanical);
    }
    if(!accepted) { fail(r, 1280, "Movement rejected by motor or cable guard"); return; }
    rotatorSetTarget(target);
}

void processUsbRotator() {
    RotatorControlGuard guard;
    if(usbOwnsRotator() && millis() - lastUsbRequest > LeaseMs) {
        if(isServoMoving() || !isServoFeedbackHealthy()) stopServo();
        rotatorSetTarget(rotatorPosition());
        usbSession = "";
        Serial.println("USB lease expired; control released");
    }
    static UsbLineBuffer input;
    static unsigned long lastByte = 0;
    if(input.pending() && millis() - lastByte > 1000) input.discardPartial();
    // Bound work per main-loop iteration so feedback and Halt remain responsive.
    for(int n = 0; n < 64 && Serial.available(); ++n) {
        char c = (char)Serial.read();
        lastByte = millis();
        if(!input.push(c)) continue;
        if(strncmp(input.line(), Prefix, strlen(Prefix)) != 0) continue;
        JsonDocument request, response;
        if(deserializeJson(request, input.line() + strlen(Prefix))) continue;
        executeUsbRotator(request, response);
        String json;
        serializeJson(response, json);
        String frame = String("\n") + Prefix + json + "\n";
        // A single write separates protocol frames from ordinary debug lines.
        Serial.write((const uint8_t*)frame.c_str(), frame.length());
    }
}
