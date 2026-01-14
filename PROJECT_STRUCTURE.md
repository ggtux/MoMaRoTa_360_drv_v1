# MoMa Rotator - Projekt-Struktur

## Datei-Übersicht

```
20260112_MoMaRota_drv_v1/
│
├── platformio.ini                    # PlatformIO Build-Konfiguration
├── README.md                         # Projekt-Dokumentation
├── REORGANIZATION_SUMMARY.md         # Details zur Code-Reorganisation
│
├── include/                          # Header-Dateien
│   ├── alpaca_handlers.h            # ✨ NEU: ALPACA API Endpunkte
│   ├── wifi_manager.h               # ✨ NEU: WiFi & Setup Verwaltung
│   ├── display_control.h            # ✨ NEU: OLED Display Steuerung
│   ├── servo_control.h              # Servo-Steuerung
│   ├── SMS_STS.h                    # Servo-Bibliothek
│   ├── SCS.h                        # Servo Communication
│   ├── SCSCL.h                      # Servo Control Layer
│   ├── SCSerial.h                   # Servo Serial
│   ├── SCServo.h                    # Servo Interface
│   ├── STSCTRL.h                    # STS Control
│   ├── INST.h                       # Instructions
│   └── PreferencesConfig.h          # Preferences Config
│
├── src/                             # Source-Dateien
│   ├── main.cpp                     # 🔥 HAUPTPROGRAMM (95 Zeilen!)
│   ├── main.cpp.backup              # 💾 Backup (630 Zeilen alt)
│   ├── alpaca_handlers.cpp          # ✨ NEU: ALPACA Implementierung
│   ├── wifi_manager.cpp             # ✨ NEU: WiFi Implementierung
│   ├── display_control.cpp          # ✨ NEU: OLED Display Implementierung
│   ├── servo_control.cpp            # Servo Implementierung
│   ├── SMS_STS.cpp                  # Servo-Bibliothek
│   ├── SCS.cpp                      # Servo Communication
│   ├── SCSCL.cpp                    # Servo Control Layer
│   └── SCSerial.cpp                 # Servo Serial
│
├── parkplatz/                       # 🗄️ Referenz-Code (nicht kompiliert)
│   ├── CONNECT.h                    # Basis für wifi_manager
│   ├── WEBPAGE.h                    # Basis für display_controlPanel
│   ├── BOARD_DEV.h                  # Display-Funktionen
│   ├── RGB_CTRL.h                   # RGB LED Steuerung
│   └── INST.h                       # Instruktionen
│
└── lib/                             # Externe Bibliotheken
    └── README

```

## Modul-Abhängigkeiten

```
┌─────────────────────────────────────────────────────────────────┐
│                          main.cpp (79 Zeilen)                    │
│  ┌──────────┐    ┌────────────────┐    ┌──────────────────┐   │
│  │ setup()  │    │    loop()      │    │  server object   │   │
│  └────┬─────┘    └────┬───────────┘    └──────────────────┘   │
└───────┼───────────────┼──────────────────────────────────────────┘
        │               │
        │               │
        ▼               ▼
┌───────────────┐ ┌───────────────┐ ┌──────────────────┐
│ servo_control │ │ wifi_manager  │ │ alpaca_handlers  │
├───────────────┤ ├───────────────┤ ├──────────────────┤
│ • initServo() │ │ • initWiFi()  │ │ • initDiscovery()│
│ • getFeedback │ │ • processDNS()│ │ • handleDiscovery│
│ • moveServox()│ │ • setupWiFix()│ │ • setupAlpacax() │
│ • getAngle()  │ │ • handleCmd() │ │ • handleMove()   │
│ • setSpeed()  │ │ • handleSave()│ │ • handlePosition│
│ • setZero()   │ │ • getIP()     │ │ • handle...()    │
└───────────────┘ └───────────────┘ └──────────────────┘
        │               │                     │
        │               │                     │
        ▼               ▼                     ▼
┌───────────────┐ ┌───────────────┐ ┌──────────────────┐
│  SMS_STS      │ │  WiFi         │ │  WiFiUDP         │
│  Servo Lib    │ │  Preferences  │ │  AsyncWebServer  │
└───────────────┘ └───────────────┘ └──────────────────┘
```

## Endpunkt-Verteilung

### WiFi Manager (`/setup/*`, `/cmd`, `/position`)
```
wifi_manager.cpp
├── GET  /                                    → Redirect to setup
├── GET  /setup/v1/rotator/0/setup           → Setup-Menü
├── GET  /setup/v1/rotator/0/wifi            → WiFi-Konfiguration
├── POST /setup/v1/rotator/0/save            → WiFi speichern & restart
├── GET  /setup/v1/rotator/0/configdevices   → Rotator Control Panel
├── GET  /reset                               → WiFi-Reset
├── GET  /cmd                                 → Rotator-Kommandos (Case 1-21)
├── GET  /position                            → Aktuelle Position
└── GET  /printip                             → IP-Adresse
    └── Captive Portal: /hotspot-detect.html, /generate_204, /connecttest.txt
```

