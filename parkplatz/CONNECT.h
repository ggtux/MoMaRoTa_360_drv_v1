// https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
#include <WiFi.h>
#include <WebServer.h>
#include <WEBPAGE.h>
#include <ESPAsyncWiFiManager.h>
#include <DNSServer.h>


int targetPosition;
int currentPos;
int targetDigitalPos;

// Create AsyncWebServer object on port 80
WebServer server(80);


void gotoPosition(int targetPosition, int currentPos){
  // Berechne relative Bewegung
  s16 relativeDelta = targetPosition - currentTargetPosition;
  
  Serial.print("Goto: target=");
  Serial.print(targetPosition);
  Serial.print(" current=");
  Serial.print(currentTargetPosition);
  Serial.print(" delta=");
  Serial.println(relativeDelta);
  
  st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, ServoInitACC_ST);
  
  // Speichere die Ziel-Position für spätere Abfragen
  currentTargetPosition = targetPosition;
}

void activeSpeed(int cmdInput){
  activeServoSpeed += cmdInput;
  if (activeServoSpeed > ServoMaxSpeed_ST){
    activeServoSpeed = ServoMaxSpeed_ST;
  }
  else if(activeServoSpeed < 0){
    activeServoSpeed = 0;
  }
}

int rangeCtrl(int rawInput, int minInput, int maxInput){
  if(rawInput > maxInput){
    return maxInput;
  }
  else if(rawInput < minInput){
    return minInput;
  }
  else{
    return rawInput;
  }
}

// Overload for float values
float rangeCtrl(float rawInput, float minInput, float maxInput){
  if(rawInput > maxInput){
    return maxInput;
  }
  else if(rawInput < minInput){
    return minInput;
  }
  else{
    return rawInput;
  }
}

void activeCtrl(int cmdInput){
  Serial.println("---   ---   ---");
  Serial.println("Motor ID: 0");
  Serial.println("---   ---   ---");
  switch(cmdInput){
    case 1: {
      // 90° Getriebe = 2047 Steps
      s16 target90 = 2047;
      s16 relativeDelta = target90 - currentTargetPosition;
      Serial.print("Case 1 - 90° Button:");
      Serial.print(" target=");
      Serial.print(target90);
      Serial.print(" delta=");
      Serial.println(relativeDelta);
      st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, ServoInitACC_ST);
      currentTargetPosition = target90;
      break;
    }
    
    case 2:
      servoStop();
      break;

    case 3:
      servoTorque(0);
      Torque_Status = false;
      break;

    case 4:
      servoTorque(1);
      Torque_Status = true;
      break;

    case 5: { // gehe zu 180° Getriebe = 4095 Steps
      s16 target180 = 4095;
      s16 relativeDelta = target180 - currentTargetPosition;
      Serial.print("Case 5 - 180° Button:");
      Serial.print(" target=");
      Serial.print(target180);
      Serial.print(" delta=");
      Serial.println(relativeDelta);
      st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, ServoInitACC_ST);
      currentTargetPosition = target180;
      break;
    }

    case 6: { // gehe zu Position Zero (0°)
      // Berechne relative Bewegung zur Zielposition
      s16 relativeDelta = 0 - currentTargetPosition;
      Serial.print("Case 6 - 0° Button: delta=");
      Serial.println(relativeDelta);
      st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, ServoInitACC_ST);
      currentTargetPosition = 0;
      break;
    }

    case 7:
      activeSpeed(100);
      break;
    case 8:
      activeSpeed(-100);
      break;
    case 11:
      setMiddle();
      break;
    case 14:
      SERIAL_FORWARDING = true;
      break;
    case 15:
      SERIAL_FORWARDING = false;
      break;
    case 18:
      setZeroPointExact(); // Exakte Nullpunkt-Setzung via Register 31
      break;
    case 22:
      setZeroPointMode3();
      break;
    case 17: {
      int targetPosition = server.arg(4).toInt();
      int currentPos = server.arg(5).toInt();   
      
      // Berechne relative Bewegung
      s16 relativeDelta = targetPosition - currentTargetPosition;
      
      Serial.print("Case 17 - Goto: target=");
      Serial.print(targetPosition);
      Serial.print(" current=");
      Serial.print(currentTargetPosition);
      Serial.print(" delta=");
      Serial.println(relativeDelta);
      
      st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, ServoInitACC_ST);
      currentTargetPosition = targetPosition;
      break;
    }
    case 20:
      RAINBOW_STATUS = 1;
      break;
    case 21:
      RAINBOW_STATUS = 0;
      break;
  }
}
 

void handleRoot(){
  server.send(200, "text/html", index_html); //Send web page
}


void handleID() {
  String IDmessage = "ID: 0";
  server.send(200, "text/plane", IDmessage);
}


void handleSTS() {
  String stsValue = "Active ID: 0";
  if(voltageRead != -1){
    stsValue += "  Position:" + String(posRead);
    stsValue += "<p>Voltage:" + String(float(voltageRead)/10);
    stsValue += "  Load:" + String(loadRead);
    stsValue += "<p>Speed:" + String(speedRead);
    stsValue += "  Temper:" + String(temperRead);
    stsValue += "<p>Speed Set:" + String(activeServoSpeed);
    stsValue += "<p>Mode:";
    if(modeRead == 0){
      stsValue += "Servo Mode";
    }
    else if(modeRead == 3){
      stsValue += "Motor Mode";
    }

    if(Torque_Status){
      stsValue += "<p>Torque On";
    }
    else{
      stsValue += "<p>Torque Off";
    }
  }
  else{
    stsValue += " FeedBack err";
  }
  server.send(200, "text/plane", stsValue); //Send ADC value only to client ajax request
}


