#ifndef OLED_H
#define OLED_H

#include <Adafruit_SSD1306.h>

extern bool relayState;
extern bool button_pushed;
void init_oled();
void handle_oled(uint16_t co2, float temperature, float humidity);

#endif