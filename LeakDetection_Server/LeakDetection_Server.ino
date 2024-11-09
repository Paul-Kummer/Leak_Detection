#include <Arduino.h>
#include "wifiSetup.h"
#include "webServer.h"
#include "settings.h"
#include "displayControl.h"
#include "valveControl.h"
#include "fileControl.h"
#include "sensorControl.h"

void setup() {
    Serial.begin(115200);

    initializeFileSystem();       // Initialize littleFS
    loadSettings();               // Load settings from JSON file
    connectToWiFi();              // Connect to Wi-Fi
    initializeDisplay();          // Initialize the OLED display
    setupValveControl();          // Setup valve control
     setupSensorHandler();         // Setup sensors
    InitializeWebServer();        // Start the web server
}

void loop() {
    server.handleClient();  // Handle incoming HTTP requests
    listenForUdp();         // Handle incoming UDP traffic
    controlValve();         // Handle valve control logic
    handleSensors();        // Process sensor data
    updateMDNS();           // Ensure MDNS stays responsive
}
