/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <StreamString.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#ifndef STASSID
#define STASSID "NETGEAR92"
#define STAPSK "breezyunicorn425"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
unsigned int localPort = 2390;  // local port to listen for UDP packets

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int led = 13;
int valvePosition = 0;
int desiredPosition = 0;

void handleRoot() {
    digitalWrite(led, 1);

    // Read the HTML template file
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        server.send(404, "text/plain", "File not found");
        digitalWrite(led, 0);
        return;
    }

    // Create a buffer to store the file content
    String htmlContent = file.readString();  // Read the entire file
    file.close();

    // Replace a placeholder in the HTML with the current valve position
    htmlContent.replace("{{valvePosition}}", String(valvePosition));

    // Send the modified HTML content to the client
    server.send(200, "text/html", htmlContent);

    digitalWrite(led, 0);
}

void handleGetValvePosition() {
    String jsonResponse = "{\"valvePosition\": " + String(valvePosition) + "}";
    server.send(200, "application/json", jsonResponse);
}

void handleSetValvePosition() {
    if (server.hasArg("valvePosition")) {
        valvePosition = server.arg("valvePosition").toInt();
        Serial.printf("Received valve position: %d%%\n", valvePosition);
        DisplayInfo(String(valvePosition) + "%");
    }
    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleSetDesiredPosition() {
    if (server.hasArg("desiredPosition")) {
        desiredPosition = server.arg("desiredPosition").toInt();
        Serial.printf("Received desired position: %d%%\n", desiredPosition);
    }
    server.send(200, "application/json", "{\"status\":\"success\"}");
}



// If a POST request is made to URI /login
void handleLogin() {                         
    // If the POST request doesn't have username and password data
  if( ! server.hasArg("username") || ! server.hasArg("password")|| 
        server.arg("username") == NULL || server.arg("password") == NULL) 
  { 
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }

  // If both the username and the password are correct
  if(server.arg("username") == "John Doe" && server.arg("password") == "password123") 
  { 
    server.send(200, "text/html", "<h1>Welcome, " + server.arg("username") + "!</h1><p>Login successful</p>");
  } 
  else
  {                                                                              
    server.send(401, "text/plain", "401: Unauthorized " + server.arg("username") + " | " +server.arg("password"));
  }
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void handleStyles() {
    File file = SPIFFS.open("/styles.css", "r");
    if (!file) {
        server.send(404, "text/plain", "CSS file not found");
        return;
    }
    server.streamFile(file, "text/css");
    file.close();
}

void handleScript() {
    File file = SPIFFS.open("/indexScripts.js", "r");
    if (!file) {
        server.send(404, "text/plain", "JavaScript file not found");
        return;
    }
    server.streamFile(file, "application/javascript");
    file.close();
}


void DisplayInfo(String dispInfo) {
    display.clearDisplay();  // Clear the display buffer

    // Dynamically adjust text size based on the length of the string
    int textSize = (dispInfo.length() > 3) ? 4 : 6;  // Smaller size for longer text
    display.setTextSize(textSize);  // Apply the chosen text size
    display.setTextColor(SSD1306_WHITE);

    // Calculate text bounds to center it on the screen
    int16_t x1, y1;
    uint16_t width, height;
    display.getTextBounds(dispInfo, 0, 0, &x1, &y1, &width, &height);

    // Center the text horizontally and vertically
    int x = (SCREEN_WIDTH - width) / 2;
    int y = (SCREEN_HEIGHT - height) / 2;

    display.setCursor(x, y);  // Set the cursor position
    display.println(dispInfo);  // Print the text
    display.display();  // Display the updated content
}


void WriteLine(char text[]) {
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(text);
  display.display();
  delay(1000);
}

void InitializeLCD(){
    Serial.println("Starting Display");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  Serial.println("Display Started.");
}

void ConnectToWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void InitializeWebServer() {
    if (MDNS.begin("whoisleaking")) {
        Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);  // Serve index.html
    server.on("/styles.css", handleStyles);  // Serve styles.css
    server.on("/indexScripts.js", handleScript);  // Serve indexScripts.js
    server.on("/setValvePosition", handleSetValvePosition);  // Handle valve position POST
    server.on("/setDesiredPosition", handleSetDesiredPosition);
    server.on("/getValvePosition", handleGetValvePosition);  // Provide valve position
    server.on("/login", HTTP_POST, handleLogin);  // Handle login
    server.onNotFound(handleNotFound);  // Handle unknown requests

    server.begin();
    Serial.println("HTTP server started");
}


void InitializeFileServer() {
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS initialization failed!");
        while (true);  // Halt the system if SPIFFS fails
    }
    Serial.println("SPIFFS initialized.");
}


void setup(void) 
{
    pinMode(led, OUTPUT);
    digitalWrite(led, 0);
    Serial.begin(115200);

    ConnectToWifi();
    InitializeFileServer();
    InitializeWebServer();
    InitializeLCD();
}

void loop(void) {
    server.handleClient();
    MDNS.update();

    // Simulate valve movement toward the desired position
    if (valvePosition < desiredPosition) {
        valvePosition++;  // Increment position toward the target
    } else if (valvePosition > desiredPosition) {
        valvePosition--;  // Decrement position toward the target
    }

    // Display the current valve position on the OLED
    DisplayInfo(String(valvePosition) + "%");

    delay(100);  // Adjust this delay to control movement speed
}
