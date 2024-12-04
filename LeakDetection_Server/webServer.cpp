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
    server.on("/getAlerts", HTTP_GET, handleGetAlerts);
    server.on("/getSavedSensors", HTTP_GET, handleGetSavedSensors);
    server.on("/getAvailableSensors", HTTP_GET, handleGetAvailableSensors);
    server.on("/updateIgnoredStatus", HTTP_POST, handleUpdateIgnoredStatus);
    server.on("/handleAddSensor", HTTP_POST, handleAddSensor);
    server.on("/handleRemoveSensor", HTTP_POST, handleRemoveSensor);
    server.on("/getSensors", HTTP_GET, handleGetSensors);
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

void handleGetAlerts() {
    StaticJsonDocument<2048> jsonDoc;
    JsonArray alertArray = jsonDoc.to<JsonArray>();

    for (const auto& sensor : sensorControl.getSensors()) {
        JsonObject alertObj = alertArray.createNestedObject();
        alertObj["Timestamp"] = sensor.timestamp;
        alertObj["Name"] = sensor.name;
        alertObj["Location"] = sensor.location;
        alertObj["ID"] = sensor.id;
        alertObj["Status"] = sensor.status;
        alertObj["Triggered"] = sensor.triggered;
        alertObj["Ignored"] = sensor.ignored;
    }

    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
}

void handleGetSavedSensors() {
    StaticJsonDocument<1024> jsonDoc;
    JsonArray savedSensors = jsonDoc.to<JsonArray>();

    for (const Sensor& sensor : sensorControl.getSensors()) {
        JsonObject sensorObj = savedSensors.createNestedObject();
        sensorObj["ID"] = sensor.id;
        sensorObj["Name"] = sensor.name;
        sensorObj["Location"] = sensor.location;
        sensorObj["Timestamp"] = sensor.timestamp;
        sensorObj["Status"] = sensor.status;
        sensorObj["Triggered"] = sensor.triggered;
        sensorObj["Ignored"] = sensor.ignored;
    }

    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
}

void handleUpdateIgnoredStatus() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No JSON body provided\"}");
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.println("Failed to parse JSON in updateIgnoredStatus.");
        return;
    }

    const char* sensorId = doc["ID"];
    bool ignoredStatus = doc["Ignored"] | false;

    if (!sensorId) {
        server.send(400, "application/json", "{\"error\":\"ID missing\"}");
        Serial.println("Missing sensor ID in updateIgnoredStatus.");
        return;
    }

    Sensor* sensor = sensorControl.getSensorById(sensorId);
    if (sensor) {
        sensor->ignored = ignoredStatus;
        if (sensorControl.saveSensors()) {
            server.send(200, "application/json", "{\"status\":\"success\"}");
            Serial.printf("Sensor %s ignored status updated to %s\n", sensorId, ignoredStatus ? "true" : "false");
        } else {
            server.send(500, "application/json", "{\"error\":\"Failed to save sensors\"}");
            Serial.println("Failed to save sensors in updateIgnoredStatus.");
        }
    } else {
        server.send(404, "application/json", "{\"error\":\"Sensor not found\"}");
        Serial.printf("Sensor %s not found in updateIgnoredStatus.\n", sensorId);
    }
}

void handleAddSensor() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No JSON body provided\"}");
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.println("Failed to parse JSON in addSensor.");
        return;
    }

    const char* sensorId = doc["ID"];
    if (!sensorId) {
        server.send(400, "application/json", "{\"error\":\"Sensor ID missing\"}");
        Serial.println("Sensor ID missing in addSensor.");
        return;
    }

    // Check if the sensor exists in availableSensors
    Sensor sensor;
    bool found = sensorControl.getAvailableSensorById(sensorId, sensor);
    if (found) {
        // Copy the sensor to savedSensors and update timestamp
        sensor.timestamp = sensorControl.generateISO8601Timestamp();
        if (sensorControl.addSavedSensor(sensor)) {
            // Remove the sensor from availableSensors
            sensorControl.removeAvailableSensor(sensorId);

            server.send(200, "application/json", "{\"status\":\"success\"}");
            Serial.printf("Sensor %s moved to saved sensors.\n", sensorId);
            return;
        } else {
            server.send(500, "application/json", "{\"error\":\"Failed to save sensor\"}");
            Serial.println("Failed to save sensor in addSensor.");
        }
    } else {
        server.send(404, "application/json", "{\"error\":\"Sensor not found in available sensors\"}");
        Serial.printf("Sensor %s not found in available sensors.\n", sensorId);
    }
}

