#include "valveControl.h"
#include "displayControl.h"  // Include display control functions

int valvePosition = 0;
int desiredPosition = 0;

void setupValveControl() {
    pinMode(LED_BUILTIN, OUTPUT);  // Example setup
}

void controlValve() {
    // Debug: Print valve position being used
    // Serial.printf("controlValve using valve position: %d%%\n", valvePosition);

    // Display the current valve position on the OLED
    String positionText = String(valvePosition) + "%";
    displayInfo(positionText);
}
