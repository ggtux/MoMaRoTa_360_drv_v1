#include <SMS_STS.h>
#include "servo_control.h"

// Hardware configuration
#define S_RXD 18
#define S_TXD 19

// Motor ID - automatically detected on startup
static int MOTOR_ID = 0;  // Default to 0, will be updated by scanForMotor()

// Servo parameters from ST3215
#define SERVO_STEPS 4096.0         // 0-4095 = 4096 Steps
#define SERVO_ANGLE_RANGE 360.0    // Full rotation
#define SERVO_INIT_ACC 100
#define SERVO_MAX_SPEED 4000
#define SERVO_INIT_SPEED 2000

// Gear ratio: 1:2 (180° gear = 360° motor = 1 full rotation)
#define GEAR_RATIO 2.0

// SMS_STS servo object
SMS_STS st;

// State variables
s16 activeServoSpeed = 400;
s16 currentTargetPosition = 0;
s16 virtualZeroOffset = 0;
bool reverseDirection = false;  // Reverse rotation direction
s16 absolutePosition = 0;  // Absolute accumulated position for display
s16 lastPosRead = 0;  // Last posRead value for delta calculation

// Feedback variables
s16 loadRead = 0;
s16 speedRead = 0;
byte voltageRead = 0;
int currentRead = 0;
s16 posRead = 0;
s16 modeRead = 0;
s16 temperRead = 0;

// Motor block detection
int feedbackRetries = 0;
const int MAX_FEEDBACK_RETRIES = 10;
bool motorBlocked = false;
int consecutiveErrors = 0;
const int ERROR_LOG_THRESHOLD = 100;

// ============================================================================
// INITIALIZATION
// ============================================================================

int scanForMotor() {
    Serial.println("\n=== Scanning for motor on bus ===");
    for(int id = 0; id <= 10; id++) {
        Serial.print("Trying ID ");
        Serial.print(id);
        Serial.print("... ");
        
        int result = st.FeedBack(id);
        if(result != -1) {
            s16 pos = st.ReadPos(-1);
            s16 mode = st.ReadMode(id);
            Serial.println("FOUND!");
            Serial.print("  Position: ");
            Serial.println(pos);
            Serial.print("  Mode: ");
            Serial.println(mode);
            
            // Automatically set the motor ID
            MOTOR_ID = id;
            Serial.print("\n>>> Motor-ID automatically set to: ");
            Serial.print(MOTOR_ID);
            Serial.println(" <<<\n");
            return id;
        }
        Serial.println("no response");
        delay(50);
    }
    Serial.println("=== No motor found ===");
    Serial.println("WARNING: Using default MOTOR_ID = 0\n");
    MOTOR_ID = 0;  // Fallback to 0
    return -1;
}

void initServo() {
    Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
    st.pSerial = &Serial1;
    delay(200);
    
    while(!Serial1) {}

    // Scan for motor to automatically detect ID
    Serial.println("Checking motor connection...");
    int foundID = scanForMotor();
    if(foundID == -1) {
        Serial.println("WARNING: No motor found! Check connections.");
        Serial.println("Continuing with default MOTOR_ID = 0");
    }
    delay(500);

    // Set angle limits for full range
    Serial.println("Setting angle limits...");
    st.unLockEprom(MOTOR_ID);
    delay(50);
    
    // Register 9: Minimum Angle Limitation (0)
    st.writeByte(MOTOR_ID, 9, 0);
    st.writeByte(MOTOR_ID, 10, 0);
    delay(50);
    
    // Register 11: Maximum Angle Limitation (4095)
    st.writeByte(MOTOR_ID, 11, 4095 & 0xFF);
    st.writeByte(MOTOR_ID, 12, (4095 >> 8) & 0xFF);
    delay(50);
    
    st.LockEprom(MOTOR_ID);
    delay(100);
    
    // Verify
    s16 minAngle = st.readWord(MOTOR_ID, 9);
    s16 maxAngle = st.readWord(MOTOR_ID, 11);
    Serial.print("Min Angle: ");
    Serial.print(minAngle);
    Serial.print(", Max Angle: ");
    Serial.println(maxAngle);
    
    // Set Motor-Mode (3) - ONLY MODE SUPPORTED
    Serial.println("Setting Motor-Mode (3) - locked permanently...");
    setMode(3);
    delay(100);
    
    // Verify mode
    modeRead = st.ReadMode(MOTOR_ID);
    Serial.print("Current Mode: ");
    Serial.println(modeRead);
    
    if(modeRead != 3) {
        Serial.println("WARNING: Mode is not 3! Retrying...");
        setMode(3);
        delay(100);
    }
    
    // Read current motor position and set as initial position
    getFeedback();
    currentTargetPosition = 0;
    absolutePosition = 0;
    lastPosRead = 0;
    Serial.println("Motor-Mode (3) initialized - position set to 0°");
}

