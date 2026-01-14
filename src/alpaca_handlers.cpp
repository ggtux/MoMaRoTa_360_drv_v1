#include "alpaca_handlers.h"
#include "servo_control.h"
#include <WiFiUdp.h>

// Device status
static bool isConnected = false;
static bool reverseState = false;
static String deviceName = "MoMa Rotator";

// UDP Discovery
static WiFiUDP udp;
static const int udpPort = 32227;
static IPAddress multicastAddress(233, 255, 255, 255);
static char packetBuffer[255];
static int alpacaPortGlobal = 80;

// ============================================================================
// INITIALIZATION
// ============================================================================

void initDiscovery(int alpacaPort) {
    alpacaPortGlobal = alpacaPort;
    udp.beginMulticast(multicastAddress, udpPort);
}

void setupAlpacaEndpoints(AsyncWebServer &server) {
    // ASCOM Alpaca Management Endpoints
    server.on("/management/v1/description", HTTP_GET, handleDescription);
    server.on("/management/apiversions", HTTP_GET, handleApiVersion);
    server.on("/management/v1/configureddevices", HTTP_GET, handleConfiguredDevices);

    // ASCOM Alpaca Common Device Endpoints
    server.on("/api/v1/rotator/0/connected", HTTP_GET, handleGetConnected);
    server.on("/api/v1/rotator/0/connected", HTTP_PUT, handleSetConnected);
    server.on("/api/v1/rotator/0/connecting", HTTP_GET, handleGetConnecting);
    server.on("/api/v1/rotator/0/connect", HTTP_PUT, handleConnect);
    server.on("/api/v1/rotator/0/description", HTTP_GET, handleGetDescription);
    server.on("/api/v1/rotator/0/devicestate", HTTP_GET, handleDeviceState);
    server.on("/api/v1/rotator/0/disconnect", HTTP_PUT, handleDisconnect);
    server.on("/api/v1/rotator/0/driverinfo", HTTP_GET, handleDriverInfo);
    server.on("/api/v1/rotator/0/driverversion", HTTP_GET, handleDriverVersion);
    server.on("/api/v1/rotator/0/interfaceversion", HTTP_GET, handleGetInterfaceVersion);
    server.on("/api/v1/rotator/0/name", HTTP_GET, handleGetName);
    server.on("/api/v1/rotator/0/supportedactions", HTTP_GET, handleSupportedActions);

    // ASCOM Alpaca Rotator Specific Endpoints
    server.on("/api/v1/rotator/0/canreverse", HTTP_GET, handleCanReverse);
    server.on("/api/v1/rotator/0/ismoving", HTTP_GET, handleIsMoving);
    server.on("/api/v1/rotator/0/mechanicalposition", HTTP_GET, handleMechanicalPosition);
    server.on("/api/v1/rotator/0/position", HTTP_GET, handlePosition);
    server.on("/api/v1/rotator/0/reverse", HTTP_GET, handleGetReverse);
    server.on("/api/v1/rotator/0/reverse", HTTP_PUT, handleSetReverse);
    server.on("/api/v1/rotator/0/stepsize", HTTP_GET, handleStepSize);
    server.on("/api/v1/rotator/0/targetposition", HTTP_GET, handleTargetPosition);
    server.on("/api/v1/rotator/0/halt", HTTP_PUT, handleHalt);
    server.on("/api/v1/rotator/0/move", HTTP_PUT, handleMove);
    server.on("/api/v1/rotator/0/moveabsolute", HTTP_PUT, handleMoveAbsolute);
    server.on("/api/v1/rotator/0/movemechanical", HTTP_PUT, handleMoveMechanical);
    server.on("/api/v1/rotator/0/sync", HTTP_PUT, handleSync);
}

// ============================================================================
// JSON RESPONSE HELPER
// ============================================================================

void sendJSONResponse(AsyncWebServerRequest *request, JsonDocument &doc, int error) {
    static int serverTransactionID = 0;
    serverTransactionID++;

    int clientID = request->hasParam("ClientID") ? request->getParam("ClientID")->value().toInt() : 0;
    int clientTransactionID = request->hasParam("ClientTransactionID") ? request->getParam("ClientTransactionID")->value().toInt() : 0;

    doc["ClientID"] = clientID;
    doc["ClientTransactionID"] = clientTransactionID;
    doc["ServerTransactionID"] = serverTransactionID;
    doc["ErrorNumber"] = error;

    String response;
    serializeJson(doc, response);
    Serial.println("ALPACA Response: " + response);

    request->send(200, "application/json", response);
}

// ============================================================================
// UDP DISCOVERY
// ============================================================================

void handleDiscovery() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        Serial.print("Discovery packet received: size=");
        Serial.println(packetSize);

        int len = udp.read(packetBuffer, 255);
        if (len > 0) {
            packetBuffer[len] = 0;
        }

        // Check for valid discovery packet
        if (len < 16 || strncmp("alpacadiscovery1", packetBuffer, 16) != 0) {
            return;
        }

        char response[36] = {0};
        sprintf(response, "{\"AlpacaPort\": %d}", alpacaPortGlobal);

        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write((uint8_t *)response, strlen(response));
        udp.endPacket();

        Serial.println("Discovery response sent");
    }
}

// ============================================================================
// MANAGEMENT ENDPOINTS
// ============================================================================

