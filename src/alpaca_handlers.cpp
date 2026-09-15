#include "alpaca_handlers.h"
#include "servo_control.h"
#include <WiFiUdp.h>
#include "rotator_transport.h"

// Device status
static bool isConnected = false;
static String deviceName = "Astro Orbit";
static double syncOffsetDegrees = 0.0;
static double alpacaTargetPosition = 0.0;

// UDP Discovery
static WiFiUDP udp;
static const int udpPort = 32227;
static IPAddress multicastAddress(233, 255, 255, 255);
static char packetBuffer[255];
static int alpacaPortGlobal = 80;
static bool udpInitialized = false;

static double normalizeAngle(double angle) {
    while(angle >= 360.0) angle -= 360.0;
    while(angle < 0.0) angle += 360.0;
    return angle;
}

static double getSyncedPosition() {
    return normalizeAngle(getServoAngle() + syncOffsetDegrees);
}

double rotatorPosition() { return getSyncedPosition(); }
double rotatorTarget() { return alpacaTargetPosition; }
void rotatorSetTarget(double v) { alpacaTargetPosition = normalizeAngle(v); }
double rotatorSyncOffset() { return syncOffsetDegrees; }
void rotatorSync(double v) {
    syncOffsetDegrees = normalizeAngle(v - getServoAngle());
    alpacaTargetPosition = v;
}
bool alpacaOwnsRotator() { return isConnected; }

// ============================================================================
// INITIALIZATION
// ============================================================================

void initDiscovery(int alpacaPort) {
    alpacaPortGlobal = alpacaPort;
    if(WiFi.status() == WL_CONNECTED) {
        udp.beginMulticast(multicastAddress, udpPort);
        udpInitialized = true;
        Serial.println("UDP Discovery initialized on port 32227");
    } else {
        Serial.println("UDP Discovery will start when WiFi connects");
        udpInitialized = false;
    }
}

void setupAlpacaEndpoints(AsyncWebServer &server) {
    // Position is the sky position angle. MechanicalPosition remains the raw
    // rotator angle; Sync only establishes the offset between the two.
    //
    // Mode 3 has only a virtual mechanical zero which is reset at every boot.
    // A persisted sky offset would therefore refer to an obsolete mechanical
    // coordinate system and corrupt the first move after reconnecting.
    syncOffsetDegrees = 0.0;
    alpacaTargetPosition = getSyncedPosition();

    Serial.println("Rotator sync offset reset; waiting for plate-solve Sync");

    auto route = [&server](const char* path, WebRequestMethodComposite method,
                           void (*handler)(AsyncWebServerRequest*)) {
        server.on(path, method, [method, handler](AsyncWebServerRequest* request) {
            RotatorControlGuard guard;
            if(method == HTTP_PUT && usbOwnsRotator() && handler != handleHalt) {
                JsonDocument doc;
                doc["ErrorMessage"] = "Rotator is controlled over USB; disconnect USB first";
                sendJSONResponse(request, doc, 1035);
                return;
            }
            handler(request);
        });
    };

    // ASCOM Alpaca Management Endpoints
    route("/management/v1/description", HTTP_GET, handleDescription);
    route("/management/apiversions", HTTP_GET, handleApiVersion);
    route("/management/v1/configureddevices", HTTP_GET, handleConfiguredDevices);

    // ASCOM Alpaca Common Device Endpoints
    route("/api/v1/rotator/0/connected", HTTP_GET, handleGetConnected);
    route("/api/v1/rotator/0/connected", HTTP_PUT, handleSetConnected);
    route("/api/v1/rotator/0/connecting", HTTP_GET, handleGetConnecting);
    route("/api/v1/rotator/0/connect", HTTP_PUT, handleConnect);
    route("/api/v1/rotator/0/description", HTTP_GET, handleGetDescription);
    route("/api/v1/rotator/0/devicestate", HTTP_GET, handleDeviceState);
    route("/api/v1/rotator/0/disconnect", HTTP_PUT, handleDisconnect);
    route("/api/v1/rotator/0/driverinfo", HTTP_GET, handleDriverInfo);
    route("/api/v1/rotator/0/driverversion", HTTP_GET, handleDriverVersion);
    route("/api/v1/rotator/0/interfaceversion", HTTP_GET, handleGetInterfaceVersion);
    route("/api/v1/rotator/0/name", HTTP_GET, handleGetName);
    route("/api/v1/rotator/0/supportedactions", HTTP_GET, handleSupportedActions);

    // ASCOM Alpaca Rotator Specific Endpoints
    route("/api/v1/rotator/0/canreverse", HTTP_GET, handleCanReverse);
    route("/api/v1/rotator/0/ismoving", HTTP_GET, handleIsMoving);
    route("/api/v1/rotator/0/mechanicalposition", HTTP_GET, handleMechanicalPosition);
    route("/api/v1/rotator/0/position", HTTP_GET, handlePosition);
    route("/api/v1/rotator/0/reverse", HTTP_GET, handleGetReverse);
    route("/api/v1/rotator/0/reverse", HTTP_PUT, handleSetReverse);
    route("/api/v1/rotator/0/stepsize", HTTP_GET, handleStepSize);
    route("/api/v1/rotator/0/targetposition", HTTP_GET, handleTargetPosition);
    route("/api/v1/rotator/0/halt", HTTP_PUT, handleHalt);
    route("/api/v1/rotator/0/move", HTTP_PUT, handleMove);
    route("/api/v1/rotator/0/moveabsolute", HTTP_PUT, handleMoveAbsolute);
    route("/api/v1/rotator/0/movemechanical", HTTP_PUT, handleMoveMechanical);
    route("/api/v1/rotator/0/sync", HTTP_PUT, handleSync);
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
    // Only handle discovery if WiFi is connected in STA mode
    if(WiFi.status() != WL_CONNECTED) {
        udpInitialized = false;
        return;
    }
    
    // Initialize UDP if not already done
    if(!udpInitialized) {
        udp.beginMulticast(multicastAddress, udpPort);
        udpInitialized = true;
        Serial.println("UDP Discovery initialized after WiFi connect");
    }
    
    int packetSize = udp.parsePacket();
    if (packetSize) {
        Serial.print("ALPACA Discovery packet received: size=");
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

        Serial.println("ALPACA Discovery response sent");
    }
}

