#include "wifiSetup.h"
#include "settings.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Check if Wi-Fi is connected
bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Connect to the saved Wi-Fi network
bool connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(wifiSSID);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to Wi-Fi.");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("Failed to connect to Wi-Fi.");
        return false;
    }
}

// Connect to the Master AP
bool connectToMaster() {
    WiFi.mode(WIFI_STA);
    WiFi.begin("Leak_Master", ""); // Open network

    Serial.println("Connecting to Master AP...");

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to Master AP.");
        return true;
    } else {
        Serial.println("Failed to connect to Master AP.");
        return false;
    }
}

bool fetchSettingsFromMaster() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Not connected to Master AP. Cannot fetch settings.");
        return false;
    }

    WiFiClient client;  // Create a WiFiClient instance
    HTTPClient http;

    // Use the new begin API with WiFiClient
    http.begin(client, "http://192.168.4.1/settings.json");

    int httpCode = http.GET();

    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Settings retrieved:");
        Serial.println(payload);

        // Parse JSON
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("Failed to parse settings JSON: ");
            Serial.println(error.c_str());
            return false;
        }

        // Update settings
        wifiSSID = doc["wifiSSID"] | "";
        wifiPassword = doc["wifiPassword"] | "";
        webToken = doc["webToken"] | "";
        motorIP = doc["motorIP"] | "";

        saveSettings();
        return true;
    } else {
        Serial.printf("Failed to fetch settings. HTTP code: %d\n", httpCode);
        return false;
    }
}
