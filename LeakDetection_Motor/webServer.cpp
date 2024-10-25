#include "webServer.h"
#include "motorControl.h"
#include "settings.h"
#include <ArduinoJson.h>
#include <FS.h>  // SPIFFS library

ESP8266WebServer server(80);  // Initialize server on port 80

void initializeWebServer() {
    setupMDNS();  // Setup mDNS for hostname discovery

    // Routes for serving files
    server.on("/", HTTP_GET, []() { serveFile("/index.html", "text/html"); });
    server.on("/calibration.html", HTTP_GET, []() { serveFile("/calibration.html", "text/html"); });
    server.on("/styles.css", HTTP_GET, []() { serveFile("/styles.css", "text/css"); });
    server.on("/scripts.js", HTTP_GET, []() { serveFile("/scripts.js", "application/javascript"); });

    // Routes for motor and settings management
    server.on("/motorControl", HTTP_POST, handleMotorControl);
    server.on("/setOpenPosition", HTTP_POST, handleSetOpenPosition);
    server.on("/setClosedPosition", HTTP_POST, handleSetClosedPosition);
    server.on("/saveSettings", HTTP_POST, handleSaveSettings);

    server.onNotFound(handleNotFound);  // 404 handler

    // Start the server
    server.begin();
    Serial.println("HTTP server started.");
}

void setupMDNS() {
    if (MDNS.begin("motor")) {
        Serial.println("mDNS responder started: motor.local");
    } else {
        Serial.println("Error setting up mDNS responder.");
    }
}

void serveFile(const char* path, const char* contentType) {
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
    } else {
        server.send(404, "text/plain", "File not found");
    }
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
    motor.setCurrentPosition(0);  // Set current position to zero
    server.send(200, "application/json", "{\"status\":\"success\"}");
    Serial.println("Open position set to 0 steps.");
}

void handleSetClosedPosition() {
    stepsToClose = motor.currentPosition();

    StaticJsonDocument<100> response;
    response["status"] = "success";
    response["stepsToClosed"] = stepsToClose;

    String jsonResponse;
    serializeJson(response, jsonResponse);
    server.send(200, "application/json", jsonResponse);

    Serial.printf("Closed position set to %d steps.\n", stepsToClose);
}

void handleSaveSettings() {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, server.arg("plain"));

    stepsToClose = doc["stepsToClose"];
    saveSettings();

    server.send(200, "application/json", "{\"status\":\"success\"}");
    Serial.printf("Saved steps to closed: %d\n", stepsToClose);
}

void handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}
