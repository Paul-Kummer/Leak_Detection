#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>  // For SPIFFS

// External server object accessible throughout the project
extern ESP8266WebServer server;

// Function to initialize the web server and register handlers
void InitializeWebServer();

// Handler function declarations
void handleRoot();
void handleGetValvePosition();
void handleControlValve();
void handleSensorUpdate();
void handleNotFound();
void handleStyles();
void handleScript();

#endif
