#include "display_control.h"
#include "servo_control.h"
#include "wifi_manager.h"
#include <Wire.h>

// Global display object
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Display state
static bool displayEnabled = true;
static unsigned long lastUpdate = 0;
static const unsigned long UPDATE_INTERVAL = 300; // ms

// ============================================================================
// INITIALIZATION
// ============================================================================

void initDisplay() {
    // Initialize I2C
    Wire.begin(S_SDA, S_SCL);
    
    // Initialize display
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        displayEnabled = false;
        return;
    }
    
    Serial.println("SSD1306 Display initialized");
    
    // Clear display
    display.clearDisplay();
    display.display();
    displayEnabled = true;
    
    // Show startup message
    displayMessage("MoMa Rotator", "Initializing...");
    delay(1000);
}

// ============================================================================
// DISPLAY CONTROL
// ============================================================================

void displayOn() {
    displayEnabled = true;
    display.ssd1306_command(SSD1306_DISPLAYON);
}

void displayOff() {
    displayEnabled = false;
    display.clearDisplay();
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
}

bool isDisplayEnabled() {
    return displayEnabled;
}

// ============================================================================
// DISPLAY UPDATE
// ============================================================================

void updateDisplay() {
    if (!displayEnabled) return;
    
    // Throttle updates
    unsigned long now = millis();
    if (now - lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    lastUpdate = now;
    
    displayMotorInfo();
}

int getMotorID();  // Forward declaration

void displayMotorInfo() {
    if (!displayEnabled) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    // Line 1: Title
    display.println(F("MoMa Rotator"));
    
    // Line 2: Motor ID & Mode
    display.print(F("ID:"));
    display.print(getMotorID());  // Get actual motor ID
    display.print(F(" Mode:"));
    display.println(getServoMode());
    
    // Line 3: Position
    display.print(F("Pos: "));
    double pos = getServoAngle();
    display.print(pos, 1);
    display.println(F(" deg"));
    
    // Line 4: IP Address
    String ip = getIPAddress();
    display.println(ip);
    
    display.display();
}

void displayMotorScan() {
    if (!displayEnabled) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Scanning for"));
    display.println(F("motor..."));
    display.display();
}

void displayMessage(const char* line1, const char* line2, const char* line3, const char* line4) {
    if (!displayEnabled) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    if (line1) display.println(line1);
    if (line2) display.println(line2);
    if (line3) display.println(line3);
    if (line4) display.println(line4);
    
    display.display();
}
