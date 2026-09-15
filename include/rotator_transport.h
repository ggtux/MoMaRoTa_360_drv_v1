#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Serialize USB, HTTP commands and the servo feedback/state machine.
void initRotatorTransport();
void lockRotatorControl();
void unlockRotatorControl();
struct RotatorControlGuard {
    RotatorControlGuard() { lockRotatorControl(); }
    ~RotatorControlGuard() { unlockRotatorControl(); }
    RotatorControlGuard(const RotatorControlGuard&) = delete;
    RotatorControlGuard& operator=(const RotatorControlGuard&) = delete;
};
bool usbOwnsRotator(); // Call while holding RotatorControlGuard.
void processUsbRotator();
void executeUsbRotator(JsonDocument& request, JsonDocument& response);
// Shared coordinate system for both transports, owned by alpaca_handlers.cpp.
double rotatorPosition();
double rotatorTarget();
void rotatorSetTarget(double value);
double rotatorSyncOffset();
void rotatorSync(double value);
bool alpacaOwnsRotator();
