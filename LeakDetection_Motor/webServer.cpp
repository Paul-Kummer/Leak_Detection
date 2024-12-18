#include "webServer.h"
#include "motorControl.h"  // Include motor control functions
#include "settings.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>  

ESP8266WebServer server(80);  // Create web server on port 80
WiFiUDP udp;
const unsigned int BROADCAST_PORT = 4210;
unsigned long lastBroadcastTime = 0;
unsigned long slowBroadcastInterval = 5000;
unsigned long fastBroadcastInterval = 1000;

void initializeWebServer() {
    // Routes
    server.on("/", HTTP_GET, []() { serveFile("/index.html", "text/html"); });
    server.on("/calibration.html", HTTP_GET, []() { serveFile("/calibration.html", "text/html"); });
    server.on("/styles.css", HTTP_GET, []() { serveFile("/styles.css", "text/css"); });
    server.on("/scripts.js", HTTP_GET, []() { serveFile("/scripts.js", "application/javascript"); });
    server.on("/getIPAddress", HTTP_GET, handleGetIPAddress);

    // Motor and settings routes
    server.on("/motorControl", HTTP_POST, handleMotorControl);
    server.on("/setOpenPosition", HTTP_POST, handleSetOpenPosition);
    server.on("/setClosedPosition", HTTP_POST, handleSetClosedPosition);
    server.on("/getSettings", HTTP_GET, handleGetSettings);
    server.on("/saveSettings", HTTP_POST, handleSaveSettings);
    server.on("/getMotorPosition", HTTP_GET, handleGetMotorPosition);
    server.on("/setDesiredPosition", HTTP_POST, handleSetDesiredPosition);

    server.onNotFound(handleNotFound);  // 404 handler

    server.begin();  // Start the server
    Serial.println("HTTP server started.");

    if (WiFi.status() == WL_CONNECTED) {
        if (MDNS.begin("motor")) {
            Serial.println("mDNS responder started: motor.local");
        } else {
            Serial.println("Error setting up mDNS responder.");
        }
    }
}

void serveFile(const char* path, const char* contentType) {
    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        Serial.printf("File Sent: %s\n", path);
    } else {
        Serial.printf("File not found: %s\n", path);
        server.send(404, "text/plain", "File not found");
    }
}

void listFiles() {
    Serial.println("Listing files on LittleFS:");
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        Serial.printf("File: %s, Size: %d bytes\n", dir.fileName().c_str(), dir.fileSize());
    }
}

void handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}

void handleMotorControl() {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, server.arg("plain"));

    String command = doc["command"];
    int steps = doc["steps"];

    if (command == "increment") {
        motor.move(steps);
    } else if (command == "decrement") {
        motor.move(-steps);
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid command\"}");
        return;
    }

    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleSetOpenPosition() {
    motor.setCurrentPosition(0);
    server.send(200, "application/json", "{\"status\":\"success\"}");
    Serial.println("Open position set to 0 steps.");
}

void handleSetClosedPosition() {
    Serial.println("Setting Closed Position...");

    stepsToClose = motor.currentPosition();
    Serial.printf("Steps to Close: %d\n", stepsToClose);
    motor.move(-stepsToClose);

    StaticJsonDocument<100> response;
    response["status"] = "success";
    response["stepsToClosed"] = stepsToClose;

    String jsonResponse;
    serializeJson(response, jsonResponse);

    server.send(200, "application/json", jsonResponse);
}

void handleGetSettings() {
    StaticJsonDocument<200> doc;
    
    // Populate the JSON document with current settings
    doc["wifiSSID"] = wifiSSID;
    doc["wifiPassword"] = wifiPassword;
    doc["motorSpeed"] = motorSpeed;
    doc["motorAcceleration"] = motorAcceleration;
    doc["stepsToClose"] = stepsToClose;
    doc["webToken"] = webToken;

    String jsonResponse;
    serializeJson(doc, jsonResponse);  // Convert JSON to string

    server.send(200, "application/json", jsonResponse);  // Send JSON response
    Serial.println("Settings sent to client.");
}

void handleSetDesiredPosition() {
    if (server.hasHeader("Authorization")) {
        String token = server.header("Authorization");

        // Check if the provided token matches the expected token
        if (token == webToken) {
            // Parse the JSON body
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, server.arg("plain"));

            if (error) {
                Serial.println("Failed to parse JSON in handleSetDesiredPosition.");
                server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            int newPercentage = doc["desiredPosition"] | -1;
            if (newPercentage == -1) {
                Serial.println("desiredPosition not found in JSON.");
                server.send(400, "application/json", "{\"error\":\"desiredPosition not found\"}");
                return;
            }

            Serial.printf("Received new position of: %d%\n", newPercentage);
            setDesiredPosition(newPercentage);
            server.send(200, "application/json", "{\"status\":\"success\"}");
        }
        else {
            server.send(403, "application/json", "{\"error\":\"Unauthorized\"}");
        }
    }
    else {
        server.send(401, "application/json", "{\"error\":\"Missing token\"}");
    }
}

void handleGetMotorPosition() {
    StaticJsonDocument<100> doc;
    doc["currentPosition"] = motor.currentPosition();

    String jsonResponse;
    serializeJson(doc, jsonResponse);

    server.send(200, "application/json", jsonResponse);
    Serial.printf("Sent motor position: %d\n", motor.currentPosition());
}

void handleSaveSettings() {
    StaticJsonDocument<512> doc; // Increased size for all settings
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        Serial.println("Failed to parse JSON in saveSettings.");
        server.send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid JSON\"}");
        return;
    }

    // Set values from parsed JSON
    wifiSSID = doc["wifiSSID"] | wifiSSID;
    wifiPassword = doc["wifiPassword"] | wifiPassword;
    motorSpeed = doc["motorSpeed"] | motorSpeed;
    motorAcceleration = doc["motorAcceleration"] | motorAcceleration;
    stepsToClose = doc["stepsToClose"] | stepsToClose;
    webToken = doc["webToken"] | webToken;

    // Call function to save settings to file
    saveSettings();

    // Send success response
    server.send(200, "application/json", "{\"status\":\"success\"}");
    Serial.println("Settings saved successfully.");
}

void broadcastMotorPosition(bool fastUpdate) {
    // Check if it's time to broadcast
    if ( millis() - lastBroadcastTime >= (fastUpdate ? fastBroadcastInterval : slowBroadcastInterval)) {
        lastBroadcastTime = millis();  // Update last broadcast time

        if (WiFi.status() == WL_CONNECTED) {
            int percentagePosition = static_cast<int>((static_cast<float>(currentMotorPosition + 1) / stepsToClose) * 100);

            // Broadcast the integer percentage over UDP
            udp.beginPacket("255.255.255.255", BROADCAST_PORT);  // Use broadcast address
            udp.print(percentagePosition);
            udp.endPacket();

            Serial.printf("Broadcasting motor position as percentage: %d%%\n", percentagePosition);
        }
    }
}

void handleGetIPAddress() {
    StaticJsonDocument<100> doc;
    doc["ip"] = WiFi.localIP().toString();

    String jsonResponse;
    serializeJson(doc, jsonResponse);

    // Ensure the content-type is correct and only send the JSON response
    server.send(200, "application/json", jsonResponse);
    Serial.printf("Sent IP Address: %s\n", WiFi.localIP().toString().c_str());
}
