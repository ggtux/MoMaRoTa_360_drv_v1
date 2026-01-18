/**
 * WiFiConfigPortal.h
 * 
 * Ein wiederverwendbares ESP32 WiFi-Konfigurationsportal mit Web-Interface
 * ANGEPASSTE VERSION: Nutzt einen bestehenden AsyncWebServer
 * 
 * Verwendung:
 *   AsyncWebServer server(80);
 *   WiFiConfigPortal portal("MoMaRoTa", "passwort123");
 *   portal.begin(server);  // Übergibt den bestehenden Server
 * 
 * Features:
 *   - Automatischer Access Point
 *   - Web-Interface zum Scannen und Verbinden
 *   - Unterstützung für versteckte Netzwerke
 *   - Status-Callbacks
 *   - Nutzt bestehenden Webserver (kein Port-Konflikt)
 */

#ifndef WIFI_CONFIG_PORTAL_H
#define WIFI_CONFIG_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

class WiFiConfigPortal {
public:
    // Konstruktor (ohne serverPort, da wir einen bestehenden Server nutzen)
    WiFiConfigPortal(const char* apSSID = "ESP32-WiFi-Config", 
                     const char* apPassword = "12345678");
    
    // Destruktor
    ~WiFiConfigPortal();
    
    // Starte das Portal mit bestehendem Server
    void begin(AsyncWebServer &server);
    
    // Update-Schleife (optional, für Statusausgaben)
    void loop();
    
    // Prüfe, ob mit WiFi verbunden
    bool isConnected();
    
    // Hole verbundene SSID
    String getConnectedSSID();
    
    // Hole IP-Adresse
    String getLocalIP();
    
    // Hole Access Point IP
    String getAPIP();
    
    // Setze Callback für erfolgreiche Verbindung
    void onConnect(void (*callback)(String ssid, String ip));
    
    // Setze Callback für Verbindungsfehler
    void onDisconnect(void (*callback)());
    
    // Stoppe den Access Point (nach erfolgreicher Verbindung)
    void stopAP();
    
    // Lade gespeicherte Credentials und verbinde
    bool tryStoredCredentials();
    
    // Speichere Credentials
    void saveCredentials(const char* ssid, const char* password);
    
private:
    // Konfiguration
    const char* _apSSID;
    const char* _apPassword;
    
    // Status
    bool _isConnecting;
    String _connectionStatus;
    unsigned long _lastStatusCheck;
    
    // Callbacks
    void (*_onConnectCallback)(String ssid, String ip);
    void (*_onDisconnectCallback)();
    
    // Interne Methoden
    void setupRoutes(AsyncWebServer &server);
    String getHTML();
    String scanNetworks();
    bool connectToWiFi(const char* ssid, const char* password);
};

#endif // WIFI_CONFIG_PORTAL_H
