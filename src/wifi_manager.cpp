#include "wifi_manager.h"
#include "servo_control.h"
#include "display_control.h"

// Access Point configuration
const char *apSSID = "MoMaRoTa";
IPAddress apIP(192, 168, 1, 1);
static DNSServer dnsServer;
static Preferences preferences;

// ============================================================================
// WIFI INITIALIZATION & CONNECTION
// ============================================================================

void initWiFi() {
    connectWiFi();
}

void connectWiFi() {
    // TEST: Fixed WiFi credentials
    String ssid = "FiegeosNetz";
    String password = "Manuelnoir1";
    
    // Disconnect and clear previous config
    WiFi.disconnect(true);
    delay(1000);
    
    // Set WiFi mode to station
    WiFi.mode(WIFI_STA);
    
    Serial.println("Connecting to WiFi: " + ssid);
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 60;  // Increased to 30 seconds
    while (WiFi.status() != WL_CONNECTED && attempts > 0) {
        delay(500);
        Serial.print(".");
        if (attempts % 10 == 0) {
            Serial.print(" [Status: ");
            Serial.print(WiFi.status());
            Serial.print("] ");
        }
        attempts--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("\nConnection failed!");
        Serial.print("WiFi status: ");
        Serial.println(WiFi.status());
        Serial.println("Status codes: 0=IDLE, 1=NO_SSID, 3=CONNECTED, 4=FAILED, 5=LOST, 6=DISCONNECTED");
    }

    // Start Access Point if connection failed
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nStarting Access Point...");
        WiFi.softAP(apSSID);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        
        // Start DNS server for captive portal
        dnsServer.start(53, "*", apIP);
        
        Serial.print("AP started: ");
        Serial.println(apSSID);
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
    }
}

void processDNS() {
    // Only process DNS requests in AP mode (for captive portal)
    // In STA mode (connected to home WiFi), DNS is not needed and causes UDP errors
    if(WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        dnsServer.processNextRequest();
    }
}

String getIPAddress() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    } else {
        return WiFi.softAPIP().toString();
    }
}

// ============================================================================
// SETUP ENDPOINTS
// ============================================================================

