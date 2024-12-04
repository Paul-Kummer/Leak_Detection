#include "sensorControl.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <iostream>
#include <ctime>
#include <sstream>

// Constructor
SensorControl::SensorControl(const std::string& jsonFile) : jsonFile(jsonFile) {}

// Load sensors from JSON file
bool SensorControl::loadSensorsFromFile() {
    File file = LittleFS.open(jsonFile.c_str(), "r");
    if (!file) {
        Serial.printf("Error opening file: %s\n", jsonFile.c_str());
        return false;
    }

    const size_t bufferSize = 2048;
    StaticJsonDocument<bufferSize> jsonDoc;

    DeserializationError error = deserializeJson(jsonDoc, file);
    file.close();

    if (error) {
        Serial.printf("Failed to parse JSON: %s\n", error.c_str());
        return false;
    }

    sensors.clear();
    for (JsonObject sensorObj : jsonDoc.as<JsonArray>()) {
        Sensor sensor;
        sensor.id = sensorObj["ID"] | "";
        sensor.name = sensorObj["Name"] | "";
        sensor.location = sensorObj["Location"] | "";
        sensor.timestamp = sensorObj["Timestamp"] | "";
        sensor.status = sensorObj["Status"] | "Unknown";
        sensor.triggered = sensorObj["Triggered"] | false;
        sensor.ignored = sensorObj["Ignored"] | false;
        sensors.push_back(sensor);
    }

    Serial.println("Sensors loaded successfully.");
    return true;
}

// Setup sensors (called during initialization)
bool SensorControl::setupSensors() {
    return loadSensorsFromFile();
}

// Save sensors to JSON file
bool SensorControl::saveSensors() {
    Serial.println("Saving sensors...");
    printSensors();
    
    File file = LittleFS.open(jsonFile.c_str(), "w");
    if (!file) {
        Serial.printf("Error saving sensors to file: %s\n", jsonFile.c_str());
        return false;
    }

    StaticJsonDocument<2048> jsonDoc;
    JsonArray sensorArray = jsonDoc.to<JsonArray>();

    for (const auto& sensor : sensors) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj["ID"] = sensor.id;
        sensorObj["Name"] = sensor.name;
        sensorObj["Location"] = sensor.location;
        sensorObj["Timestamp"] = sensor.timestamp;
        sensorObj["Status"] = sensor.status;
        sensorObj["Triggered"] = sensor.triggered;
        sensorObj["Ignored"] = sensor.ignored;
    }

    if (serializeJson(jsonDoc, file) == 0) {
        Serial.println("Failed to write JSON to file.");
        return false;
    }

    file.close();
    return true;
}

// Add a new sensor
bool SensorControl::addSensor(const Sensor& sensor) {
    Sensor newSensor = sensor;
    if (newSensor.timestamp.empty()) {
        newSensor.timestamp = generateISO8601Timestamp(); // Ensure valid timestamp
    }
    sensors.push_back(newSensor);
    return saveSensors();
}

bool SensorControl::addSensorFromAvailable(const std::string& id) {
    // Find the sensor in the available sensors list
    auto it = std::find_if(recentSensorsQueue.begin(), recentSensorsQueue.end(),
                           [&](const Sensor& sensor) { return sensor.id == id; });

    if (it == recentSensorsQueue.end()) {
        Serial.printf("Sensor %s not found in available sensors.\n", id.c_str());
        return false;
    }

    // Add the sensor to the saved list
    Sensor newSensor = *it;
    sensors.push_back(newSensor);

    // Save to JSON
    if (!saveSensors()) {
        Serial.printf("Failed to save sensor %s to JSON.\n", id.c_str());
        return false;
    }

    // Remove from available list
    recentSensorsQueue.erase(it);
    recentSensorsSet.erase(id);

    Serial.printf("Sensor %s moved to saved sensors.\n", id.c_str());
    return true;
}

bool SensorControl::addSavedSensor(const Sensor& sensor) {
    sensors.push_back(sensor);
    return saveSensors();  // Save updated sensors to JSON
}


bool SensorControl::removeSavedSensor(const std::string& id) {
    auto it = std::remove_if(sensors.begin(), sensors.end(),
                             [&id](const Sensor& s) { return s.id == id; });
    if (it != sensors.end()) {
        sensors.erase(it, sensors.end());
        return saveSensors();  // Save updated sensors to JSON
    }
    return false;
}

bool SensorControl::removeAvailableSensor(const std::string& id) {
    auto it = std::find_if(recentSensorsQueue.begin(), recentSensorsQueue.end(),
                           [&id](const Sensor& s) { return s.id == id; });
    if (it != recentSensorsQueue.end()) {
        recentSensorsSet.erase(it->id);
        recentSensorsQueue.erase(it);
        return true;
    }
    return false;
}


std::vector<Sensor> SensorControl::getSensors() const {
    return sensors; // Return the saved sensors list
}

// Retrieve a sensor by ID
Sensor* SensorControl::getSensorById(const std::string& id) {
    // Search saved sensors
    for (auto& sensor : sensors) {
        if (sensor.id == id) {
            return &sensor;
        }
    }

    // Search available sensors
    for (auto& sensor : recentSensorsQueue) {
        if (sensor.id == id) {
            return &sensor;
        }
    }

    return nullptr;
}

// Update sensor data
void SensorControl::updateSensor(const std::string& id, const std::string& status, bool triggered, const std::string& timestamp) {
    Sensor* sensor = getSensorById(id);
    if (sensor) {
        sensor->status = status;
        sensor->triggered = triggered;
        sensor->timestamp = timestamp;
        Serial.printf("Updated sensor: %s (%s)\n", sensor->name.c_str(), sensor->location.c_str());
    } else {
        Serial.printf("Unknown sensor ID: %s\n", id.c_str());
    }
}

