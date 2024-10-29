#include "settings.h"
#include <LittleFS.h>  // File system library

// Define global constants and variables
const char* SETTINGS_FILE = "/settings.json";

String wifiSSID = "yourNetwork";
String wifiPassword = "yourPassword";
int motorSpeed = 1000;
int motorAcceleration = 500;
int stepsToClose = 2048;
int currentMotorPosition = 0;
int desiredPosition = 0;
String webToken = "default_token";

// Load settings from the file system
void loadSettings() {
    File file = LittleFS.open(SETTINGS_FILE, "r");
    if (!file) {
        Serial.println("Settings file not found. Using default values.");
        saveSettings();  // Save default values to create the file
        return;
    }

    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Failed to parse settings file. Using default values.");
        file.close();
        saveSettings();  // Reset to default values
        return;
    }

    wifiSSID = doc["wifiSSID"].as<String>();
    wifiPassword = doc["wifiPassword"].as<String>();
    motorSpeed = doc["motorSpeed"] | motorSpeed;
    motorAcceleration = doc["motorAcceleration"] | motorAcceleration;
    stepsToClose = doc["stepsToClose"] | stepsToClose;
    currentMotorPosition = doc["currentMotorPosition"] | currentMotorPosition;
    desiredPosition = doc["desiredPosition"] | desiredPosition;
    webToken = doc["webToken"].as<String>();

    file.close();
    Serial.println("Settings loaded successfully.");
}

void saveSettings() {
    File file = LittleFS.open(SETTINGS_FILE, "w");
    if (!file) {
        Serial.println("Failed to open settings file for writing.");
        return;
    }

    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    doc["wifiSSID"] = wifiSSID;
    doc["wifiPassword"] = wifiPassword;
    doc["motorSpeed"] = motorSpeed;
    doc["motorAcceleration"] = motorAcceleration;
    doc["stepsToClose"] = stepsToClose;
    doc["currentMotorPosition"] = currentMotorPosition;
    doc["desiredPosition"] = desiredPosition;
    doc["webToken"] = webToken;

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write settings to file.");
    } else {
        Serial.println("Settings saved successfully.");
        file.flush();  // Ensure data is written to disk
        file.close();
    }
}