void setupWiFiEndpoints(AsyncWebServer &server) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting...");
        response->addHeader("Location", "/setup/v1/rotator/0/setup");
        request->send(response);
    });
    
    server.on("/setup/v1/rotator/0/setup", HTTP_GET, handleSetupPage);
    server.on("/setup/v1/rotator/0/wifi", HTTP_GET, handleWifiSetupPage);
    server.on("/setup/v1/rotator/0/save", HTTP_POST, handleSaveWifi);
    server.on("/setup/v1/rotator/0/configdevices", HTTP_GET, handleConfigDevices);
    server.on("/reset", HTTP_GET, handleResetWifi);
    
    // Endpoint to retrieve stored WiFi credentials (JSON format)
    server.on("/wifi/credentials", HTTP_GET, [](AsyncWebServerRequest *request) {
        Preferences preferences;
        preferences.begin("wifi_config", true);
        String ssid = preferences.getString("ssid", "");
        String password = preferences.getString("password", "");
        preferences.end();
        
        String json = "{";
        json += "\"ssid\":\"" + ssid + "\",";
        json += "\"password\":\"" + password + "\"";
        json += "}";
        
        request->send(200, "application/json", json);
    });
    
    // Endpoint to retrieve stored WiFi credentials (HTML format)
    server.on("/wifi/info", HTTP_GET, [](AsyncWebServerRequest *request) {
        Preferences preferences;
        preferences.begin("wifi_config", true);
        String ssid = preferences.getString("ssid", "");
        String password = preferences.getString("password", "");
        preferences.end();
        
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta charset='UTF-8'>";
        html += "<title>WiFi Credentials</title>";
        html += "<style>";
        html += "body { font-family: Arial, sans-serif; margin: 40px; background: #f0f0f0; }";
        html += ".container { background: white; padding: 30px; border-radius: 8px; max-width: 500px; margin: 0 auto; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
        html += "h2 { color: #333; margin-top: 0; }";
        html += ".credential { margin: 20px 0; padding: 15px; background: #f8f8f8; border-radius: 5px; }";
        html += ".label { font-weight: bold; color: #666; margin-bottom: 5px; }";
        html += ".value { font-family: monospace; color: #333; word-break: break-all; }";
        html += ".empty { color: #999; font-style: italic; }";
        html += "</style></head><body>";
        html += "<div class='container'>";
        html += "<h2>Gespeicherte WiFi Credentials</h2>";
        html += "<div class='credential'>";
        html += "<div class='label'>SSID (Netzwerkname):</div>";
        html += "<div class='value'>";
        if (ssid.length() > 0) {
            html += ssid;
        } else {
            html += "<span class='empty'>(nicht gesetzt)</span>";
        }
        html += "</div></div>";
        html += "<div class='credential'>";
        html += "<div class='label'>Password:</div>";
        html += "<div class='value'>";
        if (password.length() > 0) {
            html += password;
        } else {
            html += "<span class='empty'>(nicht gesetzt)</span>";
        }
        html += "</div></div>";
        html += "</div></body></html>";
        
        request->send(200, "text/html; charset=UTF-8", html);
    });
    
    // Position and status endpoints (needed by control panel JavaScript)
    server.on("/setup/v1/rotator/0/position", HTTP_GET, [](AsyncWebServerRequest *request) {
        double pos = getServoAngle();
        String posValue = String(pos, 2);
        request->send(200, "text/plain", posValue);
    });
    
    server.on("/setup/v1/rotator/0/printip", HTTP_GET, [](AsyncWebServerRequest *request) {
        String ip = getIPAddress();
        request->send(200, "text/plain", ip);
    });
    
    // Control panel command handler (register both short and full paths)
    auto cmdHandler = [](AsyncWebServerRequest *request) {
        int cmdT = request->arg("inputT").toInt();
        int cmdI = request->arg("inputI").toInt();
        double cmdP = request->arg("inputP").toDouble();
        
        switch(cmdI) {
            case 1:  // 90° button
                moveServoToAngle(90.0);
                break;
            case 2:  // Stop
                stopServo();
                break;
            case 5:  // 180° button
                moveServoToAngle(180.0);
                break;
            case 6:  // 0° button
                moveServoToAngle(0.0);
                break;
            case 7:  // Speed +
                setActiveSpeed(getActiveSpeed() + 100);
                break;
            case 8:  // Speed -
                setActiveSpeed(getActiveSpeed() - 100);
                break;
            case 17: // Goto position
                moveServoToAngle(cmdP);
                break;
            case 18: // Set zero
                setZeroPointExact();
                break;
            case 20: // Display OFF
                displayOff();
                break;
            case 21: // Display ON
                displayOn();
                break;
            case 22: // Reverse ON
                setReverseDirection(true);
                break;
            case 23: // Reverse OFF
                setReverseDirection(false);
                break;
        }
        request->send(200, "text/plain", "OK");
    };
    
    server.on("/cmd", HTTP_GET, cmdHandler);
    server.on("/setup/v1/rotator/0/cmd", HTTP_GET, cmdHandler);

    
    server.on("/position", HTTP_GET, [](AsyncWebServerRequest *request) {
        double pos = getServoAngle();
        String posValue = String(pos, 2);
        request->send(200, "text/plain", posValue);
    });
    
    server.on("/printip", HTTP_GET, [](AsyncWebServerRequest *request) {
        String ip = getIPAddress();
        request->send(200, "text/plain", ip);
    });
    
    // Captive portal detection
    server.on("/hotspot-detect.html", handleCaptivePortal); // Apple
    server.on("/generate_204", handleCaptivePortal);        // Android
    server.on("/connecttest.txt", handleCaptivePortal);     // Windows
}

void handleCaptivePortal(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting...");
    response->addHeader("Location", "http://192.168.1.1/");
    request->send(response);
}

void handleSetupPage(AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>MoMa Rotator Setup</title>";
    html += "<style>";
    html += "html{font-family:Arial;background:#181818;color:#ededed}";
    html += "body{max-width:360px;margin:0 auto;padding:20px;background:rgb(27,90,76);border-radius:12px;box-shadow:0 0 8px #111}";
    html += "h2{font-size:2rem;padding:20px 0 10px;margin:0;text-align:center}";
    html += ".button{display:block;width:100%;margin:12px 0;padding:16px;border:0;cursor:pointer;background:#4247b7;color:#fff;border-radius:6px;font-size:1.1rem;text-align:center;text-decoration:none;box-sizing:border-box}";
    html += ".button:hover{background:#5c61d4}";
    html += ".btn-danger{background:#b74242}";
    html += ".btn-danger:hover{background:#d45555}";
    html += ".divider{border-top:2px solid #555;margin:20px 0;width:100%}";
    html += "</style>";
    html += "</head><body>";
    html += "<h2>MoMa Rotator Setup</h2>";
    html += "<a href='/setup/v1/rotator/0/wifi' class='button'>WiFi Settings</a>";
    html += "<a href='/setup/v1/rotator/0/configdevices' class='button'>Rotator Control</a>";
    html += "<div class='divider'></div>";
    html += "<button class='button btn-danger' onclick='if(confirm(\"Reset WiFi configuration?\")) location.href=\"/reset\"'>Reset WiFi</button>";
    html += "</body></html>";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
    request->send(response);
}

