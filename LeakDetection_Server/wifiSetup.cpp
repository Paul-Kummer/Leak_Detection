#include "wifiSetup.h"
#include "settings.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

unsigned long lastMDNSUpdateTime = 0;  // Track last mDNS update time
const unsigned long MDNS_UPDATE_INTERVAL = 30000;  // 30 seconds

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress getLocalIP() {
    return WiFi.localIP();
}

void initializeWiFi() {
    Serial.println("Initializing Wi-Fi...");

    if (connectToWiFi()) {
        Serial.println("Connected to Wi-Fi.");
        Serial.print("IP Address: ");
        Serial.println(getLocalIP());
        startMDNS("whoisleaking");  // Start mDNS with hostname "motor"
    } else {
        Serial.println("Failed to connect to Wi-Fi. Starting Access Point...");
        startAccessPoint();
    }
}

bool connectToWiFi() {
    WiFi.mode(WIFI_STA);  // Set WiFi to station mode (client)
    WiFi.begin(STASSID, STAPSK);  // Connect using credentials from settings.h

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    return isWiFiConnected();
}

void startAccessPoint() {
    WiFi.mode(WIFI_AP);  // Set Wi-Fi mode to access point
    WiFi.softAP("LeakDetection_Main");

    IPAddress apIP = WiFi.softAPIP();
    Serial.print("AP IP Address: ");
    Serial.println(apIP);

    // Optionally start mDNS in AP mode
    startMDNS("whoisleaking");
}

void startMDNS(const char* hostname) {
    if (MDNS.begin(hostname)) {
        Serial.printf("mDNS responder started: %s.local\n", hostname);
    } else {
        Serial.println("Error setting up mDNS responder.");
    }
}

// Update mDNS only if enough time has passed since the last update
void updateMDNS() {
    if (millis() - lastMDNSUpdateTime >= MDNS_UPDATE_INTERVAL) {
        MDNS.update();
        lastMDNSUpdateTime = millis();  // Reset the timer
    }
}