// ============================================================================
// MANAGEMENT ENDPOINTS
// ============================================================================

void handleDescription(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonObject value = doc["Value"].to<JsonObject>();
    value["Manufacturer"] = "Astro Orbit";
    value["ManufacturerVersion"] = "1.0";
    value["ServerName"] = "Astro Orbit";
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
    device["DeviceName"] = deviceName;
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
    if(isConnected) {
        alpacaTargetPosition = getSyncedPosition();
    }
    doc["Value"] = isConnected;
    sendJSONResponse(request, doc, 0);
}

void handleConnect(AsyncWebServerRequest *request) {
    JsonDocument doc;
    isConnected = true;
    alpacaTargetPosition = getSyncedPosition();
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
    doc["Value"] = String("Astro Orbit");
    sendJSONResponse(request, doc, 0);
}

void handleDriverInfo(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = "Astro Orbit";
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
    doc["Value"] = getSyncedPosition();
    sendJSONResponse(request, doc, 0);
}

void handleGetReverse(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["Value"] = getReverseDirection();
    sendJSONResponse(request, doc, 0);
}

void handleSetReverse(AsyncWebServerRequest *request) {
    JsonDocument doc;
    String reverseStr = request->arg("Reverse");
    setReverseDirection(reverseStr.equalsIgnoreCase("true"));
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
    doc["Value"] = alpacaTargetPosition;
    sendJSONResponse(request, doc, 0);
}

void handleHalt(AsyncWebServerRequest *request) {
    JsonDocument doc;
    stopServo();
    alpacaTargetPosition = getSyncedPosition();
    sendJSONResponse(request, doc, 0);
}

void handleMove(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();
    double newPosition = normalizeAngle(getSyncedPosition() + value);

    if (value < -360.0 || value > 360.0) {
        sendJSONResponse(request, doc, 1025);
    } else {
        if(moveServoByAngle(value)) {
            alpacaTargetPosition = newPosition;
            sendJSONResponse(request, doc, 0);
        } else {
            sendJSONResponse(request, doc, 1025);
        }
    }
}

void handleMoveAbsolute(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();

    if (value < 0.0 || value > 359.99) {
        sendJSONResponse(request, doc, 1025);
    } else {
        double mechanicalTarget = normalizeAngle(value - syncOffsetDegrees);
        if(moveServoToAngle(mechanicalTarget)) {
            alpacaTargetPosition = value;
            sendJSONResponse(request, doc, 0);
        } else {
            sendJSONResponse(request, doc, 1025);
        }
    }
}

void handleMoveMechanical(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();
    
    if (value < 0.0 || value > 359.99) {
        sendJSONResponse(request, doc, 1025);
    } else {
        // Mechanical moves deliberately ignore the plate-solve Sync offset.
        if(moveServoToAngle(value)) {
            alpacaTargetPosition = normalizeAngle(value + syncOffsetDegrees);
            sendJSONResponse(request, doc, 0);
        } else {
            sendJSONResponse(request, doc, 1025);
        }
    }
}

void handleSync(AsyncWebServerRequest *request) {
    JsonDocument doc;
    double value = request->arg("Position").toDouble();

    if (value < 0.0 || value > 359.99) {
        sendJSONResponse(request, doc, 1025);
        return;
    }

    double mechanicalPosition = getServoAngle();
    syncOffsetDegrees = normalizeAngle(value - mechanicalPosition);
    alpacaTargetPosition = value;

    Serial.print("Synced to ");
    Serial.print(value);
    Serial.print("° at mechanical position ");
    Serial.print(mechanicalPosition);
    Serial.print("° (offset ");
    Serial.print(syncOffsetDegrees);
    Serial.println("°)");
    
    sendJSONResponse(request, doc, 0);
}
