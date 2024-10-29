#ifndef FILE_CONTROL_H
#define FILE_CONTROL_H

#include <LittleFS.h>

// Initialize the LittleFS filesystem
void initializeFileSystem();

// Load the content of a file from LittleFS
String loadFileContent(const char* path);

// Save content to a file on LittleFS
bool saveFileContent(const char* path, const String& content);

#endif
