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
#include "display_control.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

const int ALPACA_PORT = 80;
const bool UDP_DISCOVERY_ENABLED = false;  // Set to true if you need ASCOM Alpaca auto-discovery

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
    
    // Initialize OLED Display
    Serial.println("Initializing OLED display...");
    initDisplay();
    
    // Initialize servo motor
    Serial.println("Initializing servo...");
    displayMessage("Initializing", "Servo...");
    initServo();
    
    // Initialize WiFi with WiFiConfigPortal
    Serial.println("Initializing WiFi...");
    displayMessage("Connecting", "WiFi...");
    initWiFi(server);  // Pass server to WiFi initialization
    
    // Initialize ALPACA UDP Discovery for auto-detection by ASCOM clients
    Serial.println("Initializing ALPACA discovery...");
    initDiscovery(ALPACA_PORT);
    
    // Setup web server endpoints
    Serial.println("Setting up web server endpoints...");
    setupWiFiEndpoints(server);      // Setup WiFi and control endpoints
    setupAlpacaEndpoints(server);    // Setup ALPACA API endpoints
    
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
    
    // Show ready message
    displayMessage("MoMa Rotator", "Ready!", getIPAddress().c_str());
    delay(2000);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // Update servo feedback regularly
    getFeedback();
    
    // Update OLED display
    updateDisplay();
    
    // Update WiFi portal (for periodic status messages)
    updateWiFiPortal();
    
    // Process DNS requests (only in AP mode - captive portal)
    processDNS();
    
    // Handle ALPACA discovery packets (for ASCOM auto-detection)
    handleDiscovery();
    
    // Small delay to prevent watchdog issues
    delay(1);
}
