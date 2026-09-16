#include <SMS_STS.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "servo_control.h"
#include "rotator_motion_math.h"

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

// 70-tooth driving pulley and 140-tooth driven pulley.
#define GEAR_RATIO 2.0

// Motor polarity for this gearbox/wiring. A positive ASCOM angle requires a
// negative ST3215 step command. The ASCOM Reverse property flips this base.
const int MOTOR_BASE_DIRECTION = -1;

// SMS_STS servo object
SMS_STS st;
static SemaphoreHandle_t servoBusMutex = nullptr;

// Cable protection: the logical position may never wind more than one full
// rotation away from the position at which the controller was powered on.
const double CABLE_MIN_ANGLE = -360.0;
const double CABLE_MAX_ANGLE = 360.0;

static bool lockServoBus() {
    return servoBusMutex == nullptr ||
           xSemaphoreTake(servoBusMutex, pdMS_TO_TICKS(50)) == pdTRUE;
}

static void unlockServoBus() {
    if(servoBusMutex != nullptr) {
        xSemaphoreGive(servoBusMutex);
    }
}

// State variables
s16 activeServoSpeed = 400;
s16 currentTargetPosition = 0;
s16 virtualZeroOffset = 0;
bool reverseDirection = false;  // Reverse rotation direction
int activeMovementCommandSign = MOTOR_BASE_DIRECTION;
s16 absolutePosition = 0;  // Absolute accumulated position for display
static int physicalCableSteps = 0; // Motor steps since boot, independent of Reverse.
static Preferences positionStore;
s16 lastPosRead = 0;  // Last posRead value for delta calculation
static double stepsToOutputDegrees(int steps);

// Feedback variables
s16 loadRead = 0;
s16 speedRead = 0;
byte voltageRead = 0;
int currentRead = 0;
s16 posRead = 0;
s16 modeRead = 0;
s16 temperRead = 0;
int moveRead = 0;

// Alpaca movement state. Completion is based on a stable motor stop, not on
// exact floating-point equality with the requested angle.
const int MOVEMENT_POSITION_TOLERANCE_STEPS = 4;
// Some ST3215 units stop a few steps short although the move has completed.
// Keep the stricter movement tolerance, but snap the reported idle position
// for small residuals so Alpaca clients cannot wait on Position == TargetPosition.
// The geared mechanism can settle roughly one output degree before the exact
// step target. Treat up to 32 motor steps (about 1.4 output degrees) as reached
// once speed is zero, and report the requested target to ASCOM clients.
const int POSITION_REPORT_SNAP_STEPS = 32;
const unsigned long MOVEMENT_START_GRACE_MS = 200;
const unsigned long MOVEMENT_START_CONFIRM_MS = 1500;
const unsigned long MOVEMENT_STOP_SETTLE_MS = 500;
const unsigned long MOVEMENT_NO_PROGRESS_MS = 4000;
const unsigned long MOVEMENT_TIMEOUT_MS = 15000;
const unsigned long FEEDBACK_TIMEOUT_MS = 2000;
volatile bool movementActive = false;
volatile bool movementObserved = false;
unsigned long movementStartMillis = 0;
unsigned long lastValidFeedbackMillis = 0;
unsigned long stoppedSinceMillis = 0;
unsigned long lastProgressMillis = 0;
int bestRemainingSteps = -1;
double targetAngleDegrees = 0.0;
static const char* motionError = "";

static void saveVirtualPosition() {
    positionStore.begin("astro-orbit", false);
    positionStore.putInt("logical", absolutePosition);
    positionStore.putInt("physical", physicalCableSteps);
    positionStore.end();
}

static void restoreVirtualPosition() {
    positionStore.begin("astro-orbit", true);
    absolutePosition = positionStore.getInt("logical", 0);
    physicalCableSteps = positionStore.getInt("physical", absolutePosition * MOTOR_BASE_DIRECTION);
    positionStore.end();
    currentTargetPosition = 0;
    targetAngleDegrees = stepsToOutputDegrees(absolutePosition);
    while(targetAngleDegrees >= 360.0) targetAngleDegrees -= 360.0;
    while(targetAngleDegrees < 0.0) targetAngleDegrees += 360.0;
}

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
    servoBusMutex = xSemaphoreCreateMutex();
    Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
    st.pSerial = &Serial1;
    // Keep the library's proven timeout. Some controllers need noticeably
    // longer than the wire time before they begin their reply, especially
    // directly after startup or while the motor is under load.
    st.IOTimeOut = 100;
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
    restoreVirtualPosition();
    lastPosRead = 0;
    movementActive = false;
    Serial.print("Motor-Mode (3) initialized - restored position ");
    Serial.print(targetAngleDegrees, 2);
    Serial.println("°");
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
    if(!lockServoBus()) {
        return;
    }

    int result = st.FeedBack(MOTOR_ID);
    
    if(result != -1) {
        posRead = st.ReadPos(-1);
        speedRead = st.ReadSpeed(-1);
        loadRead = st.ReadLoad(-1);
        voltageRead = st.ReadVoltage(-1);
        currentRead = st.ReadCurrent(-1);
        temperRead = st.ReadTemper(-1);
        modeRead = st.ReadMode(MOTOR_ID);
        moveRead = st.ReadMove(-1);
        
        feedbackRetries = 0;
        consecutiveErrors = 0;
        motorBlocked = false;
        lastValidFeedbackMillis = millis();
        
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

    unlockServoBus();
}

