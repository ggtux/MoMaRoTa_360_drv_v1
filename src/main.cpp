// ============================================================================
// MoMa Rotator - ASCOM Alpaca Rotator Driver
// ============================================================================
// This is an ALPACA-compatible driver for an astronomical field rotator
// Uses ESP32 with ST3215 servo motor in Mode 3 (Motor Mode)
// ============================================================================

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "servo_control.h"
#include "wifi_manager.h"
#include "alpaca_handlers.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

const int ALPACA_PORT = 80;

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

AsyncWebServer server(ALPACA_PORT);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== MoMa Rotator - ALPACA Driver ===");
    
    // Initialize servo motor
    Serial.println("Initializing servo...");
    initServo();
    
    // Initialize WiFi
    Serial.println("Initializing WiFi...");
    initWiFi();
    
    // Initialize ALPACA discovery
    Serial.println("Initializing ALPACA discovery...");
    initDiscovery(ALPACA_PORT);
    
    // Setup web server endpoints
    Serial.println("Setting up web server endpoints...");
    setupWiFiEndpoints(server);
    setupAlpacaEndpoints(server);
    
    // 404 handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        String message = "404 Not Found: " + request->url();
        Serial.println(message);
        request->send(404, "text/plain", message);
    });
    
    // Start server
    server.begin();
    Serial.println("=== Server started ===");
    Serial.print("IP: ");
    Serial.println(getIPAddress());
    Serial.println("Ready for ALPACA connections");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // Update servo feedback regularly
    getFeedback();
    
    // Process DNS requests (for captive portal)
    processDNS();
    
    // Handle ALPACA discovery packets
    handleDiscovery();
    
    // Small delay to prevent watchdog issues
    delay(1);
}
