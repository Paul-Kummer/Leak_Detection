#include "motorControl.h"
#include "settings.h"
#include <SPIFFS.h>

// Motor Pins
#define IN1 D6
#define IN2 D7
#define IN3 D3
#define IN4 D5

AccelStepper motor(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);

// Global variables
int currentMotorPosition = 0;
int desiredPosition = 0;
int stepsToClose = 0;  // Default value if not loaded from SPIFFS

void initializeMotor() {
    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount SPIFFS");
    } else {
        loadSettings();
    }

    motor.setMaxSpeed(MOTOR_SPEED);
    motor.setAcceleration(MOTOR_ACCELERATION);
    motor.setCurrentPosition(currentMotorPosition);
}

void setDesiredPosition(int percentage) {
    desiredPosition = map(percentage, 0, 100, 0, stepsToClose);
    motor.moveTo(desiredPosition);
    currentMotorPosition = desiredPosition;
    saveSettings();
}

void saveSettings() {
    File file = SPIFFS.open(SETTINGS_FILE, "w");
    if (!file) {
        Serial.println("Failed to open settings file for writing");
        return;
    }

    // Save motor position and stepsToClose
    file.printf("%d\n%d\n", currentMotorPosition, stepsToClose);
    file.close();
    Serial.println("Settings saved");
}

void loadSettings() {
    File file = SPIFFS.open(SETTINGS_FILE, "r");
    if (!file) {
        Serial.println("Settings file not found, using default values");
        currentMotorPosition = 0;
        stepsToClose = STEPS_PER_REVOLUTION;
        return;
    }

    // Load motor position and stepsToClose
    currentMotorPosition = file.readStringUntil('\n').toInt();
    stepsToClose = file.readStringUntil('\n').toInt();
    file.close();

    Serial.printf("Loaded motor position: %d, Steps to close: %d\n", 
                   currentMotorPosition, stepsToClose);
}
