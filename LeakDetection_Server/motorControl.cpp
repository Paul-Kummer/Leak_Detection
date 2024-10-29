#include "motorControl.h"
#include "valveControl.h"

// Motor pins
#define IN1 12
#define IN2 13
#define IN3 0
#define IN4 14

// Steps required for one full valve movement (open to close)
const int steps_per_rev = 2048;

// Initialize the motor in HALF4WIRE mode
AccelStepper motor(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

void initializeMotor() {
    motor.setMaxSpeed(500);
    motor.setAcceleration(100);
    motor.setCurrentPosition(0);  // Initialize to the closed position
}

// Map the desired percentage (0 to 100%) to motor steps
void setDesiredPosition(int percentage) {
    desiredPosition = map(percentage, 0, 100, 0, steps_per_rev);
    motor.moveTo(desiredPosition);  // Set motor to move to the target position
    Serial.printf("Moving motor to %d%% (%d steps)\n", percentage, desiredPosition);
}

// Continuously run the motor until it reaches the target
void runMotorControl() {
    if (motor.distanceToGo() != 0) {
        motor.run();
    }
}
