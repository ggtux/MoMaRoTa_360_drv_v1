#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Initialize ALPACA endpoints
void setupAlpacaEndpoints(AsyncWebServer &server);

// JSON response helper
void sendJSONResponse(AsyncWebServerRequest *request, JsonDocument &doc, int error);

// ASCOM Alpaca Management Endpoints
void handleDescription(AsyncWebServerRequest *request);
void handleApiVersion(AsyncWebServerRequest *request);
void handleConfiguredDevices(AsyncWebServerRequest *request);

// ASCOM Alpaca Common Device Endpoints
void handleGetConnected(AsyncWebServerRequest *request);
void handleSetConnected(AsyncWebServerRequest *request);
void handleGetConnecting(AsyncWebServerRequest *request);
void handleConnect(AsyncWebServerRequest *request);
void handleGetDescription(AsyncWebServerRequest *request);
void handleDeviceState(AsyncWebServerRequest *request);
void handleDisconnect(AsyncWebServerRequest *request);
void handleDriverInfo(AsyncWebServerRequest *request);
void handleDriverVersion(AsyncWebServerRequest *request);
void handleGetInterfaceVersion(AsyncWebServerRequest *request);
void handleGetName(AsyncWebServerRequest *request);
void handleSupportedActions(AsyncWebServerRequest *request);

// ASCOM Alpaca Rotator Specific Endpoints
void handleCanReverse(AsyncWebServerRequest *request);
void handleIsMoving(AsyncWebServerRequest *request);
void handleMechanicalPosition(AsyncWebServerRequest *request);
void handlePosition(AsyncWebServerRequest *request);
void handleGetReverse(AsyncWebServerRequest *request);
void handleSetReverse(AsyncWebServerRequest *request);
void handleStepSize(AsyncWebServerRequest *request);
void handleTargetPosition(AsyncWebServerRequest *request);
void handleHalt(AsyncWebServerRequest *request);
void handleMove(AsyncWebServerRequest *request);
void handleMoveAbsolute(AsyncWebServerRequest *request);
void handleMoveMechanical(AsyncWebServerRequest *request);
void handleSync(AsyncWebServerRequest *request);

// UDP Discovery
void handleDiscovery();
void initDiscovery(int alpacaPort);
