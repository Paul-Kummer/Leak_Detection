#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>

// Declare the web server object globally
extern ESP8266WebServer server;

// Function Prototypes
void initializeWebServer();  // Initialize web server and routes
void setupMDNS();            // Setup mDNS responder
void handleSetPosition();    // Handle motor position updates
void handleSettingsPage();   // Serve the settings page
void handleSaveSettings();   // Save new settings from the form
void handleNotFound();       // Handle 404 errors

#endif
