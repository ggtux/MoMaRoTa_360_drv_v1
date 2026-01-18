#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WiFiConfigPortal.h>

// WiFi configuration with WiFiConfigPortal integration
void initWiFi(AsyncWebServer &server);
void handleResetWifi(AsyncWebServerRequest *request);
void updateWiFiPortal();

// Setup page handlers  
void handleSetupPage(AsyncWebServerRequest *request);
void handleCaptivePortal(AsyncWebServerRequest *request);

// Control panel
void handleConfigDevices(AsyncWebServerRequest *request);

// Helper functions
void setupWiFiEndpoints(AsyncWebServer &server);
void processDNS();

// Get IP address
String getIPAddress();
