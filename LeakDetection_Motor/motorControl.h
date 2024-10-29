#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <AccelStepper.h>

// Declare the motor instance as extern
extern AccelStepper motor;

void initializeMotor();
void setDesiredPosition(int percentage);
void saveSettings();   // Save motor state to file
void loadSettings();   // Load motor state from file
void runMotorControl();

#endif
