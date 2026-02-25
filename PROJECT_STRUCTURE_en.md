# MoMa Rotator - Project Structure (English)

## File Overview

```
20260112_MoMaRota_drv_v1_en/
│
├── platformio.ini                    # PlatformIO build configuration
├── README.md                         # Project documentation
├── REORGANIZATION_SUMMARY.md         # Details on code reorganization
│
├── include/                          # Header files
│   ├── alpaca_handlers.h             # ✨ NEW: ALPACA API endpoints
│   ├── wifi_manager.h                # ✨ NEW: WiFi & setup management
│   ├── display_control.h             # ✨ NEW: OLED display control
│   ├── servo_control.h               # Servo control
│   ├── SMS_STS.h                     # Servo library
│   ├── SCS.h                         # Servo communication
│   ├── SCSCL.h                       # Servo control layer
│   ├── SCSerial.h                    # Servo serial
│   ├── SCServo.h                     # Servo interface
│   ├── STSCTRL.h                     # STS control
│   ├── INST.h                        # Instructions
│   └── PreferencesConfig.h           # Preferences config
│
├── src/                              # Source files
│   ├── main.cpp                      # 🔥 MAIN PROGRAM (95 lines!)
│   ├── main.cpp.backup               # 💾 Backup (630 lines old)
│   ├── alpaca_handlers.cpp           # ✨ NEW: ALPACA implementation
│   ├── wifi_manager.cpp              # ✨ NEW: WiFi implementation
│   ├── display_control.cpp           # ✨ NEW: OLED display implementation
│   ├── servo_control.cpp             # Servo implementation
│   ├── SMS_STS.cpp                   # Servo library
│   ├── SCS.cpp                       # Servo communication
│   ├── SCSCL.cpp                     # Servo control layer
│   └── SCSerial.cpp                  # Servo serial
│
├── parkplatz/                        # 🗄️ Reference code (not compiled)
│   ├── CONNECT.h                     # Basis for wifi_manager
│   ├── WEBPAGE.h                     # Basis for display_controlPanel
│   ├── BOARD_DEV.h                   # Display functions
│   ├── RGB_CTRL.h                    # RGB LED control
│   └── INST.h                        # Instructions
│
└── lib/                              # External libraries
    └── README
```

## Module Dependencies

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                          main.cpp (79 lines)                                │
│  ┌───────────────┐    ┌───────────────┐    ┌─────────────────────────────┐   │
│  │ setup()      │    │    loop()     │    │  server object              │   │
│  └───────┬──────┘    └───────┬──────┘    └───────────────┬─────────────┘   │
└──────────┴───────────────────┴───────────────────────────┴─────────────────┘
        │               │
        │               │
```
