#include <SMS_STS.h>
#include <math.h>

// === ST Servo === TypeNum:9 - Single Motor with ID=0
SMS_STS st;
float ServoDigitalRange_ST  = 4096.0;  // 0-4095 = 4096 Schritte
float ServoAngleRange_ST    = 360.0;
float ServoDigitalMiddle_ST = 2047.0;
#define ServoInitACC_ST      100
#define ServoMaxSpeed_ST     4000
#define MaxSpeed_X_ST        4000
#define ServoInitSpeed_ST    2000
float gradInSteps_ST = ServoAngleRange_ST / ServoDigitalRange_ST; // degree per digital unit
float gearRatio_ST = 2.0; // gear ratio 1 rotation for 180 deg 

// set serial feedback.
bool serialFeedback = true;

// Single motor with ID = 1
const byte MOTOR_ID = 0;
bool Torque_Status = true;

// Buffer for motor feedback
s16  loadRead;
s16  speedRead;
byte voltageRead;
int  currentRead;
s16  posRead;
s16  modeRead;
s16  temperRead;

s16 activeServoSpeed = 400;

// Virtueller Nullpunkt-Offset (wird von aktueller Position abgezogen)
s16 virtualZeroOffset = 0;

// Aktuelle Soll-Position (für Motor-Mode tracking)
s16 currentTargetPosition = 0;

// Motor block detection
int feedbackRetries = 0;
const int MAX_FEEDBACK_RETRIES = 3;
bool motorBlocked = false;

// the buffer of the bytes read from USB-C and servos. 
int usbRead;
int stsRead;

// Forward declarations
void setMode(byte InputMode);


void getFeedBack(){
  int result = st.FeedBack(MOTOR_ID);
  
  if(result != -1){
    // Erfolgreiche Kommunikation
    posRead = st.ReadPos(-1);
    speedRead = st.ReadSpeed(-1);
    loadRead = st.ReadLoad(-1);
    voltageRead = st.ReadVoltage(-1);
    currentRead = st.ReadCurrent(-1);
    temperRead = st.ReadTemper(-1);
    modeRead = st.ReadMode(MOTOR_ID);
    
    feedbackRetries = 0;
    
    // Check for motor blockage via high load
    if(abs(loadRead) > 800){
      motorBlocked = true;
      if(serialFeedback){
        Serial.print("Warning: High load detected (");
        Serial.print(loadRead);
        Serial.println("). Motor may be blocked!");
      }
    } else {
      motorBlocked = false;
    }
    
  } else {
    // Kommunikation fehlgeschlagen
    feedbackRetries++;
    
    if(feedbackRetries >= MAX_FEEDBACK_RETRIES){
      motorBlocked = true;
      if(serialFeedback){
        Serial.println("FeedBack err - Motor blocked or not responding!");
      }
    } else {
      // Retry
      delay(10);
    }
  }
}


void servoInit(){
  Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
  st.pSerial = &Serial1;
  while(!Serial1) {}

  // Setze Winkel-Limits für vollen Bewegungsbereich
  Serial.println("Setting angle limits...");
  st.unLockEprom(MOTOR_ID);
  delay(50);
  
  // Register 9 (0x09): Minimum Angle Limitation (2 bytes)
  st.writeByte(MOTOR_ID, 9, 0);    // Low Byte
  st.writeByte(MOTOR_ID, 10, 0);   // High Byte
  delay(50);
  
  // Register 11 (0x0B): Maximum Angle Limitation (2 bytes)
  st.writeByte(MOTOR_ID, 11, 4095 & 0xFF);      // Low Byte = 255
  st.writeByte(MOTOR_ID, 12, (4095 >> 8) & 0xFF); // High Byte = 15
  delay(50);
  
  st.LockEprom(MOTOR_ID);
  delay(100);
  
  // Lese zurück zur Verifikation
  s16 minAngle = st.readWord(MOTOR_ID, 9);
  s16 maxAngle = st.readWord(MOTOR_ID, 11);
  Serial.print("Min Angle read back: ");
  Serial.println(minAngle);
  Serial.print("Max Angle read back: ");
  Serial.println(maxAngle);
  
  // Setze Motor-Mode (3) als Standard
  Serial.println("Setting Motor-Mode (3)...");
  setMode(3);
  delay(100);
  
  // Im Motor-Mode starten wir bei Position 0 (keine Offset-Berechnung nötig)
  currentTargetPosition = 0;
  Serial.println("Motor-Mode initialized, starting at virtual position 0");
  
  Torque_Status = true;
}


void setMiddle(){
  st.CalibrationOfs(MOTOR_ID);
}


void setMode(byte InputMode){
  if(InputMode == 0){
    st.unLockEprom(MOTOR_ID);
    st.writeWord(MOTOR_ID, 11, 4095);
    st.writeByte(MOTOR_ID, SMS_STS_MODE, InputMode);
    st.LockEprom(MOTOR_ID);
  }
  else if(InputMode == 3){
    st.unLockEprom(MOTOR_ID);
    st.writeByte(MOTOR_ID, SMS_STS_MODE, InputMode);
    st.writeWord(MOTOR_ID, 11, 0);
    st.LockEprom(MOTOR_ID);
  }
}


void setZeroPointMode3(){
  if(modeRead == 3){
    Serial.println("Setting zero point in motor mode...");
    
    // Zuerst in Mode 0 wechseln für Kalibrierung
    setMode(0);
    delay(100);
    
    // Aktuelle Position als Null-Punkt setzen
    st.SetZero(MOTOR_ID, 0);
    delay(100);
    
    // Zurück zu Mode 3
    setMode(3);
    delay(100);
    
    Serial.println("Zero point set successfully in motor mode");
  } else {
    Serial.println("Warning: Not in motor mode (Mode 3)");
    Serial.print("Current mode: ");
    Serial.println(modeRead);
  }
}

// Setzt die aktuelle Position als neuen Nullpunkt (OHNE Motor-Bewegung)
void setZeroPointExact(){
  Serial.println("Setting current position as zero point...");
  
  // Im Motor-Mode: Setze die aktuelle Ziel-Position als neuen Nullpunkt
  currentTargetPosition = 0;
  
  Serial.println("Current position set to 0° (zero point)");
}


void servoStop(){
  st.EnableTorque(MOTOR_ID, 0);
  delay(10);
  st.EnableTorque(MOTOR_ID, 1);
}


void servoTorque(u8 enableCMD){
  st.EnableTorque(MOTOR_ID, enableCMD);
  Torque_Status = (enableCMD == 1);
}
