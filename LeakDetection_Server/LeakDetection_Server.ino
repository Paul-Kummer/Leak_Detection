#include <Arduino.h>
#include "wifiSetup.h"
#include "webServer.h"
#include "settings.h"
#include "displayControl.h"
#include "valveControl.h"
#include "fileControl.h"
#include "sensorControl.h"

SensorControl sensorControl("/sensors.json");
static unsigned long lastUpdate = 0;

void setup() {
    Serial.begin(115200);

    initializeFileSystem();       // Initialize littleFS
    loadSettings();               // Load settings from JSON file
    initializeWiFi();             // Connect to Wi-Fi
    initializeDisplay();          // Initialize the OLED display
    setupValveControl();          // Setup valve control
    sensorControl.setupSensors(); // Setup sensors
    InitializeWebServer();        // Start the web server
}

void loop() {
    if (millis() - lastUpdate >= 60000) { // Check every 1 minute
        sensorControl.updateSensorCheckInStatus();
        lastUpdate = millis();
    }
    server.handleClient();  // Handle incoming HTTP requests
    listenForUdp();         // Handle incoming UDP traffic
    controlValve();         // Handle valve control logic
    updateMDNS();           // Ensure MDNS stays responsive
}
