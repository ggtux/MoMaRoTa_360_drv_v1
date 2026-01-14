#pragma once

// Servo initialization and control
void initServo();
int scanForMotor();  // Scan for motor ID on the bus

// Movement functions
void moveServoToAngle(double angleDeg);
void moveServoByAngle(double deltaDeg);
void gotoPosition(int targetPosition, int currentPos);

// Zero point and calibration
void resetServoAngleZero();
void setZeroPointExact();
void setZeroPointMode3();
void setMiddle();

// Control functions
void stopServo();
void servoTorque(bool enable);
void setMode(int mode);

// Status and feedback
double getServoAngle();
void getFeedback();
bool isServoMoving();
bool isMotorBlocked();
int getServoLoad();
int getServoSpeed();
int getServoVoltage();
int getServoCurrent();
int getServoTemperature();
int getServoMode();
int getMotorID();

// Position management
int getCurrentTargetPosition();
void setActiveSpeed(int speed);
int getActiveSpeed();