// ============================================================================
// MODE CONTROL
// ============================================================================

void setMode(int mode) {
    // This driver ONLY supports Motor-Mode (Mode 3)
    if(mode != 3) {
        Serial.println("ERROR: Only Motor-Mode (3) is supported!");
        Serial.println("Forcing Mode 3...");
        mode = 3;
    }
    
    st.unLockEprom(MOTOR_ID);
    st.writeByte(MOTOR_ID, SMS_STS_MODE, 3);
    st.writeWord(MOTOR_ID, 11, 0);  // Max angle = 0 for motor mode
    st.LockEprom(MOTOR_ID);
    delay(50);
    
    Serial.println("Motor-Mode (3) set and locked");
}

// ============================================================================
// FEEDBACK & STATUS
// ============================================================================

void getFeedback() {
    int result = st.FeedBack(MOTOR_ID);
    
    if(result != -1) {
        posRead = st.ReadPos(-1);
        speedRead = st.ReadSpeed(-1);
        loadRead = st.ReadLoad(-1);
        voltageRead = st.ReadVoltage(-1);
        currentRead = st.ReadCurrent(-1);
        temperRead = st.ReadTemper(-1);
        modeRead = st.ReadMode(MOTOR_ID);
        
        feedbackRetries = 0;
        consecutiveErrors = 0;
        motorBlocked = false;
        
        // Check for motor blockage via high load
        if(abs(loadRead) > 800) {
            Serial.print("Warning: High load (");
            Serial.print(loadRead);
            Serial.println("). Motor may be blocked!");
        }
    } else {
        feedbackRetries++;
        consecutiveErrors++;
        
        if(feedbackRetries >= MAX_FEEDBACK_RETRIES) {
            motorBlocked = true;
            // Only log error every ERROR_LOG_THRESHOLD times to avoid spam
            if(consecutiveErrors % ERROR_LOG_THRESHOLD == 1) {
                Serial.println("\n=== MOTOR COMMUNICATION ERROR ===");
                Serial.print("FeedBack failed ");
                Serial.print(consecutiveErrors);
                Serial.println(" times");
                Serial.print("Trying MOTOR_ID = ");
                Serial.println(MOTOR_ID);
                Serial.println("Check: 1) Motor power, 2) RX/TX wiring, 3) Motor ID");
                Serial.println("================================\n");
            }
        }
    }
}

bool isServoMoving() {
    getFeedback();
    return (abs(speedRead) > 10); // Consider moving if speed > 10
}

bool isMotorBlocked() {
    return motorBlocked;
}

int getServoLoad() { return loadRead; }
int getServoSpeed() { return speedRead; }
int getServoVoltage() { return voltageRead; }
int getServoCurrent() { return currentRead; }
int getServoTemperature() { return temperRead; }
int getServoMode() { return modeRead; }
int getMotorID() { return MOTOR_ID; }

void setReverseDirection(bool reverse) { reverseDirection = reverse; }
bool getReverseDirection() { return reverseDirection; }

// ============================================================================
// MOVEMENT FUNCTIONS
// ============================================================================

void gotoPosition(int targetPosition, int currentPos) {
    // Calculate relative movement
    s16 relativeDelta = targetPosition - currentTargetPosition;
    
    Serial.print("Goto: target=");
    Serial.print(targetPosition);
    Serial.print(" current=");
    Serial.print(currentTargetPosition);
    Serial.print(" delta=");
    Serial.println(relativeDelta);
    
    st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, SERVO_INIT_ACC);
    
    currentTargetPosition = targetPosition;
    absolutePosition += relativeDelta;  // Update absolute position
}

