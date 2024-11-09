#include "webServer.h"
#include "settings.h"
#include "valveControl.h"  // Access valvePosition and desiredPosition
#include <Arduino.h>  // For String class
#include <ArduinoJson.h>
#include <WiFiUdp.h> 
#include <ESP8266HTTPClient.h>


// Define the http server and port
ESP8266WebServer server(80);
WiFiUDP udp;

// Define UDP port to listen on
const unsigned int UDP_PORT = 4210;
char incomingPacket[255];  // Buffer for incoming UDP packets

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
    server.on("/settings.json", HTTP_GET,[]() { serveFile("/settings.json", "application/json"); });
    server.on("/saveSettings", HTTP_POST, handleSaveSettings);
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

    // Start UDP listening
    if (udp.begin(UDP_PORT)) {
        Serial.printf("Listening for UDP broadcasts on port %d\n", UDP_PORT);
    } else {
        Serial.println("Failed to start UDP listener.");
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

void handleGetValvePosition() {
    String jsonResponse = "{\"valvePosition\": " + String(valvePosition) + "}";
    server.send(200, "application/json", jsonResponse);
}

void handleControlValve() {
    if (server.hasHeader("Authorization") && server.header("Authorization") == "LeakDetection_Motor") {
        // Parse JSON body
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error) {
            Serial.println("Failed to parse JSON in controlValve.");
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        // Extract desiredPosition from JSON
        int position = doc["desiredPosition"] | -1;
        if (position == -1) {
            Serial.println("Error: desiredPosition not found in JSON.");
            server.send(400, "application/json", "{\"error\":\"desiredPosition not found\"}");
            return;
        }

        Serial.printf("Received desired position from main server JS: %d\n", position);

        // Send the desired position to the motor node
        sendDesiredPositionToMotorNode(position);

        server.send(200, "application/json", "{\"status\":\"success\"}");
    }
    else {
        Serial.println("Error: Missing or incorrect authorization token.");
        server.send(403, "application/json", "{\"error\":\"Unauthorized\"}");
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

void handleSaveSettings() {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    // Update global variables directly
    wifiSSID = doc["wifiSSID"] | "";
    wifiPassword = doc["wifiPassword"] | "";
    webToken = doc["webToken"] | "";
    motorIP = doc["motorIP"] | "";

    // Save updated settings to the JSON file
    saveSettings();

    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void sendDesiredPositionToMotorNode(int position) {
    WiFiClient client;
    HTTPClient http;

    // Build the URL dynamically using motorIP
    String motorNodeUrl = "http://" + motorIP + "/setDesiredPosition";
    Serial.printf("Sending to Motor Node URL: %s with position: %d\n", motorNodeUrl.c_str(), position);

    http.begin(client, motorNodeUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", webToken);

    StaticJsonDocument<100> doc;
    doc["desiredPosition"] = position;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("Motor Node Response: %s\n", response.c_str());
    } else {
        Serial.printf("Error sending desired position: %d\n", httpResponseCode);
    }
    http.end();
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

void listenForUdp() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
        if (len > 0) incomingPacket[len] = '\0';  // Null-terminate the packet

        int receivedPercentage = atoi(incomingPacket);  // Convert to integer
        valvePosition = receivedPercentage;  // Update shared variable

        Serial.printf("Updated valve position to %d%%\n", valvePosition);

        // Optionally, acknowledge the packet (optional)
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.printf("Acknowledged: %d%%", receivedPercentage);
        udp.endPacket();
    }
}
