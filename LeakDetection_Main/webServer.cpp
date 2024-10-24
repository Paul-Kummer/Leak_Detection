#include "webServer.h"
#include "valveControl.h"  // Access valvePosition and desiredPosition
#include <Arduino.h>  // For String class

ESP8266WebServer server(80);

void InitializeWebServer() {
    // Initialize MDNS for network discovery
    if (MDNS.begin("whoisleaking")) {
        Serial.println("MDNS responder started");
    } else {
        Serial.println("Error starting MDNS responder!");
    }

    // Register routes and handlers
    server.on("/", handleRoot);
    server.on("/styles.css", handleStyles);
    server.on("/indexScripts.js", handleScript);
    server.on("/getValvePosition", handleGetValvePosition);
    server.on("/controlValve", HTTP_POST, handleControlValve);
    server.on("/sensorUpdate", HTTP_POST, handleSensorUpdate);
    server.onNotFound(handleNotFound);

    // Start the web server
    server.begin();
    Serial.println("HTTP server started");
}

void handleRoot() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        server.send(404, "text/plain", "File not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void handleGetValvePosition() {
    String jsonResponse = "{\"valvePosition\": " + String(valvePosition) + "}";
    server.send(200, "application/json", jsonResponse);
}

void handleControlValve() {
    if (server.hasArg("desiredPosition")) {
        desiredPosition = server.arg("desiredPosition").toInt();
        Serial.printf("Desired valve position set to: %d%%\n", desiredPosition);
        server.send(200, "application/json", "{\"status\":\"success\"}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing desiredPosition\"}");
    }
}

void handleSensorUpdate() {
    if (server.hasArg("mac") && server.hasArg("battery") && server.hasArg("leakDetected")) {
        String mac = server.arg("mac");
        int battery = server.arg("battery").toInt();
        bool detected = (server.arg("leakDetected") == "true");

        Serial.printf("Sensor Update - MAC: %s, Battery: %d%%, Leak: %s\n", 
                      mac.c_str(), battery, detected ? "Yes" : "No");

        if (detected) {
            desiredPosition = 0;
            Serial.println("Leak detected: Closing valve.");
        }

        server.send(200, "text/plain", detected ? "Leak detected" : "No leak");
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid sensor data\"}");
    }
}

void handleNotFound() {
    Serial.printf("404 Not Found: %s %s\n", 
                  server.method() == HTTP_GET ? "GET" : "POST", 
                  server.uri().c_str());
    server.send(404, "text/plain", "404: Not Found");
}

void handleStyles() {
    File file = SPIFFS.open("/styles.css", "r");
    if (!file) {
        server.send(404, "text/plain", "CSS file not found");
        return;
    }
    server.streamFile(file, "text/css");
    file.close();
}

void handleScript() {
    File file = SPIFFS.open("/indexScripts.js", "r");
    if (!file) {
        server.send(404, "text/plain", "JavaScript file not found");
        return;
    }
    server.streamFile(file, "application/javascript");
    file.close();
}
