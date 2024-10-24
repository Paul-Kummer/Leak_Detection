#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <AccelStepper.h>

extern AccelStepper motor;  // Declare motor as extern

void initializeMotor();
void setDesiredPosition(int percentage);
void runMotorControl();

#endif