void handleWifiSetupPage(AsyncWebServerRequest *request) {
    Serial.println("WiFi setup page requested");

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>WiFi Configuration</title>";
    html += "<style>";
    html += "html{font-family:Arial;background:#181818;color:#ededed}";
    html += "body{max-width:360px;margin:0 auto;padding:20px;background:rgb(27,90,76);border-radius:12px;box-shadow:0 0 8px #111}";
    html += "h2{font-size:2rem;padding:20px 0 10px;margin:0;text-align:center}";
    html += "form{width:100%;box-sizing:border-box}";
    html += "label{display:block;margin:15px 0 5px;font-size:1.1rem;color:#fdc100}";
    html += "input[type='text'],input[type='password']{width:100%;padding:12px;margin:5px 0 15px;border:2px solid #e3eae3;border-radius:5px;background:rgb(55,137,75);color:#ededed;font-size:1rem;box-sizing:border-box}";
    html += "input[type='text']::placeholder,input[type='password']::placeholder{color:#aaa}";
    html += "input[type='submit']{display:block;width:100%;margin:20px 0 10px;padding:16px;border:0;cursor:pointer;background:#4247b7;color:#fff;border-radius:6px;font-size:1.1rem;text-align:center}";
    html += "input[type='submit']:hover{background:#5c61d4}";
    html += ".back-link{display:block;text-align:center;margin-top:20px;color:#fdc100;text-decoration:none}";
    html += ".back-link:hover{color:#fff}";
    html += "</style>";
    html += "</head><body>";
    html += "<h2>WiFi Configuration</h2>";
    html += "<form action='/setup/v1/rotator/0/save' method='POST'>";
    html += "<label>SSID (Network Name):</label>";
    html += "<input type='text' name='ssid_manual' placeholder='Enter network name' required>";
    html += "<label>Password:</label>";
    html += "<input type='password' name='password' placeholder='Enter password' required>";
    html += "<input type='submit' value='Save and Restart'>";
    html += "</form>";
    html += "<a href='/setup/v1/rotator/0/setup' class='back-link'>&larr; Back to Setup</a>";
    html += "</body></html>";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
    request->send(response);
}

void handleSaveWifi(AsyncWebServerRequest *request) {
    String ssid = request->arg("ssid_manual");
    if (ssid == "") {
        ssid = request->arg("ssid");
    }
    String password = request->arg("password");

    if (ssid != "" && password != "") {
        preferences.begin("wifi_config", false);
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.end();

        Serial.println("WiFi credentials saved: " + ssid);

        String html = "<h3>WiFi credentials saved! Restarting...</h3>";
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
        request->send(response);
        
        delay(2000);
        ESP.restart();
    } else {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", "Missing parameters");
        request->send(response);
    }
}

void handleResetWifi(AsyncWebServerRequest *request) {
    Serial.println("WiFi reset requested");

    preferences.begin("wifi_config", false);
    preferences.clear();
    preferences.end();

    Serial.println("WiFi credentials cleared");

    String html = "<h2>WiFi configuration reset! Restarting...</h2>";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
    request->send(response);

    delay(2000);
    ESP.restart();
}

