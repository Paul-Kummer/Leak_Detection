#ifndef FILE_CONTROL_H
#define FILE_CONTROL_H

#include <FS.h>
#include <ESP8266WebServer.h>
#include <FS.h>

void initializeFileSystem();
String loadFileContent(const char* path);
bool saveFileContent(const char* path, const String& content);
void serveFile(ESP8266WebServer& server, const char* path, const char* contentType);
void handleNotFound(ESP8266WebServer& server);

#endif
