#include "oled.h"

#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // If not work please try 0x3D
#define OLED_SDA D5          // Stock firmware shows wrong pins
#define OLED_SCL D6          // They swap SDA with SCL ;)

Adafruit_SSD1306* display;
bool displayEnabled = true;

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

void handle_oled(uint16_t co2, float tempCO2, float humidity, bool relayState) {
  if(!displayEnabled) return;

  display->clearDisplay();
  display->setTextSize(2);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);

  display->print("CO2: ");
  if(co2 > highCO2level) {
    display->print((millis() / 1000) % 2 ? String(co2) : "");  // blinking if high CO2
  } else {
    display->print(co2);
  }
  display->println(relayState ? " ON" : " OFF");

  display->print("tC:");
  display->println(tempCO2);
  display->print("h%:");
  display->println(humidity);

  display->display();
}

void handle_oled_setting(float tempCurrent, float tempTarget, bool relayState) {
  display->clearDisplay();
  display->setTextColor(SSD1306_WHITE);
  display->setTextSize(2);
  display->setCursor(0, 0);
  display->println(relayState ? " ON" : " OFF");

  // CURRENT температура - ЛІВА половина
  display->setTextSize(2);    // 12x16 пікселів (4 × 12 = 48px)
  display->setCursor(8, 24);  // x=8 (центруємо: (64-48)/2), y=24 (вертикально центруємо)
  display->print(tempCurrent, 1);

  // TARGET температура - ПРАВА половина
  display->setTextSize(2);
  display->setCursor(72, 24);  // x=72 (64 + 8), y=24
  display->print(tempTarget, 1);

  display->display();
}

void turnOffDisplay() {
  displayEnabled = false;
  display->clearDisplay();
  display->display();
  display->ssd1306_command(SSD1306_DISPLAYOFF);  // Фізично вимикаємо
}

void turnOnDisplay() {
  displayEnabled = true;
  display->ssd1306_command(SSD1306_DISPLAYON);  // Фізично вмикаємо
}