#include "fileControl.h"
#include <Arduino.h>  // Ensure String class is available

// Initialize the SPIFFS filesystem
void initializeFileSystem() {
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS initialization failed!");
        while (true);  // Halt if SPIFFS fails to initialize
    }
    Serial.println("SPIFFS initialized.");
}

// Load the content of a file from SPIFFS
String loadFileContent(const char* path) {
    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open file: %s\n", path);
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
}

// Save content to a file on SPIFFS
bool saveFileContent(const char* path, const String& content) {
    File file = SPIFFS.open(path, "w");
    if (!file) {
        Serial.printf("Failed to open file for writing: %s\n", path);
        return false;
    }

    file.print(content);
    file.close();
    return true;
}

// Serve a file over HTTP
void serveFile(ESP8266WebServer& server, const char* path, const char* contentType) {
    File file = SPIFFS.open(path, "r");
    if (!file) {
        server.send(404, "text/plain", "File not found");
        return;
    }

    server.streamFile(file, contentType);
    file.close();
}

// Handle 404 errors for unknown routes
void handleNotFound(ESP8266WebServer& server) {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
}
