/**
 * WiFiConfigPortal.cpp
 * 
 * Implementation des WiFi-Konfigurationsportals
 * ANGEPASSTE VERSION: Nutzt einen bestehenden AsyncWebServer
 */

#include "WiFiConfigPortal.h"

// HTML-Template (als konstante Zeichenkette)
const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MoMa Rotator - WiFi Setup</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1b5a4c 0%, #2d7a66 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            padding: 40px;
            max-width: 500px;
            width: 100%;
            animation: slideIn 0.5s ease-out;
        }
        
        @keyframes slideIn {
            from {
                opacity: 0;
                transform: translateY(-30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 10px;
            font-size: 28px;
        }
        
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        
        .back-link {
            display: block;
            text-align: center;
            margin-bottom: 20px;
            color: #1b5a4c;
            text-decoration: none;
            font-weight: 600;
        }
        
        .back-link:hover {
            color: #2d7a66;
        }
        
        .form-group {
            margin-bottom: 25px;
        }
        
        label {
            display: block;
            color: #555;
            font-weight: 600;
            margin-bottom: 8px;
            font-size: 14px;
        }
        
        select, input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px 15px;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            font-size: 15px;
            transition: all 0.3s ease;
            background: #f8f9fa;
        }
        
        select:focus, input:focus {
            outline: none;
            border-color: #1b5a4c;
            background: white;
            box-shadow: 0 0 0 3px rgba(27, 90, 76, 0.1);
        }
        
        select {
            cursor: pointer;
            appearance: none;
            background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12'%3E%3Cpath fill='%231b5a4c' d='M6 9L1 4h10z'/%3E%3C/svg%3E");
            background-repeat: no-repeat;
            background-position: right 15px center;
            padding-right: 40px;
        }
        
        .btn {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #1b5a4c 0%, #2d7a66 100%);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(27, 90, 76, 0.4);
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(27, 90, 76, 0.6);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn-secondary {
            background: linear-gradient(135deg, #4247b7 0%, #5c61d4 100%);
            box-shadow: 0 4px 15px rgba(66, 71, 183, 0.4);
            margin-top: 10px;
        }
        
        .btn-secondary:hover {
            box-shadow: 0 6px 20px rgba(66, 71, 183, 0.6);
        }
        
        .divider {
            text-align: center;
            margin: 30px 0;
            color: #999;
            font-size: 13px;
            position: relative;
        }
        
        .divider::before,
        .divider::after {
            content: '';
            position: absolute;
            top: 50%;
            width: 40%;
            height: 1px;
            background: #e0e0e0;
        }
        
        .divider::before {
            left: 0;
        }
        
        .divider::after {
            right: 0;
        }
        
        .status {
            margin-top: 20px;
            padding: 15px;
            border-radius: 10px;
            text-align: center;
            font-weight: 500;
            display: none;
        }
        
        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        
        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        
        .status.info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }
        
        .icon {
            display: inline-block;
            width: 60px;
            height: 60px;
            margin: 0 auto 20px;
            background: linear-gradient(135deg, #1b5a4c 0%, #2d7a66 100%);
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .icon svg {
            width: 30px;
            height: 30px;
            fill: white;
        }
        
        @media (max-width: 480px) {
            .container {
                padding: 30px 20px;
            }
            
            h1 {
                font-size: 24px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <a href="/setup/v1/rotator/0/setup" class="back-link">← Zurück zum Setup</a>
        
        <div class="icon">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
                <path d="M12 21L0 9C3.97 5.03 7.97 3 12 3s8.03 2.03 12 6l-12 12zm0-17.5c-3.48 0-6.98 1.72-10.5 5.25l10.5 10.5 10.5-10.5c-3.52-3.53-7.02-5.25-10.5-5.25zM12 9c-2.2 0-4 1.8-4 4h2c0-1.1.9-2 2-2s2 .9 2 2h2c0-2.2-1.8-4-4-4z"/>
            </svg>
        </div>
        
        <h1>WiFi Konfiguration</h1>
        <p class="subtitle">Verbinden Sie MoMa Rotator mit Ihrem Netzwerk</p>
        
        <form id="wifiForm">
            <div class="form-group">
                <label for="networkSelect">📡 Verfügbare Netzwerke</label>
                <select id="networkSelect" name="ssid">
                    <option value="">Netzwerk wird geladen...</option>
                </select>
            </div>
            
            <div class="form-group">
                <label for="password">🔒 Passwort</label>
                <input type="password" id="password" name="password" placeholder="Netzwerk-Passwort eingeben">
            </div>
            
            <button type="submit" class="btn">Verbinden und Speichern</button>
            
            <div class="divider">ODER</div>
            
            <div class="form-group">
                <label for="manualSSID">📝 Manuelle SSID-Eingabe (für versteckte Netzwerke)</label>
                <input type="text" id="manualSSID" name="manualSSID" placeholder="SSID eingeben">
            </div>
            
            <div class="form-group">
                <label for="manualPassword">🔒 Passwort</label>
                <input type="password" id="manualPassword" name="manualPassword" placeholder="Passwort eingeben">
            </div>
            
            <button type="button" class="btn btn-secondary" onclick="connectManual()">Mit manueller SSID verbinden</button>
        </form>
        
        <div id="status" class="status"></div>
    </div>
    
    <script>
        function loadNetworks() {
            fetch('/wifi/scan')
                .then(response => response.json())
                .then(data => {
                    const select = document.getElementById('networkSelect');
                    select.innerHTML = '<option value="">-- Netzwerk auswählen --</option>';
                    
                    data.networks.forEach(network => {
                        const option = document.createElement('option');
                        option.value = network.ssid;
                        const signal = network.rssi > -50 ? '📶📶📶' : network.rssi > -70 ? '📶📶' : '📶';
                        const security = network.encryption !== 'Open' ? '🔒' : '🔓';
                        option.textContent = `${signal} ${security} ${network.ssid} (${network.rssi} dBm)`;
                        select.appendChild(option);
                    });
                })
                .catch(error => {
                    console.error('Fehler beim Laden der Netzwerke:', error);
                    showStatus('Fehler beim Laden der Netzwerke', 'error');
                });
        }
        
        function showStatus(message, type) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
            
            if (type === 'success') {
                setTimeout(() => {
                    status.style.display = 'none';
                }, 5000);
            }
        }
        
        document.getElementById('wifiForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const ssid = document.getElementById('networkSelect').value;
            const password = document.getElementById('password').value;
            
            if (!ssid) {
                showStatus('Bitte wählen Sie ein Netzwerk aus', 'error');
                return;
            }
            
            connectToWiFi(ssid, password);
        });
        
        function connectManual() {
            const ssid = document.getElementById('manualSSID').value;
            const password = document.getElementById('manualPassword').value;
            
            if (!ssid) {
                showStatus('Bitte geben Sie eine SSID ein', 'error');
                return;
            }
            
            connectToWiFi(ssid, password);
        }
        
        function connectToWiFi(ssid, password) {
            showStatus('Verbinde mit ' + ssid + '...', 'info');
            
            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', password);
            
            fetch('/wifi/connect', {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showStatus('✓ ' + data.message + ' Gerät wird neu gestartet...', 'success');
                    setTimeout(() => {
                        window.location.href = '/setup/v1/rotator/0/setup';
                    }, 3000);
                } else {
                    showStatus('✗ ' + data.message, 'error');
                }
            })
            .catch(error => {
                console.error('Fehler:', error);
                showStatus('Verbindungsfehler aufgetreten', 'error');
            });
        }
        
        loadNetworks();
        setInterval(loadNetworks, 15000);
    </script>
</body>
</html>
)rawliteral";

// Konstruktor
WiFiConfigPortal::WiFiConfigPortal(const char* apSSID, const char* apPassword) 
    : _apSSID(apSSID), 
      _apPassword(apPassword),
      _isConnecting(false),
      _lastStatusCheck(0),
      _onConnectCallback(nullptr),
      _onDisconnectCallback(nullptr) {
}

// Destruktor
WiFiConfigPortal::~WiFiConfigPortal() {
}

// Starte das Portal mit bestehendem Server
void WiFiConfigPortal::begin(AsyncWebServer &server) {
    Serial.println("\n=================================");
    Serial.println("   WiFi Config Portal");
    Serial.println("=================================\n");
    
    // Versuche zunächst gespeicherte Credentials
    if (tryStoredCredentials()) {
        Serial.println("✓ Mit gespeichertem Netzwerk verbunden!");
        return;
    }
    
    // Falls keine Verbindung: Starte Access Point
    Serial.println("Starte Access Point für Konfiguration...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(_apSSID, _apPassword);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access Point: ");
    Serial.println(_apSSID);
    Serial.print("Passwort: ");
    Serial.println(_apPassword);
    Serial.print("Portal URL: http://");
    Serial.println(IP);
    Serial.println();
    
    // Richte Routen ein (mit bestehendem Server)
    setupRoutes(server);
    
    Serial.println("Portal gestartet!\n");
}

// Lade gespeicherte Credentials und verbinde
bool WiFiConfigPortal::tryStoredCredentials() {
    Preferences prefs;
    prefs.begin("wifi_config", true);
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");
    prefs.end();
    
    if (ssid.length() > 0) {
        Serial.println("Versuche Verbindung mit gespeichertem Netzwerk: " + ssid);
        return connectToWiFi(ssid.c_str(), password.c_str());
    }
    
    return false;
}

// Speichere Credentials
void WiFiConfigPortal::saveCredentials(const char* ssid, const char* password) {
    Preferences prefs;
    prefs.begin("wifi_config", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
    
    Serial.println("WiFi Credentials gespeichert: " + String(ssid));
}

// Setup Web-Routen
void WiFiConfigPortal::setupRoutes(AsyncWebServer &server) {
    // WiFi Setup Seite (angepasster Pfad für Integration)
    server.on("/setup/v1/rotator/0/wifi", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "text/html", PORTAL_HTML);
    });
    
    // WiFi-Scan
    server.on("/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "application/json", scanNetworks());
    });
    
    // WiFi-Verbindung und Speichern
    server.on("/wifi/connect", HTTP_POST, [this](AsyncWebServerRequest *request){
        String ssid = "";
        String password = "";
        
        if (request->hasParam("ssid", true)) {
            ssid = request->getParam("ssid", true)->value();
        }
        if (request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        }
        
        if (ssid.length() > 0) {
            // Speichere Credentials
            saveCredentials(ssid.c_str(), password.c_str());
            
            // Versuche Verbindung
            bool success = connectToWiFi(ssid.c_str(), password.c_str());
            
            if (success) {
                String response = "{\"success\":true,\"message\":\"Verbindung hergestellt und gespeichert!\"}";
                request->send(200, "application/json", response);
                
                // Callback aufrufen
                if (_onConnectCallback) {
                    _onConnectCallback(WiFi.SSID(), WiFi.localIP().toString());
                }
                
                // Neustart nach 2 Sekunden
                delay(2000);
                ESP.restart();
            } else {
                String response = "{\"success\":false,\"message\":\"Verbindung fehlgeschlagen. Bitte überprüfen Sie das Passwort.\"}";
                request->send(200, "application/json", response);
            }
        } else {
            String response = "{\"success\":false,\"message\":\"Keine SSID angegeben\"}";
            request->send(400, "application/json", response);
        }
    });
    
    // Status
    server.on("/wifi/status", HTTP_GET, [this](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
        if (WiFi.status() == WL_CONNECTED) {
            json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
            json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
            json += ",\"rssi\":" + String(WiFi.RSSI());
        }
        json += "}";
        
        request->send(200, "application/json", json);
    });
}

