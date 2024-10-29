#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#ifndef STASSID
#define STASSID "NETGEAR92"
#define STAPSK "breezyunicorn425"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

const char *serverUrl = "http://192.168.1.70";  // Web server address
const int postInterval = 10000;  // Send data every 10 seconds

bool leakDetected = false;  // Leak detection status
bool silenced = false;  // Silenced status
int batteryLevel = 100;  // Simulated battery level (0-100%)

const int ledPin = LED_BUILTIN;  // Onboard LED
const int stopPin = 5;  // Pin to monitor for stop signal (GPIO5 / D1)

WiFiClient wifiClient;  // Create a WiFi client

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);  // Set LED pin as output
    pinMode(stopPin, INPUT);  // Set stop pin as input

    connectToWiFi();  // Connect to Wi-Fi
}

void loop() {
    int stopPinState = digitalRead(stopPin);

    if (stopPinState == HIGH && !leakDetected) {
        // D1 is pulled HIGH, trigger leakDetected to true
        leakDetected = true;
        silenced = false;  // Reset silenced when a leak is detected
        Serial.println("Leak detected! Sending immediate signal.");
        //sendStatus();  // Send signal to the server
    } 
    else if (stopPinState == LOW && leakDetected) {
        // D1 is pulled LOW, stop leak detection and reset states
        leakDetected = false;
        silenced = false;  // Reset silenced when D1 goes low
        Serial.println("Leak stopped, stopping signals.");
        digitalWrite(ledPin, LOW);  // Turn off LED
        //return;  // Stop sending signals
    }

    sendStatus();  // Send signal to the server

    // Blink LED if a leak is detected
    if (leakDetected) {
        blinkLED();  // Blink the onboard LED if leakDetected is true
        sendStatus();  // Send status to the web server
    }

    delay(postInterval);  // Wait for 10 seconds before next loop
}

// Function to connect to Wi-Fi
void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    } 

    Serial.println("\nConnected to WiFi.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void sendStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        WiFiClient wifiClient;  // Ensure WiFiClient is initialized

        String url = "http://192.168.1.70/sensorUpdate";  // POST to the correct endpoint

        // Prepare POST data
        String postData = "mac=" + WiFi.macAddress();
        postData += "&battery=" + String(batteryLevel);
        postData += "&leakDetected=" + String(leakDetected ? "true" : "false");

        Serial.println("Sending POST request to: " + url);
        Serial.println("Post Data: " + postData);

        // Initialize HTTP POST
        http.begin(wifiClient, url);  
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Send the POST request
        int httpCode = http.POST(postData);

        // Handle the server response
        if (httpCode > 0) {
            String payload = http.getString();
            Serial.printf("Response (%d): %s\n", httpCode, payload.c_str());

            // Check for silence command in the response
            if (payload == "silence") {
                silenced = true;
                Serial.println("Silence command received.");
            }
        } else {
            Serial.printf("POST failed: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();  // Close the connection
    } else {
        Serial.println("WiFi not connected.");
    }
}




unsigned long previousMillis = 0;  // Store the last time the LED was toggled
const long interval = 500;         // Interval between LED states (500ms)
bool ledState = LOW;               // Track the LED state

void blinkLED() {
    unsigned long currentMillis = millis();  // Get the current time

    // Check if it's time to toggle the LED state
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;  // Update the last toggle time

        // Toggle the LED state
        ledState = !ledState;
        digitalWrite(ledPin, ledState);
    }
}
