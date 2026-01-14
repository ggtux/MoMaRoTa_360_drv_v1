# MoMa Rotator - ASCOM Alpaca Driver

Ein ASCOM Alpaca-kompatibler Treiber für einen astronomischen Bildfeldrotator.
Verwendet ESP32 mit ST3215 Servo-Motor im Mode 3 (Motor Mode).

## Projekt-Struktur

### Hauptdateien

#### `src/main.cpp` (79 Zeilen)
Das schlanke Hauptprogramm enthält nur:
- `setup()`: Initialisierung von Servo, WiFi, ALPACA Discovery und Web-Endpunkten
- `loop()`: Feedback-Updates, DNS-Verarbeitung und Discovery-Handling
- Keine Handler-Funktionen mehr - alles ist in dedizierte Module ausgelagert

#### `include/alpaca_handlers.h` & `src/alpaca_handlers.cpp`
Alle ASCOM Alpaca API-Endpunkte:
- **Management Endpoints**: `/management/v1/description`, `/management/apiversions`, `/management/v1/configureddevices`
- **Common Device Endpoints**: `/api/v1/rotator/0/connected`, `/api/v1/rotator/0/driverinfo`, etc.
- **Rotator Specific Endpoints**: `/api/v1/rotator/0/position`, `/api/v1/rotator/0/move`, `/api/v1/rotator/0/halt`, etc.
- **UDP Discovery**: Alpaca-Discovery-Protocol für automatische Geräteerkennung

#### `include/wifi_manager.h` & `src/wifi_manager.cpp`
WiFi und Setup-Verwaltung (optimiert aus parkplatz/CONNECT.h):
- WiFi-Verbindung mit gespeicherten Credentials
- Access Point Modus als Fallback
- Captive Portal für einfache Konfiguration
- Setup-Webseiten: `/setup/v1/rotator/0/setup`, `/setup/v1/rotator/0/wifi`
- Control Panel: `/setup/v1/rotator/0/configdevices`
- Kommando-Handler für Rotator-Steuerung: `/cmd`, `/position`, `/printip`

#### `include/servo_control.h` & `src/servo_control.cpp`
Servo-Motor-Steuerung (optimiert aus parkplatz/CONNECT.h):
- Initialisierung des ST3215 Servos
- Bewegungsfunktionen: `moveServoToAngle()`, `moveServoByAngle()`
- Kalibrierung: `setZeroPointExact()`, `setMiddle()`
- Feedback und Status: `getFeedback()`, `isServoMoving()`, `getServoAngle()`
- Geschwindigkeits-Management: `setActiveSpeed()`, `getActiveSpeed()`

## Wichtige Features

### Gear Ratio Berechnung
- **Übersetzung**: 1:2 (180° Getriebe = 360° Motor = 4096 Steps)
- **Formel**: `motorSteps = (gearDegrees * 2.0 / 360.0) * 4096`
- **Winkelbereich**: 0 - 359.99° (Getriebe-Position)

### ALPACA-Kompatibilität
- **Interface Version**: 3
- **API Version**: 1
- **Device Type**: Rotator
- **Discovery**: UDP Multicast auf Port 32227
- **HTTP Server**: Port 80

### WiFi Modi
1. **STA Mode**: Verbindung zu bekanntem WLAN
2. **AP Mode**: Fallback als Access Point "MoMaRoTa"
3. **Captive Portal**: Automatische Umleitung zur Konfiguration

## Verwendung

### Erste Inbetriebnahme
1. ESP32 mit Strom versorgen
2. Nach "MoMaRoTa" WLAN suchen und verbinden
3. Browser öffnet automatisch Setup-Seite (oder zu 192.168.1.1)
4. WiFi-Einstellungen konfigurieren
5. Nach Neustart verbindet sich das Gerät mit dem WLAN

### ALPACA Discovery
ASCOM-kompatible Software findet das Gerät automatisch über:
- UDP Discovery auf Port 32227
- ALPACA API auf Port 80

### Web Interface
- **Setup**: `http://<IP>/setup/v1/rotator/0/setup`
- **WiFi Config**: `http://<IP>/setup/v1/rotator/0/wifi`
- **Rotator Control**: `http://<IP>/setup/v1/rotator/0/configdevices`

### Rotator-Steuerung
- **Position anfahren**: 0-359.99° via Web-Interface oder ALPACA API
- **Geschwindigkeit**: +/- Buttons (100 Steps pro Klick)
- **Kalibrierung**: "Set Middle" oder "Set Zero"
- **Stop**: Sofortiger Halt jederzeit möglich

## Entwickler-Informationen

### Modulare Struktur
Die neue Struktur trennt klar zwischen:
- **Hauptlogik** (main.cpp): Minimales Setup und Loop
- **ALPACA Protocol** (alpaca_handlers): Alle API-Endpunkte
- **WiFi Management** (wifi_manager): Verbindung und Setup-Seiten
- **Servo Control** (servo_control): Hardware-nahe Steuerung

### Vorteile
- ✅ Übersichtlicher Code (79 Zeilen main.cpp statt 630)
- ✅ Klare Verantwortlichkeiten
- ✅ Einfache Wartung und Erweiterung
- ✅ Wiederverwendbare Module
- ✅ Optimierte Funktionen aus parkplatz-Prototyp übernommen

## Hardware

- **Mikrocontroller**: ESP32
- **Servo**: Feetech ST3215 (Mode 3 - Motor Mode)
- **Communication**: UART (Serial1, RX=18, TX=19, 1MBaud)
- **Gear Ratio**: 1:2

## Abhängigkeiten

- ESPAsyncWebServer
- WiFi (ESP32)
- DNSServer
- Preferences (ESP32)
- ArduinoJson
- SMS_STS (Servo-Bibliothek)

## Version

- **Driver Version**: 1.0
- **Interface Version**: 3 (ALPACA)
- **Manufacturer**: MoMa

## Autor

geo - 2026
