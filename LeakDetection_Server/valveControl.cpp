#include "valveControl.h"
#include "displayControl.h"  // Include display control functions

int valvePosition = 0;
int desiredPosition = 0;

void setupValveControl() {
    pinMode(LED_BUILTIN, OUTPUT);  // Example setup
}

void controlValve() {
    if (valvePosition < desiredPosition) {
        valvePosition++;
    } else if (valvePosition > desiredPosition) {
        valvePosition--;
    }

    // Display the current valve position on the OLED
    String positionText = String(valvePosition) + "%";
    displayInfo(positionText);  // Call the display function

    Serial.printf("Valve Position: %d%%\n", valvePosition);
    delay(100);  // Simulate work
}
