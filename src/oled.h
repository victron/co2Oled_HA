#ifndef OLED_H
#define OLED_H

#include <Adafruit_SSD1306.h>

const uint16_t highCO2level = 1200;

enum class OledState {
  OFF,
  CO2_DISPLAY,
  SET_TARGET
};

extern OledState oledState;
void init_oled();
void handle_oled(uint16_t co2, float tempCO2, float humidity);
void turnOffDisplay();
void turnOnDisplay();

#endif