#include "displayControl.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initializeDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        while (true);
    }
    display.clearDisplay();
    display.display();
}

void displayInfo(const String &dispInfo) {
    display.clearDisplay();
    int textSize = (dispInfo.length() > 3) ? 4 : 6;
    display.setTextSize(textSize);
    display.setTextColor(SSD1306_WHITE);

    int16_t x1, y1;
    uint16_t width, height;
    display.getTextBounds(dispInfo, 0, 0, &x1, &y1, &width, &height);

    int x = (SCREEN_WIDTH - width) / 2;
    int y = (SCREEN_HEIGHT - height) / 2;

    display.setCursor(x, y);
    display.println(dispInfo);
    display.display();
}