void moveServoToAngle(double angleDeg) {
    // Get current angle
    double currentAngle = getServoAngle();
    
    // Wrap target angle to 0-359.99
    while(angleDeg >= 360.0) angleDeg -= 360.0;
    while(angleDeg < 0.0) angleDeg += 360.0;
    
    // Calculate shortest path delta in degrees
    double deltaDeg = angleDeg - currentAngle;
    
    // Normalize delta to shortest path (-180 to +180)
    while(deltaDeg > 180.0) deltaDeg -= 360.0;
    while(deltaDeg < -180.0) deltaDeg += 360.0;
    
    // Convert delta degrees to motor steps
    double motorDegrees = deltaDeg * GEAR_RATIO;
    s16 logicalDelta = (s16)((motorDegrees / SERVO_ANGLE_RANGE) * SERVO_STEPS);
    
    // Reverse: Invert movement direction for motor command only
    s16 motorDelta = reverseDirection ? -logicalDelta : logicalDelta;
    
    Serial.print("Move to ");
    Serial.print(angleDeg);
    Serial.print("° from ");
    Serial.print(currentAngle);
    Serial.print("°");
    if(reverseDirection) Serial.print(" [REV]");
    Serial.print(" → delta: ");
    Serial.print(deltaDeg);
    Serial.print("° (");
    Serial.print(motorDelta);
    Serial.println(" steps)");
    
    st.WritePosEx(MOTOR_ID, motorDelta, activeServoSpeed, SERVO_INIT_ACC);
    currentTargetPosition += motorDelta;
    absolutePosition += logicalDelta;  // Always use logical delta for position tracking
}

void moveServoByAngle(double deltaDeg) {
    // Calculate target from current position
    double currentAngle = getServoAngle();
    double targetAngle = currentAngle + deltaDeg;
    moveServoToAngle(targetAngle);
    // absolutePosition is already updated in moveServoToAngle
}

double getServoAngle() {
    // Get live position during movement
    getFeedback();
    
    // In Motor-Mode 3, posRead shows remaining distance to target
    // Calculate actual position: absolutePosition - posRead
    s16 currentPos = absolutePosition - posRead;
    
    // Convert to gear degrees
    double motorDegrees = (currentPos / SERVO_STEPS) * SERVO_ANGLE_RANGE;
    double gearDegrees = motorDegrees / GEAR_RATIO;
    
    // Wrap to 0-360° range
    while(gearDegrees >= 360.0) gearDegrees -= 360.0;
    while(gearDegrees < 0.0) gearDegrees += 360.0;
    
    return gearDegrees;
}

// ============================================================================
// ZERO POINT & CALIBRATION
// ============================================================================

void setCurrentTargetPosition(int steps) {
    // Update current position without moving (used by Sync)
    currentTargetPosition = steps;
    absolutePosition = steps;  // Also update absolutePosition for display
    Serial.print("Position synced to ");
    Serial.print(steps);
    Serial.println(" steps");
}

void setZeroPointMode3() {
    // In Motor-Mode (3): Do NOT switch modes!
    // Simply set virtual position to 0
    Serial.println("Setting zero point in motor mode (virtual)...");
    currentTargetPosition = 0;
    absolutePosition = 0;
    Serial.println("Virtual zero point set successfully");
}

void setZeroPointExact() {
    Serial.println("Setting current position as zero point...");
    currentTargetPosition = 0;
    absolutePosition = 0;
    Serial.println("Current position set to 0° (zero point)");
}

void resetServoAngleZero() {
    setZeroPointExact();
}

// ============================================================================
// CONTROL FUNCTIONS
// ============================================================================

void stopServo() {
    // Get current feedback before stopping
    getFeedback();
    
    // Correct absolutePosition to actual current position
    // posRead shows remaining distance to target
    // So actual position = target - remaining = absolutePosition - posRead
    absolutePosition = absolutePosition - posRead;
    
    st.EnableTorque(MOTOR_ID, 0);
    delay(10);
    st.EnableTorque(MOTOR_ID, 1);
}

void servoTorque(bool enable) {
    st.EnableTorque(MOTOR_ID, enable ? 1 : 0);
}

void setActiveSpeed(int speed) {
    // Clamp speed between reasonable limits
    // Minimum: 100 (very slow but still moves)
    // Maximum: SERVO_MAX_SPEED (4000)
    if(speed < 100) {
        speed = 100;
    }
    if(speed > SERVO_MAX_SPEED) {
        speed = SERVO_MAX_SPEED;
    }
    activeServoSpeed = speed;
    
    Serial.print("Speed set to: ");
    Serial.println(activeServoSpeed);
}

int getActiveSpeed() {
    return activeServoSpeed;
}

int getCurrentTargetPosition() {
    return currentTargetPosition;
}