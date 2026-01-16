// Temporäres Programm zum Auslesen der WiFi Credentials
// In setup() der main.cpp einfügen oder als separates Testprogramm verwenden

#include <Preferences.h>

void readStoredWiFiCredentials() {
    Preferences preferences;
    preferences.begin("wifi_config", true); // true = read-only mode
    
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    
    Serial.println("\n========================================");
    Serial.println("Gespeicherte WiFi Credentials:");
    Serial.println("========================================");
    Serial.print("SSID:     ");
    Serial.println(ssid.length() > 0 ? ssid : "(nicht gesetzt)");
    Serial.print("Password: ");
    Serial.println(password.length() > 0 ? password : "(nicht gesetzt)");
    Serial.println("========================================\n");
    
    preferences.end();
}
