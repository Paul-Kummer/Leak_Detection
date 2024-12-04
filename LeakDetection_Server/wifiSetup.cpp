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
        startMDNS("whoisleaking"); 
    } else {
        Serial.println("Failed to connect to Wi-Fi.");
    }

    if(! startAccessPoint())
    {
        Serial.println("Failed to start Wi-Fi AP.");
    }
}

bool connectToWiFi() {
    WiFi.mode(WIFI_AP_STA);  // Set WiFi to station mode (client)
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    Serial.print("Connecting to WiFi");
    unsigned long startTime = millis();

    // Wait until connected or timeout after 30 seconds
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        
        // Timeout check to avoid infinite loop
        if (millis() - startTime >= 30000) {  
            Serial.println("\nFailed to connect to WiFi");
            return false;
        }
    }

    Serial.println("Connected to WiFi");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());

    return true;
}

bool startAccessPoint() {
    Serial.print("Starting WiFi AP...");

    if (!WiFi.softAP("Leak_Master", "")) {
        Serial.println("Failed to start AP.");
        return false;
    }

    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
    return true;
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
