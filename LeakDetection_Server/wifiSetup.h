#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Function declarations
void initializeWiFi();
bool connectToWiFi();
void startAccessPoint();
bool isWiFiConnected();
IPAddress getLocalIP();
void updateMDNS();
void startMDNS(const char* hostname);

#endif  // WIFI_SETUP_H
