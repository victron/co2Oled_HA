#ifndef OLED_H
#define OLED_H

#include <Adafruit_SSD1306.h>

const uint16_t highCO2level = 1200;

void init_oled();
void handle_oled(uint16_t co2, float temperature, float humidity);

#endif