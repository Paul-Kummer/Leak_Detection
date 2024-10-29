#include "webServer.h"
#include "valveControl.h"  // Access valvePosition and desiredPosition
#include <Arduino.h>  // For String class
#include <ArduinoJson.h>

ESP8266WebServer server(80);

void InitializeWebServer() {
    // Initialize MDNS for network discovery
    if (MDNS.begin("whoisleaking")) {
        Serial.println("MDNS responder started");
    } else {
        Serial.println("Error starting MDNS responder!");
    }

    // Register routes and handlers
    server.on("/", HTTP_GET, []() { serveFile("/index.html", "text/html"); });
    server.on("/index.html", HTTP_GET, []() { serveFile("/index.html", "text/html"); });
    server.on("/settings.html", HTTP_GET, []() { serveFile("/settings.html", "text/html"); });
    server.on("/sensors.html", HTTP_GET, []() { serveFile("/sensors.html", "text/html"); });
    server.on("/styles.css", HTTP_GET, []() { serveFile("/styles.css", "text/css"); });
    server.on("/indexScripts.js", HTTP_GET, []() { serveFile("/indexScripts.js", "application/javascript"); });
    server.on("/getValvePosition", handleGetValvePosition);
    server.on("/controlValve", HTTP_POST, handleControlValve);
    server.on("/sensorUpdate", HTTP_POST, handleSensorUpdate);
    server.on("/getIPAddress", HTTP_GET, handleGetIPAddress);
    server.on("/favicon.ico", HTTP_GET, []() {server.send(204);} );
    server.onNotFound(handleNotFound);



    // Start the web server
    server.begin();
    Serial.println("HTTP server started");
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

void handleGetIPAddress() {
    StaticJsonDocument<100> doc;
    doc["ip"] = WiFi.localIP().toString();

    String jsonResponse;
    serializeJson(doc, jsonResponse);

    // Ensure the content-type is correct and only send the JSON response
    server.send(200, "application/json", jsonResponse);
    Serial.printf("Sent IP Address: %s\n", WiFi.localIP().toString().c_str());
}
