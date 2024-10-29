#include "fileControl.h"
#include <Arduino.h>  // Ensure String class is available
#include <LittleFS.h> // Use LittleFS for the filesystem

// Initialize the LittleFS filesystem
void initializeFileSystem() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS initialization failed!");
        while (true);  // Halt if LittleFS fails to initialize
    }
    Serial.println("LittleFS initialized.");
}

// Load the content of a file from LittleFS
String loadFileContent(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open file: %s\n", path);
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
}

// Save content to a file on LittleFS
bool saveFileContent(const char* path, const String& content) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.printf("Failed to open file for writing: %s\n", path);
        return false;
    }

    file.print(content);
    file.close();
    return true;
}
