# WiFiConfigPortal Integration

## Übersicht

Das MoMa Rotator Projekt wurde mit dem **WiFiConfigPortal-Modul** aus dem WiFi-Scan_V1 Projekt erweitert, um eine moderne und benutzerfreundliche WiFi-Konfiguration zu ermöglichen.

## Was ist WiFiConfigPortal?

Ein wiederverwendbares C++ Modul für ESP32, das ein Web-basiertes WiFi-Konfigurationsportal mit automatischem Netzwerk-Scanning bereitstellt.

### ✨ Features

- **📡 Automatisches Netzwerk-Scanning**: Erkennt verfügbare WiFi-Netzwerke automatisch
- **📶 Signalstärke-Anzeige**: Zeigt RSSI-Werte für bessere Netzwerkauswahl  
- **🎨 Modernes Web-Interface**: Responsive Design mit Farbschema passend zu MoMaRoTa
- **🔐 Versteckte Netzwerke**: Manuelle SSID-Eingabe für Hidden SSIDs
- **💾 Persistente Speicherung**: WiFi-Credentials werden dauerhaft gespeichert
- **🔄 Auto-Reconnect**: Automatische Verbindung beim nächsten Start
- **🔌 Ein Server für alles**: Nutzt den bestehenden AsyncWebServer (kein Port-Konflikt)

## Integration in MoMa Rotator

### Architektur

Das WiFiConfigPortal wurde speziell angepasst, um **mit dem bestehenden AsyncWebServer** des Hauptprojekts zu arbeiten:

```
main.cpp
    ↓
    server (AsyncWebServer, Port 80)
    ↓
    ├─→ WiFiConfigPortal.begin(server)  // WiFi-Konfiguration
    ├─→ setupWiFiEndpoints(server)      // Control Panel
    └─→ setupAlpacaEndpoints(server)    // ALPACA API
```

**Vorteil**: Alle Funktionen laufen auf **einem einzigen Webserver** (Port 80), keine Port-Konflikte!

### Dateistruktur

```
lib/WiFiConfigPortal/
├── WiFiConfigPortal.h          # Header mit Klassen-Definition
└── WiFiConfigPortal.cpp        # Implementierung

include/
└── wifi_manager.h              # Erweitert um WiFiConfigPortal-Integration

src/
└── wifi_manager.cpp            # Nutzt WiFiConfigPortal für Setup
```

### Code-Änderungen

#### 1. WiFiConfigPortal-Klasse (angepasst)

**Original**: Eigener AsyncWebServer
```cpp
WiFiConfigPortal(const char* apSSID, const char* apPassword, uint16_t serverPort);
AsyncWebServer* _server;  // Eigener Server
```

**Angepasst**: Nutzt bestehenden Server
```cpp
WiFiConfigPortal(const char* apSSID, const char* apPassword);
void begin(AsyncWebServer &server);  // Server wird übergeben
```

#### 2. wifi_manager.cpp

**Vorher** (~340 Zeilen):
- Manuelle WiFi-Verbindung
- Einfaches HTML-Formular
- Kein Netzwerk-Scanning

**Nachher** (~250 Zeilen):
```cpp
void initWiFi(AsyncWebServer &server) {
    wifiPortal = new WiFiConfigPortal("MoMaRoTa", "12345678");
    wifiPortal->begin(server);  // Portal mit bestehendem Server starten
}
```

**Reduzierung**: ~90 Zeilen weniger Code!

#### 3. main.cpp

```cpp
void setup() {
    // ...
    initWiFi(server);  // Server wird übergeben
    setupWiFiEndpoints(server);
    setupAlpacaEndpoints(server);
    server.begin();
}

void loop() {
    updateWiFiPortal();  // Neue Funktion für Status-Updates
    // ...
}
```

## Verwendung

### Erste Inbetriebnahme

1. **ESP32 mit Strom versorgen**
2. **Mit Access Point verbinden**:
   - SSID: `MoMaRoTa`
   - Passwort: `12345678`
3. **Browser öffnet automatisch** Setup-Seite
   - Oder manuell: http://192.168.1.1
4. **"WiFi Settings" anklicken**
5. **Modernes Portal öffnet sich** mit:
   - Liste der verfügbaren Netzwerke
   - RSSI-Werte (Signalstärke)
   - Verschlüsselungstyp
6. **Netzwerk auswählen oder manuell eingeben**
7. **Passwort eingeben** und "Verbinden und Speichern"
8. **Gerät startet neu** und verbindet sich automatisch

### Spätere Verwendung

Nach der ersten Konfiguration:
- ✅ **Automatische Verbindung** beim Start
- ✅ **Fallback zum AP-Modus**, wenn Netzwerk nicht verfügbar
- ✅ **WiFi-Reset** über Setup-Menü möglich

## API-Endpunkte

### WiFi-Portal-Endpunkte (NEU)

