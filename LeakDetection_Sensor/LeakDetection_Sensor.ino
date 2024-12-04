#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "settings.h"
#include "wifiSetup.h"

const int sensorPin = D5;  // Pin connected to the sensor
const unsigned long SLEEP_DURATION = 30 * 1000000;  // 30 seconds in microseconds

WiFiUDP udp;
bool sensorTriggered = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialize LittleFS for settings management
    if (!LittleFS.begin()) {
        Serial.println("Failed to initialize file system.");
        return;
    }

    // Load saved Wi-Fi settings
    loadSettings();

    // Step 1: Attempt to connect to the saved Wi-Fi network
    if (!connectToWiFi()) {
        Serial.println("Failed to connect to saved Wi-Fi. Connecting to Master AP...");

        // Step 2: Connect to Master AP and fetch new settings
        if (connectToMaster() && fetchSettingsFromMaster()) {
            Serial.println("New settings retrieved. Reconnecting to Wi-Fi...");
            connectToWiFi(); // Step 3: Reconnect using updated settings
        } else {
            Serial.println("Failed to connect to Master AP or retrieve new settings.");
        }
    }

    // Step 4: Read sensor state
    pinMode(sensorPin, INPUT);
    sensorTriggered = digitalRead(sensorPin) == HIGH;

    if (sensorTriggered) {
        Serial.println("Sensor triggered! Broadcasting alert...");
        broadcastStatus(true);
    } else {
        Serial.println("No sensor trigger. Broadcasting normal status...");
        broadcastStatus(false);
    }

    // Step 5: Enter deep sleep
    Serial.println("Entering deep sleep...");
    ESP.deepSleep(SLEEP_DURATION, WAKE_RF_DEFAULT);
}

void loop() {}

// Broadcast sensor status over UDP
void broadcastStatus(bool triggered) {
    if (WiFi.status() == WL_CONNECTED) {
        String macAddress = WiFi.macAddress();
        String status = triggered ? "ALERT" : "OK";

        // Create JSON message
        String message = String("{\"ID\":\"") + macAddress + "\",\"Status\":\"" + status + 
                         "\",\"Triggered\":" + (triggered ? "true" : "false") + "}";

        IPAddress broadcastIP = getBroadcastAddress();

        for (int i = 0; i < 5; i++) {
            udp.beginPacket(broadcastIP, 4210);
            udp.print(message);
            udp.endPacket();
            delay(300);
        }

        Serial.printf("Broadcasted to %s: %s\n", broadcastIP.toString().c_str(), message.c_str());
    } else {
        Serial.println("Wi-Fi not connected. Cannot broadcast.");
    }
}

// Helper function to calculate the broadcast address
IPAddress getBroadcastAddress() {
    IPAddress localIP = WiFi.localIP();
    IPAddress subnetMask = WiFi.subnetMask();
    IPAddress broadcastIP;

    for (int i = 0; i < 4; i++) {
        broadcastIP[i] = (localIP[i] & subnetMask[i]) | (~subnetMask[i] & 0xFF);
    }

    return broadcastIP;
}