void handleRemoveSensor() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No JSON body provided\"}");
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.println("Failed to parse JSON in removeSensor.");
        return;
    }

    const char* sensorId = doc["ID"];
    if (!sensorId) {
        server.send(400, "application/json", "{\"error\":\"Sensor ID missing\"}");
        Serial.println("Sensor ID missing in removeSensor.");
        return;
    }

    // Check if the sensor exists in savedSensors
    Sensor* sensor = sensorControl.getSensorById(sensorId);
    if (sensor) {
        // Copy the sensor to availableSensors and update timestamp
        Sensor newSensor = *sensor;  // Copy sensor data
        newSensor.timestamp = sensorControl.generateISO8601Timestamp();
        sensorControl.addRecentSensor(newSensor);

        // Remove the sensor from savedSensors
        if (sensorControl.removeSavedSensor(sensorId)) {
            server.send(200, "application/json", "{\"status\":\"success\"}");
            Serial.printf("Sensor %s moved to available sensors.\n", sensorId);
            return;
        } else {
            server.send(500, "application/json", "{\"error\":\"Failed to remove sensor\"}");
            Serial.println("Failed to remove sensor in removeSensor.");
        }
    } else {
        server.send(404, "application/json", "{\"error\":\"Sensor not found in saved sensors\"}");
        Serial.printf("Sensor %s not found in saved sensors.\n", sensorId);
    }
}

void listenForUdp() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
        if (len > 0) {
            incomingPacket[len] = '\0';  // Null-terminate the packet
        }

        Serial.printf("Received raw UDP packet: '%s'\n", incomingPacket);

        // Check if the packet looks like JSON
        if (incomingPacket[0] == '{') {
            StaticJsonDocument<256> jsonDoc;
            DeserializationError error = deserializeJson(jsonDoc, incomingPacket);

            if (!error) {
                Serial.println("Valid JSON received.");
                checkSensorBroadcast(jsonDoc);
            } else {
                Serial.printf("JSON parsing failed: %s\n", error.c_str());
            }
        } else {
            handleNonJsonUdpPacket(incomingPacket);
        }
    }
}

void handleNonJsonUdpPacket(const char* packet) {
    String packetStr(packet);
    packetStr.trim();  // Remove leading and trailing whitespace

    // Validate that the packet is numeric
    bool isNumeric = true;
    for (unsigned int i = 0; i < packetStr.length(); i++) {
        if (!isdigit(packetStr[i])) {
            isNumeric = false;
            break;
        }
    }

    if (!isNumeric) {
        Serial.printf("Invalid UDP packet: '%s'. Ignoring.\n", packet);
        return;
    }

    // Convert to integer
    int valvePosition = packetStr.toInt();
    if (valvePosition >= 0 && valvePosition <= 100) {
        Serial.printf("Valid valve position received via UDP: %d\n", valvePosition);
        handleValvePosition(valvePosition);
    } else {
        Serial.printf("Out-of-range valve position: %d. Ignoring.\n", valvePosition);
    }
}


// Function to handle valid valve positions
void handleValvePosition(int position) {
    // Validate position range again, if necessary
    if (position < 0 || position > 100) {
        Serial.printf("Invalid valve position: %d. Ignoring.\n", position);
        return;
    }

    // Update valve position globally or notify other components
    Serial.printf("Updating valve position to: %d\n", position);
    valvePosition = position;  // Assuming `valvePosition` is a global variable
}

void handleGetValvePosition() {
    Serial.printf("Current valve position: %d\n", valvePosition);  // Log the valve position
    String jsonResponse = "{\"valvePosition\": " + String(valvePosition) + "}";
    server.send(200, "application/json", jsonResponse);
}

