#include "sensorControl.h"
#include "motorControl.h"
#include "valveControl.h"

void setupSensorHandler() {
    // Sensor initialization code here
}

void handleSensors() {
    // Use motor control or other shared functions as needed
    if (desiredPosition > 0) {
        runMotorControl();  // Use motor control functions
    }
}
