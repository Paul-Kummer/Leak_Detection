#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "motorControl.h"
#include "webServer.h"
#include "settings.h"
#include "wifiSetup.h"

void setup() {
    Serial.begin(115200);

    // Initialize LittleFS
    Serial.println("Starting File System");
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS");
        return;
    }
    Serial.println("LittleFS mounted successfully.");
    listFiles();

    // Load settings from the file system
    loadSettings();

    // Initialize Wi-Fi (handles mDNS too)
    initializeWiFi();

    // Start motor and web server
    initializeMotor();
    initializeWebServer();
}

void loop() {
    server.handleClient();  // Handle HTTP requests

    if (isWiFiConnected()) {
        runMotorControl();  // Run motor only when connected
        updateMDNS();  // Update mDNS only at intervals
    }
}
