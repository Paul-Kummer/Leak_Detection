#include "settings.h"
#include <FS.h>  // Include SPIFFS library

// Global variables
String wifiSSID = "defaultSSID";
String wifiPassword = "defaultPassword";
int motorSpeed = 1000;
int motorAcceleration = 500;
int stepsToClose = 2048;
int currentMotorPosition = 0;
int desiredPosition = 0;
String webToken = "default_token";

// Load settings from SPIFFS
void loadSettings() {
    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    File file = SPIFFS.open(SETTINGS_FILE, "r");
    if (!file) {
        Serial.println("Settings file not found. Using default values.");
        saveSettings();  // Save default values to create the settings file
        return;
    }

    // Read the file content into a JSON document
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Failed to parse settings file. Using default values.");
        file.close();
        saveSettings();  // Save default values to reset the settings file
        return;
    }

    // Load settings from JSON
    wifiSSID = doc["wifiSSID"].as<String>();
    wifiPassword = doc["wifiPassword"].as<String>();
    motorSpeed = doc["motorSpeed"].as<int>();
    motorAcceleration = doc["motorAcceleration"].as<int>();
    stepsToClose = doc["stepsToClose"].as<int>();
    currentMotorPosition = doc["currentMotorPosition"].as<int>();
    desiredPosition = doc["desiredPosition"].as<int>();
    webToken = doc["webToken"].as<String>();

    file.close();
    Serial.println("Settings loaded successfully.");
}

// Save settings to SPIFFS
void saveSettings() {
    File file = SPIFFS.open(SETTINGS_FILE, "w");
    if (!file) {
        Serial.println("Failed to open settings file for writing.");
        return;
    }

    // Create a JSON document and populate it with settings
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    doc["wifiSSID"] = wifiSSID;
    doc["wifiPassword"] = wifiPassword;
    doc["motorSpeed"] = motorSpeed;
    doc["motorAcceleration"] = motorAcceleration;
    doc["stepsToClose"] = stepsToClose;
    doc["currentMotorPosition"] = currentMotorPosition;
    doc["desiredPosition"] = desiredPosition;
    doc["webToken"] = webToken;

    // Serialize the JSON document to the file
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to settings file.");
    }

    file.close();
    Serial.println("Settings saved successfully.");
}