void checkSensorBroadcast(const StaticJsonDocument<256>& jsonDoc) {
    const char* id = jsonDoc["ID"];
    const char* status = jsonDoc["Status"];
    bool triggered = jsonDoc["Triggered"] | false;

    if (id == nullptr || status == nullptr) {
        Serial.println("Invalid broadcast: Missing ID or Status");
        return;
    }

    Serial.printf("Broadcast received - ID: %s, Status: %s, Triggered: %s\n",
                  id, status, triggered ? "true" : "false");

    // Check if the sensor exists in the Saved Sensors list
    Sensor* sensor = sensorControl.getSensorById(std::string(id));
    if (sensor) {
        // Update existing sensor's attributes
        sensor->status = status; // Assuming status can be assigned directly
        sensor->triggered = triggered;
        sensor->timestamp = std::to_string(millis() / 1000); // Update timestamp
        Serial.printf("Sensor %s updated in Saved Sensors.\n", id);

        // Call checkSensorAlert after updating the sensor
        checkSensorAlert(jsonDoc);
    } else {
        // Create a new Sensor object for available sensors
        Sensor newSensor;
        newSensor.id = id; // This is MAC address
        newSensor.status = status; // Assuming status is assignable
        newSensor.triggered = triggered;
        newSensor.timestamp = std::to_string(millis() / 1000); // Assign current time
        newSensor.name = "New Sensor"; // Default name, if applicable
        newSensor.location = "Unknown"; // Default location, if applicable
        newSensor.ignored = false; // Default to not ignored

        // Add the new sensor to Available Sensors
        bool added = sensorControl.addRecentSensor(newSensor);
        if (added) {
            Serial.printf("Sensor %s added to Available Sensors.\n", id);
        } else {
            Serial.printf("Sensor %s already exists in Available Sensors.\n", id);
        }

        // Call checkSensorAlert for the new sensor
        checkSensorAlert(jsonDoc);
    }
}

// Status that will trigger the valve to close
void checkSensorAlert(const StaticJsonDocument<256>& jsonDoc) {
    const char* id = jsonDoc["ID"] | "Unknown";               // Sensor's MAC address
    const char* sensorStatus = jsonDoc["Status"] | "Unknown"; // Status: "ok" or "alert"
    bool sensorTriggered = jsonDoc["Triggered"] | false;      // Triggered: true or false

    // Print received JSON data
    Serial.printf("Checking alert conditions for sensor ID: %s\n", id);

    // Convert sensorStatus to std::string and lowercase for case-insensitive search
    std::string statusStr(sensorStatus);
    std::transform(statusStr.begin(), statusStr.end(), statusStr.begin(), ::tolower); // Convert to lowercase

    // Retrieve the sensor
    Sensor* sensor = sensorControl.getSensorById(id);
    if (sensor) {
        // Check conditions: status contains "alert" (case-insensitive), sensor is triggered, and not ignored
        if (statusStr.find("alert") != std::string::npos && sensorTriggered && !sensor->ignored) {
            Serial.printf("Sensor '%s' triggered ALERT and is not ignored. Setting valve position to 0%%.\n", id);

            // Update valve position
            desiredPosition = 0;

            // Notify the motor node
            sendDesiredPositionToMotorNode(0);
        } else {
            Serial.printf("Sensor '%s' ignored or not alerting. No action taken.\n", id);
        }
    } else {
        Serial.printf("Sensor '%s' not found in saved sensors. Ignoring alert check.\n", id);
    }
}

void handleGetSensors() {
    StaticJsonDocument<4096> jsonDoc;
    JsonArray sensorArray = jsonDoc.to<JsonArray>();

    for (const auto& sensor : sensorControl.getSensors()) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj["Timestamp"] = sensor.timestamp;
        sensorObj["Name"] = sensor.name;
        sensorObj["Location"] = sensor.location;
        sensorObj["ID"] = sensor.id;
        sensorObj["Status"] = sensor.status;
        sensorObj["Triggered"] = sensor.triggered;
        sensorObj["Ignored"] = sensor.ignored;
    }

    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
}

void handleGetAvailableSensors() {
    StaticJsonDocument<2048> jsonDoc;
    JsonArray sensorArray = jsonDoc.to<JsonArray>();

    for (const auto& sensor : sensorControl.getAvailableSensors()) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj["ID"] = sensor.id;
        sensorObj["Name"] = sensor.name;
        sensorObj["Location"] = sensor.location;
        sensorObj["Timestamp"] = sensor.timestamp;
        sensorObj["Status"] = sensor.status;
        sensorObj["Triggered"] = sensor.triggered;
        sensorObj["Ignored"] = sensor.ignored;
    }

    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
    Serial.printf("Sent available sensors: %s\n", response.c_str());
}
