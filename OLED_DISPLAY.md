# OLED Display - Funktionstest

## Hardware

**Display**: SSD1306 OLED 128x32 Pixel
- **Adresse**: 0x3C (I2C)
- **Pins**: SDA=21, SCL=22 (ESP32 Standard)
- **Spannung**: 3.3V

## Anzeige-Layout

```
┌────────────────────────────┐
│ MoMa Rotator              │  ← Zeile 1: Titel
│ ID:1 Mode:3               │  ← Zeile 2: Motor-ID & Mode
│ Pos: 123.4 deg           │  ← Zeile 3: Aktuelle Position
│ 192.168.1.100            │  ← Zeile 4: IP-Adresse
└────────────────────────────┘
```

## Funktionen

### Auto-Update (alle 300ms)
- Position wird kontinuierlich aktualisiert
- IP-Adresse wird angezeigt
- Mode sollte immer "3" zeigen (Motor-Mode)

### Startup-Nachrichten
Beim Booten werden verschiedene Status-Nachrichten angezeigt:
1. "MoMa Rotator" + "Initializing..."
2. "Initializing" + "Servo..."
3. "Connecting" + "WiFi..."
4. "MoMa Rotator" + "Ready!" + IP-Adresse

### Display On/Off
- **Case 20**: Display ausschalten (spart Strom)
- **Case 21**: Display einschalten
- Steuerbar via Web-Interface Toggle

## Tests

### ✅ Test 1: Display Initialisierung
```
Erwarte: "SSD1306 Display initialized" in Serial
```

### ✅ Test 2: Startup-Sequenz
```
1. Display zeigt "Initializing Servo..."
2. Display zeigt "Connecting WiFi..."
3. Display zeigt "Ready!" + IP
```

### ✅ Test 3: Live-Updates
```
Bewege Servo → Position auf Display sollte sich ändern (alle 300ms)
```

### ✅ Test 4: Display Toggle (Web-Interface)
```
1. Öffne Control Panel
2. Toggle "OLED Display" Switch
3. Display sollte ein/ausschalten
```

### ✅ Test 5: Fehlerfall - Kein Display
```
Wenn kein Display angeschlossen:
- Serial: "SSD1306 allocation failed"
- System läuft weiter ohne Display
- displayEnabled = false
```

## Troubleshooting

### Display bleibt schwarz
1. **I2C-Adresse prüfen**: 
   ```cpp
   Wire.begin(21, 22);
   Wire.beginTransmission(0x3C);
   byte error = Wire.endTransmission();
   // error == 0 → Display gefunden
   ```

2. **Verkabelung prüfen**:
   - VCC → 3.3V (NICHT 5V!)
   - GND → GND
   - SDA → GPIO 21
   - SCL → GPIO 22

3. **Pull-Up Widerstände**: 
   - Meist auf Display-Board vorhanden
   - Falls nicht: 4.7kΩ zwischen SDA/SCL und 3.3V

### Display zeigt falsche Daten
- Update-Rate zu hoch? (Standard: 300ms ist OK)
- getFeedback() wird nicht aufgerufen?
- Servo-Kommunikation funktioniert?

### Display flackert
- Update-Interval erhöhen (z.B. auf 500ms)
- In `display_control.cpp` ändern:
  ```cpp
  static const unsigned long UPDATE_INTERVAL = 500; // statt 300
  ```

## Code-Referenz

### Display initialisieren
```cpp
#include "display_control.h"

void setup() {
    initDisplay();  // Automatisch beim Start
}
```

### Display manuell aktualisieren
```cpp
updateDisplay();  // Wird in loop() automatisch aufgerufen
```

### Custom Nachricht anzeigen
```cpp
displayMessage("Line 1", "Line 2", "Line 3", "Line 4");
```

### Display ein/ausschalten
```cpp
displayOff();  // Display ausschalten
displayOn();   // Display einschalten
```

## Integration mit parkplatz/BOARD_DEV.h

### Übernommene Funktionen
✅ `InitScreen()` → `initDisplay()`
✅ `screenUpdate()` → `updateDisplay()` + `displayMotorInfo()`
✅ Display-Layout mit MoMaRota Titel
✅ Anzeige von ID, Mode, Position, IP

### Angepasst für AsyncWebServer
- Threading wurde entfernt (nicht nötig mit AsyncWebServer)
- Update erfolgt in main loop()
- Einfachere Struktur, gleiche Funktionalität

### Neue Features
✅ Display On/Off via Web-Interface
✅ Startup-Nachrichten
✅ Bessere Fehlerbehandlung (kein Display = kein Crash)
✅ Update-Throttling (verhindert zu häufige Updates)

## Erwartete Serial-Ausgabe beim Start

```
=== MoMa Rotator - ALPACA Driver ===
Initializing OLED display...
SSD1306 Display initialized
Initializing servo...
Setting angle limits...
Min Angle: 0, Max Angle: 4095
Setting Motor-Mode (3) - locked permanently...
Motor-Mode (3) set and locked
Current Mode: 3
Motor-Mode (3) initialized - virtual position set to 0°
Initializing WiFi...
Connecting to saved WiFi: MyWiFi
.....
WiFi connected!
IP address: 192.168.1.100
Initializing ALPACA discovery...
Setting up web server endpoints...
=== Server started ===
IP: 192.168.1.100
Ready for ALPACA connections
```

## Status: ✅ Vollständig integriert

Das OLED Display ist vollständig integriert und einsatzbereit!
