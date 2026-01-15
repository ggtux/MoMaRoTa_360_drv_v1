# MoMaRoTo_360_drv_v1

## Änderungen gegenüber MoMaRota_drv_v1

Diese Version ermöglicht eine **volle 360° Rotation** des Rotators.

### Technische Details

- **Gear Ratio**: 1:2 (unverändert)
- **Rotator-Bereich**: 0-360° (vorher: 0-180°)
- **Motor-Drehung**: 720° (2 volle Umdrehungen)
- **Formel**: `motorSteps = (gearDegrees * 2.0 / 360.0) * 4096`

### Geänderte Dateien

#### src/servo_control.cpp
- **Entfernt**: Die Begrenzung auf 180° Rotator-Drehung
- **Effekt**: Alle Werte von 0-360° werden direkt auf Motor-Steps gemappt
- Vorher: Winkel > 180° wurden auf 0-180° zurückgemappt
- Jetzt: Voller 0-360° Bereich verfügbar
- **Position-Tracking**: Absolute Position wird durch Bewegungsbefehle getrackt
- **Live-Anzeige**: Während der Bewegung wird die aktuelle Position berechnet aus `absolutePosition - posRead`
- **Motor-Mode 3**: Verwendet relative Bewegungen mit akkumulierter absoluter Position

### Funktionsweise

#### Position-Tracking System

In Motor-Mode 3 arbeitet der Motor mit relativen Bewegungsbefehlen:
- `absolutePosition`: Akkumulierte absolute Zielposition in Steps
- `posRead`: Verbleibende Distanz zum Ziel (wird während Bewegung auf 0 reduziert)
- **Aktuelle Position**: `currentPos = absolutePosition - posRead`

Dies ermöglicht:
- Live-Anzeige der Position während der Bewegung
- Korrekte Positionsanzeige ohne Zurückzählen auf Null
- Funktionierenden Reverse-Modus in beide Richtungen
- Shortest-Path Berechnung für optimale Bewegungen

- **0° Rotator** = 0° Motor (Step 0)
- **90° Rotator** = 180° Motor (Step 2048)
- **180° Rotator** = 360° Motor (Step 4096) = 1 volle Motorumdrehung
- **270° Rotator** = 540° Motor (Step 6144)
- **360° Rotator** = 720° Motor (Step 8192) = 2 volle Motorumdrehungen

Alle Zwischenschritte sind möglich!

#### Reverse-Modus

Der Reverse-Modus invertiert nur die physische Bewegungsrichtung des Motors:
- `logicalDelta`: Logische Bewegung für Position-Tracking (bleibt gleich)
- `motorDelta`: Physische Motor-Bewegung (wird bei Reverse invertiert)
- Die Anzeige bleibt konsistent unabhängig vom Reverse-Status

### Kompatibilität

- Alle anderen Funktionen bleiben unverändert
- Web-Interface funktioniert mit vollem 0-360° Bereich
- ASCOM Alpaca API unterstützt volle 360° Rotation
- Kalibrierung und Zero-Point-Funktionen unverändert

### Plattformspezifische Anpassungen

Der Code ist plattformunabhängig und funktioniert auf **Linux**, **Windows** und **macOS**.

#### Upload Port Konfiguration (platformio.ini)

Die einzige plattformspezifische Einstellung ist der Upload-Port:

**Linux:**
```ini
upload_port = /dev/ttyUSB0
```
Oder `/dev/ttyACM0` für manche USB-Adapter

**Windows:**
```ini
upload_port = COM3
```
Port-Nummer variiert je nach System (COM1, COM3, COM4, etc.)

**macOS:**
```ini
upload_port = /dev/cu.usbserial-*
```
Oder `/dev/cu.SLAB_USBtoUART` für CP2102/CP2104

**Auto-Detection (alle Plattformen):**
Die Zeile kann auch komplett entfernt oder auskommentiert werden:
```ini
; upload_port = /dev/ttyUSB0
```
PlatformIO erkennt dann den Port automatisch.

Der restliche Code enthält **keine** plattformspezifischen Abhängigkeiten.