void handleConfigDevices(AsyncWebServerRequest *request) {
    // Embedded HTML for control panel
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>MoMa Rotator Panel</title>";
    html += "<style>";
    html += "html{font-family:Arial;background:#181818;color:#ededed}";
    html += "body{max-width:360px;margin:0 auto;padding:20px;background:rgb(27,90,76);border-radius:12px;box-shadow:0 0 8px #111}";
    html += "h2{font-size:2rem;padding:20px 0 10px;margin:0;text-align:center}";
    html += ".centered{display:flex;justify-content:center;align-items:center;margin-bottom:12px}";
    html += ".section{display:flex;align-items:center;justify-content:center;margin-bottom:8px;gap:8px}";
    html += "label{margin:0 8px 0 0;font-size:1.1rem}";
    html += ".small-label{font-size:0.85rem;color:#ddd;margin:2px 0 8px;text-align:center}";
    html += ".output-field{font-size:1.3rem;background:#e3a71d;padding:12px;border-radius:5px;text-align:center;border:1px solid #870b0b;min-width:100px;margin-bottom:4px}";
    html += "input[type='number']{width:120px;height:28px;font-size:1.1rem;padding:4px 8px;border-radius:4px;border:2px solid #e3eae3;background:rgb(55,137,75);color:#ededed;text-align:center}";
    html += ".button{margin:4px;padding:6px 12px;border:0;cursor:pointer;background:#4247b7;color:#fff;border-radius:6px;font-size:0.95rem;min-width:70px}";
    html += ".button-stop{margin:4px;padding:6px 12px;border:0;cursor:pointer;background:#b74242;color:#fff;border-radius:6px;font-size:0.95rem;min-width:70px}";
    html += ".button:hover{background:#ff494d}";
    html += ".divider{border-top:2px solid #555;margin:20px 0;width:100%}";
    html += ".section-title{text-align:center;font-size:1.2rem;margin:15px 0 10px;color:#fdc100}";
    html += ".output-line{display:flex;color:#ccc;font-size:0.95em;justify-content:center;margin-top:20px}";
    html += "</style></head><body>";
    html += "<h2>Rotator Panel</h2>";
    html += "<div class='centered' style='flex-direction:column'>";
    html += "<label>Position in &deg;</label>";
    html += "<div class='output-field' id='virtual-pos'>0.0</div></div>";
    html += "<div style='display:flex;align-items:center;justify-content:center;gap:8px;margin-bottom:8px'>";
    html += "<input type='number' id='posInput' min='-360' max='360' step='0.1' value='0'>";
    html += "<button class='button' onclick='gotoTarget()'>Goto</button></div>";
    html += "<div class='centered' style='gap:8px'><label style='font-size:0.95rem'>Reverse:</label>";
    html += "<input type='checkbox' id='reverseCheckbox' onchange='toggleReverse()' style='width:auto;height:18px'>";
    html += "</div>";
    html += "<div class='small-label'>(-360 .. +360&deg;)</div>";
    html += "<div class='divider'></div>";
    html += "<div class='section-title'>Goto Position ...</div>";
    html += "<div class='section'>";
    html += "<button class='button-stop' onclick='cmd(1,2)'>Stop</button>";
    html += "<button class='button' onclick='cmd(1,6)'>0&deg;</button>";
    html += "<button class='button' onclick='cmd(1,1)'>90&deg;</button>";
    html += "<button class='button' onclick='cmd(1,5)'>180&deg;</button></div>";
    html += "<div class='section'>";
    html += "<button class='button' onclick='moveToAngle(270)'>270&deg;</button>";
    html += "<button class='button' onclick='moveToAngle(360)'>360&deg;</button></div>";
    html += "<div class='divider'></div>";
    html += "<div class='section-title'>Sync Current Position as</div>";
    html += "<div class='section'>";
    html += "<button class='button' onclick='cmd(1,18)'>Zero</button></div>";
    html += "<div class='section'><label>Speed:</label>";
    html += "<button class='button' onclick='cmd(1,7)'>+</button>";
    html += "<button class='button' onclick='cmd(1,8)'>-</button></div>";
    html += "<div class='divider'></div>";
    html += "<div class='centered' style='gap:8px'><label style='font-size:0.95rem'>Display:</label>";
    html += "<button class='button' onclick='cmd(1,21)' style='min-width:50px'>ON</button>";
    html += "<button class='button' onclick='cmd(1,20)' style='min-width:50px'>OFF</button></div>";
    html += "<div class='output-line' id='ip'>--.--.--.--</div>";
    html += "<script>";
    html += "function cmd(t,i,a=0,b=0,p=0,d=0){";
    html += "var x=new XMLHttpRequest();";
    html += "x.open('GET',`cmd?inputT=${t}&inputI=${i}&inputA=${a}&inputB=${b}&inputP=${p}&inputD=${d}`,true);";
    html += "x.send();}";
    html += "function gotoTarget(){";
    html += "var val=parseFloat(document.getElementById('posInput').value);";
    html += "if(isNaN(val)||val<-360||val>360){alert('Value -360 to +360');return;}";
    html += "cmd(1,17,0,0,val,0);}";
    html += "function moveToAngle(angle){cmd(1,17,0,0,angle,0);}";
    html += "function getData(){";
    html += "var x=new XMLHttpRequest();";
    html += "x.onreadystatechange=function(){";
    html += "if(this.readyState==4&&this.status==200){";
    html += "document.getElementById('virtual-pos').innerHTML=parseFloat(this.responseText).toFixed(2);}};";
    html += "x.open('GET','position',true);x.send();}";
    html += "function getIP(){";
    html += "var x=new XMLHttpRequest();";
    html += "x.onreadystatechange=function(){";
    html += "if(this.readyState==4&&this.status==200){";
    html += "document.getElementById('ip').innerHTML=this.responseText;}};";
    html += "x.open('GET','printip',true);x.send();}";
    html += "function toggleReverse(){";
    html += "var checked=document.getElementById('reverseCheckbox').checked;";
    html += "cmd(1,checked?22:23);}";
    html += "setInterval(getData,300);";
    html += "setInterval(getIP,1200);";
    html += "</script></body></html>";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
    request->send(response);
}
