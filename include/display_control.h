#pragma once

#include <Adafruit_SSD1306.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// I2C pins (standard ESP32)
#ifndef S_SDA
#define S_SDA 21
#endif
#ifndef S_SCL
#define S_SCL 22
#endif

// OLED Display functions
void initDisplay();
void updateDisplay();
void displayMotorInfo();
void displayMotorScan();
void displayMessage(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr, const char* line4 = nullptr);
void displayOff();
void displayOn();
bool isDisplayEnabled();
