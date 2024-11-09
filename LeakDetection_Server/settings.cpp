#include "settings.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

// Define global settings variables
String wifiSSID;
String wifiPassword;
String webToken;
String motorIP;

void loadSettings() {
    File settingsFile = LittleFS.open("/settings.json", "r");
    if (!settingsFile) {
        Serial.println("Failed to open settings.json");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, settingsFile);
    if (error) {
        Serial.println("Failed to parse settings.json");
        return;
    }

    wifiSSID = doc["wifiSSID"] | "";
    wifiPassword = doc["wifiPassword"] | "";
    webToken = doc["webToken"] | "";
    motorIP = doc["motorIP"] | "";

    settingsFile.close();
    Serial.println("Settings loaded successfully.");
}

void saveSettings() {
    StaticJsonDocument<512> doc;
    doc["wifiSSID"] = wifiSSID;
    doc["wifiPassword"] = wifiPassword;
    doc["webToken"] = webToken;
    doc["motorIP"] = motorIP;

    File settingsFile = LittleFS.open("/settings.json", "w");
    if (!settingsFile) {
        Serial.println("Failed to open settings.json for writing");
        return;
    }

    serializeJson(doc, settingsFile);
    settingsFile.close();
    Serial.println("Settings saved successfully.");
}
