#include "wifiSetup.h"

void connectToWiFi() {
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
}
