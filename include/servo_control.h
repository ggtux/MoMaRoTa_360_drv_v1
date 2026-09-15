#pragma once

// Servo initialization and control
void initServo();
int scanForMotor();  // Scan for motor ID on the bus

// Movement functions
bool moveServoToAngle(double angleDeg);
bool moveServoByAngle(double deltaDeg);
void gotoPosition(int targetPosition, int currentPos);

// Zero point and calibration
void resetServoAngleZero();
void setZeroPointExact();
void setZeroPointMode3();
void setCurrentTargetPosition(int steps);  // For Sync command

// Control functions
void stopServo();
void servoTorque(bool enable);
void setMode(int mode);

// Status and feedback
double getServoAngle();
double getServoTargetAngle();
void getFeedback();
void updateServoMovementState();
bool isServoMoving();
bool isMotorBlocked();
int getServoLoad();
int getServoSpeed();
int getServoVoltage();
int getServoCurrent();
int getServoTemperature();
int getServoMode();
int getMotorID();
void setReverseDirection(bool reverse);
bool getReverseDirection();

// Position management
int getCurrentTargetPosition();
void setActiveSpeed(int speed);
int getActiveSpeed();

bool isServoFeedbackHealthy();
const char* getServoMotionError();