void position() {
  // Im Motor-Mode verwenden wir die gespeicherte Ziel-Position
  // Umrechnung Steps zu Motor-Grad (4096 Steps = 360°)
  float motorDegrees = (currentTargetPosition / 4096.0) * 360.0;
  
  // Übersetzung 1:2 - Getriebe-Position berechnen
  const float GEAR_RATIO = 2.0;
  float gearDegrees = motorDegrees / GEAR_RATIO;
  
  String posValue = String(gearDegrees, 2);  // 2 Dezimalstellen
  server.send(200, "text/plane", posValue); //Send Position value in gear degrees
}

void printIP(){
  IPAddress ip = WiFi.localIP();
  String ipadresse = ip.toString();  // Liefert die IP als String
  String html_ip = "<div class='output-line' id='ip'>" + ipadresse + "</div>";
  // Seite an den Browser senden (ESP8266WebServer/Funktion)
  server.send(200, "text/html", html_ip);
}

void webCtrlServer(){
  server.on("/", handleRoot);
  server.on("/readID", handleID);
  server.on("/readSTS", handleSTS);
  server.on("/position", position);
  server.on("/printip", printIP);
    
  server.on("/cmd", [](){
    int cmdT = server.arg(0).toInt();
    int cmdI = server.arg(1).toInt();
    int cmdA = server.arg(2).toInt();
    int cmdB = server.arg(3).toInt();
    int cmbP = server.arg(4).toInt();
    int cmdD = server.arg(5).toInt();
    
    switch(cmdT){
      case 1:
        activeCtrl(cmdI);
        break;
      case 17:
        gotoPosition(cmbP, cmdD);
        break;
    }
  });

  // Start server
  server.begin();
  Serial.println("Server Starts.");
}


void webServerSetup(){
  webCtrlServer();
}


void getMAC(){
  WiFi.mode(WIFI_AP_STA);
  MAC_ADDRESS = WiFi.macAddress();
  Serial.print("MAC:");
  Serial.println(WiFi.macAddress());
}

void getIP(){
  IP_ADDRESS = WiFi.localIP();
}

void getWifiStatus(){
  if(WiFi.status() == WL_CONNECTED){
    WIFI_MODE = 2;
    getIP();
    WIFI_RSSI = WiFi.RSSI();
  }
  else if(WiFi.status() == WL_CONNECTION_LOST){
    WIFI_MODE = 3;
    WiFi.reconnect();
  }
}

void checkWiFiResetButton(){
  // Prüfe ob Reset-Button beim Boot gedrückt ist
  pinMode(WIFI_RESET_BUTTON, INPUT_PULLUP);
  
  if(digitalRead(WIFI_RESET_BUTTON) == LOW){
    Serial.println("\n=== WiFi Reset Button gedrückt ===");
    Serial.println("Warte 3 Sekunden...");
    
    delay(1000);
    
    // Prüfe nochmal ob immer noch gedrückt
    if(digitalRead(WIFI_RESET_BUTTON) == LOW){
      Serial.println("Button immer noch gedrückt - Lösche WiFi Einstellungen!");
      
      // Temporärer Server für Reset
      DNSServer dns;
      AsyncWebServer tempServer(80);
      AsyncWiFiManager wifiManager(&tempServer, &dns);
      
      wifiManager.resetSettings();
      
      Serial.println("WiFi Einstellungen gelöscht!");
      Serial.println("ESP32 startet neu...");
      
      delay(2000);
      ESP.restart();
    } else {
      Serial.println("Button losgelassen - Kein Reset");
    }
  }
}

void wifiInit(){
  Serial.println("=== WiFi Manager Setup ===");
  
  // DNS Server for Captive Portal
  DNSServer dns;
  AsyncWebServer configServer(80);
  AsyncWiFiManager wifiManager(&configServer, &dns);
  
  // Reset nur über Button beim Booten (siehe checkWiFiResetButton)
  // wifiManager.resetSettings();  // Nur zum Testen aktivieren!
  
  // Timeout für Config Portal (3 Minuten)
  wifiManager.setConfigPortalTimeout(180);
  
  // AP Name für Config Portal
  String apName = "MoMaRota";
  
  Serial.println("AP Name: " + apName);
  Serial.println("Versuche Verbindung zu gespeichertem WiFi...");
  
  // Versuche Verbindung herzustellen
  // Wenn fehlgeschlagen → Starte AP mit Captive Portal
  if(!wifiManager.autoConnect(apName.c_str(), AP_PWD)){
    Serial.println("WiFi Verbindung fehlgeschlagen - Timeout");
    Serial.println("Starte Neustart...");
    delay(3000);
    ESP.restart();
  }
  
  // Erfolgreich verbunden
  Serial.println("\n=== WiFi verbunden ===");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  
  WIFI_MODE = 2;  // STA Mode
  IP_ADDRESS = WiFi.localIP();
  WIFI_RSSI = WiFi.RSSI();
  MAC_ADDRESS = WiFi.macAddress();
}
