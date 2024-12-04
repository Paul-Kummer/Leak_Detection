#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "sensorControl.h"
#include <FS.h>  // For SPIFFS

// External server object accessible throughout the project
extern ESP8266WebServer server;
extern WiFiUDP udp;

extern SensorControl sensorControl;

// Function to initialize the web server and register handlers
void InitializeWebServer();

// Handler function declarations
void handleGetValvePosition();
void handleControlValve();
void handleSensorUpdate();
void handleSaveSettings();
void handleNotFound();
void handleGetIPAddress();
void serveFile(const char* path, const char* contentType);
void listFiles();
void listenForUdp();
void handleGetAlerts();
void handleGetSavedSensors();
void handleGetAvailableSensors();
void handleUpdateIgnoredStatus();
void sendDesiredPositionToMotorNode(int position);
void checkSensorAlert(const StaticJsonDocument<256>& jsonDoc);
void handleGetSensors();
void checkSensorBroadcast(const StaticJsonDocument<256>& jsonDoc);
void handleNonJsonUdpPacket(const char* packet);
void handleValvePosition(int position);
void handleAddSensor();
void handleRemoveSensor();
#endif
