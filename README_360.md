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

### Funktionsweise

- **0° Rotator** = 0° Motor (Step 0)
- **90° Rotator** = 180° Motor (Step 2048)
- **180° Rotator** = 360° Motor (Step 4096) = 1 volle Motorumdrehung
- **270° Rotator** = 540° Motor (Step 6144)
- **360° Rotator** = 720° Motor (Step 8192) = 2 volle Motorumdrehungen

Alle Zwischenschritte sind möglich!

### Kompatibilität

- Alle anderen Funktionen bleiben unverändert
- Web-Interface funktioniert mit vollem 0-360° Bereich
- ASCOM Alpaca API unterstützt volle 360° Rotation
- Kalibrierung und Zero-Point-Funktionen unverändert
