#include "valveControl.h"
#include "displayControl.h"  // Include display control functions

int valvePosition = 0;
int desiredPosition = 0;

void setupValveControl() {
    pinMode(LED_BUILTIN, OUTPUT);  // Example setup
}

void controlValve() {
    // Display the current valve position on the OLED
    String positionText = String(valvePosition) + "%";
    displayInfo(positionText);  // Call the display function
}
