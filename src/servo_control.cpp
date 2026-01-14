#include <SMS_STS.h>
#include "servo_control.h"

// Hardware configuration
#define S_RXD 18
#define S_TXD 19
#define MOTOR_ID 1

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
const int MAX_FEEDBACK_RETRIES = 3;
bool motorBlocked = false;

// ============================================================================
// INITIALIZATION
// ============================================================================

void initServo() {
    Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
    st.pSerial = &Serial1;
    delay(200);
    
    while(!Serial1) {}

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
    
    // Set Motor-Mode (3)
    Serial.println("Setting Motor-Mode (3)...");
    setMode(3);
    delay(100);
    
    // Start at virtual position 0
    currentTargetPosition = 0;
    Serial.println("Motor-Mode initialized at virtual position 0");
}

// ============================================================================
// MODE CONTROL
// ============================================================================

void setMode(int mode) {
    if(mode == 0) {
        st.unLockEprom(MOTOR_ID);
        st.writeWord(MOTOR_ID, 11, 4095);
        st.writeByte(MOTOR_ID, SMS_STS_MODE, mode);
        st.LockEprom(MOTOR_ID);
    }
    else if(mode == 3) {
        st.unLockEprom(MOTOR_ID);
        st.writeByte(MOTOR_ID, SMS_STS_MODE, mode);
        st.writeWord(MOTOR_ID, 11, 0);
        st.LockEprom(MOTOR_ID);
    }
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
        
        // Check for motor blockage via high load
        if(abs(loadRead) > 800) {
            motorBlocked = true;
            Serial.print("Warning: High load (");
            Serial.print(loadRead);
            Serial.println("). Motor may be blocked!");
        } else {
            motorBlocked = false;
        }
    } else {
        feedbackRetries++;
        
        if(feedbackRetries >= MAX_FEEDBACK_RETRIES) {
            motorBlocked = true;
            Serial.println("FeedBack error - Motor blocked or not responding!");
        } else {
            delay(10);
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
}

void moveServoToAngle(double angleDeg) {
    // Convert gear degrees to motor steps
    // Gear ratio 1:2 means 180° gear = 360° motor = 4096 steps
    // Formula: motorSteps = (gearDegrees * GEAR_RATIO / 360.0) * SERVO_STEPS
    
    // Wrap angle to 0-359.99
    while(angleDeg >= 360.0) angleDeg -= 360.0;
    while(angleDeg < 0.0) angleDeg += 360.0;
    
    // For angles > 180°, use effective angle = abs(180 - angle)
    double effectiveAngle = angleDeg;
    if(angleDeg > 180.0) {
        effectiveAngle = abs(180.0 - angleDeg);
    }
    
    // Calculate motor degrees and steps
    double motorDegrees = effectiveAngle * GEAR_RATIO;
    int targetSteps = (int)((motorDegrees / SERVO_ANGLE_RANGE) * SERVO_STEPS);
    
    // Calculate relative delta
    s16 relativeDelta = targetSteps - currentTargetPosition;
    
    Serial.print("Move to ");
    Serial.print(angleDeg);
    Serial.print("° gear (");
    Serial.print(motorDegrees);
    Serial.print("° motor, ");
    Serial.print(targetSteps);
    Serial.print(" steps), delta=");
    Serial.println(relativeDelta);
    
    st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, SERVO_INIT_ACC);
    currentTargetPosition = targetSteps;
}

void moveServoByAngle(double deltaDeg) {
    // Calculate target from current position
    double currentAngle = getServoAngle();
    double targetAngle = currentAngle + deltaDeg;
    moveServoToAngle(targetAngle);
}

double getServoAngle() {
    // Convert current target position back to gear degrees
    // steps -> motor degrees -> gear degrees
    double motorDegrees = (currentTargetPosition / SERVO_STEPS) * SERVO_ANGLE_RANGE;
    double gearDegrees = motorDegrees / GEAR_RATIO;
    
    // Handle angles > 180°
    if(gearDegrees < 0) gearDegrees = 360.0 + gearDegrees;
    
    return gearDegrees;
}

// ============================================================================
// ZERO POINT & CALIBRATION
// ============================================================================

void setMiddle() {
    st.CalibrationOfs(MOTOR_ID);
}

void setZeroPointMode3() {
    if(modeRead == 3) {
        Serial.println("Setting zero point in motor mode...");
        
        // Switch to Mode 0 for calibration
        setMode(0);
        delay(100);
        
        // Set current position as zero
        st.CalibrationOfs(MOTOR_ID);
        delay(100);
        
        // Back to Mode 3
        setMode(3);
        delay(100);
        
        Serial.println("Zero point set successfully");
    } else {
        Serial.println("Warning: Not in motor mode");
    }
}

void setZeroPointExact() {
    Serial.println("Setting current position as zero point...");
    currentTargetPosition = 0;
    Serial.println("Current position set to 0° (zero point)");
}

void resetServoAngleZero() {
    setZeroPointExact();
}

// ============================================================================
// CONTROL FUNCTIONS
// ============================================================================

void stopServo() {
    st.EnableTorque(MOTOR_ID, 0);
    delay(10);
    st.EnableTorque(MOTOR_ID, 1);
}

void servoTorque(bool enable) {
    st.EnableTorque(MOTOR_ID, enable ? 1 : 0);
}

void setActiveSpeed(int speed) {
    activeServoSpeed = speed;
    if(activeServoSpeed > SERVO_MAX_SPEED) {
        activeServoSpeed = SERVO_MAX_SPEED;
    }
    if(activeServoSpeed < 0) {
        activeServoSpeed = 0;
    }
}

int getActiveSpeed() {
    return activeServoSpeed;
}

int getCurrentTargetPosition() {
    return currentTargetPosition;
}