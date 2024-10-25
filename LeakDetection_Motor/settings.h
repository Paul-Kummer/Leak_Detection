#ifndef SETTINGS_H
#define SETTINGS_H

#include <ArduinoJson.h>  // JSON library for parsing and serialization

// Constants
const char* SETTINGS_FILE = "/settings.json";  // Settings file path
const size_t JSON_BUFFER_SIZE = 512;           // JSON buffer size for parsing

// Shared Variables (Loaded from file)
extern String wifiSSID;
extern String wifiPassword;
extern int motorSpeed;
extern int motorAcceleration;
extern int stepsToClose;
extern int currentMotorPosition;
extern int desiredPosition;
extern String webToken;

// Function Prototypes
void loadSettings();  // Load settings from SPIFFS
void saveSettings();  // Save settings to SPIFFS

#endif
