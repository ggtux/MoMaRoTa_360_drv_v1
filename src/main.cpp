#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include "servo_control.h"
#include "Adafruit_NeoPixel.h"


void connectWifi();
void handleRoot(AsyncWebServerRequest *request);
void handleResetWifi(AsyncWebServerRequest *request);
void handleSetupPage(AsyncWebServerRequest *request);
void handleWifiSetupPage(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleConfigDevices(AsyncWebServerRequest *request);

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
void handleCanAsync(AsyncWebServerRequest *request);
void handleSetAsync(AsyncWebServerRequest *request);
void handleCancelAsync(AsyncWebServerRequest *request);
void handleStateChangeComplete(AsyncWebServerRequest *request);
void handleDiscovery();
void handleCaptivePortal(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);

// Utility
void sendJSONResponse(AsyncWebServerRequest *request, JsonDocument &doc, int error);

// Additional Methods
void moveRotatorAbsolute(double degrees);
void moveRotator(double degrees);
void moveMechanical(double degrees);

Preferences preferences;

// UDP Discovery
const int udpPort = 32227;
WiFiUDP udp;
IPAddress multicastAddress(233, 255, 255, 255);
char packetBuffer[255]; // buffer to hold incoming packet

// Webserver auf Port 80 (Alpaca Standardport)
const int alpacaPort = 80;
AsyncWebServer server(alpacaPort);

int serverTransactionID = 0;

// Gerätestatus
bool isConnected = false;
bool isMoving = false;
double mechanicalPosition = 0.0;
bool reverseState = false;
double targetPosition = 0.0;
double position = 0.0;
String name = "Rotator";

// Accesspoint
const char *apSSID = "MoMaRoTa"; // Name des Access Points
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;

const char *STA_SSID = "";
const char *STA_PWD = "";

// the MAC address of the device you want to ctrl.
uint8_t broadcastAddress[] = {0x08, 0x3A, 0xF2, 0x93, 0x5F, 0xA8};
// uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message
{
  int ID_send;
  int POS_send;
  int Spd_send;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// set the default role here.
// 0 as normal mode.
// 1 as leader, ctrl other device via ESP-NOW.
// 2 as follower, can be controled via ESP-NOW.
#define DEFAULT_ROLE 0

// set the default wifi mode here.
// 1 as [AP] mode, it will not connect other wifi.
// 2 as [STA] mode, it will connect to know wifi.
#define DEFAULT_WIFI_MODE 2

void setup()
{
  // #define RGB_LED   23
  // #define NUMPIXELS 10

  initServo();
  Serial.begin(115200);
  connectWifi();

  // Init_RGB();
  // RGBcolor(0, 64, 255);
  udp.beginMulticast(multicastAddress, udpPort);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/reset", HTTP_GET, handleResetWifi);

  // SETUP WIFI
  server.on("/setup/v1/rotator/0/setup", HTTP_GET, handleSetupPage);
  server.on("/setup/v1/rotator/0/wifi", HTTP_GET, handleWifiSetupPage);
  server.on("/setup/v1/rotator/0/save", HTTP_POST, handleSave);
  server.on("/setup/v1/rotator/0/configdevices", HTTP_GET, handleConfigDevices);

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

  server.onNotFound(handleNotFound); // Hier wird der Error-Handler gesetzt
  server.begin();
}

void connectWifi()
{
  preferences.begin("wifi_config", false);
  String ssid = "";
  String password = "";
  
  if (preferences.isKey("ssid")) {
    ssid = preferences.getString("ssid", "");
  }
  if (preferences.isKey("password")) {
    password = preferences.getString("password", "");
  }
  preferences.end();

  if (ssid.length() > 0 && password.length() > 0)
  {
    Serial.println("Connecting to saved WiFi: " + ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 20;
    while (WiFi.status() != WL_CONNECTED && attempts > 0)
    {
      delay(500);
      Serial.print(".");
      attempts--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nConnection failed!");
      Serial.print("WiFi status: ");
      Serial.println(WiFi.status());
    }
  }
  else
  {
    Serial.println("No WiFi credentials saved.");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nKonnte nicht verbinden. Starte Access Point...");
    WiFi.softAP(apSSID);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    // DNS-Server starten (Alle Anfragen auf ESP umleiten)
    dnsServer.start(53, "*", apIP);
    // Captive-Portal-Erkennung abfangen und weiterleiten
    server.on("/hotspot-detect.html", handleCaptivePortal); // Apple
    server.on("/generate_204", handleCaptivePortal);        // Android
    server.on("/connecttest.txt", handleCaptivePortal);     // Windows
  }
}

void loop()
{
  // Update feedback regularly
  getFeedback();
  
  // Get current position
  position = getServoAngle();
  
  dnsServer.processNextRequest();
  handleDiscovery();
}

// JSON-Antwort senden
void sendJSONResponse(AsyncWebServerRequest *request, JsonDocument &doc, int error)
{
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
  Serial.println("Antwort gesendet: " + response);

  request->send(200, "application/json", response);
}

void handleRoot(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting...");
  response->addHeader("Location", "/setup/v1/rotator/0/setup");
  request->send(response);
}

void handleResetWifi(AsyncWebServerRequest *request)
{
  Serial.println("WiFi reset requested");
  
  // WLAN-Konfiguration zurücksetzen
  preferences.begin("wifi_config", false);
  preferences.clear();
  preferences.end();
  
  Serial.println("WiFi credentials cleared");

  // Sende eine HTML-Antwort, um den Benutzer zu informieren
  String response = "<h2>WLAN-Konfiguration zurückgesetzt! Das Gerät wird neu gestartet...</h2>";
  AsyncWebServerResponse *asyncResponse = request->beginResponse(200, "text/html; charset=UTF-8", response);
  request->send(asyncResponse);

  // Füge eine kleine Verzögerung hinzu, bevor das Gerät neu startet
  delay(2000);
  ESP.restart();
}
// 💡 Captive-Portal-Erkennung abfangen
void handleCaptivePortal(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting...");
  response->addHeader("Location", "http://192.168.1.1/");
  request->send(response);
}

void handleSetupPage(AsyncWebServerRequest *request)
{
  String html = "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; padding: 20px; }";
  html += "h2 { color: #333; }";
  html += "form { background: white; max-width: 400px; margin: auto; padding: 20px; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); }";
  html += "input { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; }";
  html += "button { background-color: #007BFF; color: white; border: none; padding: 10px; cursor: pointer; border-radius: 5px; padding: 10px 20px; font-size: 16px; }";
  html += "button:hover { background-color: #0056b3; }";
  html += "</style>";
  html += "<title>Rotator WiFi Setup</title>";
  html += "</head><body>";
  html += "<h2>Setup-Menü</h2>";
  html += "<button type='button' onclick='location.href=\"/setup/v1/rotator/0/wifi\"'>WIFI Einstellungen</button><br><br>";
  html += "<button type='button' onclick='location.href=\"/setup/v1/rotator/0/configdevices\"'>Rotator Setup</button><br><br>";
  html += "<button type='button' onclick='if(confirm(\"WLAN-Konfiguration wirklich zurücksetzen?\")) location.href=\"/reset\"' style='background-color: #dc3545;'>WLAN zurücksetzen</button>";
  html += "</body></html>";

  AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
  request->send(response);
}

void handleWifiSetupPage(AsyncWebServerRequest *request)
{
  Serial.println("WiFi setup page requested");
  
  String html = "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; padding: 20px; }";
  html += "h2 { color: #333; }";
  html += "form { background: white; max-width: 400px; margin: auto; padding: 20px; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); }";
  html += "input, label { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; display: block; text-align: left; }";
  html += "input[type='submit'] { background-color: #007BFF; color: white; border: none; cursor: pointer; text-align: center; }";
  html += "input[type='submit']:hover { background-color: #0056b3; }";
  html += "label { border: none; padding: 5px 0; font-weight: bold; }";
  html += "</style>";
  html += "<title>WLAN Konfiguration</title>";
  html += "</head><body>";
  html += "<h2>WLAN Konfiguration</h2>";
  html += "<form action='/setup/v1/rotator/0/save' method='POST'>";
  html += "<label>SSID (Netzwerkname):</label>";
  html += "<input type='text' name='ssid_manual' placeholder='Netzwerk-Name eingeben' required>";
  html += "<label>Passwort:</label>";
  html += "<input type='password' name='password' placeholder='Passwort eingeben' required>";
  html += "<input type='submit' value='Speichern und neu starten'>";
  html += "</form>";
  html += "</body></html>";
  
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
  request->send(response);
  Serial.println("WiFi setup page sent");
}

void handleSave(AsyncWebServerRequest *request)
{
  String ssid = request->arg("ssid_manual");
  if (ssid == "")
  {
    ssid = request->arg("ssid");
  }
  String password = request->arg("password");

  if (ssid != "" && password != "")
  {
    preferences.begin("wifi_config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", "<h3>Neues WLAN gespeichert! Bitte das Gerät vom Strom trennen und wiederverbinden.</h3>");
    request->send(response);
    delay(3000);
    ESP.restart();
  }
  else
  {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", "Fehlende Parameter");
    request->send(response);
  }
}

void handleConfigDevices(AsyncWebServerRequest *request)
{
  String html = "<!DOCTYPE HTML> <html> <head> <title>MoMa Rotator Panel by geo</title> <meta name='viewport' content='width=device-width, initial-scale=1'> <style> html { font-family: Arial; background: #181818; color: #ededed; } body { max-width: 360px; margin: 0 auto; padding-bottom: 1px; background: rgb(27, 90, 76); background-image: url(/backGround4.png); background-repeat: no-repeat; background-size: cover; background-blend-mode:luminosity; border-radius: 12px; box-shadow: 0 0 8px #111; } .newbackground { max-width: 360px; margin: 0 auto; padding-bottom: 5px; background: rgb(224, 117, 117); background-image: url(backGround4.png); background-size: cover; background-repeat: no-repeat; background-blend-mode: difference; border-radius: 12px; box-shadow: 0 0 8px #111; }\
   .newbackground2 { max-width: 360px; margin: 0 auto; padding-bottom: 110px; background: rgb(77, 74, 142); background-image: url(backGround4.png); background-size: cover; background-repeat: no-repeat; background-blend-mode: difference; border-radius: 12px; box-shadow: 0 0 8px #111; } .newbackground3 { max-width: 360px; margin: 0 auto; padding-bottom: 120px; padding-top: 5px; background: rgb(77, 74, 142); background-image: url(backGround4.png); background-size: cover; background-repeat: no-repeat; background-blend-mode: difference; border-radius: 12px; box-shadow: 0 0 8px #111; } h2 { font-size: 2.0rem; padding-top: 30px; padding-bottom: 10px; margin-top: 18px; text-align: center; } h3 { font-size: 1.2rem; margin-top: 12px; padding-bottom: 12px; text-align: center; }\
    .section, .row { margin-bottom: 8px; display: flex; align-items: center; justify-content: center; } .centered { display: flex; justify-content: center; align-items: center; margin-bottom: 8px; align-items: center; justify-content: center; } label { margin-right: 5px; } input[type='text'], input[type='number'] { width: 88px; height: 22px; font-size: 1.1rem; padding: 3px 6px; border-radius: 4px; border: 3px solid #e3eae3ff; background: rgb(55, 137, 75); color: #ededed; } .button { margin: 6px 2px; padding: 6px 14px; border: 0; cursor: pointer; background: #4247b7; color: #fff; border-radius: 6px; font-size: 1.15rem; outline: 0; } .button-stop { margin: 6px 2px; padding: 6px 14px; border: 0; cursor: pointer; background: #b74242; color: #fff; border-radius: 6px; font-size: 1.15rem; outline: 0; }\
     .button:hover { background: #ff494d; } .button:active { background: #4247b7; } .output-container { display: flex; flex-direction: column; align-items: center; justify-content: center; position: relative; margin-bottom: 1px; } .output-field-wrap { position: relative; display: flex; justify-content: center; } .output-line { display: flex; color: black; font-size: 1.1em; align-items: center; justify-content: center; margin-bottom: 5px; } .output-field { font-size: 1.0em; background: #e3a71dff; padding: 8px 0; border-radius: 5px; text-align: center; border: 1px solid #870b0bff; width: 85px; margin-bottom: 0; margin-top: 0; } .tooltip { display: inline-block; position: relative; top: 3px; right: 5px; cursor: pointer; color: #fdc100; font-size: 1.05em; } .tooltip \
     .tooltiptext { visibility: hidden; width: 170px; background-color: #333; color: #fff; text-align: left; border-radius: 5px; padding: 8px; position: absolute; z-index: 2; top: -42px; left: 50%; transform: translateX(-30%); font-size: 13px; opacity: 0; transition: opacity 0.3s; } .tooltip:hover .tooltiptext { visibility: visible; opacity: 1; } .switch { position: relative; display: inline-block; width: 50px; height: 24px; margin-left: 10px; } .switch input {display:none;} .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #444; transition: .4s; border-radius: 34px; } .switch { position: relative; display: inline-block; width: 50px; height: 24px; margin-left: 10px; } \
     .slider:before { position: absolute; content: ''; height: 20px; width: 20px; left: 2px; bottom: 2px; background-color: #fff; transition: .4s; border-radius: 50%; } input:checked + .slider { background-color: #4287b7; } input:checked + .slider:before { transform: translateX(26px); } .radio-group { margin-left: 10px; margin-right: 10px; } .radio-group label { margin-right: 14px; } .steps-radio-group { margin-top: 0; margin-bottom: 0; } .steps-radio-group label { margin: 0 12px; font-size: 1.08em; } .steps-radio-flex { display: flex; justify-content: center; align-items: center; margin-bottom: 18px; } .direction-label { font-size: 1.08em; margin-right: 14px; } .inFocus { font-weight: bold; color: #ffd700; } </style> </head> \
     <body> <section id='section1' style='display:block;'> <h2>Rotator Panel</h2> <div class='centered radio-group'> <label>Mode:</label> <input type='radio' id='servo' name='mode' value='servo' onclick='setMode(12); showSection('section1');' checked> <label class='inFocus' for='servo'>Servo Mode</label> <input type='radio' id='motor' name='mode' value='motor' onclick='setMode(13); showSection('section2');'> <label for='motor'>Motor Mode</label> </div> <div class='output-container'> <div class='output-field-wrap'> <br></div> <label><br>mech. Position:</label> <div class='output-field' id='virtual-pos'>3210</div> <label for='absAngle'><br>Goto abs. Angle:<br></label> <div class='section row'> <input type='number' id='posInput' name='absAngle' min='0' max='+360' step='.5' value='0'>\
      <span class='tooltip' style='right: -7px; top: 1px;'>i <span class='tooltiptext'>full rotation 0 .. 360 deg</span> </span> <button class='button' id='goto-btn' onclick='gotoAbsTarget()'>Goto</button> </div> </div> <div class='centered button-group'> </div> <label>fixpoints for one full turn</label> <div class='left-aligned section'> <button class='button' id='Zero-btn' onclick='toggleCheckbox(1,6,0,0)'>0</button> <button class='button' id='middle-btn' onclick='toggleCheckbox(1,1,0,0)'>90</button> <button class='button' id='max-btn' onclick='toggleCheckbox(1,5,0,0)'>180</button> </div> </div> <div class='section row'> <button class='button-stop' id='stop-btn-section2' onclick='toggleCheckbox(1,2,0,0)'>Stop</button> <button class='button' onclick='toggleCheckbox(1, 11, 0, 0)'>Set Middle Position</button></div> \
      <div class='section row'> <label>Rotation Speed:</label> <button class='button' id='speed-minus' onclick='toggleCheckbox(1,8,0,0)'>-</button> <button class='button' id='speed-plus' onclick='toggleCheckbox(1,7,0,0)'>+</button> </div> <div class='centered' style='margin-bottom:4px;'> <label>toggle Torque:</label> <label class='switch'> <input type='checkbox' id='toggleTorque' checked> <span class='slider'></span> </label> </div> <div class='centered' style='margin-bottom:4px;'> <label>OLED Display:</label> <label class='switch'> <input type='checkbox' id='display-switch' checked> <span class='slider'></span> </label> </div> </section> <section id='section2' class='newbackground' style='display:none;'> <h2>Rotator Panel</h2> <div class='centered radio-group'> <label>Mode:</label>\
       <input type='radio' id='servo' name='mode' value='servo' onclick='setMode(12); showSection('section1');' > <label for='servo'>Servo Mode</label> <input type='radio' id='motor' name='mode' value='motor' onclick='setMode(13); showSection('section2');' checked> <label class='inFocus' for='motor'>Motor Mode</label> </div> <div class='centered'> <label for='relAngle'><br>Goto rel. Angle:<br></label></div> <div class='section row'> <input type='number' id='relInput' name='relAngle' min='-360' max='+360' step='.5' value='0'> <button class='button' id='goto-rel-btn' onclick='gotoRelTarget()'>Goto</button> </div> </div> <div class='section row'> <button class='button-stop' id='stop-btn' onclick='toggleCheckbox(1,2,0,0)'>Stop</button> <button class='button' onclick='toggleCheckbox(1, 18, 0, 0)'>Set Position to Zero</button>\
       </div> <div class='section row'> <label>Rotation Speed:</label> <button class='button' id='speed-minus' onclick='toggleCheckbox(1,8,0,0)'>-</button> <button class='button' id='speed-plus' onclick='toggleCheckbox(1,7,0,0)'>+</button> </div> <div class='centered' style='margin-bottom:34px;'> <label>toggle Torque:</label> <label class='switch'> <input type='checkbox' id='toggleTorque' checked> <span class='slider'></span> </label> </div> <div class='output-line' id='ip'> 192.x.x.x </div> <script> serialForwardStatus = false; function showSection(id) { for (let i = 1; i <= 4; i++) { document.getElementById('section' + i).style.display = 'none'; } document.getElementById(id).style.display = 'block'; } function toggleCheckbox(inputT, inputI, inputA, inputB ,inputP ,inputD) { var xhr = new XMLHttpRequest();\
        xhr.open('GET', 'cmd?inputT='+inputT+'&inputI='+inputI+'&inputA='+inputA+'&inputB='+inputB+'&inputP='+inputP+'&inputD='+inputD, true); xhr.send(); } function ctrlMode() { xhr.open('GET', 'ctrl', true); xhr.send(); } setInterval(function() { getData(); }, 300); setInterval(function() { getServoID(); }, 1500); setInterval(function() { getIP(); }, 1200); function getData() { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.getElementById('virtual-pos').innerHTML = this.responseText; } }; xhttp.open('GET', 'position', true); xhttp.send(); } function getIP() { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.getElementById('ip').innerHTML = this.responseText; } };\
         xhttp.open('GET', 'printip', true); xhttp.send(); } document.getElementById('toggleTorque').addEventListener('change', function() { if(this.checked) { toggleCheckbox(1, 4, 0, 0); } else { toggleCheckbox(1, 3, 0, 0); } }); function gotoAbsTarget(){ var targetPosition = parseFloat(document.getElementById('posInput').value); var currentPos = parseFloat(document.getElementById('virtual-pos').textContent.trim()); toggleCheckbox(1, 17, 0, 0, targetPosition, currentPos); } function gotoRelTarget(){ var targetPosition = parseFloat(document.getElementById('relInput').value); var currentPos = parseFloat(document.getElementById('virtual-pos').textContent.trim()); toggleCheckbox(1, 17, 0, 0, targetPosition, currentPos); } function getServoID() { return 1; }\
          function setMode(value) { toggleCheckbox(1, value, 0, 0);} document.getElementById('display-switch').addEventListener('change', function() { if(this.checked) { toggleCheckbox(1, 21, 0, 0); } else { toggleCheckbox(1, 20, 0, 0); } }); </script> </body> </html>";
  
  
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
  request->send(response);
}

void handleDescription(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  JsonObject value = doc["Value"].to<JsonObject>();
  value["Manufacturer"] = "MoMa";
  value["ManufacturerVersion"] = "1.0";
  value["ServerName"] = "MoMa Rotator";

  sendJSONResponse(request, doc, 0);
}

void handleConfiguredDevices(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  JsonArray value = doc["Value"].to<JsonArray>();

  JsonObject device = value.add<JsonObject>();
  device["DeviceName"] = "Rotator";
  device["DeviceType"] = "Rotator";
  device["DeviceNumber"] = 0;
  device["UniqueID"] = "6109ff28-84d0-4f79-aa90-05ef3c191f50";

  sendJSONResponse(request, doc, 0);
}

void handleCanAsync(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  int id = request->arg("Id").toInt();
  if (id != 0)
  {
    sendJSONResponse(request, doc, 1025);
    return;
  }
  doc["Value"] = true;
  sendJSONResponse(request, doc, 0);
}

void handleSetAsync(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  int id = request->arg("Id").toInt();
  if (id != 0)
  {
    sendJSONResponse(request, doc, 1025);
    return;
  }
  sendJSONResponse(request, doc, 0);
}

void handleCancelAsync(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  int id = request->arg("Id").toInt();
  if (id != 0)
  {
    sendJSONResponse(request, doc, 1025);
    return;
  }
  sendJSONResponse(request, doc, 0);
}

void handleStateChangeComplete(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  int id = request->arg("Id").toInt();
  if (id != 0)
  {
    sendJSONResponse(request, doc, 1025);
    return;
  }
  doc["Value"] = true;
  sendJSONResponse(request, doc, 0);
}

void handleDriverInfo(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = "MoMa DIY Rotator";
  sendJSONResponse(request, doc, 0);
}

void handleDriverVersion(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = "1.0";
  sendJSONResponse(request, doc, 0);
}

void handleGetName(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  int id = request->arg("Id").toInt();
  if (id != 0)
  {
    sendJSONResponse(request, doc, 1025);
    return;
  }
  doc["Value"] = name;
  sendJSONResponse(request, doc, 0);
}

void handleSupportedActions(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  JsonArray actions = doc["Value"].to<JsonArray>();
  sendJSONResponse(request, doc, 0);
}

void handleApiVersion(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  JsonArray versions = doc["Value"].to<JsonArray>();
  versions.add(1);
  sendJSONResponse(request, doc, 0);
}

// Gerät verbinden/trennen

void handleGetConnected(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = isConnected;
  sendJSONResponse(request, doc, 0);
}

void handleSetConnected(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  if (!request->hasArg("Connected"))
  {
    sendJSONResponse(request, doc, 1025);
    return;
  }
  isConnected = request->arg("Connected").equalsIgnoreCase("true");
  doc["Value"] = isConnected;
  sendJSONResponse(request, doc, 0);
}

void handleConnect(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  isConnected = true;
  sendJSONResponse(request, doc, 0);
}

void handleDisconnect(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  isConnected = false;
  sendJSONResponse(request, doc, 0);
}

void handleGetConnecting(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = false;
  sendJSONResponse(request, doc, 0);
}

void handleDeviceState(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  JsonArray value = doc["Value"].to<JsonArray>();

  sendJSONResponse(request, doc, 0);
}

void handleGetDescription(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  int id = request->arg("Id").toInt();
  if (id != 0)
  {
    sendJSONResponse(request, doc, 1025);
    return;
  }
  doc["Value"] = String("MoMa Rotator");
  sendJSONResponse(request, doc, 0);
}

void handleGetInterfaceVersion(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = 3;
  sendJSONResponse(request, doc, 0);
}

// Fehlerseite (404 Not Found)
void handleNotFound(AsyncWebServerRequest *request)
{
  String message = "Fehler 404 - Seite nicht gefunden: " + request->url() + " " + request->method();
  Serial.println(message);
}

void handleDiscovery()
{
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    Serial.print("Received packet of size: ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remoteIp = udp.remoteIP();
    Serial.print(remoteIp);
    Serial.print(", on port ");
    Serial.println(udp.remotePort());

    // read the packet into packetBufffer
    int len = udp.read(packetBuffer, 255);
    if (len > 0)
    {
      packetBuffer[len] = 0;
    }
    Serial.print("Contents: ");
    Serial.println(packetBuffer);

    // No undersized packets allowed
    if (len < 16)
    {
      return;
    }

    if (strncmp("alpacadiscovery1", packetBuffer, 16) != 0)
    {
      return;
    }

    char response[36] = {0};

    sprintf(response, "{\"AlpacaPort\": %d}", alpacaPort);

    // send a reply, to the IP address and port that sent us the packet we received
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((uint8_t *)response, strlen(response));
    udp.endPacket();
  }
}

void handleCanReverse(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = true;
  sendJSONResponse(request, doc, 0);
}
void handleIsMoving(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = isServoMoving();
  sendJSONResponse(request, doc, 0);
}
void handleMechanicalPosition(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = getServoAngle();
  sendJSONResponse(request, doc, 0);
}
void handlePosition(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  // Returns Current instantaneous Rotator position, in degrees (virtual gear position)
  doc["Value"] = getServoAngle();
  sendJSONResponse(request, doc, 0);
}

void handleGetReverse(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["Value"] = reverseState;
  sendJSONResponse(request, doc, 0);
}

void handleSetReverse(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  String reverseStr = request->arg("Reverse");
  bool value = (reverseStr == "true");
  //reverseState = value;
  reverseState = true;
  sendJSONResponse(request, doc, 0);
}

void handleStepSize(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  // Step size in degrees: 4096 steps for 360° motor / 2 (gear ratio) = 0.0439° per step on gear
  doc["Value"] = (360.0 / 4096.0) / 2.0; // ~0.0439°
  sendJSONResponse(request, doc, 0);
}
void handleTargetPosition(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  // The destination position angle for Move() and MoveAbsolute().
  doc["Value"] = getServoAngle();
  sendJSONResponse(request, doc, 0);
}

void handleHalt(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  stopServo();
  sendJSONResponse(request, doc, 0);
}

void handleMove(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  double value = request->arg("Position").toDouble();
  double currentAngle = getServoAngle();
  double newPosition = currentAngle + value;
  
  // Validate range
  if (newPosition < 0.0 || newPosition > 359.99)
  {
    sendJSONResponse(request, doc, 1025);
  }
  else
  {
    sendJSONResponse(request, doc, 0);
    moveRotator(value);
  }
}

void handleMoveAbsolute(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  double value = request->arg("Position").toDouble();

  if (value < 0.0 || value > 359.99)
  {
    sendJSONResponse(request, doc, 1025);
  }
  else
  {
    sendJSONResponse(request, doc, 0);
    moveRotatorAbsolute(value);
  }
}

void handleMoveMechanical(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  double value = request->arg("Position").toDouble();
  if (value < 0.0 || value > 359.99)
  {
    sendJSONResponse(request, doc, 1025);
  }
  else
  {
    sendJSONResponse(request, doc, 0);
    moveMechanical(value);
  }
}

void handleSync(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  double value = request->arg("Position").toDouble();
  // Set the current rotator position to the specified value without moving the rotator.
  // This essentially sets a new zero point
  setZeroPointExact();
  sendJSONResponse(request, doc, 0);
}

void moveRotatorAbsolute(double degrees)
{// Directly move to absolute position 0 .. 360°
  moveServoToAngle(degrees);
  targetPosition = degrees;
}

void moveRotator(double degrees)
{// Move relative +/- degrees
  moveServoByAngle(degrees);
  targetPosition = getServoAngle();
}

void moveMechanical(double degrees)
{// Directly move to mechanical position (same as absolute for rotator)
  moveServoToAngle(degrees);
  targetPosition = degrees;
}
