# MoMa Rotator - ASCOM Alpaca Driver (English)

An ASCOM Alpaca-compatible driver for an astronomical field rotator.
Uses ESP32 with ST3215 servo motor in Mode 3 (Motor Mode).

## Project Structure

### Main Files

#### `src/main.cpp` (79 lines)
The slim main program contains only:
- `setup()`: Initialization of servo, WiFi, ALPACA Discovery, and web endpoints
- `loop()`: Feedback updates, DNS processing, and discovery handling
- No more handler functions - everything is outsourced to dedicated modules

#### `include/alpaca_handlers.h` & `src/alpaca_handlers.cpp`
All ASCOM Alpaca API endpoints:
- **Management Endpoints**: `/management/v1/description`, `/management/apiversions`, `/management/v1/configureddevices`
- **Common Device Endpoints**: `/api/v1/rotator/0/connected`, `/api/v1/rotator/0/driverinfo`, etc.
- **Rotator Specific Endpoints**: `/api/v1/rotator/0/position`, `/api/v1/rotator/0/move`, `/api/v1/rotator/0/halt`, etc.
- **UDP Discovery**: Alpaca Discovery Protocol for automatic device detection

#### `include/wifi_manager.h` & `src/wifi_manager.cpp`
WiFi and setup management (optimized from parkplatz/CONNECT.h):
- WiFi connection with stored credentials
- Access Point mode as fallback
- Captive portal for easy configuration
- Setup web pages: `/setup/v1/rotator/0/setup`, `/setup/v1/rotator/0/wifi`
- Control panel: `/setup/v1/rotator/0/configdevices`
- Command handler for rotator control: `/cmd`, `/position`, `/printip`

#### `include/servo_control.h` & `src/servo_control.cpp`
Servo motor control (optimized from parkplatz/CONNECT.h):
- Initialization of the ST3215 servo
- Automatic motor ID detection (scans ID 0-10)
- Movement functions: `moveServoToAngle()`, `moveServoByAngle()`
- Reverse function: Reverses movement direction (negates delta)
- Calibration: `setZeroPointExact()`, `setMiddle()`
- Feedback and status: `getFeedback()`, `isServoMoving()`, `getServoAngle()`
- Speed management: `setActiveSpeed()`, `getActiveSpeed()`

#### `include/display_control.h` & `src/display_control.cpp`
OLED display control (optimized from parkplatz/BOARD_DEV.h):
- SSD1306 128x32 OLED display (I2C 0x3C)
- Auto-update every 300ms with motor information
- Display: title, motor ID, mode, position, IP address
- Display on/off control via web interface
- Status messages during initialization

## Key Features

### Motor-Mode (3) Exclusive
- **Mode**: Exclusively Motor-Mode (3) - no position servo function
- **Auto-ID Detection**: Automatic scanning and detection of motor ID (0-10)
- **Virtual Positioning**: Position is managed in software (no hardware calibration)

### Reverse Function
- **Direction reversal**: Reverses the movement direction
- **Example**: Position 0°, target 45° → Normal: +1024 steps, Reverse: -1024 steps
- **Usage**: To bypass mechanical obstacles in the astronomical setup