### ALPACA Handlers (`/api/*`, `/management/*`)
```
alpaca_handlers.cpp
├── Management API (/management/v1/*)
│   ├── GET /management/v1/description
│   ├── GET /management/apiversions
│   └── GET /management/v1/configureddevices
│
└── Device API (/api/v1/rotator/0/*)
    ├── Common Device Methods (13 Endpoints)
    │   ├── GET/PUT /connected
    │   ├── GET    /connecting
    │   ├── PUT    /connect, /disconnect
    │   ├── GET    /description, /driverinfo, /driverversion
    │   ├── GET    /interfaceversion, /name, /devicestate
    │   └── GET    /supportedactions
    │
    └── Rotator Specific Methods (10 Endpoints)
        ├── GET    /canreverse
        ├── GET    /ismoving
        ├── GET    /position, /mechanicalposition, /targetposition
        ├── GET/PUT /reverse
        ├── GET    /stepsize
        ├── PUT    /halt, /move, /moveabsolute, /movemechanical
        └── PUT    /sync
        
└── UDP Discovery (Port 32227)
    └── Multicast 233.255.255.255
```

## Code-Metriken

| Modul              | Dateien | Zeilen  | Verantwortung                    |
|--------------------|---------|---------|----------------------------------|
| main.cpp           | 1       | 95      | Setup & Loop                     |
| alpaca_handlers    | 2       | ~380    | ALPACA Protocol                  |
| wifi_manager       | 2       | ~340    | WiFi & Web Interface             |
| servo_control      | 2       | ~340    | Servo Hardware Control           |
| display_control    | 2       | ~120    | OLED Display                     |
| SMS_STS (Lib)      | 8       | ~1500   | Servo Communication Library      |
| **GESAMT**         | **17**  | **2775**| **Vollständiger ALPACA Driver**  |

## Workflow

### 1️⃣ Startup
```
ESP32 Boot
    Display()        → OLED Display initialisieren
    ↓
initServo()          → Servo im Mode 3 konfigurieren
    ↓
initWiFi()           → Verbindung oder AP-Modus
    ↓
initDiscovery()      → UDP Multicast starten
    ↓
setupEndpoints()     → Alle Web-Routes registrieren
    ↓
server.begin()       → Webserver starten
    ↓
READY ✓              → Display zeigt "Ready!" + IP
READY ✓
```

### 2️⃣ Runtime Loop
```updateDisplay()    → OLED Display aktualisieren
    
loop() {
    getFeedback()      → Servo Position/Status lesen
    processDNS()       → Captive Portal DNS
    handleDiscovery()  → ALPACA Discovery beantworten
    delay(1)           → Watchdog-Reset
}
```

### 3️⃣ ALPACA Client Connection
```
ASCOM Client
    ↓
UDP Discovery → ESP32 antwortet mit Port 80
    ↓
GET /api/v1/rotator/0/position → alpaca_handlers.cpp
    ↓
handlePosition() → getServoAngle() → servo_control.cpp
    ↓
JSON Response {"Value": 90.5, "ErrorNumber": 0, ...}
```

### 4️⃣ Web Control Panel
```
Browser → http://192.168.x.x/setup/v1/rotator/0/configdevices
    ↓
wifi_manager.cpp → handleConfigDevices()
    ↓
HTML/JS Control Panel
    ↓
User: "Goto 180°" → /cmd?inputI=5
    ↓
wifi_manager.cpp → moveServoToAngle(180.0)
    ↓
servo_control.cpp → st.WritePosEx(...)
```

## Optimierungen aus parkplatz/

### 🔧 CONNECT.h → wifi_manager.cpp
- ✅ WiFi-Verbindung mit Preferences
- ✅ Access Point Fallback
- ✅ Captive Portal Detection
- ✅ Command Handler (Cases 1-21)
- ✅ Position-Berechnung mit Gear Ratio
- ✅ Speed Management

### 🎨 WEBPAGE.h → wifi_manager.cpp
- ✅ Optimiertes HTML/CSS Design
- ✅ JavaScript für Goto-Steuerung
- ✅ 🖥️ BOARD_DEV.h → display_control.cpp
- ✅ OLED SSD1306 Display Support
- ✅ Auto-Update alle 300ms
- ✅ Status-Anzeige (Titel, Mode, Position, IP)
- ✅ Display On/Off Steuerung (Case 20/21)
- ✅ Startup-Nachrichten

### Auto-Refresh Position Display
- ✅ Responsive Layout

### ⚙️ Servo-Funktionen → servo_control.cpp
- ✅ Gear Ratio Berechnung (1:2)
- ✅ Angle Wrapping (0-359.99°)
- ✅ Relative & Absolute Bewegung
- ✅ Feedback-Monitoring
- ✅ Block-Detection

## Nächste Schritte

1. ✅ Code kompilieren und flashen
2. ⚡ Testen der ALPACA-Discovery
3. 🌐 Web-Interface testen
4. 🎯 Positions-Genauigkeit validieren
5. 📊 Performance-Monitoring
6. 🐛 Bug-Fixes bei Bedarf
7. 📝 User-Dokumentation erweitern