// Scanne Netzwerke
String WiFiConfigPortal::scanNetworks() {
    String json = "{\"networks\":[";
    
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        
        String encryption;
        switch (WiFi.encryptionType(i)) {
            case WIFI_AUTH_OPEN: encryption = "Open"; break;
            case WIFI_AUTH_WEP: encryption = "WEP"; break;
            case WIFI_AUTH_WPA_PSK: encryption = "WPA-PSK"; break;
            case WIFI_AUTH_WPA2_PSK: encryption = "WPA2-PSK"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: encryption = "WPA/WPA2-PSK"; break;
            case WIFI_AUTH_WPA2_ENTERPRISE: encryption = "WPA2-Enterprise"; break;
            case WIFI_AUTH_WPA3_PSK: encryption = "WPA3-PSK"; break;
            case WIFI_AUTH_WPA2_WPA3_PSK: encryption = "WPA2/WPA3-PSK"; break;
            default: encryption = "Unknown";
        }
        
        json += "\"encryption\":\"" + encryption + "\"";
        json += "}";
    }
    
    json += "]}";
    WiFi.scanDelete();
    
    return json;
}

// Verbinde mit WiFi
bool WiFiConfigPortal::connectToWiFi(const char* ssid, const char* password) {
    Serial.println("Verbinde mit: " + String(ssid));
    
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✓ Verbunden!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        return true;
    } else {
        Serial.println("✗ Verbindung fehlgeschlagen");
        Serial.print("WiFi Status: ");
        Serial.println(WiFi.status());
        return false;
    }
}

// Loop-Funktion
void WiFiConfigPortal::loop() {
    // Periodischer Status-Check
    if (millis() - _lastStatusCheck > 30000) {
        _lastStatusCheck = millis();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("✓ WiFi verbunden: " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")");
        } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            Serial.println("○ Portal aktiv: http://" + WiFi.softAPIP().toString());
        }
    }
}

// Status-Abfragen
bool WiFiConfigPortal::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiConfigPortal::getConnectedSSID() {
    return WiFi.SSID();
}

String WiFiConfigPortal::getLocalIP() {
    return WiFi.localIP().toString();
}

String WiFiConfigPortal::getAPIP() {
    return WiFi.softAPIP().toString();
}

// Callbacks
void WiFiConfigPortal::onConnect(void (*callback)(String ssid, String ip)) {
    _onConnectCallback = callback;
}

void WiFiConfigPortal::onDisconnect(void (*callback)()) {
    _onDisconnectCallback = callback;
}

// Stoppe AP
void WiFiConfigPortal::stopAP() {
    WiFi.softAPdisconnect(true);
    Serial.println("Access Point gestoppt");
}
