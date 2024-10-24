#include <Arduino.h>
#include "webServer.h"
#include "wifiSetup.h"
#include "displayControl.h"
#include "fileControl.h"
#include "motorControl.h"
#include "sensorControl.h"
#include "valveControl.h"

void setup() {
    Serial.begin(115200);

    connectToWiFi();            // Connect to Wi-Fi
    initializeDisplay();        // Initialize the OLED display
    initializeFileSystem();     // Initialize SPIFFS
    setupValveControl();        // Setup valve control
    initializeMotor();          // Setup motor control
    setupSensorHandler();       // Setup sensors
    InitializeWebServer();      // Start the web server
}

void loop() {
    server.handleClient();  // Handle incoming HTTP requests
    runMotorControl();      // Manage motor actions
    controlValve();         // Handle valve control logic
    handleSensors();        // Process sensor data
    MDNS.update();          // Ensure MDNS stays responsive
}
