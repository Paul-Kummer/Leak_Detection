#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

const int sensorPin = D4;  // Pin connected to the water sensor
const int THIRTY_MINUTES = 30 * 60 * 1000000;  // 30 minutes in microseconds
const int TEN_SECONDS = 10 * 1000000;  // 10 seconds in microseconds
WiFiUDP udp;

const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";
const char *udpAddress = "192.168.1.255";
const unsigned int udpPort = 4210;

bool leakDetected = false;

void setup() {
    Serial.begin(115200);
    pinMode(sensorPin, INPUT_PULLUP);  // D4 with pull-up for active-low sensor

    // Check if the sensor triggered the wake-up
    leakDetected = digitalRead(sensorPin) == LOW;
    Serial.println(leakDetected ? "Leak detected!" : "No leak detected.");

    connectToWiFi();

    if (leakDetected) {
        // Broadcast every 10 seconds while sensor is triggered
        for (int i = 0; i < 5 && digitalRead(sensorPin) == LOW; i++) {
            broadcastStatus(true);
            delay(10000);  // 10 seconds
        }
    } else {
        // Periodic broadcast if woken up by timer
        for (int i = 0; i < 5; i++) {
            broadcastStatus(false);
            delay(10000);  // 10 seconds
        }
    }

    // Go back to deep sleep
    ESP.deepSleep(TEN_SECONDS, WAKE_RF_DEFAULT);
}

void loop() {}

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi.");
}

void broadcastStatus(bool triggered) {
    if (WiFi.status() == WL_CONNECTED) {
        String jsonData = "{\"ID\":\"Sensor_1\",\"triggered\":" + String(triggered ? "true" : "false") + "}";
        udp.beginPacket(udpAddress, udpPort);
        udp.print(jsonData);
        udp.endPacket();
        Serial.printf("Broadcasting: %s\n", jsonData.c_str());
    } else {
        Serial.println("WiFi not connected.");
    }
}