void updateServoMovementState() {
    if(!movementActive) {
        return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - movementStartMillis;

    // A move must be visible to Alpaca immediately, even before the first
    // feedback sample reports a non-zero speed.
    if(elapsed < MOVEMENT_START_GRACE_MS) {
        return;
    }

    // Never retain a stale non-zero speed forever after communication loss.
    if(now - lastValidFeedbackMillis > FEEDBACK_TIMEOUT_MS) {
        stopServo();
        motionError = "Servo feedback timeout";
        Serial.println("Movement released: servo feedback timeout");
        movementActive = false;
        movementObserved = false;
        stoppedSinceMillis = 0;
        speedRead = 0;
        moveRead = 0;
        return;
    }

    // Fail safe: an Alpaca client must never wait indefinitely.
    if(elapsed > MOVEMENT_TIMEOUT_MS) {
        Serial.println("Movement timeout: stopping servo and releasing Alpaca");
        stopServo();
        motionError = "Movement timed out";
        return;
    }

    bool speedMoving = (abs(speedRead) > 10);
    bool hardwareMoving = (moveRead == 1);
    if(speedMoving || hardwareMoving) {
        movementObserved = true;
    }

    // Cached feedback can still show the pre-command stopped state when the
    // first IsMoving checks arrive. Do not declare completion until actual
    // motion was observed, or until the start-confirmation window expires.
    if(!movementObserved) {
        if(elapsed < MOVEMENT_START_CONFIRM_MS) {
            return;
        }

        stopServo();
        motionError = "No motor motion observed";
        Serial.println("Movement released: no motion observed after command");
        movementActive = false;
        movementObserved = false;
        stoppedSinceMillis = 0;
        return;
    }

    int remainingSteps = abs(posRead);
    if(remainingSteps > MOVEMENT_POSITION_TOLERANCE_STEPS &&
       (bestRemainingSteps < 0 ||
        remainingSteps + MOVEMENT_POSITION_TOLERANCE_STEPS < bestRemainingSteps)) {
        bestRemainingSteps = remainingSteps;
        lastProgressMillis = now;
    }

    // Any reliable completion signal may finish a move. Some ST3215 units keep
    // their moving bit set, while others report a small non-zero speed at rest.
    bool speedStopped = (abs(speedRead) <= 10);
    bool hardwareStopped = (moveRead == 0);
    bool targetReached = (remainingSteps <= MOVEMENT_POSITION_TOLERANCE_STEPS);
    bool settledWithinTolerance = speedStopped &&
                                  remainingSteps <= POSITION_REPORT_SNAP_STEPS;
    // A single stopped indication is not reliable in step mode. In particular,
    // ReadMove can briefly be zero while speed still shows real motion. Ending
    // the ASCOM move on either signal made clients issue their follow-up too
    // early. Require both stop signals, or the remaining-step target itself.
    if((speedStopped && hardwareStopped) || targetReached || settledWithinTolerance) {
        if(stoppedSinceMillis == 0) {
            stoppedSinceMillis = now;
        } else if(now - stoppedSinceMillis >= MOVEMENT_STOP_SETTLE_MS) {
            if(moveRead == 1) {
                Serial.println("Movement released: motor stopped with stale moving flag");
            }
            if(remainingSteps > POSITION_REPORT_SNAP_STEPS) motionError = "Motor stopped before target";
            movementActive = false;
            movementObserved = false;
            stoppedSinceMillis = 0;
            return;
        }
    } else {
        stoppedSinceMillis = 0;
    }

    // If the motor keeps reporting motion but no longer gets closer to the
    // target, stop it instead of blocking the imaging sequence indefinitely.
    if(remainingSteps > MOVEMENT_POSITION_TOLERANCE_STEPS &&
       bestRemainingSteps >= 0 &&
       now - lastProgressMillis >= MOVEMENT_NO_PROGRESS_MS) {
        Serial.print("Movement stalled with ");
        Serial.print(remainingSteps);
        Serial.println(" steps remaining - stopping and releasing Alpaca");
        stopServo();
        motionError = "Motor stalled";
        return;
    }
}

bool isServoMoving() {
    // The main loop owns the movement state machine. HTTP callbacks only read
    // this cached flag and therefore cannot block plate-solving requests.
    return movementActive;
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

static int getMotorCommandSign() {
    return reverseDirection ? -MOTOR_BASE_DIRECTION : MOTOR_BASE_DIRECTION;
}

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
    
    if(!lockServoBus()) {
        Serial.println("Goto rejected: servo bus busy");
        movementActive = false;
        return;
    }
    st.WritePosEx(MOTOR_ID, relativeDelta, activeServoSpeed, SERVO_INIT_ACC);
    unlockServoBus();
    
    currentTargetPosition = targetPosition;
    absolutePosition += relativeDelta;  // Update absolute position
}

static double stepsToOutputDegrees(int steps) {
    return (steps / SERVO_STEPS) * SERVO_ANGLE_RANGE / GEAR_RATIO;
}

static bool moveServoToUnwrappedAngle(double cableTargetAngle) {
    motionError = "";
    const double angleDeg = RotatorMotion::wrap(cableTargetAngle);
    targetAngleDegrees = angleDeg;

    double currentCableAngle = stepsToOutputDegrees(absolutePosition);
    double deltaDeg = cableTargetAngle - currentCableAngle;
    
    // Convert delta degrees to motor steps
    double motorDegrees = deltaDeg * GEAR_RATIO;
    s16 logicalDelta = (s16)((motorDegrees / SERVO_ANGLE_RANGE) * SERVO_STEPS);
    
    // Apply the physical motor polarity, then the optional ASCOM Reverse flag.
    int commandSign = getMotorCommandSign();
    s16 motorDelta = logicalDelta * commandSign;
    
    Serial.print("Move to ");
    Serial.print(angleDeg);
    Serial.print("° (cable ");
    Serial.print(currentCableAngle);
    Serial.print("° -> ");
    Serial.print(cableTargetAngle);
    Serial.print("°)");
    if(reverseDirection) Serial.print(" [REV]");
    Serial.print(" → delta: ");
    Serial.print(deltaDeg);
    Serial.print("° (");
    Serial.print(motorDelta);
    Serial.println(" steps)");
    
    // Positions closer than the physical resolution are already reached.
    if(abs(logicalDelta) <= MOVEMENT_POSITION_TOLERANCE_STEPS) {
        movementActive = false;
        movementObserved = false;
        stoppedSinceMillis = 0;
        return true;
    }

    if(!lockServoBus()) {
        Serial.println("Move rejected: servo bus busy");
        movementActive = false;
        movementObserved = false;
        return false;
    }

    movementStartMillis = millis();
    stoppedSinceMillis = 0;
    lastProgressMillis = movementStartMillis;
    bestRemainingSteps = -1;
    movementObserved = false;
    movementActive = true;
    activeMovementCommandSign = commandSign;
    st.WritePosEx(MOTOR_ID, motorDelta, activeServoSpeed, SERVO_INIT_ACC);
    unlockServoBus();
    currentTargetPosition += motorDelta;
    absolutePosition += logicalDelta;  // Always use logical delta for position tracking
    physicalCableSteps += motorDelta;
    saveVirtualPosition();
    return true;
}

bool moveServoToAngle(double angleDeg) {
    double cableTargetAngle;
    if(!RotatorMotion::absoluteTarget(stepsToOutputDegrees(absolutePosition), angleDeg,
                                    CABLE_MIN_ANGLE, CABLE_MAX_ANGLE, cableTargetAngle,
                                    stepsToOutputDegrees(physicalCableSteps), getMotorCommandSign())) {
        return false;
    }
    return moveServoToUnwrappedAngle(cableTargetAngle);
}

bool moveServoByAngle(double deltaDeg) {
    double cableTargetAngle;
    if(!RotatorMotion::relativeTarget(stepsToOutputDegrees(absolutePosition), deltaDeg,
                                    CABLE_MIN_ANGLE, CABLE_MAX_ANGLE, cableTargetAngle,
                                    stepsToOutputDegrees(physicalCableSteps), getMotorCommandSign())) {
        Serial.println("Relative move rejected by cable guard");
        return false;
    }
    // Preserve the requested direction and full turns. Wrapping here would
    // incorrectly turn Move(360) into no motion and Move(270) into Move(-90).
    return moveServoToUnwrappedAngle(cableTargetAngle);
}

double getServoAngle() {
    // Return the latest feedback cached by the main loop. Alpaca callbacks run
    // on another ESP32 task and must never access the serial servo bus directly.
    // posRead uses the physical motor-command sign. Convert it back to the
    // logical ASCOM direction before calculating the current angle.
    int logicalRemaining = posRead * activeMovementCommandSign;
    int currentPos = absolutePosition - logicalRemaining;

    // Once a move has completed with only a small physical residual, report
    // the requested angle exactly. This protects clients that incorrectly
    // compare Position and TargetPosition instead of relying on IsMoving.
    // 32 motor steps are about 1.4 output degrees with the 1:2 gearing.
    if(!movementActive && abs(logicalRemaining) <= POSITION_REPORT_SNAP_STEPS) {
        return targetAngleDegrees;
    }
    
    // Convert to gear degrees
    double motorDegrees = (currentPos / SERVO_STEPS) * SERVO_ANGLE_RANGE;
    double gearDegrees = motorDegrees / GEAR_RATIO;
    
    // Wrap to 0-360° range
    while(gearDegrees >= 360.0) gearDegrees -= 360.0;
    while(gearDegrees < 0.0) gearDegrees += 360.0;
    
    return gearDegrees;
}

double getServoTargetAngle() {
    return targetAngleDegrees;
}

// ============================================================================
// ZERO POINT & CALIBRATION
// ============================================================================

void setCurrentTargetPosition(int steps) {
    // Update current position without moving (used by Sync)
    currentTargetPosition = steps;
    absolutePosition = steps;  // Also update absolutePosition for display
    targetAngleDegrees = (steps / SERVO_STEPS) * SERVO_ANGLE_RANGE / GEAR_RATIO;
    while(targetAngleDegrees >= 360.0) targetAngleDegrees -= 360.0;
    while(targetAngleDegrees < 0.0) targetAngleDegrees += 360.0;
    movementActive = false;
    movementObserved = false;
    stoppedSinceMillis = 0;
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
    physicalCableSteps = 0;
    targetAngleDegrees = 0.0;
    movementActive = false;
    movementObserved = false;
    stoppedSinceMillis = 0;
    saveVirtualPosition();
    Serial.println("Virtual zero point set successfully");
}

void setZeroPointExact() {
    Serial.println("Setting current position as zero point...");
    currentTargetPosition = 0;
    absolutePosition = 0;
    physicalCableSteps = 0;
    targetAngleDegrees = 0.0;
    movementActive = false;
    movementObserved = false;
    stoppedSinceMillis = 0;
    saveVirtualPosition();
    Serial.println("Current position set to 0° (zero point)");
}

void resetServoAngleZero() {
    setZeroPointExact();
}

// ============================================================================
// CONTROL FUNCTIONS
// ============================================================================

void stopServo() {
    motionError = "";
    // Get current feedback before stopping
    getFeedback();
    
    // Correct absolutePosition to actual current position
    // Convert the physical remaining steps back to logical ASCOM direction.
    int logicalRemaining = posRead * activeMovementCommandSign;
    absolutePosition = absolutePosition - logicalRemaining;
    physicalCableSteps -= posRead;
    posRead = 0;
    
    if(lockServoBus()) {
        st.EnableTorque(MOTOR_ID, 0);
        delay(10);
        st.EnableTorque(MOTOR_ID, 1);
        unlockServoBus();
    } else {
        Serial.println("Stop warning: servo bus busy");
    }

    double stoppedMotorDegrees = (absolutePosition / SERVO_STEPS) * SERVO_ANGLE_RANGE;
    targetAngleDegrees = stoppedMotorDegrees / GEAR_RATIO;
    while(targetAngleDegrees >= 360.0) targetAngleDegrees -= 360.0;
    while(targetAngleDegrees < 0.0) targetAngleDegrees += 360.0;
    saveVirtualPosition();
    movementActive = false;
    movementObserved = false;
    stoppedSinceMillis = 0;
}

void servoTorque(bool enable) {
    if(lockServoBus()) {
        st.EnableTorque(MOTOR_ID, enable ? 1 : 0);
        unlockServoBus();
    }
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

bool isServoFeedbackHealthy() {
    return lastValidFeedbackMillis != 0 && !motorBlocked &&
           millis() - lastValidFeedbackMillis <= FEEDBACK_TIMEOUT_MS;
}
const char* getServoMotionError() { return motionError; }
