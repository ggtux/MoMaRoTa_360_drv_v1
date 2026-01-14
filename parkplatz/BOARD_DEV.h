#include <Wire.h>
TaskHandle_t ScreenUpdateHandle;
TaskHandle_t ClientCmdHandle;

// SSD1306: 0x3C
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels, 32 as default.
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void InitScreen(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.display();
}


void screenUpdate(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  // Zeile 1: Titel
  display.println(F("MoMaRota"));
  
  // Zeile 2: Motor ID
  display.print(F("ID:"));
  display.println(MOTOR_ID);
  
  // Zeile 3: Mode
  display.print(F("Mode:"));
  display.println(modeRead);
  
  // Zeile 4: IP Adresse
  display.println(IP_ADDRESS);
  
  display.display();
}


void pingMotor(){
  RGBcolor(0, 255, 64);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Scanning for motor..."));
  display.display();

  int foundID = -1;
  Serial.println("========================================");
  Serial.println("Starting Servo ID Scan...");
  Serial.println("Scanning IDs 0-20...");
  Serial.println("========================================");
  
  // Scanne IDs von 0 bis 20
  for(int id = 0; id <= 20; id++){
    Serial.print("Checking ID: ");
    Serial.print(id);
    Serial.print(" ... ");
    
    int PingStatus = st.Ping(id);
    if(PingStatus != -1){
      foundID = id;
      Serial.println("FOUND!");
      Serial.println("========================================");
      Serial.print(">>> SERVO DETECTED AT ID: ");
      Serial.print(id);
      Serial.println(" <<<");
      Serial.println("========================================");
      
      display.clearDisplay();
      display.setCursor(0,0);
      display.println(F("Motor found!"));
      display.print(F("ID: "));
      display.println(id);
      display.display();
      
      if(id != MOTOR_ID){
        Serial.println("");
        Serial.println("!!! WARNING !!!");
        Serial.print("Motor has ID ");
        Serial.print(id);
        Serial.print(" but MOTOR_ID is set to ");
        Serial.println(MOTOR_ID);
        Serial.println("Please change motor ID or update MOTOR_ID constant");
        Serial.println("========================================");
      }
      break;
    } else {
      Serial.println("not found");
    }
  }

  if(foundID == -1){
    Serial.println("========================================");
    Serial.println(">>> NO SERVO DETECTED <<<");
    Serial.println("No motor found on any ID (0-20)");
    Serial.println("Please check:");
    Serial.println("- Servo power supply");
    Serial.println("- Serial connection (RX/TX)");
    Serial.println("- Baud rate (1000000)");
    Serial.println("========================================");
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("No motor found!"));
    display.println(F("Check connection"));
    display.display();
  } else {
    Serial.println("");
    Serial.print("Scan complete. Using Servo ID: ");
    Serial.println(foundID);
    Serial.println("========================================");
  }

  RGBoff();
  delay(2000);
}


void boardDevInit(){
    Wire.begin(S_SDA, S_SCL);
    InitScreen();
    InitRGB();
}


void InfoUpdateThreading(void *pvParameter){
  while(1){
    if(!SERIAL_FORWARDING && !RAINBOW_STATUS){
      getFeedBack();
      getWifiStatus();
      screenUpdate();
      delay(threadingInterval);
    }
    else if(SERIAL_FORWARDING){
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,0);
      display.println(F(" - - - - - - - -"));
      display.println(F("SERIAL_FORWARDING"));
      display.println(F(" - - - - - - - -"));
      display.display();
      delay(1000);
    }
    else if(RAINBOW_STATUS){
      display.clearDisplay();
      display.setTextSize(3);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,0);
      display.println(F(""));
      display.display();
      rainbow(0);
    }
  }
}


void workingModeSelect(){
  if(SERIAL_FORWARDING){
    while(SERIAL_FORWARDING){
      server.handleClient();
      if (Serial.available()){
        usbRead = Serial.read();
        Serial1.write(usbRead);
      }
      if (Serial1.available()){
        stsRead = Serial1.read();
        Serial.write(stsRead);
      }
    }
  }
}


void clientThreading(void *pvParameter){
  while(1){
    server.handleClient();
    workingModeSelect();
    delay(clientInterval);
  }
}


void threadInit(){
  xTaskCreatePinnedToCore(InfoUpdateThreading, "ScreenUpdateHandle", 4096, NULL, 1, &ScreenUpdateHandle, !ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(clientThreading, "ClientCmdHandle", 4096, NULL, 1, &ClientCmdHandle, ARDUINO_RUNNING_CORE);
}
