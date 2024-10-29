#ifndef SETTINGS_H
#define SETTINGS_H

#include <ArduinoJson.h>  // JSON library for parsing and serialization

// Constants
extern const char* SETTINGS_FILE;
const size_t JSON_BUFFER_SIZE = 512;  // Buffer size for JSON parsing

// Shared Variables (Extern Declaration)
extern String wifiSSID;
extern String wifiPassword;
extern int motorSpeed;
extern int motorAcceleration;
extern int stepsToClose;
extern int currentMotorPosition;
extern int desiredPosition;
extern String webToken;

// Function Prototypes
void loadSettings();  // Load settings from the file system
void saveSettings();  // Save settings to the file system
void listFiles();     // Check what files are in the filte system

#endif  // SETTINGS_H