void handleDescription(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonObject value = doc["Value"].to<JsonObject>();
    value["Manufacturer"] = "MoMa";
    value["ManufacturerVersion"] = "1.0";
    value["ServerName"] = "MoMa Rotator";
    sendJSONResponse(request, doc, 0);
}

void handleApiVersion(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray versions = doc["Value"].to<JsonArray>();
    versions.add(1);
    sendJSONResponse(request, doc, 0);
}

void handleConfiguredDevices(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray value = doc["Value"].to<JsonArray>();

    JsonObject device = value.add<JsonObject>();
    device["DeviceName"] = "Rotator";
    device["DeviceType"] = "Rotator";
    device["DeviceNumber"] = 0;
    device["UniqueID"] = "6109ff28-84d0-4f79-aa90-05ef3c191f50";

    sendJSONResponse(request, doc, 0);
}

// ============================================================================
// COMMON DEVICE ENDPOINTS
// ============================================================================

void handleGetConnected(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = isConnected;
    sendJSONResponse(request, doc, 0);
}

void handleSetConnected(AsyncWebServerRequest *request) {
    JsonDocument doc;
    if (!request->hasArg("Connected")) {
        sendJSONResponse(request, doc, 1025);
        return;
    }
    isConnected = request->arg("Connected").equalsIgnoreCase("true");
    doc["Value"] = isConnected;
    sendJSONResponse(request, doc, 0);
}

void handleConnect(AsyncWebServerRequest *request) {
    JsonDocument doc;
    isConnected = true;
    sendJSONResponse(request, doc, 0);
}

void handleDisconnect(AsyncWebServerRequest *request) {
    JsonDocument doc;
    isConnected = false;
    sendJSONResponse(request, doc, 0);
}

void handleGetConnecting(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = false;
    sendJSONResponse(request, doc, 0);
}

void handleDeviceState(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray value = doc["Value"].to<JsonArray>();
    sendJSONResponse(request, doc, 0);
}

void handleGetDescription(AsyncWebServerRequest *request) {
    JsonDocument doc;
    int id = request->arg("Id").toInt();
    if (id != 0) {
        sendJSONResponse(request, doc, 1025);
        return;
    }
    doc["Value"] = String("MoMa Rotator");
    sendJSONResponse(request, doc, 0);
}

void handleDriverInfo(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = "MoMa DIY Rotator";
    sendJSONResponse(request, doc, 0);
}

void handleDriverVersion(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = "1.0";
    sendJSONResponse(request, doc, 0);
}

void handleGetInterfaceVersion(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = 3;
    sendJSONResponse(request, doc, 0);
}

void handleGetName(AsyncWebServerRequest *request) {
    JsonDocument doc;
    int id = request->arg("Id").toInt();
    if (id != 0) {
        sendJSONResponse(request, doc, 1025);
        return;
    }
    doc["Value"] = deviceName;
    sendJSONResponse(request, doc, 0);
}

void handleSupportedActions(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray actions = doc["Value"].to<JsonArray>();
    sendJSONResponse(request, doc, 0);
}

// ============================================================================
// ROTATOR SPECIFIC ENDPOINTS
// ============================================================================

void handleCanReverse(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = true;
    sendJSONResponse(request, doc, 0);
}

void handleIsMoving(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = isServoMoving();
    sendJSONResponse(request, doc, 0);
}

void handleMechanicalPosition(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = getServoAngle();
    sendJSONResponse(request, doc, 0);
}

void handlePosition(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = getServoAngle();
    sendJSONResponse(request, doc, 0);
}

void handleGetReverse(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = reverseState;
    sendJSONResponse(request, doc, 0);
}

void handleSetReverse(AsyncWebServerRequest *request) {
    JsonDocument doc;
    String reverseStr = request->arg("Reverse");
    reverseState = (reverseStr == "true");
    sendJSONResponse(request, doc, 0);
}

void handleStepSize(AsyncWebServerRequest *request) {
    JsonDocument doc;
    // Step size in degrees: 4096 steps for 360° motor / 2 (gear ratio) = 0.0439° per step on gear
    doc["Value"] = (360.0 / 4096.0) / 2.0;
    sendJSONResponse(request, doc, 0);
}

void handleTargetPosition(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = getServoAngle();
    sendJSONResponse(request, doc, 0);
}

void handleHalt(AsyncWebServerRequest *request) {
    JsonDocument doc;
    stopServo();
    sendJSONResponse(request, doc, 0);
}

void handleMove(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();
    double currentAngle = getServoAngle();
    double newPosition = currentAngle + value;

    // Validate range
    if (newPosition < 0.0 || newPosition > 359.99) {
        sendJSONResponse(request, doc, 1025);
    } else {
        sendJSONResponse(request, doc, 0);
        moveServoByAngle(value);
    }
}

void handleMoveAbsolute(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();

    if (value < 0.0 || value > 359.99) {
        sendJSONResponse(request, doc, 1025);
    } else {
        sendJSONResponse(request, doc, 0);
        moveServoToAngle(value);
    }
}

void handleMoveMechanical(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();
    
    if (value < 0.0 || value > 359.99) {
        sendJSONResponse(request, doc, 1025);
    } else {
        sendJSONResponse(request, doc, 0);
        moveServoToAngle(value);
    }
}

void handleSync(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();
    // Set the current rotator position to the specified value without moving
    setZeroPointExact();
    sendJSONResponse(request, doc, 0);
}
