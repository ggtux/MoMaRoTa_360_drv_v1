# Motor-Mode (Mode 3) - Implementierung

## Status: ✅ Vollständig implementiert

Der MoMa Rotator Driver verwendet **ausschließlich Motor-Mode (Mode 3)** des ST3215 Servos.

## Wichtige Änderungen

### 1. `setMode()` - Nur Mode 3 erlaubt
```cpp
void setMode(int mode) {
    // This driver ONLY supports Motor-Mode (Mode 3)
    if(mode != 3) {
        Serial.println("ERROR: Only Motor-Mode (3) is supported!");
        Serial.println("Forcing Mode 3...");
        mode = 3;
    }
    
    st.unLockEprom(MOTOR_ID);
    st.writeByte(MOTOR_ID, SMS_STS_MODE, 3);
    st.writeWord(MOTOR_ID, 11, 0);  // Max angle = 0 for motor mode
    st.LockEprom(MOTOR_ID);
}
```

### 2. `initServo()` - Mode-Verifizierung
- Setzt Mode 3 beim Start
- Verifiziert den Mode nach dem Setzen
- Wiederholt Setzen bei Fehler
- Gibt Warnung aus, falls Mode nicht 3 ist

### 3. `setZeroPointMode3()` - Kein Mode-Wechsel mehr
**Vorher (FALSCH):**
```cpp
setMode(0);           // ❌ Wechsel zu Servo-Mode
st.CalibrationOfs();  // Hardware-Kalibrierung
setMode(3);           // Zurück zu Motor-Mode
```

**Nachher (KORREKT):**
```cpp
void setZeroPointMode3() {
    // In Motor-Mode (3): Do NOT switch modes!
    // Simply set virtual position to 0
    currentTargetPosition = 0;
}
```

### 4. `setMiddle()` - Virtuelle Position
**Vorher (FALSCH):**
```cpp
st.CalibrationOfs(MOTOR_ID);  // ❌ Hardware-Kalibrierung (nur für Servo-Mode)
```

**Nachher (KORREKT):**
```cpp
void setMiddle() {
    // In Motor-Mode (3): Set current position as middle (90°)
    // 90° * 2 (gear ratio) / 360° * 4096 steps = 2048
    currentTargetPosition = 2048; // 90° on gear
}
```

## Motor-Mode Prinzip

### Virtuelle Positions-Verwaltung
Im Motor-Mode gibt es **keine absoluten Positionen**. Alle Bewegungen sind relativ:

```cpp
// Aktuelle virtuelle Position
s16 currentTargetPosition = 0;

// Bewegung relativ zur aktuellen Position
void moveServoToAngle(double angleDeg) {
    int targetSteps = calculateSteps(angleDeg);
    s16 relativeDelta = targetSteps - currentTargetPosition;
    
    // Bewege RELATIV vom aktuellen Punkt
    st.WritePosEx(MOTOR_ID, relativeDelta, speed, acc);
    
    // Update virtuelle Position
    currentTargetPosition = targetSteps;
}
```

### Nullpunkt-Setzung
Im Motor-Mode bedeutet "Nullpunkt setzen" einfach:
```cpp
currentTargetPosition = 0;  // Virtuelle Position auf 0 setzen
```

Der Servo bewegt sich **nicht**, nur die interne Referenz wird verschoben.

## Unterschied Mode 0 vs Mode 3

| Feature | Mode 0 (Servo) | Mode 3 (Motor) |
|---------|----------------|----------------|
| Position | Absolut (0-4095) | Relativ (unbegrenzt) |
| Kalibrierung | Hardware (CalibrationOfs) | Virtuell (Variable) |
| Winkel-Limits | Ja (Min/Max Register) | Nein (kontinuierlich) |
| Mehrfach-Umdrehungen | Nein | Ja (unbegrenzt) |
| Power-Loss Memory | Ja | Nein (virtuell) |

## Warum nur Motor-Mode?

1. **Rotator-Anwendung**: Kontinuierliche Drehung ohne Limits
2. **Gear Ratio**: 1:2 Übersetzung - Motor kann mehrere Umdrehungen machen
3. **Flexibilität**: Keine Hardware-Limits
4. **Präzision**: Relative Bewegungen akkumulieren nicht Fehler

## Verifikation

### Beim Start
```
Setting Motor-Mode (3) - locked permanently...
Motor-Mode (3) set and locked
Current Mode: 3
Motor-Mode (3) initialized - virtual position set to 0°
```

### Mode-Check im Feedback
```cpp
void getFeedback() {
    // ...
    modeRead = st.ReadMode(MOTOR_ID);
    // Sollte immer 3 sein
}
```

## Gefahr: Versehentlicher Mode-Wechsel

### ❌ Was vermieden wird:
- `st.CalibrationOfs()` - Nur für Servo-Mode
- `setMode(0)` - Wechsel zu Servo-Mode
- Hardware-Kalibrierungen - Ändern EEPROM

### ✅ Was verwendet wird:
- `currentTargetPosition = 0` - Virtuelle Nullpunkt-Setzung
- `st.WritePosEx(relativeDelta, ...)` - Relative Bewegung
- Virtuelle Positions-Verwaltung in Software

## Command-Handler (alle Mode 3 kompatibel)

| Command | Case | Funktion | Mode 3? |
|---------|------|----------|---------|
| 0° | 6 | moveServoToAngle(0.0) | ✅ |
| 90° | 1 | moveServoToAngle(90.0) | ✅ |
| 180° | 5 | moveServoToAngle(180.0) | ✅ |
| Stop | 2 | stopServo() | ✅ |
| Speed + | 7 | setActiveSpeed(+100) | ✅ |
| Speed - | 8 | setActiveSpeed(-100) | ✅ |
| Set Middle | 11 | setMiddle() | ✅ (virtuell) |
| Goto | 17 | moveServoToAngle(P) | ✅ |
| Set Zero | 18 | setZeroPointExact() | ✅ (virtuell) |

## Zusammenfassung

✅ **Motor-Mode (3) ist korrekt implementiert**
- Keine Mode-Wechsel mehr möglich
- Alle Kalibrierungen sind virtuell
- Hardware bleibt permanent in Mode 3
- Relative Bewegungen funktionieren perfekt
- Gear Ratio wird korrekt berücksichtigt

🔒 **Sicherheits-Features**
- `setMode()` erzwingt Mode 3
- Initialisierung verifiziert Mode
- Warnung bei falschem Mode
- Keine Hardware-Kalibrierung möglich

Der Driver ist vollständig auf Motor-Mode (3) ausgerichtet und kann nicht versehentlich in einen anderen Mode wechseln.
