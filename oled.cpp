#include "oled.h"
#include <Adafruit_GFX.h>



#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1   // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // If not work please try 0x3D
#define OLED_SDA D5         // Stock firmware shows wrong pins
#define OLED_SCL D6         // They swap SDA with SCL ;)


Adafruit_SSD1306 *display;
void init_oled() {
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  // // OLED used nonstandard SDA and SCL pins
  // Wire.begin(D5, D6);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
}

void handle_oled(uint16_t co2, float temperature, float humidity ) {
  display->clearDisplay();
  display->setTextSize(2);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);

  display->print("CO2: ");
  display->println(co2);
  display->print("tC:");
  display->println(temperature);
  display->print("h%:");
  display->println(humidity);

  display->display();
}



