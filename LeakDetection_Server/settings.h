#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// Declare global settings variables
extern String wifiSSID;
extern String wifiPassword;
extern String webToken;
extern String motorIP;

// Function prototypes for loading and saving settings
void loadSettings();
void saveSettings();

#endif
