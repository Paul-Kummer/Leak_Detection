#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>  
#include <FS.h>  // For SPIFFS

// External server object accessible throughout the project
extern ESP8266WebServer server;
extern WiFiUDP udp;

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
void sendDesiredPositionToMotorNode(int position);


#endif
