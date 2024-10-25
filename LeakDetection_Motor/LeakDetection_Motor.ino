#include <ESP8266WiFi.h>
#include "motorControl.h"
#include "webServer.h"
#include "settings.h"

void setup() {
    Serial.begin(115200);

    // Load settings from SPIFFS
    loadSettings();

    // Attempt to connect to Wi-Fi
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    Serial.print("Connecting to Wi-Fi");
    unsigned long startAttemptTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        initializeMotor();
        initializeWebServer();
    } else {
        // If connection fails, start as an access point
        Serial.println("\nFailed to connect. Starting AP mode...");
        WiFi.softAP("LeakDetection_Motor");

        IPAddress apIP = WiFi.softAPIP();
        Serial.print("AP IP Address: ");
        Serial.println(apIP);

        initializeWebServer();  // Start web server in AP mode
    }
}

void loop() {
    server.handleClient();  // Handle HTTP requests

    if (WiFi.status() == WL_CONNECTED && motor.distanceToGo() != 0) {
        motor.run();  // Run the motor to the target position
    }
}
