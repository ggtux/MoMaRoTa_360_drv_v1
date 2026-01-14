#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// WiFi configuration
void initWiFi();
void connectWiFi();
void handleResetWifi(AsyncWebServerRequest *request);

// Setup page handlers  
void handleSetupPage(AsyncWebServerRequest *request);
void handleWifiSetupPage(AsyncWebServerRequest *request);
void handleSaveWifi(AsyncWebServerRequest *request);
void handleCaptivePortal(AsyncWebServerRequest *request);

// Control panel
void handleConfigDevices(AsyncWebServerRequest *request);

// Helper functions
void setupWiFiEndpoints(AsyncWebServer &server);
void processDNS();

// Get IP address
String getIPAddress();
