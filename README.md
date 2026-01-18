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
WiFi und Setup-Verwaltung mit integriertem WiFiConfigPortal:
- **WiFiConfigPortal-Integration**: Modernes Web-Interface für WiFi-Konfiguration
- **Automatisches Netzwerk-Scanning**: Zeigt verfügbare WiFi-Netzwerke mit RSSI-Werten
- **Credentials-Speicherung**: Automatische Verbindung beim nächsten Start
- **Versteckte Netzwerke**: Manuelle SSID-Eingabe möglich
- **Access Point Fallback**: "MoMaRoTa" AP bei fehlender Verbindung
- **Captive Portal**: Automatische Umleitung zur Konfiguration
- Setup-Webseiten: `/setup/v1/rotator/0/setup`, `/setup/v1/rotator/0/wifi`
- Control Panel: `/setup/v1/rotator/0/configdevices`
- Kommando-Handler für Rotator-Steuerung: `/cmd`, `/position`, `/printip`

#### `include/servo_control.h` & `src/servo_control.cpp`
Servo-Motor-Steuerung (optimiert aus parkplatz/CONNECT.h):
- Initialisierung des ST3215 Servos
- Automatische Motor-ID Erkennung (scannt ID 0-10)
- Bewegungsfunktionen: `moveServoToAngle()`, `moveServoByAngle()`
- Reverse-Funktion: Kehrt Bewegungsrichtung um (negiert Delta)
- Kalibrierung: `setZeroPointExact()`, `setMiddle()`
- Feedback und Status: `getFeedback()`, `isServoMoving()`, `getServoAngle()`
- Geschwindigkeits-Management: `setActiveSpeed()`, `getActiveSpeed()`

#### `include/display_control.h` & `src/display_control.cpp`
OLED Display-Steuerung (optimiert aus parkplatz/BOARD_DEV.h):
- SSD1306 128x32 OLED Display (I2C 0x3C)
- Auto-Update alle 300ms mit Motor-Informationen
- Anzeige: Titel, Motor-ID, Mode, Position, IP-Adresse
- Display On/Off Steuerung via Web-Interface
- Status-Nachrichten während Initialisierung

## Wichtige Features

### Motor-Mode (3) Exclusive
- **Mode**: Ausschließlich Motor-Mode (3) - keine Positions-Servofunktion
- **Auto-ID Detection**: Automatisches Scannen und Erkennen der Motor-ID (0-10)
- **Virtuelle Positionierung**: Position wird in Software verwaltet (keine Hardware-Kalibrierung)

### Reverse-Funktion
- **Richtungsumkehr**: Kehrt die Bewegungsrichtung um
- **Beispiel**: Position 0°, Ziel 45° → Normal: +1024 steps, Reverse: -1024 steps
- **Verwendung**: Umgehen mechanischer Hindernisse im astronomischen Setup

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

### WiFi-Konfiguration

#### WiFiConfigPortal Features
- 🔍 **Automatisches Netzwerk-Scanning**: Erkennt verfügbare WiFi-Netzwerke automatisch
- 📶 **Signalstärke-Anzeige**: Zeigt RSSI-Werte für bessere Netzwerkauswahl
- 🎨 **Modernes Web-Interface**: Responsive Design mit Gradient-Hintergrund
- 🔐 **Versteckte Netzwerke**: Manuelle SSID-Eingabe für Hidden SSIDs
- 💾 **Persistente Speicherung**: WiFi-Credentials werden gespeichert
- 🔄 **Auto-Reconnect**: Automatische Verbindung beim nächsten Start

#### WiFi Modi
1. **STA Mode**: Verbindung zu bekanntem WLAN (aus gespeicherten Credentials)
2. **AP Mode**: Fallback als Access Point "MoMaRoTa" (Passwort: 12345678)
3. **Captive Portal**: Automatische Umleitung zur Konfiguration

## Verwendung

### Erste Inbetriebnahme
1. ESP32 mit Strom versorgen
2. Nach "MoMaRoTa" WLAN suchen und verbinden (Passwort: 12345678)
3. Browser öffnet automatisch Setup-Seite (oder zu http://192.168.1.1)
4. "WiFi Settings" anklicken
5. **Neues WiFi-Portal öffnet sich** mit folgenden Optionen:
   - Netzwerk aus der automatisch gescannten Liste auswählen
   - Oder: Versteckte Netzwerke manuell eingeben
   - RSSI-Werte und Verschlüsselungstyp werden angezeigt
6. Passwort eingeben und "Verbinden und Speichern" klicken
7. Nach Neustart verbindet sich das Gerät automatisch mit dem WLAN

**Tipp**: Bei zukünftigen Starts verbindet sich der Rotator automatisch mit dem gespeicherten Netzwerk!

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
- **Geschwindigkeit**: +/- Buttons (100 Steps pro Klick, Min: 100, Max: 4000)
- **Reverse**: Kehrt Bewegungsrichtung um (z.B. +45° wird zu -45°)
- **Kalibrierung**: "Set Middle" oder "Set Zero"
- **Stop**: Sofortiger Halt jederzeit möglich
- **Display**: On/Off Steuerung des OLED-Displays

## Entwickler-Informationen

### Modulare Struktur
Die neue Struktur trennt klar zwischen:
- **Hauptlogik** (main.cpp): Minimales Setup und Loop
- **ALPACA Protocol** (alpaca_handlers): Alle API-Endpunkte
- **WiFi Management** (wifi_manager + WiFiConfigPortal): Verbindung und Setup-Seiten
- **Servo Control** (servo_control): Hardware-nahe Steuerung
- **Display Control** (display_control): OLED Display-Verwaltung

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
- **Display**: SSD1306 OLED 128x32 (I2C 0x3C, SDA=21, SCL=22)
- **Gear Ratio**: 1:2

## Abhängigkeiten

- ESPAsyncWebServer
- WiFi (ESP32)
- DNSServer
- Preferences (ESP32)
- ArduinoJson
- SMS_STS (Servo-Bibliothek)
- Adafruit_SSD1306 (OLED Display)
- Adafruit_GFX (Graphics Library)

## Version

- **Driver Version**: 1.0
- **Interface Version**: 3 (ALPACA)
- **Manufacturer**: MoMa

## Autor

geo - 2026
# MoMoRoTa_360_drv_v1