| Endpunkt | Methode | Beschreibung |
|----------|---------|--------------|
| `/setup/v1/rotator/0/wifi` | GET | WiFi-Konfigurationsportal (HTML) |
| `/wifi/scan` | GET | Netzwerk-Scan (JSON) |
| `/wifi/connect` | POST | Verbindung herstellen und speichern |
| `/wifi/status` | GET | Verbindungsstatus abfragen (JSON) |

### Beispiel: Netzwerk-Scan Response

```json
{
  "networks": [
    {
      "ssid": "MeinWLAN",
      "rssi": -45,
      "encryption": "WPA2-PSK"
    },
    {
      "ssid": "Nachbar-WLAN",
      "rssi": -72,
      "encryption": "WPA/WPA2-PSK"
    }
  ]
}
```

### Beispiel: Verbindungsstatus

```json
{
  "connected": true,
  "ssid": "MeinWLAN",
  "ip": "192.168.1.105",
  "rssi": -45
}
```

## Technische Details

### Abhängigkeiten

Das WiFiConfigPortal benötigt:
- `WiFi.h` (ESP32 WiFi)
- `ESPAsyncWebServer.h` (Webserver)
- `Preferences.h` (ESP32 Speicher)

### Speicherung

WiFi-Credentials werden dauerhaft im **ESP32 Preferences** gespeichert:
- Namespace: `wifi_config`
- Keys: `ssid`, `password`

### Workflow beim Start

```
ESP32 Boot
    ↓
WiFiConfigPortal.begin(server)
    ↓
tryStoredCredentials() ─→ Credentials gefunden?
    │                           ├─→ JA: connectToWiFi()
    │                           │         ├─→ Erfolg: ✓ Verbunden
    │                           │         └─→ Fehler: ↓
    │                           └─→ NEIN: ↓
    ↓
Starte Access Point "MoMaRoTa"
    ↓
Registriere Portal-Routen auf bestehendem Server
    ↓
Portal bereit: http://192.168.1.1/setup/v1/rotator/0/wifi
```

### Design

Das Portal nutzt ein modernes, responsives Design:
- **Farbschema**: Angepasst an MoMaRoTa (Grüntöne statt Lila)
- **Animationen**: Smooth Slide-In beim Laden
- **Icons**: Emoji-basierte Icons für bessere UX
- **Responsive**: Funktioniert auf Desktop und Mobil

## Vorteile der Integration

### Code-Qualität
- ✅ **Weniger Code**: ~90 Zeilen entfernt aus wifi_manager.cpp
- ✅ **Wartbarer**: Klare Trennung zwischen Portal und Manager
- ✅ **Wiederverwendbar**: WiFiConfigPortal kann in anderen Projekten genutzt werden

### Funktionalität
- ✅ **Bessere UX**: Automatisches Scanning statt manuelle Eingabe
- ✅ **Mehr Information**: RSSI, Verschlüsselung sichtbar
- ✅ **Zuverlässiger**: Auto-Reconnect beim nächsten Start

### Architektur
- ✅ **Ein Server**: Kein Port-Konflikt zwischen Portal und ALPACA
- ✅ **Sauber integriert**: Nutzt bestehende Infrastruktur
- ✅ **Flexibel**: Portal kann jederzeit aktiviert/deaktiviert werden

## Migration von altem Setup

### Alte Implementierung (entfernt)
```cpp
// Alte manuelle WiFi-Setup-Seite
void handleWifiSetupPage(AsyncWebServerRequest *request) {
    // ~60 Zeilen HTML mit Textfeldern
}

void handleSaveWifi(AsyncWebServerRequest *request) {
    // Manuelle Speicherung und Neustart
}
```

### Neue Implementierung
```cpp
// WiFiConfigPortal übernimmt alles
wifiPortal->begin(server);
// Automatisch: Scan, Anzeige, Speicherung, Reconnect
```

## Branch-Information

Die WiFiConfigPortal-Integration wurde im Branch **`feature/wifi-config-portal-integration`** entwickelt.

### Änderungen im Branch
- ✅ WiFiConfigPortal-Modul hinzugefügt (`lib/WiFiConfigPortal/`)
- ✅ wifi_manager vereinfacht (~90 Zeilen weniger)
- ✅ main.cpp aktualisiert (Server wird übergeben)
- ✅ Dokumentation erweitert

### Build-Status
```
RAM:   14.2% (46.480 / 327.680 Bytes)
Flash: 71.4% (935.421 / 1.310.720 Bytes)
✅ Kompiliert erfolgreich!
```

## Nächste Schritte

1. ✅ Branch auf ESP32 flashen und testen
2. ⚡ WiFi-Konfiguration mit verschiedenen Netzwerken testen
3. 🌐 Netzwerk-Scan-Funktionalität validieren
4. 🔄 Auto-Reconnect nach Neustart prüfen
5. 📱 Mobile Ansicht testen
6. 🔀 Bei Erfolg: Merge in `main` Branch

## Autor

Integration durchgeführt von GitHub Copilot  
Datum: 18. Januar 2026  
Branch: feature/wifi-config-portal-integration