void SensorControl::updateSensorCheckInStatus() {
    unsigned long currentEpoch = millis() / 1000; // Current time in seconds
    for (auto& sensor : sensors) {
        unsigned long lastCheckIn = parseISO8601ToEpoch(sensor.timestamp);

        if (lastCheckIn == 0 || (currentEpoch - lastCheckIn > 300)) { // No Check-In after 5 minutes
            sensor.status = "No Check-In";
            sensor.timestamp = generateISO8601Timestamp(); // Update timestamp
        }
    }

    saveSensors(); // Save updated sensor states
}

unsigned long SensorControl::parseISO8601ToEpoch(const std::string& timestamp) {
    // Example timestamp: "2023-11-26T10:00:00"
    int year, month, day, hour, minute, second;
    if (sscanf(timestamp.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) {
        Serial.println("Failed to parse timestamp");
        return 0; // Return 0 if parsing fails
    }

    struct tm timeStruct = {};
    timeStruct.tm_year = year - 1900; // tm_year is years since 1900
    timeStruct.tm_mon = month - 1;    // tm_mon is 0-based
    timeStruct.tm_mday = day;
    timeStruct.tm_hour = hour;
    timeStruct.tm_min = minute;
    timeStruct.tm_sec = second;

    time_t epochTime = mktime(&timeStruct);
    if (epochTime == -1) {
        Serial.println("Failed to convert to epoch time");
        return 0; // Return 0 if conversion fails
    }

    return static_cast<unsigned long>(epochTime);
}

std::string SensorControl::generateISO8601Timestamp() const {
    time_t now = time(nullptr); // Get current time
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo); // Get UTC time

    char buffer[25];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo); // Format as ISO8601

    return std::string(buffer);
}

// Print all sensors (for debugging)
void SensorControl::printSensors() const {
    for (const auto& sensor : sensors) {
        Serial.printf(
            "ID: %s, Name: %s, Location: %s, Timestamp: %s, Status: %s, Triggered: %s\n",
            sensor.id.c_str(),
            sensor.name.c_str(),
            sensor.location.c_str(),
            sensor.timestamp.c_str(),
            sensor.status.c_str(),
            sensor.triggered ? "True" : "False",
            sensor.ignored ? "True" : "False"
        );
    }
}

bool SensorControl::addRecentSensor(const Sensor& newSensor) {
    // Check if the sensor already exists
    auto it = std::find_if(recentSensorsQueue.begin(), recentSensorsQueue.end(),
                           [&](const Sensor& sensor) { return sensor.id == newSensor.id; });

    if (it != recentSensorsQueue.end()) {
        // Update existing sensor's attributes
        it->status = newSensor.status;
        it->triggered = newSensor.triggered;
        if (newSensor.timestamp.empty()) {
            it->timestamp = generateISO8601Timestamp(); // Update timestamp to current
        } else {
            it->timestamp = newSensor.timestamp;
        }
        Serial.printf("Updated sensor ID %s in available sensors.\n", newSensor.id.c_str());
        return false; // Sensor already existed, so no addition
    }

    // Add the new sensor if not already in the queue
    if (recentSensorsQueue.size() >= 50) {
        // Remove the oldest sensor if the queue is full
        recentSensorsSet.erase(recentSensorsQueue.front().id);
        recentSensorsQueue.pop_front();
    }

    Sensor sensorWithTimestamp = newSensor;
    if (sensorWithTimestamp.timestamp.empty()) {
        sensorWithTimestamp.timestamp = generateISO8601Timestamp(); // Ensure valid timestamp
    }

    recentSensorsQueue.push_back(sensorWithTimestamp);
    recentSensorsSet.insert(sensorWithTimestamp.id);
    Serial.printf("Added sensor ID %s to available sensors.\n", sensorWithTimestamp.id.c_str());
    return true;
}

std::vector<Sensor> SensorControl::getAvailableSensors() const {
    std::vector<Sensor> availableSensors;
    for (const auto& sensor : recentSensorsQueue) {
        Sensor temp = sensor;
        if (temp.timestamp.empty()) {
            temp.timestamp = generateISO8601Timestamp(); // Assign valid timestamp if missing
        }
        availableSensors.push_back(temp);
    }
    return availableSensors;
}

bool SensorControl::getAvailableSensorById(const std::string& id, Sensor& sensor) const {
    for (const auto& s : recentSensorsQueue) {
        if (s.id == id) {
            sensor = s;
            return true;
        }
    }
    return false;
}

void SensorControl::checkSensorBroadcast(const StaticJsonDocument<256>& jsonDoc) {
    Sensor newSensor;
    newSensor.id = jsonDoc["ID"] | "";
    newSensor.status = jsonDoc["Status"] | "Unknown";
    newSensor.triggered = jsonDoc["Triggered"] | false;
    newSensor.timestamp = generateISO8601Timestamp();

    if (newSensor.id.empty()) {
        Serial.println("Invalid broadcast: Missing sensor ID.");
        return;
    }

    // Check and update saved sensors
    Sensor* existingSensor = getSensorById(newSensor.id);
    if (existingSensor) {
        // Update saved sensor
        updateSensor(newSensor.id, newSensor.status, newSensor.triggered, newSensor.timestamp);
    } else {
        // Add to saved sensors
        addSensor(newSensor);
    }

    // Add to available sensors (recentSensorsQueue)
    addRecentSensor(newSensor);
}
