#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <ESP8266WiFi.h>

// Function prototypes
bool connectToWiFi();
bool connectToMaster();
bool fetchSettingsFromMaster();
bool isWiFiConnected();

#endif
