#include "motorControl.h"
#include "settings.h"
#include <FS.h>          // Generic file system support
#include <LittleFS.h>    // LittleFS support

// Define motor control pins
#define IN1 D1  // GPIO5
#define IN2 D2  // GPIO4
#define IN3 D5  // GPIO12
#define IN4 D6  // GPIO14

AccelStepper motor(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);

void initializeMotor() {
    loadSettings();  // Load settings from the file system

    motor.setMaxSpeed(motorSpeed);
    motor.setAcceleration(motorAcceleration);
    motor.setCurrentPosition(currentMotorPosition);
}

void setDesiredPosition(int percentage) {
    // Calculate desired position based on the percentage of steps to close
    desiredPosition = map(percentage, 0, 100, 0, stepsToClose);

    motor.moveTo(desiredPosition);  // Move motor to the desired position

    // Update current position and save it
    currentMotorPosition = desiredPosition;
    saveSettings();
}

void runMotorControl() {
    if (motor.distanceToGo() != 0) {
        motor.run();  // Run the motor until it reaches the desired position
    }
    else
    {
        motor.disableOutputs();  // This turns off motor signals
    }
}
